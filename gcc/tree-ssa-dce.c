/* Dead code elimination pass for the GNU compiler.
   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Ben Elliston <bje@redhat.com>
   and Andrew MacLeod <amacleod@redhat.com>
   Adapted to use control dependence by Steven Bosscher, SUSE Labs.
 
This file is part of GCC.
   
GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.
   
GCC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.
   
You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* Dead code elimination.

   References:

     Building an Optimizing Compiler,
     Robert Morgan, Butterworth-Heinemann, 1998, Section 8.9.

     Advanced Compiler Design and Implementation,
     Steven Muchnick, Morgan Kaufmann, 1997, Section 18.10.

   Dead-code elimination is the removal of statements which have no
   impact on the program's output.  "Dead statements" have no impact
   on the program's output, while "necessary statements" may have
   impact on the output.

   The algorithm consists of three phases:
   1. Marking as necessary all statements known to be necessary,
      e.g. most function calls, writing a value to memory, etc;
   2. Propagating necessary statements, e.g., the statements
      giving values to operands in necessary statements; and
   3. Removing dead statements.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "errors.h"
#include "ggc.h"

/* These RTL headers are needed for basic-block.h.  */
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"

#include "tree.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "tree-gimple.h"
#include "tree-dump.h"
#include "tree-pass.h"
#include "timevar.h"
#include "flags.h"

static struct stmt_stats
{
  int total;
  int total_phis;
  int removed;
  int removed_phis;
} stats;

static varray_type worklist;

/* Vector indicating an SSA name has already been processed and marked
   as necessary.  */
static sbitmap processed;

/* Vector indicating that last_stmt if a basic block has already been
   marked as necessary.  */
static sbitmap last_stmt_necessary;

/* Before we can determine whether a control branch is dead, we need to
   compute which blocks are control dependent on which edges.

   We expect each block to be control dependent on very few edges so we
   use a bitmap for each block recording its edges.  An array holds the
   bitmap.  The Ith bit in the bitmap is set if that block is dependent
   on the Ith edge.  */
bitmap *control_dependence_map;

/* Execute CODE for each edge (given number EDGE_NUMBER within the CODE)
   for which the block with index N is control dependent.  */
#define EXECUTE_IF_CONTROL_DEPENDENT(N, EDGE_NUMBER, CODE)		      \
  EXECUTE_IF_SET_IN_BITMAP (control_dependence_map[N], 0, EDGE_NUMBER, CODE)

/* Local function prototypes.  */
static inline void set_control_dependence_map_bit (basic_block, int);
static inline void clear_control_dependence_bitmap (basic_block);
static void find_all_control_dependences (struct edge_list *);
static void find_control_dependence (struct edge_list *, int);
static inline basic_block find_pdom (basic_block);

static inline void mark_stmt_necessary (tree, bool);
static inline void mark_operand_necessary (tree);

static bool need_to_preserve_store (tree);
static void mark_stmt_if_obviously_necessary (tree, bool);
static void find_obviously_necessary_stmts (struct edge_list *);

static void mark_control_dependent_edges_necessary (basic_block, struct edge_list *);
static void propagate_necessity (struct edge_list *);

static void eliminate_unnecessary_stmts (void);
static void remove_dead_phis (basic_block);
static void remove_dead_stmt (block_stmt_iterator *, basic_block);

static void print_stats (void);
static void tree_dce_init (bool);
static void tree_dce_done (bool);

/* Indicate block BB is control dependent on an edge with index EDGE_INDEX.  */
static inline void
set_control_dependence_map_bit (basic_block bb, int edge_index)
{
  if (bb == ENTRY_BLOCK_PTR)
    return;
  if (bb == EXIT_BLOCK_PTR)
    abort ();
  bitmap_set_bit (control_dependence_map[bb->index], edge_index);
}

/* Clear all control dependences for block BB.  */
static inline
void clear_control_dependence_bitmap (basic_block bb)
{
  bitmap_clear (control_dependence_map[bb->index]);
}

/* Record all blocks' control dependences on all edges in the edge
   list EL, ala Morgan, Section 3.6.  */

static void
find_all_control_dependences (struct edge_list *el)
{
  int i;

  for (i = 0; i < NUM_EDGES (el); ++i)
    find_control_dependence (el, i);
}

/* Determine all blocks' control dependences on the given edge with edge_list
   EL index EDGE_INDEX, ala Morgan, Section 3.6.  */

static void
find_control_dependence (struct edge_list *el, int edge_index)
{
  basic_block current_block;
  basic_block ending_block;

#ifdef ENABLE_CHECKING
  if (INDEX_EDGE_PRED_BB (el, edge_index) == EXIT_BLOCK_PTR)
    abort ();
#endif

  if (INDEX_EDGE_PRED_BB (el, edge_index) == ENTRY_BLOCK_PTR)
    ending_block = ENTRY_BLOCK_PTR->next_bb;
  else
    ending_block = find_pdom (INDEX_EDGE_PRED_BB (el, edge_index));

  for (current_block = INDEX_EDGE_SUCC_BB (el, edge_index);
       current_block != ending_block && current_block != EXIT_BLOCK_PTR;
       current_block = find_pdom (current_block))
    {
      edge e = INDEX_EDGE (el, edge_index);

      /* For abnormal edges, we don't make current_block control
	 dependent because instructions that throw are always necessary
	 anyway.  */
      if (e->flags & EDGE_ABNORMAL)
	continue;

      set_control_dependence_map_bit (current_block, edge_index);
    }
}

/* Find the immediate postdominator PDOM of the specified basic block BLOCK.
   This function is necessary because some blocks have negative numbers.  */

static inline basic_block
find_pdom (basic_block block)
{
  if (block == ENTRY_BLOCK_PTR)
    abort ();
  else if (block == EXIT_BLOCK_PTR)
    return EXIT_BLOCK_PTR;
  else
    {
      basic_block bb = get_immediate_dominator (CDI_POST_DOMINATORS, block);
      if (! bb)
	return EXIT_BLOCK_PTR;
      return bb;
    }
}

#define NECESSARY(stmt)		stmt->common.asm_written_flag

/* If STMT is not already marked necessary, mark it, and add it to the
   worklist if ADD_TO_WORKLIST is true.  */
static inline void
mark_stmt_necessary (tree stmt, bool add_to_worklist)
{
#ifdef ENABLE_CHECKING
  if (stmt == NULL
      || stmt == error_mark_node
      || (stmt && DECL_P (stmt)))
    abort ();
#endif

  if (NECESSARY (stmt))
    return;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Marking useful stmt: ");
      print_generic_stmt (dump_file, stmt, TDF_SLIM);
      fprintf (dump_file, "\n");
    }

  NECESSARY (stmt) = 1;
  if (add_to_worklist)
    VARRAY_PUSH_TREE (worklist, stmt);
}

/* Mark the statement defining operand OP as necessary.  */

static inline void
mark_operand_necessary (tree op)
{
  tree stmt;
  int ver;

#ifdef ENABLE_CHECKING
  if (op == NULL)
    abort ();
#endif

  ver = SSA_NAME_VERSION (op);
  if (TEST_BIT (processed, ver))
    return;
  SET_BIT (processed, ver);

  stmt = SSA_NAME_DEF_STMT (op);
#ifdef ENABLE_CHECKING
  if (stmt == NULL)
    abort ();
#endif

  if (NECESSARY (stmt)
      || IS_EMPTY_STMT (stmt))
    return;

  NECESSARY (stmt) = 1;
  VARRAY_PUSH_TREE (worklist, stmt);
}

/* Return true if a store to a variable needs to be preserved.  */

static inline bool
need_to_preserve_store (tree ssa_name)
{
  return (needs_to_live_in_memory (SSA_NAME_VAR (ssa_name)));
}


/* Mark STMT as necessary if it is obviously is.  Add it to the worklist if
   it can make other statements necessary.

   If AGGRESSIVE is false, control statements are conservatively marked as
   necessary.  */

static void
mark_stmt_if_obviously_necessary (tree stmt, bool aggressive)
{
  def_optype defs;
  vdef_optype vdefs;
  stmt_ann_t ann;
  size_t i;

  /* Statements that are implicitly live.  Most function calls, asm and return
     statements are required.  Labels and BIND_EXPR nodes are kept because
     they are control flow, and we have no way of knowing whether they can be
     removed.  DCE can eliminate all the other statements in a block, and CFG
     can then remove the block and labels.  */
  switch (TREE_CODE (stmt))
    {
    case BIND_EXPR:
    case LABEL_EXPR:
    case CASE_LABEL_EXPR:
      mark_stmt_necessary (stmt, false);
      return;

    case ASM_EXPR:
    case RESX_EXPR:
    case RETURN_EXPR:
      mark_stmt_necessary (stmt, true);
      return;

    case CALL_EXPR:
      /* Most, but not all function calls are required.  Function calls that
	 produce no result and have no side effects (i.e. const pure
	 functions) are unnecessary.  */
      if (TREE_SIDE_EFFECTS (stmt))
	mark_stmt_necessary (stmt, true);
      return;

    case MODIFY_EXPR:
      if (TREE_CODE (TREE_OPERAND (stmt, 1)) == CALL_EXPR
	  && TREE_SIDE_EFFECTS (TREE_OPERAND (stmt, 1)))
	{
	  mark_stmt_necessary (stmt, true);
	  return;
	}

      /* These values are mildly magic bits of the EH runtime.  We can't
	 see the entire lifetime of these values until landing pads are
	 generated.  */
      if (TREE_CODE (TREE_OPERAND (stmt, 0)) == EXC_PTR_EXPR
	  || TREE_CODE (TREE_OPERAND (stmt, 0)) == FILTER_EXPR)
	{
	  mark_stmt_necessary (stmt, true);
	  return;
	}
      break;

    case GOTO_EXPR:
      if (! simple_goto_p (stmt))
	mark_stmt_necessary (stmt, true);
      return;

    case COND_EXPR:
      if (GOTO_DESTINATION (COND_EXPR_THEN (stmt))
	  == GOTO_DESTINATION (COND_EXPR_ELSE (stmt)))
	{
	  /* A COND_EXPR is obviously dead if the target labels are the same.
	     We cannot kill the statement at this point, so to prevent the
	     statement from being marked necessary, we replace the condition
	     with a constant.  The stmt is killed later on in cfg_cleanup.  */
	  COND_EXPR_COND (stmt) = integer_zero_node;
	  modify_stmt (stmt);
	  return;
	}
      /* Fall through.  */

    case SWITCH_EXPR:
      if (! aggressive)
	mark_stmt_necessary (stmt, true);
      break;

    default:
      break;
    }

  ann = stmt_ann (stmt);
  /* If the statement has volatile operands, it needs to be preserved.  Same
     for statements that can alter control flow in unpredictable ways.  */
  if (ann->has_volatile_ops
      || is_ctrl_altering_stmt (stmt))
    {
      mark_stmt_necessary (stmt, true);
      return;
    }

  get_stmt_operands (stmt);

  defs = DEF_OPS (ann);
  for (i = 0; i < NUM_DEFS (defs); i++)
    {
      tree def = DEF_OP (defs, i);
      if (need_to_preserve_store (def))
	{
	  mark_stmt_necessary (stmt, true);
	  return;
        }
    }

  vdefs = VDEF_OPS (ann);
  for (i = 0; i < NUM_VDEFS (vdefs); i++)
    {
      tree vdef = VDEF_RESULT (vdefs, i);
      if (need_to_preserve_store (vdef))
	{
	  mark_stmt_necessary (stmt, true);
	  return;
        }
    }

  return;
}

/* Find obviously necessary statements.  These are things like most function
   calls, and stores to file level variables.

   If EL is NULL, control statements are conservatively marked as
   necessary.  Otherwise it contains the list of edges used by control
   dependence analysis.  */

static void
find_obviously_necessary_stmts (struct edge_list *el)
{
  basic_block bb;
  block_stmt_iterator i;
  edge e;

  FOR_EACH_BB (bb)
    {
      tree phi;

      /* Check any PHI nodes in the block.  */
      for (phi = phi_nodes (bb); phi; phi = TREE_CHAIN (phi))
	{
	  NECESSARY (phi) = 0;

	  /* PHIs for virtual variables do not directly affect code
	     generation and need not be considered inherently necessary
	     regardless of the bits set in their decl.

	     Thus, we only need to mark PHIs for real variables which
	     need their result preserved as being inherently necessary.  */
	  if (is_gimple_reg (PHI_RESULT (phi))
	      && need_to_preserve_store (PHI_RESULT (phi)))
	    mark_stmt_necessary (phi, true);
        }

      /* Check all statements in the block.  */
      for (i = bsi_start (bb); ! bsi_end_p (i); bsi_next (&i))
	{
	  tree stmt = bsi_stmt (i);
	  NECESSARY (stmt) = 0;
	  mark_stmt_if_obviously_necessary (stmt, el != NULL);
	}

      /* Mark this basic block as `not visited'.  A block will be marked
	 visited when the edges that it is control dependent on have been
	 marked.  */
      bb->flags &= ~BB_VISITED;
    }

  if (el)
    {
      /* Prevent the loops from being removed.  We must keep the infinite loops,
	 and we currently do not have a means to recognize the finite ones.  */
      FOR_EACH_BB (bb)
	{
	  for (e = bb->succ; e; e = e->succ_next)
	    if (e->flags & EDGE_DFS_BACK)
	      mark_control_dependent_edges_necessary (e->dest, el);
	}
    }
}

/* Make corresponding control dependent edges necessary.  We only
   have to do this once for each basic block, so we clear the bitmap
   after we're done.  */
static void
mark_control_dependent_edges_necessary (basic_block bb, struct edge_list *el)
{
  int edge_number;

  EXECUTE_IF_CONTROL_DEPENDENT (bb->index, edge_number,
    {
      tree t;
      basic_block cd_bb = INDEX_EDGE_PRED_BB (el, edge_number);

      if (TEST_BIT (last_stmt_necessary, cd_bb->index))
	continue;
      SET_BIT (last_stmt_necessary, cd_bb->index);

      t = last_stmt (cd_bb);
      if (is_ctrl_stmt (t))
	mark_stmt_necessary (t, true);
    });
}

/* Propagate necessity using the operands of necessary statements.  Process
   the uses on each statement in the worklist, and add all feeding statements
   which contribute to the calculation of this value to the worklist.

   In conservative mode, EL is NULL.  */

static void
propagate_necessity (struct edge_list *el)
{
  tree i;
  bool aggressive = (el ? true : false); 

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\nProcessing worklist:\n");

  while (VARRAY_ACTIVE_SIZE (worklist) > 0)
    {
      /* Take `i' from worklist.  */
      i = VARRAY_TOP_TREE (worklist);
      VARRAY_POP (worklist);

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "processing: ");
	  print_generic_stmt (dump_file, i, TDF_SLIM);
	  fprintf (dump_file, "\n");
	}

      if (aggressive)
	{
	  /* Mark the last statements of the basic blocks that the block
	     containing `i' is control dependent on, but only if we haven't
	     already done so.  */
	  basic_block bb = bb_for_stmt (i);
	  if (! (bb->flags & BB_VISITED))
	    {
	      bb->flags |= BB_VISITED;
	      mark_control_dependent_edges_necessary (bb, el);
	    }
	}

      if (TREE_CODE (i) == PHI_NODE)
	{
	  /* PHI nodes are somewhat special in that each PHI alternative has
	     data and control dependencies.  All the statements feeding the
	     PHI node's arguments are always necessary.  In aggressive mode,
	     we also consider the control dependent edges leading to the
	     predecessor block associated with each PHI alternative as
	     necessary.  */
	  int k;
	  for (k = 0; k < PHI_NUM_ARGS (i); k++)
            {
	      tree arg = PHI_ARG_DEF (i, k);
	      if (TREE_CODE (arg) == SSA_NAME)
		mark_operand_necessary (arg);
	    }

	  if (aggressive)
	    {
	      for (k = 0; k < PHI_NUM_ARGS (i); k++)
		{
		  basic_block arg_bb = PHI_ARG_EDGE (i, k)->src;
		  if (! (arg_bb->flags & BB_VISITED))
		    {
		      arg_bb->flags |= BB_VISITED;
		      mark_control_dependent_edges_necessary (arg_bb, el);
		    }
		}
	    }
	}
      else
	{
	  /* Propagate through the operands.  Examine all the USE, VUSE and
	     VDEF operands in this statement.  Mark all the statements which
	     feed this statement's uses as necessary.  */
	  vuse_optype vuses;
	  vdef_optype vdefs;
	  use_optype uses;
	  stmt_ann_t ann;
	  size_t k;

	  get_stmt_operands (i);
	  ann = stmt_ann (i);

	  uses = USE_OPS (ann);
	  for (k = 0; k < NUM_USES (uses); k++)
	    mark_operand_necessary (USE_OP (uses, k));

	  vuses = VUSE_OPS (ann);
	  for (k = 0; k < NUM_VUSES (vuses); k++)
	    mark_operand_necessary (VUSE_OP (vuses, k));

	  /* The operands of VDEF expressions are also needed as they
	     represent potential definitions that may reach this
	     statement (VDEF operands allow us to follow def-def links).  */
	  vdefs = VDEF_OPS (ann);
	  for (k = 0; k < NUM_VDEFS (vdefs); k++)
	    mark_operand_necessary (VDEF_OP (vdefs, k));
	}
    }
}

/* Eliminate unnecessary statements. Any instruction not marked as necessary
   contributes nothing to the program, and can be deleted.  */

static void
eliminate_unnecessary_stmts (void)
{
  basic_block bb;
  block_stmt_iterator i;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\nEliminating unnecessary statements:\n");

  clear_special_calls ();
  FOR_EACH_BB (bb)
    {
      /* Remove dead PHI nodes.  */
      remove_dead_phis (bb);

      /* Remove dead statements.  */
      for (i = bsi_start (bb); ! bsi_end_p (i) ; )
	{
	  tree t = bsi_stmt (i);

	  stats.total++;

	  /* If `i' is not necessary then remove it.  */
	  if (! NECESSARY (t))
	    remove_dead_stmt (&i, bb);
	  else
	    {
	      if (TREE_CODE (t) == CALL_EXPR)
		notice_special_calls (t);
	      else if (TREE_CODE (t) == MODIFY_EXPR
		       && TREE_CODE (TREE_OPERAND (t, 1)) == CALL_EXPR)
		notice_special_calls (TREE_OPERAND (t, 1));
	      bsi_next (&i);
	    }
	}
    }
}

/* Remove dead PHI nodes from block BB.  */

static void
remove_dead_phis (basic_block bb)
{
  tree prev, phi;

  prev = NULL_TREE;
  phi = phi_nodes (bb);
  while (phi)
    {
      stats.total_phis++;

      if (! NECESSARY (phi))
	{
	  tree next = TREE_CHAIN (phi);

	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "Deleting : ");
	      print_generic_stmt (dump_file, phi, TDF_SLIM);
	      fprintf (dump_file, "\n");
	    }

	  remove_phi_node (phi, prev, bb);
	  stats.removed_phis++;
	  phi = next;
	}
      else
	{
	  prev = phi;
	  phi = TREE_CHAIN (phi);
	}
    }
}

/* Remove dead statement pointed by iterator I.  Receives the basic block BB
   containing I so that we don't have to look it up.  */

static void
remove_dead_stmt (block_stmt_iterator *i, basic_block bb)
{
  tree t = bsi_stmt (*i);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Deleting : ");
      print_generic_stmt (dump_file, t, TDF_SLIM);
      fprintf (dump_file, "\n");
    }

  stats.removed++;

  /* If we have determined that a conditional branch statement contributes
     nothing to the program, then we not only remove it, but we also change
     the flow graph so that the current block will simply fall-thru to its
     immediate post-dominator.  The blocks we are circumventing will be
     removed by cleaup_cfg if this change in the flow graph makes them
     unreachable.  */
  if (is_ctrl_stmt (t))
    {
      basic_block post_dom_bb;
      edge e;
#ifdef ENABLE_CHECKING
      /* The post dominance info has to be up-to-date.  */
      if (dom_computed[CDI_POST_DOMINATORS] != DOM_OK)
	abort ();
#endif
      /* Get the immediate post dominator of bb.  */
      post_dom_bb = get_immediate_dominator (CDI_POST_DOMINATORS, bb);
      /* Some blocks don't have an immediate post dominator.  This can happen
	 for example with infinite loops.  Removing an infinite loop is an
	 inappropriate transformation anyway...  */
      if (! post_dom_bb)
	{
	  bsi_next (i);
	  return;
	}

      /* Redirect the first edge out of BB to reach POST_DOM_BB.  */
      redirect_edge_and_branch (bb->succ, post_dom_bb);
      PENDING_STMT (bb->succ) = NULL;

      /* Dominators are wrong now.  */
      free_dominance_info (CDI_DOMINATORS);

      /* The edge is no longer associated with a conditional, so it does
	 not have TRUE/FALSE flags.  */
      bb->succ->flags &= ~(EDGE_TRUE_VALUE | EDGE_FALSE_VALUE);

      /* If the edge reaches any block other than the exit, then it is a
	 fallthru edge; if it reaches the exit, then it is not a fallthru
	 edge.  */
      if (post_dom_bb != EXIT_BLOCK_PTR)
	bb->succ->flags |= EDGE_FALLTHRU;
      else
	bb->succ->flags &= ~EDGE_FALLTHRU;

      /* Remove the remaining the outgoing edges.  */
      for (e = bb->succ->succ_next; e != NULL;)
	{
	  edge tmp = e;
	  e = e->succ_next;
	  remove_edge (tmp);
	}
    }

  bsi_remove (i);
}

/* Print out removed statement statistics.  */

static void
print_stats (void)
{
  if (dump_file && (dump_flags & (TDF_STATS|TDF_DETAILS)))
    {
      float percg;

      percg = ((float) stats.removed / (float) stats.total) * 100;
      fprintf (dump_file, "Removed %d of %d statements (%d%%)\n",
	       stats.removed, stats.total, (int) percg);

      if (stats.total_phis == 0)
	percg = 0;
      else
	percg = ((float) stats.removed_phis / (float) stats.total_phis) * 100;

      fprintf (dump_file, "Removed %d of %d PHI nodes (%d%%)\n",
	       stats.removed_phis, stats.total_phis, (int) percg);
    }
}


/* Initialization for this pass.  Set up the used data structures.  */

static void
tree_dce_init (bool aggressive)
{
  memset ((void *) &stats, 0, sizeof (stats));

  if (aggressive)
    {
      int i;

      control_dependence_map 
	= xmalloc (last_basic_block * sizeof (bitmap));
      for (i = 0; i < last_basic_block; ++i)
	control_dependence_map[i] = BITMAP_XMALLOC ();

      last_stmt_necessary = sbitmap_alloc (last_basic_block);
      sbitmap_zero (last_stmt_necessary);
    }

  processed = sbitmap_alloc (highest_ssa_version + 1);
  sbitmap_zero (processed);

  VARRAY_TREE_INIT (worklist, 64, "work list");
}

/* Cleanup after this pass.  */

static void
tree_dce_done (bool aggressive)
{
  if (aggressive)
    {
      int i;

      for (i = 0; i < last_basic_block; ++i)
	BITMAP_XFREE (control_dependence_map[i]);
      free (control_dependence_map);

      sbitmap_free (last_stmt_necessary);
    }

  sbitmap_free (processed);
}

/* Main routine to eliminate dead code.

   AGGRESSIVE controls the aggressiveness of the algorithm.
   In conservative mode, we ignore control dependence and simply declare
   all but the most trivially dead branches necessary.  This mode is fast.
   In aggressive mode, control dependences are taken into account, which
   results in more dead code elimination, but at the cost of some time.

   If NO_CFG_CHANGES is true, avoid changing cfg.

   FIXME: Aggressive mode before PRE doesn't work currently because
	  the dominance info is not invalidated after DCE1.  This is
	  not an issue right now because we only run aggressive DCE
	  as the last tree SSA pass, but keep this in mind when you
	  start experimenting with pass ordering.  */

static void
perform_tree_ssa_dce (bool aggressive, bool no_cfg_changes)
{
  struct edge_list *el = NULL;

  if (no_cfg_changes && aggressive)
    abort ();

  tree_dce_init (aggressive);

  if (aggressive)
    {
      /* Compute control dependence.  */
      timevar_push (TV_CONTROL_DEPENDENCES);
      calculate_dominance_info (CDI_POST_DOMINATORS);
      el = create_edge_list ();
      find_all_control_dependences (el);
      timevar_pop (TV_CONTROL_DEPENDENCES);

      mark_dfs_back_edges ();
    }

  find_obviously_necessary_stmts (el);

  propagate_necessity (el);

  eliminate_unnecessary_stmts ();

  if (aggressive)
    free_dominance_info (CDI_POST_DOMINATORS);

  if (!no_cfg_changes)
    cleanup_tree_cfg ();

  /* Debugging dumps.  */
  if (dump_file)
    {
      dump_function_to_file (current_function_decl, dump_file, dump_flags);
      print_stats ();
    }

  tree_dce_done (aggressive);

  free_edge_list (el);
}

/* Cleanup the dead code, but avoid cfg changes.  */

void
tree_ssa_dce_no_cfg_changes (void)
{
  perform_tree_ssa_dce (false, true);
}

/* Pass entry points.  */
static void
tree_ssa_dce (void)
{
  perform_tree_ssa_dce (/*aggressive=*/false, false);
}

static void
tree_ssa_cd_dce (void)
{
  perform_tree_ssa_dce (/*aggressive=*/optimize >= 2, false);
}

static bool
gate_dce (void)
{
  return flag_tree_dce != 0;
}

struct tree_opt_pass pass_dce =
{
  "dce",				/* name */
  gate_dce,				/* gate */
  tree_ssa_dce,				/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_TREE_DCE,				/* tv_id */
  PROP_cfg | PROP_ssa,			/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_ggc_collect | TODO_verify_ssa	/* todo_flags_finish */
};

struct tree_opt_pass pass_cd_dce =
{
  "cddce",				/* name */
  gate_dce,				/* gate */
  tree_ssa_cd_dce,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_TREE_CD_DCE,			/* tv_id */
  PROP_cfg | PROP_ssa,			/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_ggc_collect | TODO_verify_ssa | TODO_verify_flow
					/* todo_flags_finish */
};

