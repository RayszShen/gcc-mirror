/* Loop Vectorization
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
   Contributed by Dorit Naishlos <dorit@il.ibm.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* Loop Vectorization Pass.

   This pass tries to vectorize loops. This first implementation focuses on
   simple inner-most loops, with no conditional control flow, and a set of
   simple operations which vector form can be expressed using existing
   tree codes (PLUS, MULT etc).

   For example, the vectorizer transforms the following simple loop:

	short a[N]; short b[N]; short c[N]; int i;

	for (i=0; i<N; i++){
	  a[i] = b[i] + c[i];
	}

   as if it was manually vectorized by rewriting the source code into:

	typedef int __attribute__((mode(V8HI))) v8hi;
	short a[N];  short b[N]; short c[N];   int i;
	v8hi *pa = (v8hi*)a, *pb = (v8hi*)b, *pc = (v8hi*)c;
	v8hi va, vb, vc;

	for (i=0; i<N/8; i++){
	  vb = pb[i];
	  vc = pc[i];
	  va = vb + vc;
	  pa[i] = va;
	}

	The main entry to this pass is vectorize_loops(), in which
   the vectorizer applies a set of analyses on a given set of loops,
   followed by the actual vectorization transformation for the loops that
   had successfully passed the analysis phase.

	Throughout this pass we make a distinction between two types of
   data: scalars (which are represented by SSA_NAMES), and memory references
   ("data-refs"). These two types of data require different handling both 
   during analysis and transformation. The types of data-refs that the 
   vectorizer currently supports are ARRAY_REFS which base is an array DECL 
   (not a pointer), and INDIRECT_REFS through pointers; both array and pointer
   accesses are required to have a  simple (consecutive) access pattern.

   Analysis phase:
   ===============
	The driver for the analysis phase is vect_analyze_loop_nest().
   It applies a set of analyses, some of which rely on the scalar evolution 
   analyzer (scev) developed by Sebastian Pop.

	During the analysis phase the vectorizer records some information
   per stmt in a "stmt_vec_info" struct which is attached to each stmt in the 
   loop, as well as general information about the loop as a whole, which is
   recorded in a "loop_vec_info" struct attached to each loop.

   Transformation phase:
   =====================
	The loop transformation phase scans all the stmts in the loop, and
   creates a vector stmt (or a sequence of stmts) for each scalar stmt S in
   the loop that needs to be vectorized. It insert the vector code sequence
   just before the scalar stmt S, and records a pointer to the vector code
   in STMT_VINFO_VEC_STMT (stmt_info) (stmt_info is the stmt_vec_info struct 
   attached to S). This pointer will be used for the vectorization of following
   stmts which use the def of stmt S. Stmt S is removed if it writes to memory;
   otherwise, we rely on dead code elimination for removing it.

	For example, say stmt S1 was vectorized into stmt VS1:

   VS1: vb = px[i];
   S1:	b = x[i];    STMT_VINFO_VEC_STMT (stmt_info (S1)) = VS1
   S2:  a = b;

   To vectorize stmt S2, the vectorizer first finds the stmt that defines
   the operand 'b' (S1), and gets the relevant vector def 'vb' from the
   vector stmt VS1 pointed by STMT_VINFO_VEC_STMT (stmt_info (S1)). The
   resulting sequence would be:

   VS1: vb = px[i];
   S1:	b = x[i];	STMT_VINFO_VEC_STMT (stmt_info (S1)) = VS1
   VS2: va = vb;
   S2:  a = b;          STMT_VINFO_VEC_STMT (stmt_info (S2)) = VS2

	Operands that are not SSA_NAMEs, are data-refs that appear in 
   load/store operations (like 'x[i]' in S1), and are handled differently.

   Target modeling:
   =================
	Currently the only target specific information that is used is the
   size of the vector (in bytes) - "UNITS_PER_SIMD_WORD". Targets that can 
   support different sizes of vectors, for now will need to specify one value 
   for "UNITS_PER_SIMD_WORD". More flexibility will be added in the future.

	Since we only vectorize operations which vector form can be
   expressed using existing tree codes, to verify that an operation is
   supported, the vectorizer checks the relevant optab at the relevant
   machine_mode (e.g, add_optab->handlers[(int) V8HImode].insn_code). If
   the value found is CODE_FOR_nothing, then there's no target support, and
   we can't vectorize the stmt.

   For additional information on this project see:
   http://gcc.gnu.org/projects/tree-ssa/vectorization.html
*/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "errors.h"
#include "ggc.h"
#include "tree.h"
#include "target.h"

#include "rtl.h"
#include "basic-block.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "tree-dump.h"
#include "timevar.h"
#include "cfgloop.h"
#include "cfglayout.h"
#include "expr.h"
#include "optabs.h"
#include "toplev.h"
#include "tree-chrec.h"
#include "tree-data-ref.h"
#include "tree-scalar-evolution.h"
#include "input.h"
#include "tree-vectorizer.h"
#include "tree-pass.h"
#include "langhooks.h"


/*************************************************************************
  Simple Loop Peeling Utilities
 *************************************************************************/
   
/* Entry point for peeling of simple loops.
   Peel the first/last iterations of a loop.
   It can be used outside of the vectorizer for loops that are simple enough
   (see function documentation).  In the vectorizer it is used to peel the
   last few iterations when the loop bound is unknown or does not evenly
   divide by the vectorization factor, and to peel the first few iterations
   to force the alignment of data references in the loop.  */
struct loop *slpeel_tree_peel_loop_to_edge 
  (struct loop *, struct loops *, edge, tree, tree, bool);
static struct loop *slpeel_tree_duplicate_loop_to_edge_cfg 
  (struct loop *, struct loops *, edge);
static void slpeel_update_phis_for_duplicate_loop 
  (struct loop *, struct loop *, bool after);
static void slpeel_update_phi_nodes_for_guard (edge, struct loop *, bool, bool);
static void slpeel_make_loop_iterate_ntimes (struct loop *, tree);
static edge slpeel_add_loop_guard (basic_block, tree, basic_block, basic_block);
static bool slpeel_can_duplicate_loop_p (struct loop *, edge);
static void allocate_new_names (bitmap);
static void rename_use_op (use_operand_p);
static void rename_def_op (def_operand_p, tree);
static void rename_variables_in_bb (basic_block);
static void free_new_names (bitmap);
static void rename_variables_in_loop (struct loop *);
#ifdef ENABLE_CHECKING
static void slpeel_verify_cfg_after_peeling (struct loop *, struct loop *);
#endif
static LOC find_loop_location (struct loop *);


/*************************************************************************
  Vectorization Utilities. 
 *************************************************************************/

/* Main analysis functions.  */
static loop_vec_info vect_analyze_loop (struct loop *);
static loop_vec_info vect_analyze_loop_form (struct loop *);
static bool vect_analyze_data_refs (loop_vec_info);
static bool vect_mark_stmts_to_be_vectorized (loop_vec_info);
static bool vect_analyze_scalar_cycles (loop_vec_info);
static bool vect_analyze_data_ref_accesses (loop_vec_info);
static bool vect_analyze_data_ref_dependence
  (struct data_reference *, struct data_reference *, loop_vec_info);
static bool vect_analyze_data_ref_dependences (loop_vec_info);
static bool vect_analyze_data_refs_alignment (loop_vec_info);
static bool vect_compute_data_refs_alignment (loop_vec_info);
static bool vect_analyze_operations (loop_vec_info);

/* Main code transformation functions.  */
static void vect_transform_loop (loop_vec_info, struct loops *);
static bool vect_transform_stmt (tree, block_stmt_iterator *);
static bool vectorizable_load (tree, block_stmt_iterator *, tree *);
static bool vectorizable_store (tree, block_stmt_iterator *, tree *);
static bool vectorizable_operation (tree, block_stmt_iterator *, tree *);
static bool vectorizable_assignment (tree, block_stmt_iterator *, tree *);
static enum dr_alignment_support vect_supportable_dr_alignment
  (struct data_reference *);
/* APPLE LOCAL begin AV cond expr. -dpatel */
/* Patch is waiting FSF review since mid Sep,  2004.
   New function, vectoriable_select.  */
static bool vectorizable_select (tree, block_stmt_iterator *, tree *);
/* APPLE LOCAL end AV cond expr. -dpatel */
static void vect_align_data_ref (tree);
static void vect_enhance_data_refs_alignment (loop_vec_info);

/* Utility functions for the analyses.  */
static bool vect_is_simple_use (tree , loop_vec_info, tree *);
/* APPLE LOCAL begin AV cond expr. -dpatel */
/* Patch is waiting FSF review since mid Sep,  2004.
   New function, vect_is_simple_cond.  */
static bool vect_is_simple_cond (tree, loop_vec_info);
/* APPLE LOCAL end AV cond expr. -dpatel */
static bool exist_non_indexing_operands_for_use_p (tree, tree);
static bool vect_is_simple_iv_evolution (unsigned, tree, tree *, tree *);
static void vect_mark_relevant (varray_type *, tree);
static bool vect_stmt_relevant_p (tree, loop_vec_info);
/* APPLE LOCAL loops-to-memset  */
extern tree vect_get_loop_niters (struct loop *, tree *);
static bool vect_compute_data_ref_alignment (struct data_reference *);
static bool vect_analyze_data_ref_access (struct data_reference *);
static bool vect_can_force_dr_alignment_p (tree, unsigned int);
/* APPLE LOCAL loops-to-memset  */
extern struct data_reference * vect_analyze_pointer_ref_access 
  (tree, tree, bool, tree, tree *, tree *);
static bool vect_can_advance_ivs_p (loop_vec_info);
static tree vect_get_ptr_offset (tree, tree, tree *);
static bool vect_analyze_offset_expr (tree, struct loop *, tree, tree *, 
				      tree *, tree *);
static tree vect_strip_conversion (tree);
static bool vect_base_addr_differ_p (struct data_reference *,
				     struct data_reference *drb, bool *);
static tree vect_object_analysis (tree, tree, bool, tree, 
				  struct data_reference **, tree *, tree *, 
				  tree *, bool *);
static tree vect_address_analysis (tree, tree, bool, tree, 
				   struct data_reference *, tree *, tree *, 
				   tree *, bool *);
static tree vect_get_memtag (tree, struct data_reference *);

/* Utility functions for the code transformation.  */
static tree vect_create_destination_var (tree, tree);
static tree vect_create_data_ref_ptr 
  (tree, block_stmt_iterator *, tree, tree *, bool); 
static tree vect_create_index_for_vector_ref (loop_vec_info);
static tree vect_create_addr_base_for_vector_ref (tree, tree *, tree);
static tree get_vectype_for_scalar_type (tree);
static tree vect_get_new_vect_var (tree, enum vect_var_kind, const char *);
static tree vect_get_vec_def_for_operand (tree, tree);
static tree vect_init_vector (tree, tree);
static void vect_finish_stmt_generation 
  (tree stmt, tree vec_stmt, block_stmt_iterator *bsi);

/* Utility function dealing with loop peeling (not peeling itself).  */
static void vect_generate_tmps_on_preheader 
  (loop_vec_info, tree *, tree *, tree *);
static tree vect_build_loop_niters (loop_vec_info);
static void vect_update_ivs_after_vectorizer (loop_vec_info, tree, edge); 
static tree vect_gen_niters_for_prolog_loop (loop_vec_info, tree);
static void vect_update_inits_of_dr (struct data_reference *, tree niters);
static void vect_update_inits_of_drs (loop_vec_info, tree);
static void vect_do_peeling_for_alignment (loop_vec_info, struct loops *);
static void vect_do_peeling_for_loop_bound 
  (loop_vec_info, tree *, struct loops *);

/* Utilities for creation and deletion of vec_info structs.  */
loop_vec_info new_loop_vec_info (struct loop *loop);
void destroy_loop_vec_info (loop_vec_info);
stmt_vec_info new_stmt_vec_info (tree, loop_vec_info);

/*************************************************************************
  Vectorization Debug Information.
 *************************************************************************/

/* vect_verbosity_level set to invalid verbosity level to mark that it's
   uninitialized.  */
enum verbosity_levels vect_verbosity_level = MAX_VERBOSITY_LEVEL;

/* vect_dump will be set to stderr or dump_file if exist.  */
FILE *vect_dump;

/* Utilities for output formatting. */
static bool vect_print_dump_info (enum verbosity_levels, LOC);
static void vect_set_dump_settings (void);
void vect_set_verbosity_level (const char *);


/* APPLE LOCAL begin AV data dependence. -dpatel */
/* Patch is waiting FSF review since mid Sep,  2004.
   New variable, loops_num.  */
static unsigned int loops_num;
/* APPLE LOCAL end AV data dependence. -dpatel */

/*************************************************************************
  Simple Loop Peeling Utilities

  Utilities to support loop peeling for vectorization purposes.
 *************************************************************************/

/* APPLE LOCAL begin AV data dependence. -dpatel */
/* Utilities for dependence analysis.  */
/* Patch is waiting FSF review since mid Sep,  2004.
   New function, vect_build_dist_vector.  */
static unsigned int vect_build_dist_vector (struct loop *,
					    struct data_dependence_relation *);
/* APPLE LOCAL end AV data dependence. -dpatel */

/* For each definition in DEFINITIONS this function allocates 
   new ssa name.  */

static void
allocate_new_names (bitmap definitions)
{
  unsigned ver;
  bitmap_iterator bi;

  EXECUTE_IF_SET_IN_BITMAP (definitions, 0, ver, bi)
    {
      tree def = ssa_name (ver);
      tree *new_name_ptr = xmalloc (sizeof (tree));

      bool abnormal = SSA_NAME_OCCURS_IN_ABNORMAL_PHI (def);

      *new_name_ptr = duplicate_ssa_name (def, SSA_NAME_DEF_STMT (def));
      SSA_NAME_OCCURS_IN_ABNORMAL_PHI (*new_name_ptr) = abnormal;

      SSA_NAME_AUX (def) = new_name_ptr;
    }
}


/* Renames the use *OP_P.  */

static void
rename_use_op (use_operand_p op_p)
{
  tree *new_name_ptr;

  if (TREE_CODE (USE_FROM_PTR (op_p)) != SSA_NAME)
    return;

  new_name_ptr = SSA_NAME_AUX (USE_FROM_PTR (op_p));

  /* Something defined outside of the loop.  */
  if (!new_name_ptr)
    return;

  /* An ordinary ssa name defined in the loop.  */

  SET_USE (op_p, *new_name_ptr);
}


/* Renames the def *OP_P in statement STMT.  */

static void
rename_def_op (def_operand_p op_p, tree stmt)
{
  tree *new_name_ptr;

  if (TREE_CODE (DEF_FROM_PTR (op_p)) != SSA_NAME)
    return;

  new_name_ptr = SSA_NAME_AUX (DEF_FROM_PTR (op_p));

  /* Something defined outside of the loop.  */
  if (!new_name_ptr)
    return;

  /* An ordinary ssa name defined in the loop.  */

  SET_DEF (op_p, *new_name_ptr);
  SSA_NAME_DEF_STMT (DEF_FROM_PTR (op_p)) = stmt;
}


/* Renames the variables in basic block BB.  */

static void
rename_variables_in_bb (basic_block bb)
{
  tree phi;
  block_stmt_iterator bsi;
  tree stmt;
  stmt_ann_t ann;
  use_optype uses;
  vuse_optype vuses;
  def_optype defs;
  v_may_def_optype v_may_defs;
  v_must_def_optype v_must_defs;
  unsigned i;
  edge e;
  edge_iterator ei;
  struct loop *loop = bb->loop_father;

  for (phi = phi_nodes (bb); phi; phi = PHI_CHAIN (phi))
    rename_def_op (PHI_RESULT_PTR (phi), phi);

  for (bsi = bsi_start (bb); !bsi_end_p (bsi); bsi_next (&bsi))
    {
      stmt = bsi_stmt (bsi);
      get_stmt_operands (stmt);
      ann = stmt_ann (stmt);

      uses = USE_OPS (ann);
      for (i = 0; i < NUM_USES (uses); i++)
	rename_use_op (USE_OP_PTR (uses, i));

      defs = DEF_OPS (ann);
      for (i = 0; i < NUM_DEFS (defs); i++)
	rename_def_op (DEF_OP_PTR (defs, i), stmt);

      vuses = VUSE_OPS (ann);
      for (i = 0; i < NUM_VUSES (vuses); i++)
	rename_use_op (VUSE_OP_PTR (vuses, i));

      v_may_defs = V_MAY_DEF_OPS (ann);
      for (i = 0; i < NUM_V_MAY_DEFS (v_may_defs); i++)
	{
	  rename_use_op (V_MAY_DEF_OP_PTR (v_may_defs, i));
	  rename_def_op (V_MAY_DEF_RESULT_PTR (v_may_defs, i), stmt);
	}

      v_must_defs = V_MUST_DEF_OPS (ann);
      for (i = 0; i < NUM_V_MUST_DEFS (v_must_defs); i++)
	{
	  rename_use_op (V_MUST_DEF_KILL_PTR (v_must_defs, i));
	  rename_def_op (V_MUST_DEF_RESULT_PTR (v_must_defs, i), stmt);
	}
    }

  FOR_EACH_EDGE (e, ei, bb->succs)
    {
      if (!flow_bb_inside_loop_p (loop, e->dest))
	continue;
      for (phi = phi_nodes (e->dest); phi; phi = PHI_CHAIN (phi))
        rename_use_op (PHI_ARG_DEF_PTR_FROM_EDGE (phi, e));
    }
}


/* Releases the structures holding the new ssa names.  */

static void
free_new_names (bitmap definitions)
{
  unsigned ver;
  bitmap_iterator bi;

  EXECUTE_IF_SET_IN_BITMAP (definitions, 0, ver, bi)
    {
      tree def = ssa_name (ver);

      if (SSA_NAME_AUX (def))
	{
	  free (SSA_NAME_AUX (def));
	  SSA_NAME_AUX (def) = NULL;
	}
    }
}


/* Renames variables in new generated LOOP.  */

static void
rename_variables_in_loop (struct loop *loop)
{
  unsigned i;
  basic_block *bbs;

  bbs = get_loop_body (loop);

  for (i = 0; i < loop->num_nodes; i++)
    rename_variables_in_bb (bbs[i]);

  free (bbs);
}


/* Update the PHI nodes of NEW_LOOP.

   NEW_LOOP is a duplicate of ORIG_LOOP.
   AFTER indicates whether NEW_LOOP executes before or after ORIG_LOOP:
   AFTER is true if NEW_LOOP executes after ORIG_LOOP, and false if it
   executes before it.  */

static void
slpeel_update_phis_for_duplicate_loop (struct loop *orig_loop,
				       struct loop *new_loop, bool after)
{
  tree *new_name_ptr, new_ssa_name;
  tree phi_new, phi_orig;
  tree def;
  edge orig_loop_latch = loop_latch_edge (orig_loop);
  edge orig_entry_e = loop_preheader_edge (orig_loop);
  edge new_loop_exit_e = new_loop->exit_edges[0];
  edge new_loop_entry_e = loop_preheader_edge (new_loop);
  edge entry_arg_e = (after ? orig_loop_latch : orig_entry_e);

  /*
     step 1. For each loop-header-phi:
             Add the first phi argument for the phi in NEW_LOOP
            (the one associated with the entry of NEW_LOOP)

     step 2. For each loop-header-phi:
             Add the second phi argument for the phi in NEW_LOOP
            (the one associated with the latch of NEW_LOOP)

     step 3. Update the phis in the successor block of NEW_LOOP.

        case 1: NEW_LOOP was placed before ORIG_LOOP:
                The successor block of NEW_LOOP is the header of ORIG_LOOP.
                Updating the phis in the successor block can therefore be done
                along with the scanning of the loop header phis, because the
                header blocks of ORIG_LOOP and NEW_LOOP have exactly the same
                phi nodes, organized in the same order.

        case 2: NEW_LOOP was placed after ORIG_LOOP:
                The successor block of NEW_LOOP is the original exit block of 
                ORIG_LOOP - the phis to be updated are the loop-closed-ssa phis.
                We postpone updating these phis to a later stage (when
                loop guards are added).
   */


  /* Scan the phis in the headers of the old and new loops
     (they are organized in exactly the same order).  */

  for (phi_new = phi_nodes (new_loop->header),
       phi_orig = phi_nodes (orig_loop->header);
       phi_new && phi_orig;
       phi_new = PHI_CHAIN (phi_new), phi_orig = PHI_CHAIN (phi_orig))
    {
      /* step 1.  */
      def = PHI_ARG_DEF_FROM_EDGE (phi_orig, entry_arg_e);
      add_phi_arg (phi_new, def, new_loop_entry_e);

      /* step 2.  */
      def = PHI_ARG_DEF_FROM_EDGE (phi_orig, orig_loop_latch);
      if (TREE_CODE (def) != SSA_NAME)
        continue;

      new_name_ptr = SSA_NAME_AUX (def);
      if (!new_name_ptr)
        /* Something defined outside of the loop.  */
        continue;

      /* An ordinary ssa name defined in the loop.  */
      new_ssa_name = *new_name_ptr;
      add_phi_arg (phi_new, new_ssa_name, loop_latch_edge (new_loop));

      /* step 3 (case 1).  */
      if (!after)
        {
          gcc_assert (new_loop_exit_e == orig_entry_e);
          SET_PHI_ARG_DEF (phi_orig,
                           new_loop_exit_e->dest_idx,
                           new_ssa_name);
        }
    }
}


/* Update PHI nodes for a guard of the LOOP.

   Input:
   - LOOP, GUARD_EDGE: LOOP is a loop for which we added guard code that
        controls whether LOOP is to be executed.  GUARD_EDGE is the edge that
        originates from the guard-bb, skips LOOP and reaches the (unique) exit
        bb of LOOP.  This loop-exit-bb is an empty bb with one successor.
        We denote this bb NEW_MERGE_BB because it had a single predecessor (the
        LOOP header) before the guard code was added, and now it became a merge
        point of two paths - the path that ends with the LOOP exit-edge, and
        the path that ends with GUARD_EDGE.

        This function creates and updates the relevant phi nodes to account for
        the new incoming edge (GUARD_EDGE) into NEW_MERGE_BB:
        1. Create phi nodes at NEW_MERGE_BB.
        2. Update the phi nodes at the successor of NEW_MERGE_BB (denoted
           UPDATE_BB).  UPDATE_BB was the exit-bb of LOOP before NEW_MERGE_BB
           was added:

        ===> The CFG before the guard-code was added:
        LOOP_header_bb:
          if (exit_loop) goto update_bb : LOOP_header_bb
        update_bb:

        ==> The CFG after the guard-code was added:
        guard_bb: 
          if (LOOP_guard_condition) goto new_merge_bb : LOOP_header_bb
        LOOP_header_bb:
          if (exit_loop_condition) goto new_merge_bb : LOOP_header_bb
        new_merge_bb:
          goto update_bb
        update_bb:

   - ENTRY_PHIS: If ENTRY_PHIS is TRUE, this indicates that the phis in 
        UPDATE_BB are loop entry phis, like the phis in the LOOP header,
        organized in the same order. 
        If ENTRY_PHIs is FALSE, this indicates that the phis in UPDATE_BB are
        loop exit phis.

   - IS_NEW_LOOP: TRUE if LOOP is a new loop (a duplicated copy of another
        "original" loop).  FALSE if LOOP is an original loop (not a newly 
        created copy).  The SSA_NAME_AUX fields of the defs in the original
        loop are the corresponding new ssa-names used in the new duplicated
        loop copy.  IS_NEW_LOOP indicates which of the two args of the phi 
        nodes in UPDATE_BB takes the original ssa-name, and which takes the 
        new name: If IS_NEW_LOOP is TRUE, the phi-arg that is associated with
        the LOOP-exit-edge takes the new-name, and the phi-arg that is 
        associated with GUARD_EDGE takes the original name.  If IS_NEW_LOOP is
        FALSE, it's the other way around.
  */

static void
slpeel_update_phi_nodes_for_guard (edge guard_edge, 
				   struct loop *loop,
				   bool entry_phis,
				   bool is_new_loop)
{
  tree orig_phi, new_phi, update_phi;
  tree guard_arg, loop_arg;
  basic_block new_merge_bb = guard_edge->dest;
  edge e = EDGE_SUCC (new_merge_bb, 0);
  basic_block update_bb = e->dest;
  basic_block orig_bb = (entry_phis ? loop->header : update_bb);

  for (orig_phi = phi_nodes (orig_bb), update_phi = phi_nodes (update_bb);
       orig_phi && update_phi;
       orig_phi = PHI_CHAIN (orig_phi), update_phi = PHI_CHAIN (update_phi))
    {
      /* 1. Generate new phi node in NEW_MERGE_BB:  */
      new_phi = create_phi_node (SSA_NAME_VAR (PHI_RESULT (orig_phi)),
                                 new_merge_bb);

      /* 2. NEW_MERGE_BB has two incoming edges: GUARD_EDGE and the exit-edge
            of LOOP. Set the two phi args in NEW_PHI for these edges:  */
      if (entry_phis)
        {
          loop_arg = PHI_ARG_DEF_FROM_EDGE (orig_phi,
                                            EDGE_SUCC (loop->latch, 0));
          guard_arg = PHI_ARG_DEF_FROM_EDGE (orig_phi, loop->entry_edges[0]);
        }
      else /* exit phis */
        {
          tree orig_def = PHI_ARG_DEF_FROM_EDGE (orig_phi, e);
          tree *new_name_ptr = SSA_NAME_AUX (orig_def);
          tree new_name;

          if (new_name_ptr)
            new_name = *new_name_ptr;
          else
            /* Something defined outside of the loop  */
            new_name = orig_def;

          if (is_new_loop)
            {
              guard_arg = orig_def;
              loop_arg = new_name;
            }
          else
            {
              guard_arg = new_name;
              loop_arg = orig_def;
            }
        }
      add_phi_arg (new_phi, loop_arg, loop->exit_edges[0]);
      add_phi_arg (new_phi, guard_arg, guard_edge);

      /* 3. Update phi in successor block.  */
      gcc_assert (PHI_ARG_DEF_FROM_EDGE (update_phi, e) == loop_arg
                  || PHI_ARG_DEF_FROM_EDGE (update_phi, e) == guard_arg);
      SET_PHI_ARG_DEF (update_phi, e->dest_idx, PHI_RESULT (new_phi));
    }

  set_phi_nodes (new_merge_bb, phi_reverse (phi_nodes (new_merge_bb)));
}


/* Make the LOOP iterate NITERS times. This is done by adding a new IV
   that starts at zero, increases by one and its limit is NITERS.

   Assumption: the exit-condition of LOOP is the last stmt in the loop.  */

static void
slpeel_make_loop_iterate_ntimes (struct loop *loop, tree niters)
{
  tree indx_before_incr, indx_after_incr, cond_stmt, cond;
  tree orig_cond;
  edge exit_edge = loop->exit_edges[0];
  block_stmt_iterator loop_cond_bsi;
  block_stmt_iterator incr_bsi;
  bool insert_after;
  tree begin_label = tree_block_label (loop->latch);
  tree exit_label = tree_block_label (loop->single_exit->dest);
  tree init = build_int_cst (TREE_TYPE (niters), 0);
  tree step = build_int_cst (TREE_TYPE (niters), 1);
  tree then_label;
  tree else_label;
  LOC loop_loc;

  orig_cond = get_loop_exit_condition (loop);
#ifdef ENABLE_CHECKING
  gcc_assert (orig_cond);
#endif
  loop_cond_bsi = bsi_for_stmt (orig_cond);

  standard_iv_increment_position (loop, &incr_bsi, &insert_after);
  create_iv (init, step, NULL_TREE, loop,
             &incr_bsi, insert_after, &indx_before_incr, &indx_after_incr);

  if (exit_edge->flags & EDGE_TRUE_VALUE) /* 'then' edge exits the loop.  */
    {
      cond = build2 (GE_EXPR, boolean_type_node, indx_after_incr, niters);
      then_label = build1 (GOTO_EXPR, void_type_node, exit_label);
      else_label = build1 (GOTO_EXPR, void_type_node, begin_label);
    }
  else /* 'then' edge loops back.  */
    {
      cond = build2 (LT_EXPR, boolean_type_node, indx_after_incr, niters);
      then_label = build1 (GOTO_EXPR, void_type_node, begin_label);
      else_label = build1 (GOTO_EXPR, void_type_node, exit_label);
    }

  cond_stmt = build3 (COND_EXPR, TREE_TYPE (orig_cond), cond,
		     then_label, else_label);
  bsi_insert_before (&loop_cond_bsi, cond_stmt, BSI_SAME_STMT);

  /* Remove old loop exit test:  */
  bsi_remove (&loop_cond_bsi);

  loop_loc = find_loop_location (loop);
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      if (loop_loc != UNKNOWN_LOC)
        fprintf (dump_file, "\nloop at %s:%d: ",
                 LOC_FILE (loop_loc), LOC_LINE (loop_loc));
      print_generic_expr (dump_file, cond_stmt, TDF_SLIM);
    }

  loop->nb_iterations = niters;
}


/* Given LOOP this function generates a new copy of it and puts it 
   on E which is either the entry or exit of LOOP.  */

static struct loop *
slpeel_tree_duplicate_loop_to_edge_cfg (struct loop *loop, struct loops *loops, 
					edge e)
{
  struct loop *new_loop;
  basic_block *new_bbs, *bbs;
  bool at_exit;
  bool was_imm_dom;
  basic_block exit_dest; 
  tree phi, phi_arg;

  at_exit = (e == loop->exit_edges[0]); 
  if (!at_exit && e != loop_preheader_edge (loop))
    return NULL;

  bbs = get_loop_body (loop);

  /* Check whether duplication is possible.  */
  if (!can_copy_bbs_p (bbs, loop->num_nodes))
    {
      free (bbs);
      return NULL;
    }

  /* Generate new loop structure.  */
  new_loop = duplicate_loop (loops, loop, loop->outer);
  if (!new_loop)
    {
      free (bbs);
      return NULL;
    }

  exit_dest = loop->exit_edges[0]->dest;
  was_imm_dom = (get_immediate_dominator (CDI_DOMINATORS, 
					  exit_dest) == loop->header ? 
		 true : false);

  new_bbs = xmalloc (sizeof (basic_block) * loop->num_nodes);

  copy_bbs (bbs, loop->num_nodes, new_bbs, NULL, 0, NULL, NULL);

  /* Duplicating phi args at exit bbs as coming 
     also from exit of duplicated loop.  */
  for (phi = phi_nodes (exit_dest); phi; phi = PHI_CHAIN (phi))
    {
      phi_arg = PHI_ARG_DEF_FROM_EDGE (phi, loop->exit_edges[0]);
      if (phi_arg)
	{
	  edge new_loop_exit_edge;

	  if (EDGE_SUCC (new_loop->header, 0)->dest == new_loop->latch)
	    new_loop_exit_edge = EDGE_SUCC (new_loop->header, 1);
	  else
	    new_loop_exit_edge = EDGE_SUCC (new_loop->header, 0);
  
	  add_phi_arg (phi, phi_arg, new_loop_exit_edge);	
	}
    }    
   
  if (at_exit) /* Add the loop copy at exit.  */
    {
      redirect_edge_and_branch_force (e, new_loop->header);
      set_immediate_dominator (CDI_DOMINATORS, new_loop->header, e->src);
      if (was_imm_dom)
	set_immediate_dominator (CDI_DOMINATORS, exit_dest, new_loop->header);
    }
  else /* Add the copy at entry.  */
    {
      edge new_exit_e;
      edge entry_e = loop_preheader_edge (loop);
      basic_block preheader = entry_e->src;
           
      if (!flow_bb_inside_loop_p (new_loop, 
				  EDGE_SUCC (new_loop->header, 0)->dest))
        new_exit_e = EDGE_SUCC (new_loop->header, 0);
      else
	new_exit_e = EDGE_SUCC (new_loop->header, 1); 

      redirect_edge_and_branch_force (new_exit_e, loop->header);
      set_immediate_dominator (CDI_DOMINATORS, loop->header,
			       new_exit_e->src);

      /* We have to add phi args to the loop->header here as coming 
	 from new_exit_e edge.  */
      for (phi = phi_nodes (loop->header); phi; phi = PHI_CHAIN (phi))
	{
	  phi_arg = PHI_ARG_DEF_FROM_EDGE (phi, entry_e);
	  if (phi_arg)
	    add_phi_arg (phi, phi_arg, new_exit_e);	
	}    

      redirect_edge_and_branch_force (entry_e, new_loop->header);
      set_immediate_dominator (CDI_DOMINATORS, new_loop->header, preheader);
    }

  flow_loop_scan (new_loop, LOOP_ALL);
  flow_loop_scan (loop, LOOP_ALL);  
  free (new_bbs);
  free (bbs);

  return new_loop;
}


/* Given the condition statement COND, put it as the last statement
   of GUARD_BB; EXIT_BB is the basic block to skip the loop;
   Assumes that this is the single exit of the guarded loop.  
   Returns the skip edge.  */

static edge
slpeel_add_loop_guard (basic_block guard_bb, tree cond, basic_block exit_bb,
		        basic_block dom_bb)
{
  block_stmt_iterator bsi;
  edge new_e, enter_e;
  tree cond_stmt, then_label, else_label;

  enter_e = EDGE_SUCC (guard_bb, 0);
  enter_e->flags &= ~EDGE_FALLTHRU;
  enter_e->flags |= EDGE_FALSE_VALUE;
  bsi = bsi_last (guard_bb);

  then_label = build1 (GOTO_EXPR, void_type_node,
                       tree_block_label (exit_bb));
  else_label = build1 (GOTO_EXPR, void_type_node,
                       tree_block_label (enter_e->dest));
  cond_stmt = build3 (COND_EXPR, void_type_node, cond,
   		     then_label, else_label);
  bsi_insert_after (&bsi, cond_stmt, BSI_NEW_STMT);
  /* Add new edge to connect entry block to the second loop.  */
  new_e = make_edge (guard_bb, exit_bb, EDGE_TRUE_VALUE);
  set_immediate_dominator (CDI_DOMINATORS, exit_bb, dom_bb);
  return new_e;
}


/* This function verifies that the following restrictions apply to LOOP:
   (1) it is innermost
   (2) it consists of exactly 2 basic blocks - header, and an empty latch.
   (3) it is single entry, single exit
   (4) its exit condition is the last stmt in the header
   (5) E is the entry/exit edge of LOOP.
 */

static bool
slpeel_can_duplicate_loop_p (struct loop *loop, edge e)
{
  edge exit_e = loop->exit_edges [0];
  edge entry_e = loop_preheader_edge (loop);
  tree orig_cond = get_loop_exit_condition (loop);
  block_stmt_iterator loop_exit_bsi = bsi_last (exit_e->src);

  if (any_marked_for_rewrite_p ())
    return false;

  if (loop->inner
      /* All loops have an outer scope; the only case loop->outer is NULL is for
         the function itself.  */
      || !loop->outer
      || loop->num_nodes != 2
      || !empty_block_p (loop->latch)
      || loop->num_exits != 1
      || loop->num_entries != 1
      /* Verify that new loop exit condition can be trivially modified.  */
      || (!orig_cond || orig_cond != bsi_stmt (loop_exit_bsi))
      || (e != exit_e && e != entry_e))
    return false;

  return true;
}

#ifdef ENABLE_CHECKING
static void
slpeel_verify_cfg_after_peeling (struct loop *first_loop,
                                 struct loop *second_loop)
{
  basic_block loop1_exit_bb = first_loop->exit_edges[0]->dest;
  basic_block loop2_entry_bb = second_loop->pre_header;
  basic_block loop1_entry_bb = loop_preheader_edge (first_loop)->src;

  /* A guard that controls whether the second_loop is to be executed or skipped
     is placed in first_loop->exit.  first_loopt->exit therefore has two
     successors - one is the preheader of second_loop, and the other is a bb
     after second_loop.
   */
  gcc_assert (EDGE_COUNT (loop1_exit_bb->succs) == 2);
   
   
  /* 1. Verify that one of the successors of first_loopt->exit is the preheader
        of second_loop.  */
   
  /* The preheader of new_loop is expected to have two predessors:
     first_loop->exit and the block that precedes first_loop.  */

  gcc_assert (EDGE_COUNT (loop2_entry_bb->preds) == 2 
              && ((EDGE_PRED (loop2_entry_bb, 0)->src == loop1_exit_bb
                   && EDGE_PRED (loop2_entry_bb, 1)->src == loop1_entry_bb)
               || (EDGE_PRED (loop2_entry_bb, 1)->src ==  loop1_exit_bb
                   && EDGE_PRED (loop2_entry_bb, 0)->src == loop1_entry_bb)));
  
  /* Verify that the other successor of first_loopt->exit is after the
     second_loop.  */
  /* TODO */
}
#endif

/* Function slpeel_tree_peel_loop_to_edge.

   Peel the first (last) iterations of LOOP into a new prolog (epilog) loop
   that is placed on the entry (exit) edge E of LOOP. After this transformation
   we have two loops one after the other - first-loop iterates FIRST_NITERS
   times, and second-loop iterates the remainder NITERS - FIRST_NITERS times.

   Input:
   - LOOP: the loop to be peeled.
   - E: the exit or entry edge of LOOP.
        If it is the entry edge, we peel the first iterations of LOOP. In this
        case first-loop is LOOP, and second-loop is the newly created loop.
        If it is the exit edge, we peel the last iterations of LOOP. In this
        case, first-loop is the newly created loop, and second-loop is LOOP.
   - NITERS: the number of iterations that LOOP iterates.
   - FIRST_NITERS: the number of iterations that the first-loop should iterate.
   - UPDATE_FIRST_LOOP_COUNT:  specified whether this function is responsible
        for updating the loop bound of the first-loop to FIRST_NITERS.  If it
        is false, the caller of this function may want to take care of this
        (this can be useful if we don't want new stmts added to first-loop).

   Output:
   The function returns a pointer to the new loop-copy, or NULL if it failed
   to perform the transformation.

   The function generates two if-then-else guards: one before the first loop,
   and the other before the second loop:
   The first guard is:
     if (FIRST_NITERS == 0) then skip the first loop,
     and go directly to the second loop.
   The second guard is:
     if (FIRST_NITERS == NITERS) then skip the second loop.

   FORNOW only simple loops are supported (see slpeel_can_duplicate_loop_p).
   FORNOW the resulting code will not be in loop-closed-ssa form.
*/

struct loop*
slpeel_tree_peel_loop_to_edge (struct loop *loop, struct loops *loops, 
			       edge e, tree first_niters, 
			       tree niters, bool update_first_loop_count)
{
  struct loop *new_loop = NULL, *first_loop, *second_loop;
  edge skip_e;
  tree pre_condition;
  bitmap definitions;
  basic_block bb_before_second_loop, bb_after_second_loop;
  basic_block bb_before_first_loop;
  basic_block bb_between_loops;
  edge exit_e = loop->exit_edges [0];
  LOC loop_loc;
  
  if (!slpeel_can_duplicate_loop_p (loop, e))
    return NULL;
  
  /* We have to initialize cfg_hooks. Then, when calling
   cfg_hooks->split_edge, the function tree_split_edge 
   is actually called and, when calling cfg_hooks->duplicate_block,
   the function tree_duplicate_bb is called.  */
  tree_register_cfg_hooks ();


  /* 1. Generate a copy of LOOP and put it on E (E is the entry/exit of LOOP).
        Resulting CFG would be:

        first_loop:
        do {
        } while ...

        second_loop:
        do {
        } while ...

        orig_exit_bb:
   */
  
  if (!(new_loop = slpeel_tree_duplicate_loop_to_edge_cfg (loop, loops, e)))
    {
      loop_loc = find_loop_location (loop);
      if (dump_file && (dump_flags & TDF_DETAILS))
        {
          if (loop_loc != UNKNOWN_LOC)
            fprintf (dump_file, "\n%s:%d: note: ",
                     LOC_FILE (loop_loc), LOC_LINE (loop_loc));
          fprintf (dump_file, "tree_duplicate_loop_to_edge_cfg failed.\n");
        }
      return NULL;
    }
  
  if (e == exit_e)
    {
      /* NEW_LOOP was placed after LOOP.  */
      first_loop = loop;
      second_loop = new_loop;
    }
  else
    {
      /* NEW_LOOP was placed before LOOP.  */
      first_loop = new_loop;
      second_loop = loop;
    }

  definitions = marked_ssa_names ();
  allocate_new_names (definitions);
  slpeel_update_phis_for_duplicate_loop (loop, new_loop, e == exit_e);
  rename_variables_in_loop (new_loop);


  /* 2. Add the guard that controls whether the first loop is executed.
        Resulting CFG would be:

        bb_before_first_loop:
        if (FIRST_NITERS == 0) GOTO bb_before_second_loop
                               GOTO first-loop

        first_loop:
        do {
        } while ...

        bb_before_second_loop:

        second_loop:
        do {
        } while ...

        orig_exit_bb:
   */

  bb_before_first_loop = split_edge (loop_preheader_edge (first_loop));
  add_bb_to_loop (bb_before_first_loop, first_loop->outer);
  bb_before_second_loop = split_edge (first_loop->exit_edges[0]);
  add_bb_to_loop (bb_before_second_loop, first_loop->outer);
  flow_loop_scan (first_loop, LOOP_ALL);
  flow_loop_scan (second_loop, LOOP_ALL);

  pre_condition =
        build2 (LE_EXPR, boolean_type_node, first_niters, integer_zero_node);
  skip_e = slpeel_add_loop_guard (bb_before_first_loop, pre_condition,
                                  bb_before_second_loop, bb_before_first_loop);
  slpeel_update_phi_nodes_for_guard (skip_e, first_loop, true /* entry-phis */,
                                     first_loop == new_loop);


  /* 3. Add the guard that controls whether the second loop is executed.
        Resulting CFG would be:

        bb_before_first_loop:
        if (FIRST_NITERS == 0) GOTO bb_before_second_loop (skip first loop)
                               GOTO first-loop

        first_loop:
        do {
        } while ...

        bb_between_loops:
        if (FIRST_NITERS == NITERS) GOTO bb_after_second_loop (skip second loop)
                                    GOTO bb_before_second_loop

        bb_before_second_loop:

        second_loop:
        do {
        } while ...

        bb_after_second_loop:

        orig_exit_bb:
   */

  bb_between_loops = split_edge (first_loop->exit_edges[0]);
  add_bb_to_loop (bb_between_loops, first_loop->outer);
  bb_after_second_loop = split_edge (second_loop->exit_edges[0]);
  add_bb_to_loop (bb_after_second_loop, second_loop->outer);
  flow_loop_scan (first_loop, LOOP_ALL);
  flow_loop_scan (second_loop, LOOP_ALL);

  pre_condition = build2 (EQ_EXPR, boolean_type_node, first_niters, niters);
  skip_e = slpeel_add_loop_guard (bb_between_loops, pre_condition,
                                  bb_after_second_loop, bb_before_first_loop);
  slpeel_update_phi_nodes_for_guard (skip_e, second_loop, false /* exit-phis */,
                                     second_loop == new_loop);

  /* Flow loop scan does not update loop->single_exit field.  */
  first_loop->single_exit = first_loop->exit_edges[0];
  second_loop->single_exit = second_loop->exit_edges[0];

  /* 4. Make first-loop iterate FIRST_NITERS times, if requested.
   */
  if (update_first_loop_count)
    slpeel_make_loop_iterate_ntimes (first_loop, first_niters);

  free_new_names (definitions);
  BITMAP_XFREE (definitions);
  unmark_all_for_rewrite ();

  return new_loop;
}

/* Function vect_get_loop_location.

   Extract the location of the loop in the source code.
   If the loop is not well formed for vectorization, an estimated
   location is calculated.
   Return the loop location if succeed and NULL if not.  */

static LOC
find_loop_location (struct loop *loop)
{
  tree node = NULL_TREE;
  basic_block bb;
  block_stmt_iterator si;

  if (!loop)
    return UNKNOWN_LOC;

  node = get_loop_exit_condition (loop);

  if (node && EXPR_P (node) && EXPR_HAS_LOCATION (node)
      && EXPR_FILENAME (node) && EXPR_LINENO (node))
    return EXPR_LOC (node);

  /* If we got here the loop is probably not "well formed",
     try to estimate the loop location */

  if (!loop->header)
    return UNKNOWN_LOC;

  bb = loop->header;

  for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
    {
      node = bsi_stmt (si);
      if (node && EXPR_P (node) && EXPR_HAS_LOCATION (node))
        return EXPR_LOC (node);
    }

  return UNKNOWN_LOC;
}


/*************************************************************************
  Vectorization Debug Information.
 *************************************************************************/

/* Function vect_set_verbosity_level.

   Called from toplev.c upon detection of the
   -ftree-vectorizer-verbose=N option.  */

void
vect_set_verbosity_level (const char *val)
{
   unsigned int vl;

   vl = atoi (val);
   if (vl < MAX_VERBOSITY_LEVEL)
     vect_verbosity_level = vl;
   else
     vect_verbosity_level = MAX_VERBOSITY_LEVEL - 1;
}


/* Function vect_set_dump_settings.

   Fix the verbosity level of the vectorizer if the
   requested level was not set explicitly using the flag
   -ftree-vectorizer-verbose=N.
   Decide where to print the debugging information (dump_file/stderr).
   If the user defined the verbosity level, but there is no dump file,
   print to stderr, otherwise print to the dump file.  */

static void
vect_set_dump_settings (void)
{
  vect_dump = dump_file;

  /* Check if the verbosity level was defined by the user:  */
  if (vect_verbosity_level != MAX_VERBOSITY_LEVEL)
    {
      /* If there is no dump file, print to stderr.  */
      if (!dump_file)
        vect_dump = stderr;
      return;
    }

  /* User didn't specify verbosity level:  */
  if (dump_file && (dump_flags & TDF_DETAILS))
    vect_verbosity_level = REPORT_DETAILS;
  else if (dump_file && (dump_flags & TDF_STATS))
    vect_verbosity_level = REPORT_UNVECTORIZED_LOOPS;
  else
    vect_verbosity_level = REPORT_NONE;

  gcc_assert (dump_file || vect_verbosity_level == REPORT_NONE);
}


/* Function debug_loop_details.

   For vectorization debug dumps.  */

static bool
vect_print_dump_info (enum verbosity_levels vl, LOC loc)
{
  if (vl > vect_verbosity_level)
    return false;

  if (loc == UNKNOWN_LOC)
    fprintf (vect_dump, "\n%s:%d: note: ",
		 DECL_SOURCE_FILE (current_function_decl),
		 DECL_SOURCE_LINE (current_function_decl));
  else
    fprintf (vect_dump, "\n%s:%d: note: ", LOC_FILE (loc), LOC_LINE (loc));


  return true;
}



/* Here the proper Vectorizer starts.  */

/*************************************************************************
  Vectorization Utilities.
 *************************************************************************/

/* Function new_stmt_vec_info.

   Create and initialize a new stmt_vec_info struct for STMT.  */

stmt_vec_info
new_stmt_vec_info (tree stmt, loop_vec_info loop_vinfo)
{
  stmt_vec_info res;
  res = (stmt_vec_info) xcalloc (1, sizeof (struct _stmt_vec_info));

  STMT_VINFO_TYPE (res) = undef_vec_info_type;
  STMT_VINFO_STMT (res) = stmt;
  STMT_VINFO_LOOP_VINFO (res) = loop_vinfo;
  STMT_VINFO_RELEVANT_P (res) = 0;
  STMT_VINFO_VECTYPE (res) = NULL;
  STMT_VINFO_VEC_STMT (res) = NULL;
  STMT_VINFO_DATA_REF (res) = NULL;
  STMT_VINFO_MEMTAG (res) = NULL;
  STMT_VINFO_VECT_DR_BASE_ADDRESS (res) = NULL;
  STMT_VINFO_VECT_INIT_OFFSET (res) = NULL_TREE;
  STMT_VINFO_VECT_STEP (res) = NULL_TREE;
  STMT_VINFO_VECT_BASE_ALIGNED_P (res) = false;
  STMT_VINFO_VECT_MISALIGNMENT (res) = NULL_TREE;

  return res;
}


/* Function new_loop_vec_info.

   Create and initialize a new loop_vec_info struct for LOOP, as well as
   stmt_vec_info structs for all the stmts in LOOP.  */

loop_vec_info
new_loop_vec_info (struct loop *loop)
{
  loop_vec_info res;
  basic_block *bbs;
  block_stmt_iterator si;
  unsigned int i;

  res = (loop_vec_info) xcalloc (1, sizeof (struct _loop_vec_info));

  bbs = get_loop_body (loop);

  /* Create stmt_info for all stmts in the loop.  */
  for (i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = bbs[i];
      for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
	{
	  tree stmt = bsi_stmt (si);
	  stmt_ann_t ann;

	  get_stmt_operands (stmt);
	  ann = stmt_ann (stmt);
	  set_stmt_info (ann, new_stmt_vec_info (stmt, res));
	}
    }

  LOOP_VINFO_LOOP (res) = loop;
  LOOP_VINFO_BBS (res) = bbs;
  LOOP_VINFO_EXIT_COND (res) = NULL;
  LOOP_VINFO_NITERS (res) = NULL;
  LOOP_VINFO_VECTORIZABLE_P (res) = 0;
  LOOP_DO_PEELING_FOR_ALIGNMENT (res) = false;
  LOOP_VINFO_VECT_FACTOR (res) = 0;
  VARRAY_GENERIC_PTR_INIT (LOOP_VINFO_DATAREF_WRITES (res), 20,
			   "loop_write_datarefs");
  VARRAY_GENERIC_PTR_INIT (LOOP_VINFO_DATAREF_READS (res), 20,
			   "loop_read_datarefs");
  LOOP_VINFO_UNALIGNED_DR (res) = NULL;
  LOOP_VINFO_LOC (res) = UNKNOWN_LOC;

  return res;
}


/* Function destroy_loop_vec_info.
 
   Free LOOP_VINFO struct, as well as all the stmt_vec_info structs of all the 
   stmts in the loop.  */

void
destroy_loop_vec_info (loop_vec_info loop_vinfo)
{
  struct loop *loop;
  basic_block *bbs;
  int nbbs;
  block_stmt_iterator si;
  int j;

  if (!loop_vinfo)
    return;

  loop = LOOP_VINFO_LOOP (loop_vinfo);

  bbs = LOOP_VINFO_BBS (loop_vinfo);
  nbbs = loop->num_nodes;

  for (j = 0; j < nbbs; j++)
    {
      basic_block bb = bbs[j];
      for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
	{
	  tree stmt = bsi_stmt (si);
	  stmt_ann_t ann = stmt_ann (stmt);
	  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
	  free (stmt_info);
	  set_stmt_info (ann, NULL);
	}
    }

  free (LOOP_VINFO_BBS (loop_vinfo));
  varray_clear (LOOP_VINFO_DATAREF_WRITES (loop_vinfo));
  varray_clear (LOOP_VINFO_DATAREF_READS (loop_vinfo));

  free (loop_vinfo);
}


/* Function vect_get_ptr_offset

   Compute the OFFSET modulo vector-type alignment of pointer REF in bits.  */

static tree 
vect_get_ptr_offset (tree ref ATTRIBUTE_UNUSED, 
		     tree vectype ATTRIBUTE_UNUSED, 
		     tree *offset ATTRIBUTE_UNUSED)
{
  /* TODO: Use alignment information.  */
  return NULL_TREE; 
}


/* Function vect_strip_conversions

   Strip conversions that don't narrow the mode.  */

static tree 
vect_strip_conversion (tree expr)
{
  tree to, ti, oprnd0;
  
  while (TREE_CODE (expr) == NOP_EXPR || TREE_CODE (expr) == CONVERT_EXPR)
    {
      to = TREE_TYPE (expr);
      oprnd0 = TREE_OPERAND (expr, 0);
      ti = TREE_TYPE (oprnd0);
 
      if (!INTEGRAL_TYPE_P (to) || !INTEGRAL_TYPE_P (ti))
	return NULL_TREE;
      if (GET_MODE_SIZE (TYPE_MODE (to)) < GET_MODE_SIZE (TYPE_MODE (ti)))
	return NULL_TREE;
      
      expr = oprnd0;
    }
  return expr; 
}


/* Function vect_analyze_offset_expr

   Given an offset expression EXPR received from get_inner_reference, analyze
   it and create an expression for INITIAL_OFFSET by substituting the variables 
   of EXPR with initial_condition of the corresponding access_fn in the loop. 
   E.g., 
      for i
         for (j = 3; j < N; j++)
            a[j].b[i][j] = 0;
	 
   For a[j].b[i][j], EXPR will be 'i * C_i + j * C_j + C'. 'i' cannot be 
   substituted, since its access_fn in the inner loop is i. 'j' will be 
   substituted with 3. An INITIAL_OFFSET will be 'i * C_i + C`', where
   C` =  3 * C_j + C.

   Compute MISALIGN (the misalignment of the data reference initial access from
   its base) if possible. Misalignment can be calculated only if all the
   variables can be substituted with constants, or if a variable is multiplied
   by a multiple of VECTYPE_ALIGNMENT. In the above example, since 'i' cannot
   be substituted, MISALIGN will be NULL_TREE in case that C_i is not a multiple
   of VECTYPE_ALIGNMENT, and C` otherwise. (We perform MISALIGN modulo 
   VECTYPE_ALIGNMENT computation in the caller of this function).

   STEP is an evolution of the data reference in this loop in bytes.
   In the above example, STEP is C_j.

   Return FALSE, if the analysis fails, e.g., there is no access_fn for a 
   variable. In this case, all the outputs (INITIAL_OFFSET, MISALIGN and STEP) 
   are NULL_TREEs. Otherwise, return TRUE.

*/

static bool
vect_analyze_offset_expr (tree expr, 
			  struct loop *loop, 
			  tree vectype_alignment,
			  tree *initial_offset,
			  tree *misalign,
			  tree *step)
{
  tree oprnd0;
  tree oprnd1;
  tree left_offset = ssize_int (0);
  tree right_offset = ssize_int (0);
  tree left_misalign = ssize_int (0);
  tree right_misalign = ssize_int (0);
  tree left_step = ssize_int (0);
  tree right_step = ssize_int (0);
  enum tree_code code;
  tree init, evolution;

  *step = NULL_TREE;
  *misalign = NULL_TREE;
  *initial_offset = NULL_TREE;

  /* Strip conversions that don't narrow the mode.  */
  expr = vect_strip_conversion (expr);
  if (!expr)
    return false;

  /* Stop conditions:
     1. Constant.  */
  if (TREE_CODE (expr) == INTEGER_CST)
    {
      *initial_offset = fold_convert (ssizetype, expr);
      *misalign = fold_convert (ssizetype, expr);      
      *step = ssize_int (0);
      return true;
    }

  /* 2. Variable. Try to substitute with initial_condition of the corresponding
     access_fn in the current loop.  */
  if (SSA_VAR_P (expr))
    {
      tree access_fn = analyze_scalar_evolution (loop, expr);

      if (access_fn == chrec_dont_know)
	/* No access_fn.  */
	return false;

      init = initial_condition_in_loop_num (access_fn, loop->num);
      if (init == expr && !expr_invariant_in_loop_p (loop, init))
	/* Not enough information: may be not loop invariant.  
	   E.g., for a[b[i]], we get a[D], where D=b[i]. EXPR is D, its 
	   initial_condition is D, but it depends on i - loop's induction
	   variable.  */	  
	return false;

      evolution = evolution_part_in_loop_num (access_fn, loop->num);
      if (evolution && TREE_CODE (evolution) != INTEGER_CST)
	/* Evolution is not constant.  */
	return false;

      if (TREE_CODE (init) == INTEGER_CST)
	*misalign = fold_convert (ssizetype, init);
      else
	/* Not constant, misalignment cannot be calculated.  */
	*misalign = NULL_TREE;

      *initial_offset = fold_convert (ssizetype, init); 

      *step = evolution ? fold_convert (ssizetype, evolution) : ssize_int (0);
      return true;      
    }

  /* Recursive computation.  */
  if (!BINARY_CLASS_P (expr))
    {
      /* We expect to get binary expressions (PLUS/MINUS and MULT).  */
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        {
	  fprintf (vect_dump, "Not binary expression ");
          print_generic_expr (vect_dump, expr, TDF_SLIM);
	}
      return false;
    }
  oprnd0 = TREE_OPERAND (expr, 0);
  oprnd1 = TREE_OPERAND (expr, 1);

  if (!vect_analyze_offset_expr (oprnd0, loop, vectype_alignment, &left_offset, 
				&left_misalign, &left_step)
      || !vect_analyze_offset_expr (oprnd1, loop, vectype_alignment, 
				   &right_offset, &right_misalign, &right_step))
    return false;

  /* The type of the operation: plus, minus or mult.  */
  code = TREE_CODE (expr);
  switch (code)
    {
    case MULT_EXPR:
      if (TREE_CODE (right_offset) != INTEGER_CST)
	/* RIGHT_OFFSET can be not constant. For example, for arrays of variable 
	   sized types. 
	   FORNOW: We don't support such cases.  */
	return false;

      /* Strip conversions that don't narrow the mode.  */
      left_offset = vect_strip_conversion (left_offset);      
      if (!left_offset)
	return false;      
      /* Misalignment computation.  */
      if (SSA_VAR_P (left_offset))
	{
	  /* If the left side contains variables that can't be substituted with 
	     constants, we check if the right side is a multiple of ALIGNMENT.
	   */
	  if (integer_zerop (size_binop (TRUNC_MOD_EXPR, right_offset, 
			          fold_convert (ssizetype, vectype_alignment))))
	    *misalign = ssize_int (0);
	  else
	    /* If the remainder is not zero or the right side isn't constant,
	       we can't compute  misalignment.  */
	    *misalign = NULL_TREE;
	}
      else 
	{
	  /* The left operand was successfully substituted with constant.  */	  
	  if (left_misalign)
	    /* In case of EXPR '(i * C1 + j) * C2', LEFT_MISALIGN is 
	       NULL_TREE.  */
	    *misalign  = size_binop (code, left_misalign, right_misalign);
	  else
	    *misalign = NULL_TREE; 
	}

      /* Step calculation.  */
      /* Multiply the step by the right operand.  */
      *step  = size_binop (MULT_EXPR, left_step, right_offset);
      break;
   
    case PLUS_EXPR:
    case MINUS_EXPR:
      /* Combine the recursive calculations for step and misalignment.  */
      *step = size_binop (code, left_step, right_step);
   
      if (left_misalign && right_misalign)
	*misalign  = size_binop (code, left_misalign, right_misalign);
      else
	*misalign = NULL_TREE;
    
      break;

    default:
      gcc_unreachable ();
    }

  /* Compute offset.  */
  *initial_offset = fold_convert (ssizetype, 
				  fold (build2 (code, TREE_TYPE (left_offset), 
						left_offset, 
						right_offset)));
  return true;
}


/* Function vect_force_dr_alignment_p.

   Returns whether the alignment of a DECL can be forced to be aligned
   on ALIGNMENT bit boundary.  */

static bool 
vect_can_force_dr_alignment_p (tree decl, unsigned int alignment)
{
  if (TREE_CODE (decl) != VAR_DECL)
    return false;

  if (DECL_EXTERNAL (decl))
    return false;

  if (TREE_ASM_WRITTEN (decl))
    return false;

  if (TREE_STATIC (decl))
    return (alignment <= MAX_OFILE_ALIGNMENT);
  else
    /* This is not 100% correct.  The absolute correct stack alignment
       is STACK_BOUNDARY.  We're supposed to hope, but not assume, that
       PREFERRED_STACK_BOUNDARY is honored by all translation units.
       However, until someone implements forced stack alignment, SSE
       isn't really usable without this.  */  
    return (alignment <= PREFERRED_STACK_BOUNDARY); 
}


/* Function vect_get_new_vect_var.

   Returns a name for a new variable. The current naming scheme appends the 
   prefix "vect_" or "vect_p" (depending on the value of VAR_KIND) to 
   the name of vectorizer generated variables, and appends that to NAME if 
   provided.  */

static tree
vect_get_new_vect_var (tree type, enum vect_var_kind var_kind, const char *name)
{
  const char *prefix;
  int prefix_len;
  tree new_vect_var;

  if (var_kind == vect_simple_var)
    prefix = "vect_"; 
  else
    prefix = "vect_p";

  prefix_len = strlen (prefix);

  if (name)
    new_vect_var = create_tmp_var (type, concat (prefix, name, NULL));
  else
    new_vect_var = create_tmp_var (type, prefix);

  return new_vect_var;
}


/* Function vect_create_index_for_vector_ref.

   Create (and return) an index variable, along with it's update chain in the
   loop. This variable will be used to access a memory location in a vector
   operation.

   Input:
   LOOP: The loop being vectorized.
   BSI: The block_stmt_iterator where STMT is. Any new stmts created by this
        function can be added here, or in the loop pre-header.

   Output:
   Return an index that will be used to index a vector array.  It is expected
   that a pointer to the first vector will be used as the base address for the
   indexed reference.

   FORNOW: we are not trying to be efficient, just creating a new index each
   time from scratch.  At this time all vector references could use the same
   index.

   TODO: create only one index to be used by all vector references.  Record
   the index in the LOOP_VINFO the first time this procedure is called and
   return it on subsequent calls.  The increment of this index must be placed
   just before the conditional expression that ends the single block loop.  */

static tree
vect_create_index_for_vector_ref (loop_vec_info loop_vinfo)
{
  tree init, step;
  block_stmt_iterator incr_bsi;
  bool insert_after;
  tree indx_before_incr, indx_after_incr;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree incr;

  /* It is assumed that the base pointer used for vectorized access contains
     the address of the first vector.  Therefore the index used for vectorized
     access must be initialized to zero and incremented by 1.  */

  init = integer_zero_node;
  step = integer_one_node;

  standard_iv_increment_position (loop, &incr_bsi, &insert_after);
  create_iv (init, step, NULL_TREE, loop, &incr_bsi, insert_after,
	&indx_before_incr, &indx_after_incr);
  incr = bsi_stmt (incr_bsi);
  get_stmt_operands (incr);
  set_stmt_info (stmt_ann (incr), new_stmt_vec_info (incr, loop_vinfo));

  return indx_before_incr;
}


/* Function vect_create_addr_base_for_vector_ref.

   Create an expression that computes the address of the first memory location
   that will be accessed for a data reference.

   Input:
   STMT: The statement containing the data reference.
   NEW_STMT_LIST: Must be initialized to NULL_TREE or a statement list.
   OFFSET: Optional. If supplied, it is be added to the initial address.

   Output:
   1. Return an SSA_NAME whose value is the address of the memory location of 
      the first vector of the data reference.
   2. If new_stmt_list is not NULL_TREE after return then the caller must insert
      these statement(s) which define the returned SSA_NAME.

   FORNOW: We are only handling array accesses with step 1.  */

static tree
vect_create_addr_base_for_vector_ref (tree stmt,
                                      tree *new_stmt_list,
				      tree offset)
{
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  struct data_reference *dr = STMT_VINFO_DATA_REF (stmt_info);
  tree data_ref_base = 
    unshare_expr (STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info));
  tree base_name = build_fold_indirect_ref (data_ref_base);
  tree ref = DR_REF (dr);
  tree scalar_type = TREE_TYPE (ref);
  tree scalar_ptr_type = build_pointer_type (scalar_type);
  tree vec_stmt;
  tree new_temp;
  tree addr_base, addr_expr;
  tree dest, new_stmt;
  tree base_offset = unshare_expr (STMT_VINFO_VECT_INIT_OFFSET (stmt_info));

  /* Create base_offset */
  dest = create_tmp_var (TREE_TYPE (base_offset), "base_off");
  add_referenced_tmp_var (dest);
  base_offset = force_gimple_operand (base_offset, &new_stmt, false, dest);  
  append_to_statement_list_force (new_stmt, new_stmt_list);

  if (offset)
    {
      tree tmp = create_tmp_var (TREE_TYPE (base_offset), "offset");
      add_referenced_tmp_var (tmp);
      offset = fold (build2 (MULT_EXPR, TREE_TYPE (offset), offset, 
			     STMT_VINFO_VECT_STEP (stmt_info)));
      base_offset = fold (build2 (PLUS_EXPR, TREE_TYPE (base_offset), 
				  base_offset, offset));
      base_offset = force_gimple_operand (base_offset, &new_stmt, false, tmp);  
      append_to_statement_list_force (new_stmt, new_stmt_list);
    }
  
  /* base + base_offset */
  addr_base = fold (build2 (PLUS_EXPR, TREE_TYPE (data_ref_base), data_ref_base, 
			    base_offset));

  /* addr_expr = addr_base */
  addr_expr = vect_get_new_vect_var (scalar_ptr_type, vect_pointer_var,
                                     get_name (base_name));
  add_referenced_tmp_var (addr_expr);
  vec_stmt = build2 (MODIFY_EXPR, void_type_node, addr_expr, addr_base);
  new_temp = make_ssa_name (addr_expr, vec_stmt);
  TREE_OPERAND (vec_stmt, 0) = new_temp;
  append_to_statement_list_force (vec_stmt, new_stmt_list);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "created ");
      print_generic_expr (vect_dump, vec_stmt, TDF_SLIM);
    }
  return new_temp;
}


/* Function get_vectype_for_scalar_type.

   Returns the vector type corresponding to SCALAR_TYPE as supported
   by the target.  */

static tree
get_vectype_for_scalar_type (tree scalar_type)
{
  enum machine_mode inner_mode = TYPE_MODE (scalar_type);
  int nbytes = GET_MODE_SIZE (inner_mode);
  int nunits;
  tree vectype;

  if (nbytes == 0)
    return NULL_TREE;

  /* FORNOW: Only a single vector size per target (UNITS_PER_SIMD_WORD)
     is expected.  */
  nunits = UNITS_PER_SIMD_WORD / nbytes;

  vectype = build_vector_type (scalar_type, nunits);
  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "get vectype with %d units of type ", nunits);
      print_generic_expr (vect_dump, scalar_type, TDF_SLIM);
    }

  if (!vectype)
    return NULL_TREE;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "vectype: ");
      print_generic_expr (vect_dump, vectype, TDF_SLIM);
    }

  if (!VECTOR_MODE_P (TYPE_MODE (vectype)))
    {
      /* TODO: tree-complex.c sometimes can parallelize operations
         on generic vectors.  We can vectorize the loop in that case,
         but then we should re-run the lowering pass.  */
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "mode not supported by target.");
      return NULL_TREE;
    }

  return vectype;
}


/* Function vect_align_data_ref.

   Handle mislignment of a memory accesses.

   FORNOW: Can't handle misaligned accesses. 
   Make sure that the dataref is aligned.  */

static void
vect_align_data_ref (tree stmt)
{
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  struct data_reference *dr = STMT_VINFO_DATA_REF (stmt_info);

  /* FORNOW: can't handle misaligned accesses; 
             all accesses expected to be aligned.  */
  gcc_assert (aligned_access_p (dr));
}


/* Function vect_create_data_ref_ptr.

   Create a memory reference expression for vector access, to be used in a
   vector load/store stmt. The reference is based on a new pointer to vector
   type (vp).

   Input:
   1. STMT: a stmt that references memory. Expected to be of the form
         MODIFY_EXPR <name, data-ref> or MODIFY_EXPR <data-ref, name>.
   2. BSI: block_stmt_iterator where new stmts can be added.
   3. OFFSET (optional): an offset to be added to the initial address accessed
        by the data-ref in STMT.
   4. ONLY_INIT: indicate if vp is to be updated in the loop, or remain
        pointing to the initial address.

   Output:
   1. Declare a new ptr to vector_type, and have it point to the base of the
      data reference (initial addressed accessed by the data reference).
      For example, for vector of type V8HI, the following code is generated:

      v8hi *vp;
      vp = (v8hi *)initial_address;

      if OFFSET is not supplied:
         initial_address = &a[init];
      if OFFSET is supplied:
         initial_address = &a[init + OFFSET];

      Return the initial_address in INITIAL_ADDRESS.

   2. Create a data-reference in the loop based on the new vector pointer vp,
      and using a new index variable 'idx' as follows:

      vp' = vp + update

      where if ONLY_INIT is true:
         update = zero
      and otherwise
         update = idx + vector_type_size

      Return the pointer vp'.


   FORNOW: handle only aligned and consecutive accesses.  */

static tree
vect_create_data_ref_ptr (tree stmt, block_stmt_iterator *bsi, tree offset,
                          tree *initial_address, bool only_init)
{
  tree base_name;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  tree vect_ptr_type;
  tree vect_ptr;
  tree tag;
  v_may_def_optype v_may_defs = STMT_V_MAY_DEF_OPS (stmt);
  v_must_def_optype v_must_defs = STMT_V_MUST_DEF_OPS (stmt);
  vuse_optype vuses = STMT_VUSE_OPS (stmt);
  int nvuses, nv_may_defs, nv_must_defs;
  int i;
  tree new_temp;
  tree vec_stmt;
  tree new_stmt_list = NULL_TREE;
  tree idx;
  edge pe = loop_preheader_edge (loop);
  basic_block new_bb;
  tree vect_ptr_init;
  tree vectype_size;
  tree ptr_update;
  tree data_ref_ptr;
  tree type, tmp, size;

  base_name =  build_fold_indirect_ref (unshare_expr (
		      STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info)));

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      tree data_ref_base = base_name;
      fprintf (vect_dump, "create array_ref of type: ");
      print_generic_expr (vect_dump, vectype, TDF_SLIM);
      if (TREE_CODE (data_ref_base) == VAR_DECL)
        fprintf (vect_dump, "  vectorizing a one dimensional array ref: ");
      else if (TREE_CODE (data_ref_base) == ARRAY_REF)
        fprintf (vect_dump, "  vectorizing a multidimensional array ref: ");
      else if (TREE_CODE (data_ref_base) == COMPONENT_REF)
        fprintf (vect_dump, "  vectorizing a record based array ref: ");
      else if (TREE_CODE (data_ref_base) == SSA_NAME)
        fprintf (vect_dump, "  vectorizing a pointer ref: ");
      print_generic_expr (vect_dump, base_name, TDF_SLIM);
    }

  /** (1) Create the new vector-pointer variable:  **/

  vect_ptr_type = build_pointer_type (vectype);
  vect_ptr = vect_get_new_vect_var (vect_ptr_type, vect_pointer_var,
                                    get_name (base_name));
  add_referenced_tmp_var (vect_ptr);
  
  
  /** (2) Handle aliasing information of the new vector-pointer:  **/
  
  tag = STMT_VINFO_MEMTAG (stmt_info);
  gcc_assert (tag);
  get_var_ann (vect_ptr)->type_mem_tag = tag;
  
  /* Mark for renaming all aliased variables
     (i.e, the may-aliases of the type-mem-tag).  */
  nvuses = NUM_VUSES (vuses);
  nv_may_defs = NUM_V_MAY_DEFS (v_may_defs);
  nv_must_defs = NUM_V_MUST_DEFS (v_must_defs);
  for (i = 0; i < nvuses; i++)
    {
      tree use = VUSE_OP (vuses, i);
      if (TREE_CODE (use) == SSA_NAME)
        bitmap_set_bit (vars_to_rename, var_ann (SSA_NAME_VAR (use))->uid);
    }
  for (i = 0; i < nv_may_defs; i++)
    {
      tree def = V_MAY_DEF_RESULT (v_may_defs, i);
      if (TREE_CODE (def) == SSA_NAME)
        bitmap_set_bit (vars_to_rename, var_ann (SSA_NAME_VAR (def))->uid);
    }
  for (i = 0; i < nv_must_defs; i++)
    {
      tree def = V_MUST_DEF_RESULT (v_must_defs, i);
      if (TREE_CODE (def) == SSA_NAME)
        bitmap_set_bit (vars_to_rename, var_ann (SSA_NAME_VAR (def))->uid);
    }


  /** (3) Calculate the initial address the vector-pointer, and set
          the vector-pointer to point to it before the loop:  **/

  /* Create: (&(base[init_val+offset]) in the loop preheader.  */
  new_temp = vect_create_addr_base_for_vector_ref (stmt, &new_stmt_list,
                                                   offset);
  pe = loop_preheader_edge (loop);
  new_bb = bsi_insert_on_edge_immediate (pe, new_stmt_list);
  gcc_assert (!new_bb);
  *initial_address = new_temp;

  /* Create: p = (vectype *) initial_base  */
  vec_stmt = fold_convert (vect_ptr_type, new_temp);
  vec_stmt = build2 (MODIFY_EXPR, void_type_node, vect_ptr, vec_stmt);
  new_temp = make_ssa_name (vect_ptr, vec_stmt);
  TREE_OPERAND (vec_stmt, 0) = new_temp;
  new_bb = bsi_insert_on_edge_immediate (pe, vec_stmt);
  gcc_assert (!new_bb);
  vect_ptr_init = TREE_OPERAND (vec_stmt, 0);


  /** (4) Handle the updating of the vector-pointer inside the loop: **/

  if (only_init) /* No update in loop is required.  */
    return vect_ptr_init;

  idx = vect_create_index_for_vector_ref (loop_vinfo);

  /* Create: update = idx * vectype_size  */
  tmp = create_tmp_var (integer_type_node, "update");
  add_referenced_tmp_var (tmp);
  size = TYPE_SIZE (vect_ptr_type); 
  type = lang_hooks.types.type_for_size (tree_low_cst (size, 1), 1);
  ptr_update = create_tmp_var (type, "update");
  add_referenced_tmp_var (ptr_update);
  vectype_size = TYPE_SIZE_UNIT (vectype);
  vec_stmt = build2 (MULT_EXPR, integer_type_node, idx, vectype_size);
  vec_stmt = build2 (MODIFY_EXPR, void_type_node, tmp, vec_stmt);
  new_temp = make_ssa_name (tmp, vec_stmt);
  TREE_OPERAND (vec_stmt, 0) = new_temp;
  bsi_insert_before (bsi, vec_stmt, BSI_SAME_STMT);
  vec_stmt = fold_convert (type, new_temp);
  vec_stmt = build2 (MODIFY_EXPR, void_type_node, ptr_update, vec_stmt);
  new_temp = make_ssa_name (ptr_update, vec_stmt);
  TREE_OPERAND (vec_stmt, 0) = new_temp;
  bsi_insert_before (bsi, vec_stmt, BSI_SAME_STMT);

  /* Create: data_ref_ptr = vect_ptr_init + update  */
  vec_stmt = build2 (PLUS_EXPR, vect_ptr_type, vect_ptr_init, new_temp);
  vec_stmt = build2 (MODIFY_EXPR, void_type_node, vect_ptr, vec_stmt);
  new_temp = make_ssa_name (vect_ptr, vec_stmt);
  TREE_OPERAND (vec_stmt, 0) = new_temp;
  bsi_insert_before (bsi, vec_stmt, BSI_SAME_STMT);
  data_ref_ptr = TREE_OPERAND (vec_stmt, 0);

  return data_ref_ptr;
}


/* Function vect_create_destination_var.

   Create a new temporary of type VECTYPE.  */

static tree
vect_create_destination_var (tree scalar_dest, tree vectype)
{
  tree vec_dest;
  const char *new_name;

  gcc_assert (TREE_CODE (scalar_dest) == SSA_NAME);

  new_name = get_name (scalar_dest);
  if (!new_name)
    new_name = "var_";
  vec_dest = vect_get_new_vect_var (vectype, vect_simple_var, new_name);
  add_referenced_tmp_var (vec_dest);

  return vec_dest;
}


/* Function vect_init_vector.

   Insert a new stmt (INIT_STMT) that initializes a new vector variable with
   the vector elements of VECTOR_VAR. Return the DEF of INIT_STMT. It will be
   used in the vectorization of STMT.  */

static tree
vect_init_vector (tree stmt, tree vector_var)
{
  stmt_vec_info stmt_vinfo = vinfo_for_stmt (stmt);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_vinfo);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree new_var;
  tree init_stmt;
  tree vectype = STMT_VINFO_VECTYPE (stmt_vinfo); 
  tree vec_oprnd;
  edge pe;
  tree new_temp;
  basic_block new_bb;
 
  new_var = vect_get_new_vect_var (vectype, vect_simple_var, "cst_");
  add_referenced_tmp_var (new_var); 
 
  init_stmt = build2 (MODIFY_EXPR, vectype, new_var, vector_var);
  new_temp = make_ssa_name (new_var, init_stmt);
  TREE_OPERAND (init_stmt, 0) = new_temp;

  pe = loop_preheader_edge (loop);
  new_bb = bsi_insert_on_edge_immediate (pe, init_stmt);
  gcc_assert (!new_bb);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "created new init_stmt: ");
      print_generic_expr (vect_dump, init_stmt, TDF_SLIM);
    }

  vec_oprnd = TREE_OPERAND (init_stmt, 0);
  return vec_oprnd;
}


/* Function vect_get_vec_def_for_operand.

   OP is an operand in STMT. This function returns a (vector) def that will be
   used in the vectorized stmt for STMT.

   In the case that OP is an SSA_NAME which is defined in the loop, then
   STMT_VINFO_VEC_STMT of the defining stmt holds the relevant def.

   In case OP is an invariant or constant, a new stmt that creates a vector def
   needs to be introduced.  */

static tree
vect_get_vec_def_for_operand (tree op, tree stmt)
{
  tree vec_oprnd;
  tree vec_stmt;
  tree def_stmt;
  stmt_vec_info def_stmt_info = NULL;
  stmt_vec_info stmt_vinfo = vinfo_for_stmt (stmt);
  tree vectype = STMT_VINFO_VECTYPE (stmt_vinfo);
  int nunits = GET_MODE_NUNITS (TYPE_MODE (vectype));
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_vinfo);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block bb;
  tree vec_inv;
  tree t = NULL_TREE;
  tree def;
  int i;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "vect_get_vec_def_for_operand: ");
      print_generic_expr (vect_dump, op, TDF_SLIM);
    }

  /** ===> Case 1: operand is a constant.  **/

  if (TREE_CODE (op) == INTEGER_CST || TREE_CODE (op) == REAL_CST)
    {
      /* Create 'vect_cst_ = {cst,cst,...,cst}'  */

      tree vec_cst;

      /* Build a tree with vector elements.  */
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "Create vector_cst. nunits = %d", nunits);

      for (i = nunits - 1; i >= 0; --i)
        {
          t = tree_cons (NULL_TREE, op, t);
        }
      vec_cst = build_vector (vectype, t);
      return vect_init_vector (stmt, vec_cst);
    }

  gcc_assert (TREE_CODE (op) == SSA_NAME);
 
  /** ===> Case 2: operand is an SSA_NAME - find the stmt that defines it.  **/

  def_stmt = SSA_NAME_DEF_STMT (op);
  def_stmt_info = vinfo_for_stmt (def_stmt);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "vect_get_vec_def_for_operand: def_stmt: ");
      print_generic_expr (vect_dump, def_stmt, TDF_SLIM);
    }


  /** ==> Case 2.1: operand is defined inside the loop.  **/

  if (def_stmt_info)
    {
      /* Get the def from the vectorized stmt.  */

      vec_stmt = STMT_VINFO_VEC_STMT (def_stmt_info);
      gcc_assert (vec_stmt);
      vec_oprnd = TREE_OPERAND (vec_stmt, 0);
      return vec_oprnd;
    }


  /** ==> Case 2.2: operand is defined by the loop-header phi-node - 
                    it is a reduction/induction.  **/

  bb = bb_for_stmt (def_stmt);
  if (TREE_CODE (def_stmt) == PHI_NODE && flow_bb_inside_loop_p (loop, bb))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "reduction/induction - unsupported.");
      internal_error ("no support for reduction/induction"); /* FORNOW */
    }


  /** ==> Case 2.3: operand is defined outside the loop - 
                    it is a loop invariant.  */

  switch (TREE_CODE (def_stmt))
    {
    case PHI_NODE:
      def = PHI_RESULT (def_stmt);
      break;
    case MODIFY_EXPR:
      def = TREE_OPERAND (def_stmt, 0);
      break;
    case NOP_EXPR:
      def = TREE_OPERAND (def_stmt, 0);
      gcc_assert (IS_EMPTY_STMT (def_stmt));
      def = op;
      break;
    default:
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
          fprintf (vect_dump, "unsupported defining stmt: ");
	  print_generic_expr (vect_dump, def_stmt, TDF_SLIM);
	}
      internal_error ("unsupported defining stmt");
    }

  /* Build a tree with vector elements.
     Create 'vec_inv = {inv,inv,..,inv}'  */

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "Create vector_inv.");

  for (i = nunits - 1; i >= 0; --i)
    {
      t = tree_cons (NULL_TREE, def, t);
    }

  vec_inv = build_constructor (vectype, t);
  return vect_init_vector (stmt, vec_inv);
}


/* Function vect_finish_stmt_generation.

   Insert a new stmt.  */

static void
vect_finish_stmt_generation (tree stmt, tree vec_stmt, block_stmt_iterator *bsi)
{
  bsi_insert_before (bsi, vec_stmt, BSI_SAME_STMT);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "add new stmt: ");
      print_generic_expr (vect_dump, vec_stmt, TDF_SLIM);
    }

#ifdef ENABLE_CHECKING
  /* Make sure bsi points to the stmt that is being vectorized.  */
  gcc_assert (stmt == bsi_stmt (*bsi));
#endif

#ifdef USE_MAPPED_LOCATION
  SET_EXPR_LOCATION (vec_stmt, EXPR_LOCUS (stmt));
#else
  SET_EXPR_LOCUS (vec_stmt, EXPR_LOCUS (stmt));
#endif
}


/* Function vectorizable_assignment.

   Check if STMT performs an assignment (copy) that can be vectorized. 
   If VEC_STMT is also passed, vectorize the STMT: create a vectorized 
   stmt to replace it, put it in VEC_STMT, and insert it at BSI.
   Return FALSE if not a vectorizable STMT, TRUE otherwise.  */

static bool
vectorizable_assignment (tree stmt, block_stmt_iterator *bsi, tree *vec_stmt)
{
  tree vec_dest;
  tree scalar_dest;
  tree op;
  tree vec_oprnd;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  tree new_temp;

  /* Is vectorizable assignment?  */

  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return false;

  scalar_dest = TREE_OPERAND (stmt, 0);
  if (TREE_CODE (scalar_dest) != SSA_NAME)
    return false;

  op = TREE_OPERAND (stmt, 1);
  if (!vect_is_simple_use (op, loop_vinfo, NULL))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "use not simple.");
      return false;
    }

  if (!vec_stmt) /* transformation not required.  */
    {
      STMT_VINFO_TYPE (stmt_info) = assignment_vec_info_type;
      return true;
    }

  /** Transform.  **/
  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "transform assignment.");

  /* Handle def.  */
  vec_dest = vect_create_destination_var (scalar_dest, vectype);

  /* Handle use.  */
  op = TREE_OPERAND (stmt, 1);
  vec_oprnd = vect_get_vec_def_for_operand (op, stmt);

  /* Arguments are ready. create the new vector stmt.  */
  *vec_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, vec_oprnd);
  new_temp = make_ssa_name (vec_dest, *vec_stmt);
  TREE_OPERAND (*vec_stmt, 0) = new_temp;
  vect_finish_stmt_generation (stmt, *vec_stmt, bsi);
  
  return true;
}


/* Function vectorizable_operation.

   Check if STMT performs a binary or unary operation that can be vectorized. 
   If VEC_STMT is also passed, vectorize the STMT: create a vectorized 
   stmt to replace it, put it in VEC_STMT, and insert it at BSI.
   Return FALSE if not a vectorizable STMT, TRUE otherwise.  */

static bool
vectorizable_operation (tree stmt, block_stmt_iterator *bsi, tree *vec_stmt)
{
  tree vec_dest;
  tree scalar_dest;
  tree operation;
  tree op0, op1 = NULL;
  tree vec_oprnd0, vec_oprnd1=NULL;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  int i;
  enum tree_code code;
  enum machine_mode vec_mode;
  tree new_temp;
  int op_type;
  tree op;
  optab optab;

  /* Is STMT a vectorizable binary/unary operation?   */
  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return false;

  if (TREE_CODE (TREE_OPERAND (stmt, 0)) != SSA_NAME)
    return false;

  operation = TREE_OPERAND (stmt, 1);
  code = TREE_CODE (operation);
  optab = optab_for_tree_code (code, vectype);

  /* Support only unary or binary operations.  */
  op_type = TREE_CODE_LENGTH (code);
  if (op_type != unary_op && op_type != binary_op)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "num. args = %d (not unary/binary op).", op_type);
      return false;
    }

  for (i = 0; i < op_type; i++)
    {
      op = TREE_OPERAND (operation, i);
      if (!vect_is_simple_use (op, loop_vinfo, NULL))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "use not simple.");
	  return false;
	}	
    } 

  /* Supportable by target?  */
  if (!optab)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "no optab.");
      return false;
    }
  vec_mode = TYPE_MODE (vectype);
  if (optab->handlers[(int) vec_mode].insn_code == CODE_FOR_nothing)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "op not supported by target.");
      return false;
    }

  if (!vec_stmt) /* transformation not required.  */
    {
      STMT_VINFO_TYPE (stmt_info) = op_vec_info_type;
      return true;
    }

  /** Transform.  **/

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "transform binary/unary operation.");

  /* Handle def.  */
  scalar_dest = TREE_OPERAND (stmt, 0);
  vec_dest = vect_create_destination_var (scalar_dest, vectype);

  /* Handle uses.  */
  op0 = TREE_OPERAND (operation, 0);
  vec_oprnd0 = vect_get_vec_def_for_operand (op0, stmt);

  if (op_type == binary_op)
    {
      op1 = TREE_OPERAND (operation, 1);
      vec_oprnd1 = vect_get_vec_def_for_operand (op1, stmt); 
    }

  /* Arguments are ready. create the new vector stmt.  */

  if (op_type == binary_op)
    *vec_stmt = build2 (MODIFY_EXPR, vectype, vec_dest,
		build2 (code, vectype, vec_oprnd0, vec_oprnd1));
  else
    *vec_stmt = build2 (MODIFY_EXPR, vectype, vec_dest,
		build1 (code, vectype, vec_oprnd0));
  new_temp = make_ssa_name (vec_dest, *vec_stmt);
  TREE_OPERAND (*vec_stmt, 0) = new_temp;
  vect_finish_stmt_generation (stmt, *vec_stmt, bsi);

  return true;
}


/* Function vectorizable_store.

   Check if STMT defines a non scalar data-ref (array/pointer/structure) that 
   can be vectorized. 
   If VEC_STMT is also passed, vectorize the STMT: create a vectorized 
   stmt to replace it, put it in VEC_STMT, and insert it at BSI.
   Return FALSE if not a vectorizable STMT, TRUE otherwise.  */

static bool
vectorizable_store (tree stmt, block_stmt_iterator *bsi, tree *vec_stmt)
{
  tree scalar_dest;
  tree data_ref;
  tree op;
  tree vec_oprnd1;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  struct data_reference *dr = STMT_VINFO_DATA_REF (stmt_info);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  enum machine_mode vec_mode;
  tree dummy;
  enum dr_alignment_support alignment_support_cheme;

  /* Is vectorizable store? */

  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return false;

  scalar_dest = TREE_OPERAND (stmt, 0);
  if (TREE_CODE (scalar_dest) != ARRAY_REF
      && TREE_CODE (scalar_dest) != INDIRECT_REF)
    return false;

  op = TREE_OPERAND (stmt, 1);
  if (!vect_is_simple_use (op, loop_vinfo, NULL))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "use not simple.");
      return false;
    }

  vec_mode = TYPE_MODE (vectype);
  /* FORNOW. In some cases can vectorize even if data-type not supported
     (e.g. - array initialization with 0).  */
  if (mov_optab->handlers[(int)vec_mode].insn_code == CODE_FOR_nothing)
    return false;

  if (!STMT_VINFO_DATA_REF (stmt_info))
    return false;


  if (!vec_stmt) /* transformation not required.  */
    {
      STMT_VINFO_TYPE (stmt_info) = store_vec_info_type;
      return true;
    }

  /** Transform.  **/

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "transform store");

  alignment_support_cheme = vect_supportable_dr_alignment (dr);
  gcc_assert (alignment_support_cheme);
  gcc_assert (alignment_support_cheme = dr_aligned);  /* FORNOW */

  /* Handle use - get the vectorized def from the defining stmt.  */
  vec_oprnd1 = vect_get_vec_def_for_operand (op, stmt);

  /* Handle def.  */
  /* FORNOW: make sure the data reference is aligned.  */
  vect_align_data_ref (stmt);
  data_ref = vect_create_data_ref_ptr (stmt, bsi, NULL_TREE, &dummy, false);
  data_ref = build_fold_indirect_ref (data_ref);

  /* Arguments are ready. create the new vector stmt.  */
  *vec_stmt = build2 (MODIFY_EXPR, vectype, data_ref, vec_oprnd1);
  vect_finish_stmt_generation (stmt, *vec_stmt, bsi);

  return true;
}


/* vectorizable_load.

   Check if STMT reads a non scalar data-ref (array/pointer/structure) that 
   can be vectorized. 
   If VEC_STMT is also passed, vectorize the STMT: create a vectorized 
   stmt to replace it, put it in VEC_STMT, and insert it at BSI.
   Return FALSE if not a vectorizable STMT, TRUE otherwise.  */

static bool
vectorizable_load (tree stmt, block_stmt_iterator *bsi, tree *vec_stmt)
{
  tree scalar_dest;
  tree vec_dest = NULL;
  tree data_ref = NULL;
  tree op;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  struct data_reference *dr = STMT_VINFO_DATA_REF (stmt_info);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  tree new_temp;
  int mode;
  tree init_addr;
  tree new_stmt;
  tree dummy;
  basic_block new_bb;
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  edge pe = loop_preheader_edge (loop);
  enum dr_alignment_support alignment_support_cheme;

  /* Is vectorizable load? */

  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return false;

  scalar_dest = TREE_OPERAND (stmt, 0);
  if (TREE_CODE (scalar_dest) != SSA_NAME)
    return false;

  op = TREE_OPERAND (stmt, 1);
  if (TREE_CODE (op) != ARRAY_REF && TREE_CODE (op) != INDIRECT_REF)
    return false;

  if (!STMT_VINFO_DATA_REF (stmt_info))
    return false;

  mode = (int) TYPE_MODE (vectype);

  /* FORNOW. In some cases can vectorize even if data-type not supported
    (e.g. - data copies).  */
  if (mov_optab->handlers[mode].insn_code == CODE_FOR_nothing)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "Aligned load, but unsupported type.");
      return false;
    }

  if (!vec_stmt) /* transformation not required.  */
    {
      STMT_VINFO_TYPE (stmt_info) = load_vec_info_type;
      return true;
    }

  /** Transform.  **/

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "transform load.");

  alignment_support_cheme = vect_supportable_dr_alignment (dr);
  gcc_assert (alignment_support_cheme);

  if (alignment_support_cheme == dr_aligned
      || alignment_support_cheme == dr_unaligned_supported)
    {
      /* Create:
         p = initial_addr;
         indx = 0;
         loop {
           vec_dest = *(p);
           indx = indx + 1;
         }
      */

      vec_dest = vect_create_destination_var (scalar_dest, vectype);
      data_ref = vect_create_data_ref_ptr (stmt, bsi, NULL_TREE, &dummy, false);
      if (aligned_access_p (dr))
        data_ref = build_fold_indirect_ref (data_ref);
      else
	{
	  int mis = DR_MISALIGNMENT (dr);
	  tree tmis = (mis == -1 ? size_zero_node : size_int (mis));
	  tmis = size_binop (MULT_EXPR, tmis, size_int(BITS_PER_UNIT));
	  data_ref = build2 (MISALIGNED_INDIRECT_REF, vectype, data_ref, tmis);
	}
      new_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, data_ref);
      new_temp = make_ssa_name (vec_dest, new_stmt);
      TREE_OPERAND (new_stmt, 0) = new_temp;
      vect_finish_stmt_generation (stmt, new_stmt, bsi);
    }
  else if (alignment_support_cheme == dr_unaligned_software_pipeline)
    {
      /* Create:
	 p1 = initial_addr;
	 msq_init = *(floor(p1))
	 p2 = initial_addr + VS - 1;
	 magic = have_builtin ? builtin_result : initial_address;
	 indx = 0;
	 loop {
	   p2' = p2 + indx * vectype_size
	   lsq = *(floor(p2'))
	   vec_dest = realign_load (msq, lsq, magic)
	   indx = indx + 1;
	   msq = lsq;
	 }
      */

      tree offset;
      tree magic;
      tree phi_stmt;
      tree msq_init;
      tree msq, lsq;
      tree dataref_ptr;
      tree params;

      /* <1> Create msq_init = *(floor(p1)) in the loop preheader  */
      vec_dest = vect_create_destination_var (scalar_dest, vectype);
      data_ref = vect_create_data_ref_ptr (stmt, bsi, NULL_TREE, 
					   &init_addr, true);
      data_ref = build1 (ALIGN_INDIRECT_REF, vectype, data_ref);
      new_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, data_ref);
      new_temp = make_ssa_name (vec_dest, new_stmt);
      TREE_OPERAND (new_stmt, 0) = new_temp;
      new_bb = bsi_insert_on_edge_immediate (pe, new_stmt);
      gcc_assert (!new_bb);
      msq_init = TREE_OPERAND (new_stmt, 0);


      /* <2> Create lsq = *(floor(p2')) in the loop  */ 
      offset = build_int_cst (integer_type_node, 
			      GET_MODE_NUNITS (TYPE_MODE (vectype)));
      offset = int_const_binop (MINUS_EXPR, offset, integer_one_node, 1);
      vec_dest = vect_create_destination_var (scalar_dest, vectype);
      dataref_ptr = vect_create_data_ref_ptr (stmt, bsi, offset, &dummy, false);
      data_ref = build1 (ALIGN_INDIRECT_REF, vectype, dataref_ptr);
      new_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, data_ref);
      new_temp = make_ssa_name (vec_dest, new_stmt);
      TREE_OPERAND (new_stmt, 0) = new_temp;
      vect_finish_stmt_generation (stmt, new_stmt, bsi);
      lsq = TREE_OPERAND (new_stmt, 0);


      /* <3> */
      if (targetm.vectorize.builtin_mask_for_load)
	{
	  /* Create permutation mask, if required, in loop preheader.  */
	  tree builtin_decl;
	  params = build_tree_list (NULL_TREE, init_addr);
	  vec_dest = vect_create_destination_var (scalar_dest, vectype);
	  builtin_decl = targetm.vectorize.builtin_mask_for_load ();
	  new_stmt = build_function_call_expr (builtin_decl, params);
	  new_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, new_stmt);
	  new_temp = make_ssa_name (vec_dest, new_stmt);
	  TREE_OPERAND (new_stmt, 0) = new_temp;
	  new_bb = bsi_insert_on_edge_immediate (pe, new_stmt);
	  gcc_assert (!new_bb);
	  magic = TREE_OPERAND (new_stmt, 0);

	  /* Since we have just created a CALL_EXPR, we may need to
	     rename call-clobbered variables.  */
	  mark_call_clobbered_vars_to_rename ();
	}
      else
	{
	  /* Use current address instead of init_addr for reduced reg pressure.
	   */
	  magic = dataref_ptr;
	}


      /* <4> Create msq = phi <msq_init, lsq> in loop  */ 
      vec_dest = vect_create_destination_var (scalar_dest, vectype);
      msq = make_ssa_name (vec_dest, NULL_TREE);
      phi_stmt = create_phi_node (msq, loop->header); /* CHECKME */
      SSA_NAME_DEF_STMT (msq) = phi_stmt;
      add_phi_arg (phi_stmt, msq_init, loop_preheader_edge (loop));
      add_phi_arg (phi_stmt, lsq, loop_latch_edge (loop));


      /* <5> Create <vec_dest = realign_load (msq, lsq, magic)> in loop  */
      vec_dest = vect_create_destination_var (scalar_dest, vectype);
      new_stmt = build3 (REALIGN_LOAD_EXPR, vectype, msq, lsq, magic);
      new_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, new_stmt);
      new_temp = make_ssa_name (vec_dest, new_stmt); 
      TREE_OPERAND (new_stmt, 0) = new_temp;
      vect_finish_stmt_generation (stmt, new_stmt, bsi);
    }
  else
    gcc_unreachable ();

  *vec_stmt = new_stmt;
  return true;
}


/* Function vect_supportable_dr_alignment

   Return whether the data reference DR is supported with respect to its
   alignment.  */

static enum dr_alignment_support
vect_supportable_dr_alignment (struct data_reference *dr)
{
  tree vectype = STMT_VINFO_VECTYPE (vinfo_for_stmt (DR_STMT (dr)));
  enum machine_mode mode = (int) TYPE_MODE (vectype);

  if (aligned_access_p (dr))
    return dr_aligned;

  /* Possibly unaligned access.  */
  
  if (DR_IS_READ (dr))
    {
      if (vec_realign_load_optab->handlers[mode].insn_code != CODE_FOR_nothing
	  && (!targetm.vectorize.builtin_mask_for_load
	      || targetm.vectorize.builtin_mask_for_load ()))
	return dr_unaligned_software_pipeline;

      if (movmisalign_optab->handlers[mode].insn_code != CODE_FOR_nothing)
	/* Can't software pipeline the loads, but can at least do them.  */
	return dr_unaligned_supported;
    }

  /* Unsupported.  */
  return dr_unaligned_unsupported;
}


/* APPLE LOCAL begin AV cond expr. -dpatel */
/* Patch is waiting FSF review since mid Sep,  2004.  */
/* vectorizable_select.

   Check if STMT is conditional modify expression that can be vectorized. 
   If VEC_STMT is also passed, vectorize the STMT: create a vectorized 
   stmt using VEC_COND_EXPR  to replace it, put it in VEC_STMT, and insert it 
   at BSI.

   Return FALSE if not a vectorizable STMT, TRUE otherwise.  */

static bool
vectorizable_select (tree stmt, block_stmt_iterator *bsi, tree *vec_stmt)
{
  tree scalar_dest = NULL_TREE;
  tree vec_dest = NULL_TREE;
  tree op = NULL_TREE;
  tree cond_expr, then_clause, else_clause;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  tree vec_cond_lhs, vec_cond_rhs, vec_then_clause, vec_else_clause;
  tree vec_compare, vec_cond_expr;
  tree new_temp;
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  enum machine_mode vec_mode;

  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return false;

  op = TREE_OPERAND (stmt, 1);

  if (TREE_CODE (op) != COND_EXPR)
    return false;

  cond_expr = TREE_OPERAND (op, 0);
  then_clause = TREE_OPERAND (op, 1);
  else_clause = TREE_OPERAND (op, 2);

  if (!vect_is_simple_cond (cond_expr, loop_vinfo))
    return false;

  if (TREE_CODE (then_clause) == SSA_NAME)
    {
      tree then_def_stmt = SSA_NAME_DEF_STMT (then_clause);
      if (!vect_is_simple_use (then_clause, loop_vinfo, &then_def_stmt))
	return false;
    }
  else if (TREE_CODE (then_clause) != INTEGER_CST 
	   && TREE_CODE (then_clause) != REAL_CST)
    return false;

  if (TREE_CODE (else_clause) == SSA_NAME)
    {
      tree else_def_stmt = SSA_NAME_DEF_STMT (else_clause);
      if (!vect_is_simple_use (else_clause, loop_vinfo, &else_def_stmt))
	return false;
    }
  else if (TREE_CODE (else_clause) != INTEGER_CST 
	   && TREE_CODE (else_clause) != REAL_CST)
    return false;


  vec_mode = TYPE_MODE (vectype);

  if (!vec_stmt) 
    {
      STMT_VINFO_TYPE (stmt_info) = select_vec_info_type;
      return expand_vec_cond_expr_p (op, vec_mode);
    }

  /* Transform */

  /* Handle def.  */
  scalar_dest = TREE_OPERAND (stmt, 0);
  vec_dest = vect_create_destination_var (scalar_dest, vectype);

  /* Handle cond expr.  */
  vec_cond_lhs = vect_get_vec_def_for_operand (TREE_OPERAND (cond_expr, 0), stmt);
  vec_cond_rhs = vect_get_vec_def_for_operand (TREE_OPERAND (cond_expr, 1), stmt);
  vec_then_clause = vect_get_vec_def_for_operand (then_clause, stmt);
  vec_else_clause = vect_get_vec_def_for_operand (else_clause, stmt);

  /* Arguments are ready. create the new vector stmt.  */
  vec_compare = build2 (TREE_CODE (cond_expr), vectype, 
			vec_cond_lhs, vec_cond_rhs);
  vec_cond_expr = build (VEC_COND_EXPR, vectype, 
			 vec_compare, vec_then_clause, vec_else_clause);

  *vec_stmt = build2 (MODIFY_EXPR, vectype, vec_dest, vec_cond_expr);
  new_temp = make_ssa_name (vec_dest, *vec_stmt);
  TREE_OPERAND (*vec_stmt, 0) = new_temp;
  vect_finish_stmt_generation (stmt, *vec_stmt, bsi);
  
  return true;
}
/* APPLE LOCAL end AV cond expr. -dpatel */

/* Function vect_transform_stmt.

   Create a vectorized stmt to replace STMT, and insert it at BSI.  */

static bool
vect_transform_stmt (tree stmt, block_stmt_iterator *bsi)
{
  bool is_store = false;
  tree vec_stmt = NULL_TREE;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  bool done;

  switch (STMT_VINFO_TYPE (stmt_info))
    {
    case op_vec_info_type:
      done = vectorizable_operation (stmt, bsi, &vec_stmt);
      gcc_assert (done);
      break;

    case assignment_vec_info_type:
      done = vectorizable_assignment (stmt, bsi, &vec_stmt);
      gcc_assert (done);
      break;

    case load_vec_info_type:
      done = vectorizable_load (stmt, bsi, &vec_stmt);
      gcc_assert (done);
      break;

    case store_vec_info_type:
      done = vectorizable_store (stmt, bsi, &vec_stmt);
      gcc_assert (done);
      is_store = true;
      break;

/* APPLE LOCAL begin AV cond expr. -dpatel */
/* Patch is waiting FSF review since mid Sep, 2004.  */
    case select_vec_info_type:
      if (!vectorizable_select (stmt, bsi, &vec_stmt))
	abort ();
      break;
/* APPLE LOCAL end AV cond expr. -dpatel */

    default:
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "stmt not supported.");
      gcc_unreachable ();
    }

  STMT_VINFO_VEC_STMT (stmt_info) = vec_stmt;

  return is_store;
}


/* This function builds ni_name = number of iterations loop executes
   on the loop preheader.  */

static tree
vect_build_loop_niters (loop_vec_info loop_vinfo)
{
  tree ni_name, stmt, var;
  edge pe;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree ni = unshare_expr (LOOP_VINFO_NITERS (loop_vinfo));

  var = create_tmp_var (TREE_TYPE (ni), "niters");
  add_referenced_tmp_var (var);
  ni_name = force_gimple_operand (ni, &stmt, false, var);

  pe = loop_preheader_edge (loop);
  if (stmt)
    {
      basic_block new_bb = bsi_insert_on_edge_immediate (pe, stmt);
      gcc_assert (!new_bb);
    }
      
  return ni_name;
}


/* This function generates the following statements:

 ni_name = number of iterations loop executes
 ratio = ni_name / vf
 ratio_mult_vf_name = ratio * vf

 and places them at the loop preheader edge.  */

static void 
vect_generate_tmps_on_preheader (loop_vec_info loop_vinfo, 
				 tree *ni_name_ptr,
				 tree *ratio_mult_vf_name_ptr, 
				 tree *ratio_name_ptr)
{

  edge pe;
  basic_block new_bb;
  tree stmt, ni_name;
  tree var;
  tree ratio_name;
  tree ratio_mult_vf_name;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree ni = LOOP_VINFO_NITERS (loop_vinfo);
  int vf = LOOP_VINFO_VECT_FACTOR (loop_vinfo);
  tree log_vf = build_int_cst (unsigned_type_node, exact_log2 (vf));

  pe = loop_preheader_edge (loop);

  /* Generate temporary variable that contains 
     number of iterations loop executes.  */

  ni_name = vect_build_loop_niters (loop_vinfo);

  /* Create: ratio = ni >> log2(vf) */

  var = create_tmp_var (TREE_TYPE (ni), "bnd");
  add_referenced_tmp_var (var);
  ratio_name = make_ssa_name (var, NULL_TREE);
  stmt = build2 (MODIFY_EXPR, void_type_node, ratio_name,
	   build2 (RSHIFT_EXPR, TREE_TYPE (ni_name), ni_name, log_vf));
  SSA_NAME_DEF_STMT (ratio_name) = stmt;

  pe = loop_preheader_edge (loop);
  new_bb = bsi_insert_on_edge_immediate (pe, stmt);
  gcc_assert (!new_bb);
       
  /* Create: ratio_mult_vf = ratio << log2 (vf).  */

  var = create_tmp_var (TREE_TYPE (ni), "ratio_mult_vf");
  add_referenced_tmp_var (var);
  ratio_mult_vf_name = make_ssa_name (var, NULL_TREE);
  stmt = build2 (MODIFY_EXPR, void_type_node, ratio_mult_vf_name,
	   build2 (LSHIFT_EXPR, TREE_TYPE (ratio_name), ratio_name, log_vf));
  SSA_NAME_DEF_STMT (ratio_mult_vf_name) = stmt;

  pe = loop_preheader_edge (loop);
  new_bb = bsi_insert_on_edge_immediate (pe, stmt);
  gcc_assert (!new_bb);

  *ni_name_ptr = ni_name;
  *ratio_mult_vf_name_ptr = ratio_mult_vf_name;
  *ratio_name_ptr = ratio_name;
    
  return;  
}


/*   Function vect_update_ivs_after_vectorizer.

     "Advance" the induction variables of LOOP to the value they should take
     after the execution of LOOP.  This is currently necessary because the
     vectorizer does not handle induction variables that are used after the
     loop.  Such a situation occurs when the last iterations of LOOP are
     peeled, because:
     1. We introduced new uses after LOOP for IVs that were not originally used
        after LOOP: the IVs of LOOP are now used by an epilog loop.
     2. LOOP is going to be vectorized; this means that it will iterate N/VF
        times, whereas the loop IVs should be bumped N times.

     Input:
     - LOOP - a loop that is going to be vectorized. The last few iterations
              of LOOP were peeled.
     - NITERS - the number of iterations that LOOP executes (before it is
                vectorized). i.e, the number of times the ivs should be bumped.
     - UPDATE_E - a successor edge of LOOP->exit that is on the (only) path
                  coming out from LOOP on which there are uses of the LOOP ivs
		  (this is the path from LOOP->exit to epilog_loop->preheader).

                  The new definitions of the ivs are placed in LOOP->exit.
                  The phi args associated with the edge UPDATE_E in the bb
                  UPDATE_E->dest are updated accordingly.

     Assumption 1: Like the rest of the vectorizer, this function assumes
     a single loop exit that has a single predecessor.

     Assumption 2: The phi nodes in the LOOP header and in update_bb are
     organized in the same order.

     Assumption 3: The access function of the ivs is simple enough (see
     vect_can_advance_ivs_p).  This assumption will be relaxed in the future.

     Assumption 4: Exactly one of the successors of LOOP exit-bb is on a path
     coming out of LOOP on which the ivs of LOOP are used (this is the path 
     that leads to the epilog loop; other paths skip the epilog loop).  This
     path starts with the edge UPDATE_E, and its destination (denoted update_bb)
     needs to have its phis updated.
 */

static void
vect_update_ivs_after_vectorizer (loop_vec_info loop_vinfo, tree niters, 
				  edge update_e)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block exit_bb = loop->exit_edges[0]->dest;
  tree phi, phi1;
  basic_block update_bb = update_e->dest;

  /* gcc_assert (vect_can_advance_ivs_p (loop_vinfo)); */

  /* Make sure there exists a single-predecessor exit bb:  */
  gcc_assert (EDGE_COUNT (exit_bb->preds) == 1);

  for (phi = phi_nodes (loop->header), phi1 = phi_nodes (update_bb); 
       phi && phi1; 
       phi = PHI_CHAIN (phi), phi1 = PHI_CHAIN (phi1))
    {
      tree access_fn = NULL;
      tree evolution_part;
      tree init_expr;
      tree step_expr;
      tree var, stmt, ni, ni_name;
      block_stmt_iterator last_bsi;

      /* Skip virtual phi's.  */
      if (!is_gimple_reg (SSA_NAME_VAR (PHI_RESULT (phi))))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "virtual phi. skip.");
	  continue;
	}

      access_fn = analyze_scalar_evolution (loop, PHI_RESULT (phi)); 
      gcc_assert (access_fn);
      evolution_part =
	 unshare_expr (evolution_part_in_loop_num (access_fn, loop->num));
      gcc_assert (evolution_part != NULL_TREE);
      
      /* FORNOW: We do not support IVs whose evolution function is a polynomial
         of degree >= 2 or exponential.  */
      gcc_assert (!tree_is_chrec (evolution_part));

      step_expr = evolution_part;
      init_expr = unshare_expr (initial_condition_in_loop_num (access_fn, 
							       loop->num));

      ni = build2 (PLUS_EXPR, TREE_TYPE (init_expr),
		  build2 (MULT_EXPR, TREE_TYPE (niters),
		       niters, step_expr), init_expr);

      var = create_tmp_var (TREE_TYPE (init_expr), "tmp");
      add_referenced_tmp_var (var);

      ni_name = force_gimple_operand (ni, &stmt, false, var);
      
      /* Insert stmt into exit_bb.  */
      last_bsi = bsi_last (exit_bb);
      if (stmt)
        bsi_insert_before (&last_bsi, stmt, BSI_SAME_STMT);   

      /* Fix phi expressions in the successor bb.  */
      gcc_assert (PHI_ARG_DEF_FROM_EDGE (phi1, update_e) ==
                  PHI_ARG_DEF_FROM_EDGE (phi, EDGE_SUCC (loop->latch, 0)));
      SET_PHI_ARG_DEF (phi1, update_e->dest_idx, ni_name);
    }
}


/* Function vect_do_peeling_for_loop_bound

   Peel the last iterations of the loop represented by LOOP_VINFO.
   The peeled iterations form a new epilog loop.  Given that the loop now 
   iterates NITERS times, the new epilog loop iterates
   NITERS % VECTORIZATION_FACTOR times.
   
   The original loop will later be made to iterate 
   NITERS / VECTORIZATION_FACTOR times (this value is placed into RATIO).  */

static void 
vect_do_peeling_for_loop_bound (loop_vec_info loop_vinfo, tree *ratio,
				struct loops *loops)
{

  tree ni_name, ratio_mult_vf_name;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  struct loop *new_loop;
  edge update_e;
#ifdef ENABLE_CHECKING
  int loop_num;
#endif

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_transtorm_for_unknown_loop_bound ===");

  /* Generate the following variables on the preheader of original loop:
	 
     ni_name = number of iteration the original loop executes
     ratio = ni_name / vf
     ratio_mult_vf_name = ratio * vf  */
  vect_generate_tmps_on_preheader (loop_vinfo, &ni_name,
				   &ratio_mult_vf_name, ratio);

  /* Update loop info.  */
  loop->pre_header = loop_preheader_edge (loop)->src;
  loop->pre_header_edges[0] = loop_preheader_edge (loop);

#ifdef ENABLE_CHECKING
  loop_num  = loop->num; 
#endif
  new_loop = slpeel_tree_peel_loop_to_edge (loop, loops, loop->exit_edges[0],
					    ratio_mult_vf_name, ni_name, false);
#ifdef ENABLE_CHECKING
  gcc_assert (new_loop);
  gcc_assert (loop_num == loop->num);
  slpeel_verify_cfg_after_peeling (loop, new_loop);
#endif

  /* A guard that controls whether the new_loop is to be executed or skipped
     is placed in LOOP->exit.  LOOP->exit therefore has two successors - one
     is the preheader of NEW_LOOP, where the IVs from LOOP are used.  The other
     is a bb after NEW_LOOP, where these IVs are not used.  Find the edge that
     is on the path where the LOOP IVs are used and need to be updated.  */

  if (EDGE_PRED (new_loop->pre_header, 0)->src == loop->exit_edges[0]->dest)
    update_e = EDGE_PRED (new_loop->pre_header, 0);
  else
    update_e = EDGE_PRED (new_loop->pre_header, 1);

  /* Update IVs of original loop as if they were advanced 
     by ratio_mult_vf_name steps.  */
  vect_update_ivs_after_vectorizer (loop_vinfo, ratio_mult_vf_name, update_e); 

  /* After peeling we have to reset scalar evolution analyzer.  */
  scev_reset ();

  return;
}


/* Function vect_gen_niters_for_prolog_loop

   Set the number of iterations for the loop represented by LOOP_VINFO
   to the minimum between LOOP_NITERS (the original iteration count of the loop)
   and the misalignment of DR - the first data reference recorded in
   LOOP_VINFO_UNALIGNED_DR (LOOP_VINFO).  As a result, after the execution of 
   this loop, the data reference DR will refer to an aligned location.

   The following computation is generated:

   compute address misalignment in bytes:
   addr_mis = addr & (vectype_size - 1)

   prolog_niters = min ( LOOP_NITERS , (VF - addr_mis/elem_size)&(VF-1) )
   
   (elem_size = element type size; an element is the scalar element 
	whose type is the inner type of the vectype)  */

static tree 
vect_gen_niters_for_prolog_loop (loop_vec_info loop_vinfo, tree loop_niters)
{
  struct data_reference *dr = LOOP_VINFO_UNALIGNED_DR (loop_vinfo);
  int vf = LOOP_VINFO_VECT_FACTOR (loop_vinfo);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree var, stmt;
  tree iters, iters_name;
  edge pe;
  basic_block new_bb;
  tree dr_stmt = DR_STMT (dr);
  stmt_vec_info stmt_info = vinfo_for_stmt (dr_stmt);
  tree vectype = STMT_VINFO_VECTYPE (stmt_info);
  int vectype_align = TYPE_ALIGN (vectype) / BITS_PER_UNIT;
  tree elem_misalign;
  tree byte_misalign;
  tree new_stmts = NULL_TREE;
  tree start_addr = 
	vect_create_addr_base_for_vector_ref (dr_stmt, &new_stmts, NULL_TREE);
  tree ptr_type = TREE_TYPE (start_addr);
  tree size = TYPE_SIZE (ptr_type);
  tree type = lang_hooks.types.type_for_size (tree_low_cst (size, 1), 1);
  tree vectype_size_minus_1 = build_int_cst (type, vectype_align - 1);
  tree vf_minus_1 = build_int_cst (unsigned_type_node, vf - 1);
  tree niters_type = TREE_TYPE (loop_niters);
  tree elem_size_log = 
	build_int_cst (unsigned_type_node, exact_log2 (vectype_align/vf));
  tree vf_tree = build_int_cst (unsigned_type_node, vf);

  pe = loop_preheader_edge (loop); 
  new_bb = bsi_insert_on_edge_immediate (pe, new_stmts); 
  gcc_assert (!new_bb);

  /* Create:  byte_misalign = addr & (vectype_size - 1)  */
  byte_misalign = build2 (BIT_AND_EXPR, type, start_addr, vectype_size_minus_1);

  /* Create:  elem_misalign = byte_misalign / element_size  */
  elem_misalign = 
	build2 (RSHIFT_EXPR, unsigned_type_node, byte_misalign, elem_size_log);
  
  /* Create:  (niters_type) (VF - elem_misalign)&(VF - 1)  */
  iters = build2 (MINUS_EXPR, unsigned_type_node, vf_tree, elem_misalign);
  iters = build2 (BIT_AND_EXPR, unsigned_type_node, iters, vf_minus_1);
  iters = fold_convert (niters_type, iters);
  
  /* Create:  prolog_loop_niters = min (iters, loop_niters) */
  /* If the loop bound is known at compile time we already verified that it is
     greater than vf; since the misalignment ('iters') is at most vf, there's
     no need to generate the MIN_EXPR in this case.  */
  if (TREE_CODE (loop_niters) != INTEGER_CST)
    iters = build2 (MIN_EXPR, niters_type, iters, loop_niters);

  var = create_tmp_var (niters_type, "prolog_loop_niters");
  add_referenced_tmp_var (var);
  iters_name = force_gimple_operand (iters, &stmt, false, var);

  /* Insert stmt on loop preheader edge.  */
  pe = loop_preheader_edge (loop);
  if (stmt)
    {
      basic_block new_bb = bsi_insert_on_edge_immediate (pe, stmt);
      gcc_assert (!new_bb);
    }

  return iters_name; 
}


/* Function vect_update_inits_of_dr

   NITERS iterations were peeled from LOOP.  DR represents a data reference
   in LOOP.  This function updates the information recorded in DR to
   account for the fact that the first NITERS iterations had already been 
   executed.  Specifically, it updates the OFFSET field of stmt_info.  */

static void
vect_update_inits_of_dr (struct data_reference *dr, tree niters)
{
  stmt_vec_info stmt_info = vinfo_for_stmt (DR_STMT (dr));
  tree offset = STMT_VINFO_VECT_INIT_OFFSET (stmt_info);
      
  niters = fold (build2 (MULT_EXPR, TREE_TYPE (niters), niters, 
			 STMT_VINFO_VECT_STEP (stmt_info)));
  offset = fold (build2 (PLUS_EXPR, TREE_TYPE (offset), offset, niters));
  STMT_VINFO_VECT_INIT_OFFSET (stmt_info) = offset;
}


/* Function vect_update_inits_of_drs

   NITERS iterations were peeled from the loop represented by LOOP_VINFO.  
   This function updates the information recorded for the data references in 
   the loop to account for the fact that the first NITERS iterations had 
   already been executed.  Specifically, it updates the initial_condition of the
   access_function of all the data_references in the loop.  */

static void
vect_update_inits_of_drs (loop_vec_info loop_vinfo, tree niters)
{
  unsigned int i;
  varray_type loop_write_datarefs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  varray_type loop_read_datarefs = LOOP_VINFO_DATAREF_READS (loop_vinfo);

  if (vect_dump && (dump_flags & TDF_DETAILS))
    fprintf (vect_dump, "=== vect_update_inits_of_dr ===");

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      vect_update_inits_of_dr (dr, niters);
    }

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_read_datarefs, i);
      vect_update_inits_of_dr (dr, niters);
    }
}


/* Function vect_do_peeling_for_alignment

   Peel the first 'niters' iterations of the loop represented by LOOP_VINFO.
   'niters' is set to the misalignment of one of the data references in the
   loop, thereby forcing it to refer to an aligned location at the beginning
   of the execution of this loop.  The data reference for which we are
   peeling is recorded in LOOP_VINFO_UNALIGNED_DR.  */

static void
vect_do_peeling_for_alignment (loop_vec_info loop_vinfo, struct loops *loops)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree niters_of_prolog_loop, ni_name;
  tree n_iters;
  struct loop *new_loop;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_do_peeling_for_alignment ===");

  ni_name = vect_build_loop_niters (loop_vinfo);
  niters_of_prolog_loop = vect_gen_niters_for_prolog_loop (loop_vinfo, ni_name);
  
  /* Peel the prolog loop and iterate it niters_of_prolog_loop.  */
  new_loop = 
	slpeel_tree_peel_loop_to_edge (loop, loops, loop_preheader_edge (loop), 
				       niters_of_prolog_loop, ni_name, true); 
#ifdef ENABLE_CHECKING
  gcc_assert (new_loop);
  slpeel_verify_cfg_after_peeling (new_loop, loop);
#endif

  /* Update number of times loop executes.  */
  n_iters = LOOP_VINFO_NITERS (loop_vinfo);
  LOOP_VINFO_NITERS (loop_vinfo) =
    build2 (MINUS_EXPR, TREE_TYPE (n_iters), n_iters, niters_of_prolog_loop);

  /* Update the init conditions of the access functions of all data refs.  */
  vect_update_inits_of_drs (loop_vinfo, niters_of_prolog_loop);

  /* After peeling we have to reset scalar evolution analyzer.  */
  scev_reset ();

  return;
}


/* Function vect_transform_loop.

   The analysis phase has determined that the loop is vectorizable.
   Vectorize the loop - created vectorized stmts to replace the scalar
   stmts in the loop, and update the loop exit condition.  */

static void
vect_transform_loop (loop_vec_info loop_vinfo, 
		     struct loops *loops ATTRIBUTE_UNUSED)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block *bbs = LOOP_VINFO_BBS (loop_vinfo);
  int nbbs = loop->num_nodes;
  block_stmt_iterator si;
  int i;
  tree ratio = NULL;
  int vectorization_factor = LOOP_VINFO_VECT_FACTOR (loop_vinfo);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vec_transform_loop ===");

  
  /* Peel the loop if there are data refs with unknown alignment.
     Only one data ref with unknown store is allowed.  */

  if (LOOP_DO_PEELING_FOR_ALIGNMENT (loop_vinfo))
    vect_do_peeling_for_alignment (loop_vinfo, loops);
  
  /* If the loop has a symbolic number of iterations 'n' (i.e. it's not a
     compile time constant), or it is a constant that doesn't divide by the
     vectorization factor, then an epilog loop needs to be created.
     We therefore duplicate the loop: the original loop will be vectorized,
     and will compute the first (n/VF) iterations. The second copy of the loop
     will remain scalar and will compute the remaining (n%VF) iterations.
     (VF is the vectorization factor).  */

  if (!LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo)
      || (LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo)
          && LOOP_VINFO_INT_NITERS (loop_vinfo) % vectorization_factor != 0))
    vect_do_peeling_for_loop_bound (loop_vinfo, &ratio, loops);
  else
    ratio = build_int_cst (TREE_TYPE (LOOP_VINFO_NITERS (loop_vinfo)),
		LOOP_VINFO_INT_NITERS (loop_vinfo) / vectorization_factor);

  /* 1) Make sure the loop header has exactly two entries
     2) Make sure we have a preheader basic block.  */

  gcc_assert (EDGE_COUNT (loop->header->preds) == 2);

  loop_split_edge_with (loop_preheader_edge (loop), NULL);


  /* FORNOW: the vectorizer supports only loops which body consist
     of one basic block (header + empty latch). When the vectorizer will 
     support more involved loop forms, the order by which the BBs are 
     traversed need to be reconsidered.  */

  for (i = 0; i < nbbs; i++)
    {
      basic_block bb = bbs[i];

      for (si = bsi_start (bb); !bsi_end_p (si);)
	{
	  tree stmt = bsi_stmt (si);
	  stmt_vec_info stmt_info;
	  bool is_store;

	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "------>vectorizing statement: ");
	      print_generic_expr (vect_dump, stmt, TDF_SLIM);
	    }	
	  stmt_info = vinfo_for_stmt (stmt);
	  gcc_assert (stmt_info);
	  if (!STMT_VINFO_RELEVANT_P (stmt_info))
	    {
	      bsi_next (&si);
	      continue;
	    }
#ifdef ENABLE_CHECKING
	  /* FORNOW: Verify that all stmts operate on the same number of
	             units and no inner unrolling is necessary.  */
	  gcc_assert 
		(GET_MODE_NUNITS (TYPE_MODE (STMT_VINFO_VECTYPE (stmt_info)))
		 == vectorization_factor);
#endif
	  /* -------- vectorize statement ------------ */
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "transform statement.");

	  is_store = vect_transform_stmt (stmt, &si);
	  if (is_store)
	    {
	      /* free the attached stmt_vec_info and remove the stmt.  */
	      stmt_ann_t ann = stmt_ann (stmt);
	      free (stmt_info);
	      set_stmt_info (ann, NULL);
	      bsi_remove (&si);
	      continue;
	    }

	  bsi_next (&si);
	}		        /* stmts in BB */
    }				/* BBs in loop */

  slpeel_make_loop_iterate_ntimes (loop, ratio);

  if (vect_print_dump_info (REPORT_VECTORIZED_LOOPS, LOOP_LOC (loop_vinfo)))
    fprintf (vect_dump, "LOOP VECTORIZED.");
}

/* APPLE LOCAL begin AV cond expr. -dpatel */
/* Patch is waiting FSF review since mid Sep, 2004.  */
/* FUnction vect_is_simple_cond.
  
   Input:
   LOOP - the loop that is being vectorized.
   COND - Condition that is checked for simple use.

   Returns whether a COND can be vectorized. Checkes whether
   condition operands are supportable using vec_is_simple_use.  */

static bool
vect_is_simple_cond (tree cond, loop_vec_info loop_vinfo)
{
  tree lhs, rhs;

  if (TREE_CODE_CLASS (TREE_CODE (cond)) != tcc_comparison)
    return false;

  lhs = TREE_OPERAND (cond, 0);
  rhs = TREE_OPERAND (cond, 1);

  if (TREE_CODE (lhs) == SSA_NAME)
    {
      tree lhs_def_stmt = SSA_NAME_DEF_STMT (lhs);
      if (!vect_is_simple_use (lhs, loop_vinfo, &lhs_def_stmt))
	return false;
    }
  else if (TREE_CODE (lhs) != INTEGER_CST && TREE_CODE (lhs) != REAL_CST)
    return false;

  if (TREE_CODE (rhs) == SSA_NAME)
    {
      tree rhs_def_stmt = SSA_NAME_DEF_STMT (rhs);
      if (!vect_is_simple_use (rhs, loop_vinfo, &rhs_def_stmt))
	return false;
    }
  else if (TREE_CODE (rhs) != INTEGER_CST  && TREE_CODE (rhs) != REAL_CST)
    return false;

  return true;
}
/* APPLE LOCAL end AV cond expr. -dpatel */

/* Function vect_is_simple_use.

   Input:
   LOOP - the loop that is being vectorized.
   OPERAND - operand of a stmt in LOOP.
   DEF - the defining stmt in case OPERAND is an SSA_NAME.

   Returns whether a stmt with OPERAND can be vectorized.
   Supportable operands are constants, loop invariants, and operands that are
   defined by the current iteration of the loop. Unsupportable operands are 
   those that are defined by a previous iteration of the loop (as is the case
   in reduction/induction computations).  */

static bool
vect_is_simple_use (tree operand, loop_vec_info loop_vinfo, tree *def)
{ 
  tree def_stmt;
  basic_block bb;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);

  if (def)
    *def = NULL_TREE;

  if (TREE_CODE (operand) == INTEGER_CST || TREE_CODE (operand) == REAL_CST)
    return true;

  if (TREE_CODE (operand) != SSA_NAME)
    return false;

  def_stmt = SSA_NAME_DEF_STMT (operand);
  if (def_stmt == NULL_TREE )
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "no def_stmt.");
      return false;
    }

  /* empty stmt is expected only in case of a function argument.
     (Otherwise - we expect a phi_node or a modify_expr).  */
  if (IS_EMPTY_STMT (def_stmt))
    {
      tree arg = TREE_OPERAND (def_stmt, 0);
      if (TREE_CODE (arg) == INTEGER_CST || TREE_CODE (arg) == REAL_CST)
	return true;
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
	  fprintf (vect_dump, "Unexpected empty stmt: ");
	  print_generic_expr (vect_dump, def_stmt, TDF_SLIM);
	}
      return false;  
    }

  /* phi_node inside the loop indicates an induction/reduction pattern.
     This is not supported yet.  */
  bb = bb_for_stmt (def_stmt);
  if (TREE_CODE (def_stmt) == PHI_NODE && flow_bb_inside_loop_p (loop, bb))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "reduction/induction - unsupported.");
      return false; /* FORNOW: not supported yet.  */
    }

  /* Expecting a modify_expr or a phi_node.  */
  if (TREE_CODE (def_stmt) == MODIFY_EXPR
      || TREE_CODE (def_stmt) == PHI_NODE)
    {
      if (def)
        *def = def_stmt; 	
      return true;
    }

  return false;
}


/* Function vect_analyze_operations.

   Scan the loop stmts and make sure they are all vectorizable.  */

static bool
vect_analyze_operations (loop_vec_info loop_vinfo)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block *bbs = LOOP_VINFO_BBS (loop_vinfo);
  int nbbs = loop->num_nodes;
  block_stmt_iterator si;
  unsigned int vectorization_factor = 0;
  int i;
  bool ok;
  tree scalar_type;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_operations ===");

  for (i = 0; i < nbbs; i++)
    {
      basic_block bb = bbs[i];

      for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
	{
	  tree stmt = bsi_stmt (si);
	  unsigned int nunits;
	  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
	  tree vectype;

	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "==> examining statement: ");
	      print_generic_expr (vect_dump, stmt, TDF_SLIM);
	    }

	  gcc_assert (stmt_info);

	  /* skip stmts which do not need to be vectorized.
	     this is expected to include:
	     - the COND_EXPR which is the loop exit condition
	     - any LABEL_EXPRs in the loop
	     - computations that are used only for array indexing or loop
	     control  */

	  if (!STMT_VINFO_RELEVANT_P (stmt_info))
	    {
	      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	        fprintf (vect_dump, "irrelevant.");
	      continue;
	    }

	  if (VECTOR_MODE_P (TYPE_MODE (TREE_TYPE (stmt))))
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                         LOOP_LOC (loop_vinfo)))
		{
                  fprintf (vect_dump, "not vectorized: vector stmt in loop:");
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		}
	      return false;
	    }

          if (STMT_VINFO_DATA_REF (stmt_info))
            scalar_type = TREE_TYPE (DR_REF (STMT_VINFO_DATA_REF (stmt_info)));    
          else if (TREE_CODE (stmt) == MODIFY_EXPR)
	    scalar_type = TREE_TYPE (TREE_OPERAND (stmt, 0));
	  else
	    scalar_type = TREE_TYPE (stmt);

	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "get vectype for scalar type:  ");
	      print_generic_expr (vect_dump, scalar_type, TDF_SLIM);
	    }

	  vectype = get_vectype_for_scalar_type (scalar_type);
	  if (!vectype)
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                         LOOP_LOC (loop_vinfo)))
		{
                  fprintf (vect_dump,
                           "not vectorized: unsupported data-type ");
		  print_generic_expr (vect_dump, scalar_type, TDF_SLIM);
		}
	      return false;
	    }

	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "vectype: ");
	      print_generic_expr (vect_dump, vectype, TDF_SLIM);
	    }
	  STMT_VINFO_VECTYPE (stmt_info) = vectype;

	  ok = (vectorizable_operation (stmt, NULL, NULL)
		|| vectorizable_assignment (stmt, NULL, NULL)
		|| vectorizable_load (stmt, NULL, NULL)
		/* APPLE LOCAL AV cond expr. -dpatel */
		|| vectorizable_select (stmt, NULL, NULL)
		|| vectorizable_store (stmt, NULL, NULL));

	  if (!ok)
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                         LOOP_LOC (loop_vinfo)))
		{
                  fprintf (vect_dump, "not vectorized: stmt not supported: ");
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		}
	      return false;
	    }

	  nunits = GET_MODE_NUNITS (TYPE_MODE (vectype));
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "nunits = %d", nunits);

	  if (vectorization_factor)
	    {
	      /* FORNOW: don't allow mixed units.
	         This restriction will be relaxed in the future.  */
	      if (nunits != vectorization_factor)
		{
	          if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                             LOOP_LOC (loop_vinfo)))
		    fprintf (vect_dump, "not vectorized: mixed data-types");
		  return false;
		}
	    }
	  else
	    vectorization_factor = nunits;

#ifdef ENABLE_CHECKING
	  gcc_assert (GET_MODE_SIZE (TYPE_MODE (scalar_type))
			* vectorization_factor == UNITS_PER_SIMD_WORD);
#endif
	}
    }

  /* TODO: Analyze cost. Decide if worth while to vectorize.  */

  if (vectorization_factor <= 1)
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                 LOOP_LOC (loop_vinfo)))
        fprintf (vect_dump, "not vectorized: unsupported data-type");
      return false;
    }
  LOOP_VINFO_VECT_FACTOR (loop_vinfo) = vectorization_factor;

  if (LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo)
      && vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump,
        "vectorization_factor = %d, niters = " HOST_WIDE_INT_PRINT_DEC,
        vectorization_factor, LOOP_VINFO_INT_NITERS (loop_vinfo));

  if (LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo)
      && LOOP_VINFO_INT_NITERS (loop_vinfo) < vectorization_factor)
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                 LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "not vectorized: iteration count too small.");
      return false;
    }

  if (!LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo)
      || LOOP_VINFO_INT_NITERS (loop_vinfo) % vectorization_factor != 0)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
        fprintf (vect_dump, "epilog loop required.");
      if (!vect_can_advance_ivs_p (loop_vinfo))
        {
          if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                     LOOP_LOC (loop_vinfo)))
            fprintf (vect_dump,
                     "not vectorized: can't create epilog loop 1.");
          return false;
        }
      if (!slpeel_can_duplicate_loop_p (loop, loop->exit_edges[0]))
        {
          if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                     LOOP_LOC (loop_vinfo)))
            fprintf (vect_dump,
                     "not vectorized: can't create epilog loop 2.");
          return false;
        }
    }

  return true;
}


/* Function exist_non_indexing_operands_for_use_p 

   USE is one of the uses attached to STMT. Check if USE is 
   used in STMT for anything other than indexing an array.  */

static bool
exist_non_indexing_operands_for_use_p (tree use, tree stmt)
{
  tree operand;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
 
  /* USE corresponds to some operand in STMT. If there is no data
     reference in STMT, then any operand that corresponds to USE
     is not indexing an array.  */
  if (!STMT_VINFO_DATA_REF (stmt_info))
    return true;
 
  /* STMT has a data_ref. FORNOW this means that its of one of
     the following forms:
     -1- ARRAY_REF = var
     -2- var = ARRAY_REF
     (This should have been verified in analyze_data_refs).

     'var' in the second case corresponds to a def, not a use,
     so USE cannot correspond to any operands that are not used 
     for array indexing.

     Therefore, all we need to check is if STMT falls into the
     first case, and whether var corresponds to USE.  */
 
  if (TREE_CODE (TREE_OPERAND (stmt, 0)) == SSA_NAME)
    return false;

  operand = TREE_OPERAND (stmt, 1);

  if (TREE_CODE (operand) != SSA_NAME)
    return false;

  if (operand == use)
    return true;

  return false;
}


/* Function vect_is_simple_iv_evolution.

   FORNOW: A simple evolution of an induction variables in the loop is
   considered a polynomial evolution with constant step.  */

static bool
vect_is_simple_iv_evolution (unsigned loop_nb, tree access_fn, tree * init, 
			     tree * step)
{
  tree init_expr;
  tree step_expr;
  
  tree evolution_part = evolution_part_in_loop_num (access_fn, loop_nb);

  /* When there is no evolution in this loop, the evolution function
     is not "simple".  */  
  if (evolution_part == NULL_TREE)
    return false;
  
  /* When the evolution is a polynomial of degree >= 2
     the evolution function is not "simple".  */
  if (tree_is_chrec (evolution_part))
    return false;
  
  step_expr = evolution_part;
  init_expr = unshare_expr (initial_condition_in_loop_num (access_fn,
                                                           loop_nb));

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "step: ");
      print_generic_expr (vect_dump, step_expr, TDF_SLIM);
      fprintf (vect_dump, ",  init: ");
      print_generic_expr (vect_dump, init_expr, TDF_SLIM);
    }

  *init = init_expr;
  *step = step_expr;

  if (TREE_CODE (step_expr) != INTEGER_CST)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "step unknown.");
      return false;
    }

  return true;
}


/* Function vect_analyze_scalar_cycles.

   Examine the cross iteration def-use cycles of scalar variables, by
   analyzing the loop (scalar) PHIs; verify that the cross iteration def-use
   cycles that they represent do not impede vectorization.

   FORNOW: Reduction as in the following loop, is not supported yet:
              loop1:
              for (i=0; i<N; i++)
                 sum += a[i];
	   The cross-iteration cycle corresponding to variable 'sum' will be
	   considered too complicated and will impede vectorization.

   FORNOW: Induction as in the following loop, is not supported yet:
              loop2:
              for (i=0; i<N; i++)
                 a[i] = i;

           However, the following loop *is* vectorizable:
              loop3:
              for (i=0; i<N; i++)
                 a[i] = b[i];

           In both loops there exists a def-use cycle for the variable i:
              loop: i_2 = PHI (i_0, i_1)
                    a[i_2] = ...;
                    i_1 = i_2 + 1;
                    GOTO loop;

           The evolution of the above cycle is considered simple enough,
	   however, we also check that the cycle does not need to be
	   vectorized, i.e - we check that the variable that this cycle
	   defines is only used for array indexing or in stmts that do not
	   need to be vectorized. This is not the case in loop2, but it
	   *is* the case in loop3.  */

static bool
vect_analyze_scalar_cycles (loop_vec_info loop_vinfo)
{
  tree phi;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block bb = loop->header;
  tree dummy;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_scalar_cycles ===");

  for (phi = phi_nodes (bb); phi; phi = PHI_CHAIN (phi))
    {
      tree access_fn = NULL;

      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
          fprintf (vect_dump, "Analyze phi: ");
          print_generic_expr (vect_dump, phi, TDF_SLIM);
	}

      /* Skip virtual phi's. The data dependences that are associated with
         virtual defs/uses (i.e., memory accesses) are analyzed elsewhere.  */

      if (!is_gimple_reg (SSA_NAME_VAR (PHI_RESULT (phi))))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "virtual phi. skip.");
	  continue;
	}

      /* Analyze the evolution function.  */

      /* FORNOW: The only scalar cross-iteration cycles that we allow are
         those of loop induction variables; This property is verified here.

         Furthermore, if that induction variable is used in an operation
         that needs to be vectorized (i.e, is not solely used to index
         arrays and check the exit condition) - we do not support its
         vectorization yet. This property is verified in vect_is_simple_use,
         during vect_analyze_operations.  */

      access_fn = /* instantiate_parameters
		     (loop,*/
	 analyze_scalar_evolution (loop, PHI_RESULT (phi));

      if (!access_fn)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: unsupported scalar cycle.");
	  return false;
	}

      if (vect_print_dump_info (REPORT_DETAILS,
				LOOP_LOC (loop_vinfo)))
        {
           fprintf (vect_dump, "Access function of PHI: ");
           print_generic_expr (vect_dump, access_fn, TDF_SLIM);
        }

      if (!vect_is_simple_iv_evolution (loop->num, access_fn, &dummy, &dummy))
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: unsupported scalar cycle.");
	  return false;
	}
    }

  return true;
}

/* APPLE LOCAL begin AV data dependence. -dpatel */
/* Patch is waiting FSF review since mid Sep, 2004.  */
/* Function vect_build_dist_vector.

   Build classic dist vector for dependence relation DDR using LOOP's loop
   nest. Return LOOP's depth in its loop nest.  */

static unsigned int
vect_build_dist_vector (struct loop *loop,
			struct data_dependence_relation *ddr)
{
  struct loop *loop_nest = loop;
  unsigned int loop_depth = 1;

  /* Find loop nest and loop depth.  */
  while (loop_nest)
    {
      if (loop_nest->outer && loop_nest->outer->outer)
	{
	  loop_nest = loop_nest->outer;
	  loop_depth++;
	}
      else
	break;
    }

  /* Compute distance vector.  */
  compute_subscript_distance (ddr);
  build_classic_dist_vector (ddr, loops_num, loop_nest->depth);

  return loop_depth - 1;
}
/* APPLE LOCAL end AV data dependence. -dpatel */

/* Function vect_base_addr_differ_p.

   This is the simplest data dependence test: determines whether the
   data references A and B access the same array/region.  Returns
   false when the property is not computable at compile time.
   Otherwise return true, and DIFFER_P will record the result. This
   utility will not be necessary when alias_sets_conflict_p will be
   less conservative.  */


static bool
vect_base_addr_differ_p (struct data_reference *dra,
			 struct data_reference *drb,
			 bool *differ_p)
{
  tree stmt_a = DR_STMT (dra);
  stmt_vec_info stmt_info_a = vinfo_for_stmt (stmt_a);   
  tree stmt_b = DR_STMT (drb);
  stmt_vec_info stmt_info_b = vinfo_for_stmt (stmt_b);   
  tree addr_a = STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info_a);
  tree addr_b = STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info_b);
  tree type_a = TREE_TYPE (addr_a);
  tree type_b = TREE_TYPE (addr_b);
  HOST_WIDE_INT alias_set_a, alias_set_b;

  gcc_assert (POINTER_TYPE_P (type_a) &&  POINTER_TYPE_P (type_b));
  
  /* Both references are ADDR_EXPR, i.e., we have the objects.  */
  if (TREE_CODE (addr_a) == ADDR_EXPR && TREE_CODE (addr_b) == ADDR_EXPR)
    return array_base_name_differ_p (dra, drb, differ_p);  

  alias_set_a = (TREE_CODE (addr_a) == ADDR_EXPR) ? 
    get_alias_set (TREE_OPERAND (addr_a, 0)) : get_alias_set (addr_a);
  alias_set_b = (TREE_CODE (addr_b) == ADDR_EXPR) ? 
    get_alias_set (TREE_OPERAND (addr_b, 0)) : get_alias_set (addr_b);

  if (!alias_sets_conflict_p (alias_set_a, alias_set_b))
    {
      *differ_p = true;
      return true;
    }
  
  /* An instruction writing through a restricted pointer is "independent" of any 
     instruction reading or writing through a different pointer, in the same 
     block/scope.  */
  else if ((TYPE_RESTRICT (type_a) && !DR_IS_READ (dra))
      || (TYPE_RESTRICT (type_b) && !DR_IS_READ (drb)))
    {
      *differ_p = true;
      return true;
    }
  return false;
}


/* Function vect_analyze_data_ref_dependence.

   Return TRUE if there (might) exist a dependence between a memory-reference
   DRA and a memory-reference DRB.  */

static bool
vect_analyze_data_ref_dependence (struct data_reference *dra,
				  struct data_reference *drb, 
				  loop_vec_info loop_vinfo)
{
  bool differ_p; 
  struct data_dependence_relation *ddr;
  /* APPLE LOCAL begin AV data dependence. -dpatel */
  int vectorization_factor = LOOP_VINFO_VECT_FACTOR (loop_vinfo);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  unsigned int loop_depth = 0;
  int dist;
  /* APPLE LOCAL end AV data dependence. -dpatel */
  
  if (!vect_base_addr_differ_p (dra, drb, &differ_p))
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo)))
        {
          fprintf (vect_dump,
                "not vectorized: can't determine dependence between: ");
          print_generic_expr (vect_dump, DR_REF (dra), TDF_SLIM);
          fprintf (vect_dump, " and ");
          print_generic_expr (vect_dump, DR_REF (drb), TDF_SLIM);
        }
      return true;
    }

  if (differ_p)
    return false;

  ddr = initialize_data_dependence_relation (dra, drb);
  compute_affine_dependence (ddr);

  if (DDR_ARE_DEPENDENT (ddr) == chrec_known)
    return false;

  /* APPLE LOCAL begin AV data dependence. -dpatel */
  if (DDR_ARE_DEPENDENT (ddr) == chrec_dont_know)
    return true;

  loop_depth = vect_build_dist_vector (loop, ddr);

  if (!DDR_DIST_VECT (ddr))
    return true;

  dist = DDR_DIST_VECT (ddr)[loop_depth];

  /* Same loop iteration.  */
  if (dist == 0)
     return false;

  if (dist >= vectorization_factor)
    /* Dependence distance does not create dependence, as far as vectorization
       is concerned, in this case.  */
    return false;
  /* APPLE LOCAL end AV data dependence. -dpatel */
  
  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
			    LOOP_LOC (loop_vinfo)))
    {
      fprintf (vect_dump,
	"not vectorized: possible dependence between data-refs ");
      print_generic_expr (vect_dump, DR_REF (dra), TDF_SLIM);
      fprintf (vect_dump, " and ");
      print_generic_expr (vect_dump, DR_REF (drb), TDF_SLIM);
    }

  return true;
}


/* Function vect_analyze_data_ref_dependences.

   Examine all the data references in the loop, and make sure there do not
   exist any data dependences between them.

   TODO: dependences which distance is greater than the vectorization factor
         can be ignored.  */

static bool
vect_analyze_data_ref_dependences (loop_vec_info loop_vinfo)
{
  unsigned int i, j;
  varray_type loop_write_refs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  varray_type loop_read_refs = LOOP_VINFO_DATAREF_READS (loop_vinfo);

  /* Examine store-store (output) dependences.  */

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_dependences ===");

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "compare all store-store pairs.");

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_refs); i++)
    {
      for (j = i + 1; j < VARRAY_ACTIVE_SIZE (loop_write_refs); j++)
	{
	  struct data_reference *dra =
	    VARRAY_GENERIC_PTR (loop_write_refs, i);
	  struct data_reference *drb =
	    VARRAY_GENERIC_PTR (loop_write_refs, j);
	  if (vect_analyze_data_ref_dependence (dra, drb, loop_vinfo))
	    return false;
	}
    }

  /* Examine load-store (true/anti) dependences.  */

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "compare all load-store pairs.");

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_refs); i++)
    {
      for (j = 0; j < VARRAY_ACTIVE_SIZE (loop_write_refs); j++)
	{
	  struct data_reference *dra = VARRAY_GENERIC_PTR (loop_read_refs, i);
	  struct data_reference *drb =
	    VARRAY_GENERIC_PTR (loop_write_refs, j);
	  if (vect_analyze_data_ref_dependence (dra, drb, loop_vinfo))
	    return false;
	}
    }

  return true;
}


/* Function vect_compute_data_ref_alignment

   Compute the misalignment of the data reference DR.

   Output:
   1. If during the misalignment computation it is found that the data reference
      cannot be vectorized then false is returned.
   2. DR_MISALIGNMENT (DR) is defined.

   FOR NOW: No analysis is actually performed. Misalignment is calculated
   only for trivial cases. TODO.  */

static bool
vect_compute_data_ref_alignment (struct data_reference *dr)
{
  tree stmt = DR_STMT (dr);
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);  
  tree ref = DR_REF (dr);
  tree vectype;
  tree base, alignment;
  bool base_aligned_p;
  tree misalign;
   
  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "vect_compute_data_ref_alignment:");

  /* Initialize misalignment to unknown.  */
  DR_MISALIGNMENT (dr) = -1;

  misalign = STMT_VINFO_VECT_MISALIGNMENT (stmt_info);
  base_aligned_p = STMT_VINFO_VECT_BASE_ALIGNED_P (stmt_info);
  base = build_fold_indirect_ref (STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info));
  vectype = STMT_VINFO_VECTYPE (stmt_info);

  if (!misalign)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC)) 
	{
	  fprintf (vect_dump, "Unknown alignment for access: ");
	  print_generic_expr (vect_dump, base, TDF_SLIM);
	}
      return true;
    }

  if (!base_aligned_p) 
    {
      if (!vect_can_force_dr_alignment_p (base, TYPE_ALIGN (vectype)))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "can't force alignment of ref: ");
	      print_generic_expr (vect_dump, ref, TDF_SLIM);
	    }
	  return true;
	}
      
      /* Force the alignment of the decl.
	 NOTE: This is the only change to the code we make during
	 the analysis phase, before deciding to vectorize the loop.  */
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "force alignment");
      DECL_ALIGN (base) = TYPE_ALIGN (vectype);
      DECL_USER_ALIGN (base) = 1;
    }

  /* At this point we assume that the base is aligned.  */
  gcc_assert (base_aligned_p 
	      || (TREE_CODE (base) == VAR_DECL 
		  && DECL_ALIGN (base) >= TYPE_ALIGN (vectype)));

  /* Alignment required, in bytes:  */
  alignment = ssize_int (TYPE_ALIGN (vectype)/BITS_PER_UNIT);

  /* Modulo alignment.  */
  misalign = size_binop (TRUNC_MOD_EXPR, misalign, alignment);
  if (tree_int_cst_sgn (misalign) < 0)
    {
      /* Negative misalignment value.  */
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "unexpected misalign value");
      return false;
    }

  DR_MISALIGNMENT (dr) = tree_low_cst (misalign, 1);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "misalign = %d bytes", DR_MISALIGNMENT (dr));

  return true;
}


/* Function vect_compute_data_refs_alignment

   Compute the misalignment of data references in the loop.
   This pass may take place at function granularity instead of at loop
   granularity.

   FOR NOW: No analysis is actually performed. Misalignment is calculated
   only for trivial cases. TODO.  */

static bool
vect_compute_data_refs_alignment (loop_vec_info loop_vinfo)
{
  varray_type loop_write_datarefs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  varray_type loop_read_datarefs = LOOP_VINFO_DATAREF_READS (loop_vinfo);
  unsigned int i;

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      if (!vect_compute_data_ref_alignment (dr))
	return false;
    }

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_read_datarefs, i);
      if (!vect_compute_data_ref_alignment (dr))
	return false;
    }

  return true;
}


/* Function vect_enhance_data_refs_alignment

   This pass will use loop versioning and loop peeling in order to enhance
   the alignment of data references in the loop.

   FOR NOW: we assume that whatever versioning/peeling takes place, only the
   original loop is to be vectorized; Any other loops that are created by
   the transformations performed in this pass - are not supposed to be
   vectorized. This restriction will be relaxed.  */

static void
vect_enhance_data_refs_alignment (loop_vec_info loop_vinfo)
{
  varray_type loop_read_datarefs = LOOP_VINFO_DATAREF_READS (loop_vinfo);
  varray_type loop_write_datarefs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  unsigned int i;

  /*
     This pass will require a cost model to guide it whether to apply peeling 
     or versioning or a combination of the two. For example, the scheme that
     intel uses when given a loop with several memory accesses, is as follows:
     choose one memory access ('p') which alignment you want to force by doing 
     peeling. Then, either (1) generate a loop in which 'p' is aligned and all 
     other accesses are not necessarily aligned, or (2) use loop versioning to 
     generate one loop in which all accesses are aligned, and another loop in 
     which only 'p' is necessarily aligned. 

     ("Automatic Intra-Register Vectorization for the Intel Architecture",
      Aart J.C. Bik, Milind Girkar, Paul M. Grey and Ximmin Tian, International
      Journal of Parallel Programming, Vol. 30, No. 2, April 2002.)	

     Devising a cost model is the most critical aspect of this work. It will 
     guide us on which access to peel for, whether to use loop versioning, how 
     many versions to create, etc. The cost model will probably consist of 
     generic considerations as well as target specific considerations (on 
     powerpc for example, misaligned stores are more painful than misaligned 
     loads). 

     Here is the general steps involved in alignment enhancements:
    
     -- original loop, before alignment analysis:
	for (i=0; i<N; i++){
	  x = q[i];			# DR_MISALIGNMENT(q) = unknown
	  p[i] = y;			# DR_MISALIGNMENT(p) = unknown
	}

     -- After vect_compute_data_refs_alignment:
	for (i=0; i<N; i++){
	  x = q[i];			# DR_MISALIGNMENT(q) = 3
	  p[i] = y;			# DR_MISALIGNMENT(p) = unknown
	}

     -- Possibility 1: we do loop versioning:
     if (p is aligned) {
	for (i=0; i<N; i++){	# loop 1A
	  x = q[i];			# DR_MISALIGNMENT(q) = 3
	  p[i] = y;			# DR_MISALIGNMENT(p) = 0
	}
     } 
     else {
	for (i=0; i<N; i++){	# loop 1B
	  x = q[i];			# DR_MISALIGNMENT(q) = 3
	  p[i] = y;			# DR_MISALIGNMENT(p) = unaligned
	}
     }
   
     -- Possibility 2: we do loop peeling:
     for (i = 0; i < 3; i++){	# (scalar loop, not to be vectorized).
	x = q[i];
	p[i] = y;
     }
     for (i = 3; i < N; i++){	# loop 2A
	x = q[i];			# DR_MISALIGNMENT(q) = 0
	p[i] = y;			# DR_MISALIGNMENT(p) = unknown
     }

     -- Possibility 3: combination of loop peeling and versioning:
     for (i = 0; i < 3; i++){	# (scalar loop, not to be vectorized).
	x = q[i];
	p[i] = y;
     }
     if (p is aligned) {
	for (i = 3; i<N; i++){  # loop 3A
	  x = q[i];			# DR_MISALIGNMENT(q) = 0
	  p[i] = y;			# DR_MISALIGNMENT(p) = 0
	}
     } 
     else {
	for (i = 3; i<N; i++){	# loop 3B
	  x = q[i];			# DR_MISALIGNMENT(q) = 0
	  p[i] = y;			# DR_MISALIGNMENT(p) = unaligned
	}
     }

     These loops are later passed to loop_transform to be vectorized. The 
     vectorizer will use the alignment information to guide the transformation 
     (whether to generate regular loads/stores, or with special handling for 
     misalignment). 
   */

  /* (1) Peeling to force alignment.  */

  /* (1.1) Decide whether to perform peeling, and how many iterations to peel:
     Considerations:
     + How many accesses will become aligned due to the peeling
     - How many accesses will become unaligned due to the peeling,
       and the cost of misaligned accesses.
     - The cost of peeling (the extra runtime checks, the increase 
       in code size).

     The scheme we use FORNOW: peel to force the alignment of the first
     misaligned store in the loop.
     Rationale: misaligned stores are not yet supported.

     TODO: Use a better cost model.  */

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      if (!aligned_access_p (dr))
        {
          LOOP_VINFO_UNALIGNED_DR (loop_vinfo) = dr;
          LOOP_DO_PEELING_FOR_ALIGNMENT (loop_vinfo) = true;
	  break;
        }
    }

  if (!LOOP_VINFO_UNALIGNED_DR (loop_vinfo))
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "Peeling for alignment will not be applied.");
      return;
    }
  else
    if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
      fprintf (vect_dump, "Peeling for alignment will be applied.");


  /* (1.2) Update the alignment info according to the peeling factor.
	   If the misalignment of the DR we peel for is M, then the
	   peeling factor is VF - M, and the misalignment of each access DR_i
	   in the loop is DR_MISALIGNMENT (DR_i) + VF - M.
	   If the misalignment of the DR we peel for is unknown, then the 
	   misalignment of each access DR_i in the loop is also unknown.

	   FORNOW: set the misalignment of the accesses to unknown even
	           if the peeling factor is known at compile time.

	   TODO: - if the peeling factor is known at compile time, use that
		   when updating the misalignment info of the loop DRs.
		 - consider accesses that are known to have the same 
		   alignment, even if that alignment is unknown.  */
   
  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      if (dr == LOOP_VINFO_UNALIGNED_DR (loop_vinfo))
	{
	  DR_MISALIGNMENT (dr) = 0;
	  if (vect_print_dump_info (REPORT_ALIGNMENT, LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "Alignment of access forced using peeling.");
	}
      else
	DR_MISALIGNMENT (dr) = -1;
    }
  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_read_datarefs, i);
      if (dr == LOOP_VINFO_UNALIGNED_DR (loop_vinfo))
	{
	  DR_MISALIGNMENT (dr) = 0;
	  if (vect_print_dump_info (REPORT_ALIGNMENT, LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "Alignment of access forced using peeling.");
	}
      else
	DR_MISALIGNMENT (dr) = -1;
    }
}


/* Function vect_analyze_data_refs_alignment

   Analyze the alignment of the data-references in the loop.
   FOR NOW: Until support for misliagned accesses is in place, only if all
   accesses are aligned can the loop be vectorized. This restriction will be 
   relaxed.  */ 

static bool
vect_analyze_data_refs_alignment (loop_vec_info loop_vinfo)
{
  varray_type loop_read_datarefs = LOOP_VINFO_DATAREF_READS (loop_vinfo);
  varray_type loop_write_datarefs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  enum dr_alignment_support supportable_dr_alignment;
  unsigned int i;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_data_refs_alignment ===");


  /* This pass may take place at function granularity instead of at loop
     granularity.  */

  if (!vect_compute_data_refs_alignment (loop_vinfo))
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, 
		 "not vectorized: can't calculate alignment for data ref.");
      return false;
    }


  /* This pass will decide on using loop versioning and/or loop peeling in 
     order to enhance the alignment of data references in the loop.  */

  vect_enhance_data_refs_alignment (loop_vinfo);


  /* Finally, check that all the data references in the loop can be
     handled with respect to their alignment.  */

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_read_datarefs, i);
      supportable_dr_alignment = vect_supportable_dr_alignment (dr);
      if (!supportable_dr_alignment)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: unsupported unaligned load.");
	  return false;
	}
      if (supportable_dr_alignment != dr_aligned 
	  && (vect_print_dump_info (REPORT_ALIGNMENT, LOOP_LOC (loop_vinfo))))
	fprintf (vect_dump, "Vectorizing an unaligned access.");
    }
  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      supportable_dr_alignment = vect_supportable_dr_alignment (dr);
      if (!supportable_dr_alignment)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: unsupported unaligned store.");
	  return false;
	}
      if (supportable_dr_alignment != dr_aligned 
	  && (vect_print_dump_info (REPORT_ALIGNMENT, LOOP_LOC (loop_vinfo))))
	fprintf (vect_dump, "Vectorizing an unaligned access.");
    }

  return true;
}


/* Function vect_analyze_data_ref_access.

   Analyze the access pattern of the data-reference DR. For now, a data access
   has to consecutive to be considered vectorizable.  */

static bool
vect_analyze_data_ref_access (struct data_reference *dr)
{
  tree stmt = DR_STMT (dr);
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt); 
  tree step = STMT_VINFO_VECT_STEP (stmt_info);
  tree scalar_type = TREE_TYPE (DR_REF (dr));

  if (!step || tree_int_cst_compare (step, TYPE_SIZE_UNIT (scalar_type)))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "not consecutive access");
      return false;
    }
  return true;
}


/* Function vect_analyze_data_ref_accesses.

   Analyze the access pattern of all the data references in the loop.

   FORNOW: the only access pattern that is considered vectorizable is a
	   simple step 1 (consecutive) access.

   FORNOW: handle only arrays and pointer accesses.  */

static bool
vect_analyze_data_ref_accesses (loop_vec_info loop_vinfo)
{
  unsigned int i;
  varray_type loop_write_datarefs = LOOP_VINFO_DATAREF_WRITES (loop_vinfo);
  varray_type loop_read_datarefs = LOOP_VINFO_DATAREF_READS (loop_vinfo);

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_data_ref_accesses ===");

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_write_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_write_datarefs, i);
      bool ok = vect_analyze_data_ref_access (dr);
      if (!ok)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
                                      LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: complicated access pattern.");
	  return false;
	}
    }

  for (i = 0; i < VARRAY_ACTIVE_SIZE (loop_read_datarefs); i++)
    {
      struct data_reference *dr = VARRAY_GENERIC_PTR (loop_read_datarefs, i);
      bool ok = vect_analyze_data_ref_access (dr);
      if (!ok)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: complicated access pattern.");
	  return false;
	}
    }

  return true;
}


/* Function vect_analyze_pointer_ref_access.

   Input:
   STMT - a stmt that contains a data-ref.
   MEMREF - a data-ref in STMT, which is an INDIRECT_REF.
   ACCESS_FN - the access function of MEMREF.

   Output:
   If the data-ref access is vectorizable, return a data_reference structure
   that represents it (DR). Otherwise - return NULL.  
   STEP - the stride of MEMREF in the loop.
   INIT - the initial condition of MEMREF in the loop.
*/

/* APPLE LOCAL loops-to-memset  */
struct data_reference *
vect_analyze_pointer_ref_access (tree memref, tree stmt, bool is_read, 
				 tree access_fn, tree *ptr_init, tree *ptr_step)
{
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  tree step, init;	
  tree reftype, innertype;
  tree indx_access_fn; 
  int loopnum = loop->num;
  struct data_reference *dr;

  if (!vect_is_simple_iv_evolution (loopnum, access_fn, &init, &step))
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS, 
				LOOP_LOC (loop_vinfo))) 
	fprintf (vect_dump, "not vectorized: pointer access is not simple.");	
      return NULL;
    }

  STRIP_NOPS (init);

  if (!expr_invariant_in_loop_p (loop, init))
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo))) 
	fprintf (vect_dump, 
		 "not vectorized: initial condition is not loop invariant.");	
      return NULL;
    }

  if (TREE_CODE (step) != INTEGER_CST)
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo))) 
	fprintf (vect_dump, 
		"not vectorized: non constant step for pointer access.");	
      return NULL;
    }

  reftype = TREE_TYPE (TREE_OPERAND (memref, 0));
  if (TREE_CODE (reftype) != POINTER_TYPE) 
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "not vectorized: unexpected pointer access form.");	
      return NULL;
    }

  reftype = TREE_TYPE (init);
  if (TREE_CODE (reftype) != POINTER_TYPE) 
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo))) 
	fprintf (vect_dump, "not vectorized: unexpected pointer access form.");
      return NULL;
    }

  *ptr_step = fold_convert (ssizetype, step);
  innertype = TREE_TYPE (reftype);
  /* Check that STEP is a multiple of type size.  */
  if (!integer_zerop (size_binop (TRUNC_MOD_EXPR, *ptr_step, 
 		        fold_convert (ssizetype, TYPE_SIZE_UNIT (innertype)))))
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				LOOP_LOC (loop_vinfo))) 
	fprintf (vect_dump, "not vectorized: non consecutive access.");	
      return NULL;
    }
   
  indx_access_fn = 
	build_polynomial_chrec (loopnum, integer_zero_node, integer_one_node);
  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "Access function of ptr indx: ");
      print_generic_expr (vect_dump, indx_access_fn, TDF_SLIM);
    }
  dr = init_data_ref (stmt, memref, NULL_TREE, indx_access_fn, is_read);
  *ptr_init = init;
  return dr;
}


/* Function vect_get_memtag.  

   The function returns the relevant variable for memory tag (for aliasing 
   purposes).  */

static tree
vect_get_memtag (tree memref, struct data_reference *dr)
{
  tree symbl, tag;

  switch (TREE_CODE (memref))
    {
    case SSA_NAME:
      symbl = SSA_NAME_VAR (memref);
      tag = get_var_ann (symbl)->type_mem_tag;
      if (!tag)
	{
	  tree ptr = TREE_OPERAND (DR_REF (dr), 0);
	  if (TREE_CODE (ptr) == SSA_NAME)
	    tag = get_var_ann (SSA_NAME_VAR (ptr))->type_mem_tag;
	}
      return tag;

    case ADDR_EXPR:
      return TREE_OPERAND (memref, 0);

    default:
      return NULL_TREE;
    }  
}


/* Function vect_address_analysis

   Return the BASE of the address expression EXPR.
   Also compute the INITIAL_OFFSET from BASE, MISALIGN and STEP.

   Input:
   EXPR - the address expression that is being analyzed
   STMT - the statement that contains EXPR or its original memory reference
   IS_READ - TRUE if STMT reads from EXPR, FALSE if writes to EXPR
   VECTYPE - the type that defines the alignment (i.e, we compute
             alignment relative to TYPE_ALIGN(VECTYPE))
   DR - data_reference struct for the original memory reference

   Output:
   BASE (returned value) - the base of the data reference EXPR.
   INITIAL_OFFSET - initial offset of EXPR from BASE (an expression)
   MISALIGN - offset of EXPR from BASE in bytes (a constant) or NULL_TREE if the
              computation is impossible
   STEP - evolution of EXPR in the loop
   BASE_ALIGNED - indicates if BASE is aligned
 
   If something unexpected is encountered (an unsupported form of data-ref),
   then NULL_TREE is returned.  
 */

static tree
vect_address_analysis (tree expr, tree stmt, bool is_read, tree vectype, 
		       struct data_reference *dr, tree *offset, tree *misalign,
		       tree *step, bool *base_aligned)
{
  tree oprnd0, oprnd1, base_address, offset_expr, base_addr0, base_addr1;
  tree address_offset = ssize_int (0), address_misalign = ssize_int (0);

  switch (TREE_CODE (expr))
    {
    case PLUS_EXPR:
    case MINUS_EXPR:
      /* EXPR is of form {base +/- offset} (or {offset +/- base}).  */
      oprnd0 = TREE_OPERAND (expr, 0);
      oprnd1 = TREE_OPERAND (expr, 1);

      STRIP_NOPS (oprnd0);
      STRIP_NOPS (oprnd1);
      
      /* Recursively try to find the base of the address contained in EXPR.
	 For offset, the returned base will be NULL.  */
      base_addr0 = vect_address_analysis (oprnd0, stmt, is_read, vectype, dr, 
				     &address_offset, &address_misalign, step, 
				     base_aligned);

      base_addr1 = vect_address_analysis (oprnd1, stmt, is_read, vectype, dr, 
				     &address_offset, &address_misalign, step, 
				     base_aligned);

      /* We support cases where only one of the operands contains an 
	 address.  */
      if ((base_addr0 && base_addr1) || (!base_addr0 && !base_addr1))
	return NULL_TREE;

      /* To revert STRIP_NOPS.  */
      oprnd0 = TREE_OPERAND (expr, 0);
      oprnd1 = TREE_OPERAND (expr, 1);
      
      offset_expr = base_addr0 ? 
	fold_convert (ssizetype, oprnd1) : fold_convert (ssizetype, oprnd0);

      /* EXPR is of form {base +/- offset} (or {offset +/- base}). If offset is 
	 a number, we can add it to the misalignment value calculated for base,
	 otherwise, misalignment is NULL.  */
      if (TREE_CODE (offset_expr) == INTEGER_CST && address_misalign)
	*misalign = size_binop (TREE_CODE (expr), address_misalign, 
				offset_expr);
      else
	*misalign = NULL_TREE;

      /* Combine offset (from EXPR {base + offset}) with the offset calculated
	 for base.  */
      *offset = size_binop (TREE_CODE (expr), address_offset, offset_expr);
      return base_addr0 ? base_addr0 : base_addr1;

    case ADDR_EXPR:
      base_address = vect_object_analysis (TREE_OPERAND (expr, 0), stmt, is_read, 
				   vectype, &dr, offset, misalign, step, 
				   base_aligned);
      return base_address;

    case SSA_NAME:
      if (TREE_CODE (TREE_TYPE (expr)) != POINTER_TYPE)
	return NULL_TREE;
      
      if (TYPE_ALIGN (TREE_TYPE (TREE_TYPE (expr))) < TYPE_ALIGN (vectype)) 
	{
	  if (vect_get_ptr_offset (expr, vectype, misalign))
	    *base_aligned = true;	  
	  else
	    *base_aligned = false;
	}
      else
	{	  
	  *base_aligned = true;
	  *misalign = ssize_int (0);
	}
      *offset = ssize_int (0);
      *step = ssize_int (0);
      return expr;
      
    default:
      return NULL_TREE;
    }
}


/* Function vect_object_analysis

   Return the BASE of the data reference MEMREF.
   Also compute the INITIAL_OFFSET from BASE, MISALIGN and STEP.
   E.g., for EXPR a.b[i] + 4B, BASE is a, and OFFSET is the overall offset  
   'a.b[i] + 4B' from a (can be an expression), MISALIGN is an OFFSET 
   instantiated with initial_conditions of access_functions of variables, 
   modulo alignment, and STEP is the evolution of the DR_REF in this loop.

   Function get_inner_reference is used for the above in case of ARRAY_REF and
   COMPONENT_REF.

   The structure of the function is as follows:
   Part 1:
   Case 1. For handled_component_p refs 
          1.1 call get_inner_reference
            1.1.1 analyze offset expr received from get_inner_reference
	  1.2. build data-reference structure for MEMREF
        (fall through with BASE)
   Case 2. For declarations 
          2.1 check alignment
          2.2 update DR_BASE_NAME if necessary for alias
   Case 3. For INDIRECT_REFs 
          3.1 get the access function
	  3.2 analyze evolution of MEMREF
	  3.3 set data-reference structure for MEMREF
          3.4 call vect_address_analysis to analyze INIT of the access function

   Part 2:
   Combine the results of object and address analysis to calculate 
   INITIAL_OFFSET, STEP and misalignment info.   

   Input:
   MEMREF - the memory reference that is being analyzed
   STMT - the statement that contains MEMREF
   IS_READ - TRUE if STMT reads from MEMREF, FALSE if writes to MEMREF
   VECTYPE - the type that defines the alignment (i.e, we compute
             alignment relative to TYPE_ALIGN(VECTYPE))
   
   Output:
   BASE_ADDRESS (returned value) - the base address of the data reference MEMREF
                                   E.g, if MEMREF is a.b[k].c[i][j] the returned
			           base is &a.
   DR - data_reference struct for MEMREF
   INITIAL_OFFSET - initial offset of MEMREF from BASE (an expression)
   MISALIGN - offset of MEMREF from BASE in bytes (a constant) or NULL_TREE if 
              the computation is impossible
   STEP - evolution of the DR_REF in the loop
   BASE_ALIGNED - indicates if BASE is aligned
 
   If something unexpected is encountered (an unsupported form of data-ref),
   then NULL_TREE is returned.  */

static tree
vect_object_analysis (tree memref, tree stmt, bool is_read,
		      tree vectype, struct data_reference **dr,
		      tree *offset, tree *misalign, tree *step,
		      bool *base_aligned)
{
  tree base = NULL_TREE, base_address = NULL_TREE;
  tree object_offset = ssize_int (0), object_misalign = ssize_int (0);
  tree object_step = ssize_int (0), address_step = ssize_int (0);
  bool object_base_aligned = true, address_base_aligned = true;
  tree address_offset = ssize_int (0), address_misalign = ssize_int (0);
  HOST_WIDE_INT pbitsize, pbitpos;
  tree poffset, bit_pos_in_bytes;
  enum machine_mode pmode;
  int punsignedp, pvolatilep;
  tree ptr_step = ssize_int (0), ptr_init = NULL_TREE;
  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
  loop_vec_info loop_vinfo = STMT_VINFO_LOOP_VINFO (stmt_info);
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  struct data_reference *ptr_dr = NULL;
  tree access_fn, evolution_part, address_to_analyze;
   
  /* Part 1: */
  /* Case 1. handled_component_p refs.  */
  if (handled_component_p (memref))
    {
      /* 1.1 call get_inner_reference.  */
      /* Find the base and the offset from it.  */
      base = get_inner_reference (memref, &pbitsize, &pbitpos, &poffset,
				  &pmode, &punsignedp, &pvolatilep, false);
      if (!base)
	return NULL_TREE;

      /* 1.1.1 analyze offset expr received from get_inner_reference.  */
      if (poffset 
	  && !vect_analyze_offset_expr (poffset, loop, TYPE_SIZE_UNIT (vectype), 
				&object_offset, &object_misalign, &object_step))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "failed to compute offset or step for ");
	      print_generic_expr (vect_dump, memref, TDF_SLIM);
	    }
	  return NULL_TREE;
	}

      /* Add bit position to OFFSET and MISALIGN.  */

      bit_pos_in_bytes = ssize_int (pbitpos/BITS_PER_UNIT);
      /* Check that there is no remainder in bits.  */
      if (pbitpos%BITS_PER_UNIT)
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "bit offset alignment.");
	  return NULL_TREE;
	}
      object_offset = size_binop (PLUS_EXPR, bit_pos_in_bytes, object_offset);     
      if (object_misalign) 
	object_misalign = size_binop (PLUS_EXPR, object_misalign, 
				      bit_pos_in_bytes); 

      /* Create data-reference for MEMREF. TODO: handle COMPONENT_REFs.  */
      if (!(*dr))
	{ 
	  if (TREE_CODE (memref) == ARRAY_REF)
	    *dr = analyze_array (stmt, memref, is_read);
	  else
	    /* FORNOW.  */
	    return NULL_TREE;
	}
      memref = base; /* To continue analysis of BASE.  */
      /* fall through  */
    }
  
  /*  Part 1: Case 2. Declarations.  */ 
  if (DECL_P (memref))
    {
      /* We expect to get a decl only if we already have a DR.  */
      if (!(*dr))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "unhandled decl ");
	      print_generic_expr (vect_dump, memref, TDF_SLIM);
	    }
	  return NULL_TREE;
	}

      /* 2.1 check the alignment.  */
      if (DECL_ALIGN (memref) >= TYPE_ALIGN (vectype))
	object_base_aligned = true;
      else
	object_base_aligned = false;

      /* 2.2 update DR_BASE_NAME if necessary.  */
      if (!DR_BASE_NAME ((*dr)))
	/* For alias analysis.  In case the analysis of INDIRECT_REF brought 
	   us to object.  */
	DR_BASE_NAME ((*dr)) = memref;

      base_address = build_fold_addr_expr (memref);
    }

  /* Part 1:  Case 3. INDIRECT_REFs.  */
  else if (TREE_CODE (memref) == INDIRECT_REF)
    {      
      /* 3.1 get the access function.  */
      access_fn = analyze_scalar_evolution (loop, TREE_OPERAND (memref, 0));
      if (!access_fn)
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "not vectorized: complicated pointer access.");	
	  return NULL_TREE;
	}
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
	  fprintf (vect_dump, "Access function of ptr: ");
	  print_generic_expr (vect_dump, access_fn, TDF_SLIM);
	}

      /* 3.2 analyze evolution of MEMREF.  */
      evolution_part = evolution_part_in_loop_num (access_fn, loop->num);
      if (evolution_part)
	{
	  ptr_dr = vect_analyze_pointer_ref_access (memref, stmt, is_read, 
				         access_fn, &ptr_init, &ptr_step);
	  if (!(ptr_dr))
	    return NULL_TREE; 
	  
	  object_step = size_binop (PLUS_EXPR, object_step, ptr_step);
	  address_to_analyze = ptr_init;
	}
      else
	{
	  if (!(*dr))
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
					LOOP_LOC (loop_vinfo))) 
		fprintf (vect_dump, "not vectorized: ptr is loop invariant.");	
	      return NULL_TREE;
	    }
	  /* Since there exists DR for MEMREF, we are analyzing the base of
	     handled component, which not necessary has evolution in the 
	     loop.  */
	  address_to_analyze = TREE_OPERAND (base, 0);
	}
      
      /* 3.3 set data-reference structure for MEMREF.  */
      *dr = (*dr) ? *dr : ptr_dr;

      /* 3.4 call vect_address_analysis to analyze INIT of the access 
	 function.  */
      base_address = vect_address_analysis (address_to_analyze, stmt, is_read, 
			       vectype, *dr, &address_offset, &address_misalign, 
			       &address_step, &address_base_aligned);
    }
    	    
  if (!base_address)
    /* MEMREF cannot be analyzed.  */
    return NULL_TREE;

  /* Part 2: Combine the results of object and address analysis to calculate 
     INITIAL_OFFSET, STEP and misalignment info. */
  *offset = size_binop (PLUS_EXPR, object_offset, address_offset);
  if (object_misalign && address_misalign)
    *misalign = size_binop (PLUS_EXPR, object_misalign, address_misalign);
  else
    *misalign = NULL_TREE;
  *step = size_binop (PLUS_EXPR, object_step, address_step); 
  *base_aligned = object_base_aligned && address_base_aligned;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    {
      fprintf (vect_dump, "Results of object analysis for: ");
      print_generic_expr (vect_dump, memref, TDF_SLIM);
      fprintf (vect_dump, "\n\tbase: ");
      print_generic_expr (vect_dump, base, TDF_SLIM);
      fprintf (vect_dump, "\n\toffset: ");
      print_generic_expr (vect_dump, *offset, TDF_SLIM);
      fprintf (vect_dump, "\n\tstep: ");
      print_generic_expr (vect_dump, *step, TDF_SLIM);
      fprintf (vect_dump, "\n\tbase aligned %d\n\tmisalign: ", *base_aligned);
      print_generic_expr (vect_dump, *misalign, TDF_SLIM);
    }
  return base_address;
}


/* Function vect_analyze_data_refs.

   Find all the data references in the loop.

   The general structure of the analysis of data refs in the vectorizer is as 
   follows:
   1- vect_analyze_data_refs(loop): 
      Find and analyze all data-refs in the loop:
          foreach ref
	     base_address = vect_object_analysis(ref)
             ref_stmt.memtag =  vect_get_memtag(base)
      1.1- vect_object_analysis(ref): 
           Analyze ref, and build a DR (data_referece struct) for it;
           compute base, initial_offset, step and alignment. 
           Call get_inner_reference for refs handled in this function.
           Call vect_addr_analysis(addr) to analyze pointer type expressions.
      Set ref_stmt.base, ref_stmt.initial_offset, ref_stmt.alignment, and 
      ref_stmt.step accordingly. 
   2- vect_analyze_dependences(): apply dependence testing using ref_stmt.DR
   3- vect_analyze_drs_alignment(): check that ref_stmt.alignment is ok.
   4- vect_analyze_drs_access(): check that ref_stmt.step is ok.

   FORNOW: Handle aligned INDIRECT_REFs and ARRAY_REFs 
	   which base is really an array (not a pointer) and which alignment 
	   can be forced. This restriction will be relaxed.  */

static bool
vect_analyze_data_refs (loop_vec_info loop_vinfo)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block *bbs = LOOP_VINFO_BBS (loop_vinfo);
  int nbbs = loop->num_nodes;
  block_stmt_iterator si;
  int j;
  struct data_reference *dr;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_analyze_data_refs ===");

  for (j = 0; j < nbbs; j++)
    {
      basic_block bb = bbs[j];
      for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
	{
	  bool is_read = false;
	  tree stmt = bsi_stmt (si);
	  stmt_vec_info stmt_info = vinfo_for_stmt (stmt);
	  v_may_def_optype v_may_defs = STMT_V_MAY_DEF_OPS (stmt);
	  v_must_def_optype v_must_defs = STMT_V_MUST_DEF_OPS (stmt);
	  vuse_optype vuses = STMT_VUSE_OPS (stmt);
	  varray_type *datarefs = NULL;
	  int nvuses, nv_may_defs, nv_must_defs;
	  tree memref = NULL;
	  tree scalar_type, vectype;	  
	  tree base, offset, misalign, step, tag;
	  bool base_aligned;

	  /* Assumption: there exists a data-ref in stmt, if and only if 
             it has vuses/vdefs.  */

	  if (!vuses && !v_may_defs && !v_must_defs)
	    continue;

	  nvuses = NUM_VUSES (vuses);
	  nv_may_defs = NUM_V_MAY_DEFS (v_may_defs);
	  nv_must_defs = NUM_V_MUST_DEFS (v_must_defs);

	  if (nvuses && (nv_may_defs || nv_must_defs))
	    {
	      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
		{
		  fprintf (vect_dump, "unexpected vdefs and vuses in stmt: ");
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		}
	      return false;
	    }

	  if (TREE_CODE (stmt) != MODIFY_EXPR)
	    {
	      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
		{
		  fprintf (vect_dump, "unexpected vops in stmt: ");
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		}
	      return false;
	    }

	  if (vuses)
	    {
	      memref = TREE_OPERAND (stmt, 1);
	      datarefs = &(LOOP_VINFO_DATAREF_READS (loop_vinfo));
	      is_read = true;
	    } 
	  else /* vdefs */
	    {
	      memref = TREE_OPERAND (stmt, 0);
	      datarefs = &(LOOP_VINFO_DATAREF_WRITES (loop_vinfo));
	      is_read = false;
	    }
	  
	  scalar_type = TREE_TYPE (memref);
	  vectype = get_vectype_for_scalar_type (scalar_type);
	  if (!vectype)
	    {
	      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
		{
		  fprintf (vect_dump, "no vectype for stmt: ");
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		  fprintf (vect_dump, " scalar_type: ");
		  print_generic_expr (vect_dump, scalar_type, TDF_DETAILS);
		}
	      /* It is not possible to vectorize this data reference.  */
	      return false;
	    }
	 /* Analyze MEMREF. If it is of a supported form, build data_reference
	     struct for it (DR).  */
	  dr = NULL; 
	  base = vect_object_analysis (memref, stmt, is_read, vectype, &dr, 
				       &offset, &misalign, &step, 
				       &base_aligned);
	  if (!base)
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
					LOOP_LOC (loop_vinfo)))
		{
		  fprintf (vect_dump, "not vectorized: unhandled data ref: "); 
		  print_generic_expr (vect_dump, stmt, TDF_SLIM);
		}
	      return false;
	    }
	  /*  Find memtag for aliasing purposes.  */
	  tag = vect_get_memtag (base, dr);
	  if (!tag)
	    {
	      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
					LOOP_LOC (loop_vinfo)))
		{
		  fprintf (vect_dump, "not vectorized: no memtag ref: "); 
		  print_generic_expr (vect_dump, memref, TDF_SLIM);
		}
	      return false;
	    }
	  STMT_VINFO_VECT_DR_BASE_ADDRESS (stmt_info) = base;
	  STMT_VINFO_VECT_INIT_OFFSET (stmt_info) = offset;
	  STMT_VINFO_VECT_STEP (stmt_info) = step;
	  STMT_VINFO_VECT_MISALIGNMENT (stmt_info) = misalign;
	  STMT_VINFO_VECT_BASE_ALIGNED_P (stmt_info) = base_aligned;
	  STMT_VINFO_MEMTAG (stmt_info) = tag;
	  STMT_VINFO_VECTYPE (stmt_info) = vectype;
	  VARRAY_PUSH_GENERIC_PTR (*datarefs, dr);
	  STMT_VINFO_DATA_REF (stmt_info) = dr;
	}
    }

  return true;
}


/* Utility functions used by vect_mark_stmts_to_be_vectorized.  */

/* Function vect_mark_relevant.

   Mark STMT as "relevant for vectorization" and add it to WORKLIST.  */

static void
vect_mark_relevant (varray_type *worklist, tree stmt)
{
  stmt_vec_info stmt_info;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "mark relevant.");

  if (TREE_CODE (stmt) == PHI_NODE)
    {
      VARRAY_PUSH_TREE (*worklist, stmt);
      return;
    }

  stmt_info = vinfo_for_stmt (stmt);

  if (!stmt_info)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
	  fprintf (vect_dump, "mark relevant: no stmt info!!.");
	  print_generic_expr (vect_dump, stmt, TDF_SLIM);
	}
      return;
    }

  if (STMT_VINFO_RELEVANT_P (stmt_info))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        fprintf (vect_dump, "already marked relevant.");
      return;
    }

  STMT_VINFO_RELEVANT_P (stmt_info) = 1;
  VARRAY_PUSH_TREE (*worklist, stmt);
}


/* Function vect_stmt_relevant_p.

   Return true if STMT in loop that is represented by LOOP_VINFO is
   "relevant for vectorization".

   A stmt is considered "relevant for vectorization" if:
   - it has uses outside the loop.
   - it has vdefs (it alters memory).
   - control stmts in the loop (except for the exit condition).

   CHECKME: what other side effects would the vectorizer allow?  */

static bool
vect_stmt_relevant_p (tree stmt, loop_vec_info loop_vinfo)
{
  v_may_def_optype v_may_defs;
  v_must_def_optype v_must_defs;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  int i;
  dataflow_t df;
  int num_uses;

  /* cond stmt other than loop exit cond.  */
  if (is_ctrl_stmt (stmt) && (stmt != LOOP_VINFO_EXIT_COND (loop_vinfo)))
    return true;

  /* changing memory.  */
  if (TREE_CODE (stmt) != PHI_NODE)
    {
      v_may_defs = STMT_V_MAY_DEF_OPS (stmt);
      v_must_defs = STMT_V_MUST_DEF_OPS (stmt);
      if (v_may_defs || v_must_defs)
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "vec_stmt_relevant_p: stmt has vdefs.");
	  return true;
	}
    }

  /* uses outside the loop.  */
  df = get_immediate_uses (stmt);
  num_uses = num_immediate_uses (df);
  for (i = 0; i < num_uses; i++)
    {
      tree use = immediate_use (df, i);
      basic_block bb = bb_for_stmt (use);
      if (!flow_bb_inside_loop_p (loop, bb))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "vec_stmt_relevant_p: used out of loop.");
	  return true;
	}
    }

  return false;
}


/* Function vect_mark_stmts_to_be_vectorized.

   Not all stmts in the loop need to be vectorized. For example:

     for i...
       for j...
   1.    T0 = i + j
   2.	 T1 = a[T0]

   3.    j = j + 1

   Stmt 1 and 3 do not need to be vectorized, because loop control and
   addressing of vectorized data-refs are handled differently.

   This pass detects such stmts.  */

static bool
vect_mark_stmts_to_be_vectorized (loop_vec_info loop_vinfo)
{
  varray_type worklist;
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block *bbs = LOOP_VINFO_BBS (loop_vinfo);
  unsigned int nbbs = loop->num_nodes;
  block_stmt_iterator si;
  tree stmt;
  stmt_ann_t ann;
  unsigned int i;
  int j;
  use_optype use_ops;
  stmt_vec_info stmt_info;
  basic_block bb;
  tree phi;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== vect_mark_stmts_to_be_vectorized ===");

  bb = loop->header;
  for (phi = phi_nodes (bb); phi; phi = PHI_CHAIN (phi))
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        {
          fprintf (vect_dump, "init: phi relevant? ");
          print_generic_expr (vect_dump, phi, TDF_SLIM);
        }

      if (vect_stmt_relevant_p (phi, loop_vinfo))
	{
	  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
				    LOOP_LOC (loop_vinfo)))
	    fprintf (vect_dump, "unsupported reduction/induction.");
          return false;
	}
    }

  VARRAY_TREE_INIT (worklist, 64, "work list");

  /* 1. Init worklist.  */

  for (i = 0; i < nbbs; i++)
    {
      bb = bbs[i];
      for (si = bsi_start (bb); !bsi_end_p (si); bsi_next (&si))
	{
	  stmt = bsi_stmt (si);

	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    {
	      fprintf (vect_dump, "init: stmt relevant? ");
	      print_generic_expr (vect_dump, stmt, TDF_SLIM);
	    } 

	  stmt_info = vinfo_for_stmt (stmt);
	  STMT_VINFO_RELEVANT_P (stmt_info) = 0;

	  if (vect_stmt_relevant_p (stmt, loop_vinfo))
	    vect_mark_relevant (&worklist, stmt);
	}
    }


  /* 2. Process_worklist */

  while (VARRAY_ACTIVE_SIZE (worklist) > 0)
    {
      stmt = VARRAY_TOP_TREE (worklist);
      VARRAY_POP (worklist);

      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
          fprintf (vect_dump, "worklist: examine stmt: ");
          print_generic_expr (vect_dump, stmt, TDF_SLIM);
	}

      /* Examine the USES in this statement. Mark all the statements which
         feed this statement's uses as "relevant", unless the USE is used as
         an array index.  */

      if (TREE_CODE (stmt) == PHI_NODE)
	{
	  /* follow the def-use chain inside the loop.  */
	  for (j = 0; j < PHI_NUM_ARGS (stmt); j++)
	    {
	      tree arg = PHI_ARG_DEF (stmt, j);
	      tree def_stmt = NULL_TREE;
	      basic_block bb;
	      if (!vect_is_simple_use (arg, loop_vinfo, &def_stmt))
		{
		  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
					    LOOP_LOC (loop_vinfo)))
		    fprintf (vect_dump, "not vectorized: unsupported use in stmt.");
		  varray_clear (worklist);
		  return false;
		}
	      if (!def_stmt)
		continue;

	      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	        {
	          fprintf (vect_dump, "worklist: def_stmt: ");
		  print_generic_expr (vect_dump, def_stmt, TDF_SLIM);
		}

	      bb = bb_for_stmt (def_stmt);
	      if (flow_bb_inside_loop_p (loop, bb))
	        vect_mark_relevant (&worklist, def_stmt);
	    }
	} 

      ann = stmt_ann (stmt);
      use_ops = USE_OPS (ann);

      for (i = 0; i < NUM_USES (use_ops); i++)
	{
	  tree use = USE_OP (use_ops, i);

	  /* We are only interested in uses that need to be vectorized. Uses 
	     that are used for address computation are not considered relevant.
	   */
	  if (exist_non_indexing_operands_for_use_p (use, stmt))
	    {
              tree def_stmt = NULL_TREE;
              basic_block bb;
              if (!vect_is_simple_use (use, loop_vinfo, &def_stmt))
                {
                  if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS,
					    LOOP_LOC (loop_vinfo)))
                    fprintf (vect_dump, "not vectorized: unsupported use in stmt.");
                  varray_clear (worklist);
                  return false;
                }

	      if (!def_stmt)
		continue;

              if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
                {
                  fprintf (vect_dump, "worklist: examine use %d: ", i);
                  print_generic_expr (vect_dump, use, TDF_SLIM);
                }

	      bb = bb_for_stmt (def_stmt);
	      if (flow_bb_inside_loop_p (loop, bb))
		vect_mark_relevant (&worklist, def_stmt);
	    }
	}
    }				/* while worklist */

  varray_clear (worklist);
  return true;
}


/* Function vect_can_advance_ivs_p

   In case the number of iterations that LOOP iterates in unknown at compile 
   time, an epilog loop will be generated, and the loop induction variables 
   (IVs) will be "advanced" to the value they are supposed to take just before 
   the epilog loop.  Here we check that the access function of the loop IVs
   and the expression that represents the loop bound are simple enough.
   These restrictions will be relaxed in the future.  */

static bool 
vect_can_advance_ivs_p (loop_vec_info loop_vinfo)
{
  struct loop *loop = LOOP_VINFO_LOOP (loop_vinfo);
  basic_block bb = loop->header;
  tree phi;

  /* Analyze phi functions of the loop header.  */

  for (phi = phi_nodes (bb); phi; phi = PHI_CHAIN (phi))
    {
      tree access_fn = NULL;
      tree evolution_part;

      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
          fprintf (vect_dump, "Analyze phi: ");
          print_generic_expr (vect_dump, phi, TDF_SLIM);
	}

      /* Skip virtual phi's. The data dependences that are associated with
         virtual defs/uses (i.e., memory accesses) are analyzed elsewhere.  */

      if (!is_gimple_reg (SSA_NAME_VAR (PHI_RESULT (phi))))
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "virtual phi. skip.");
	  continue;
	}

      /* Analyze the evolution function.  */

      access_fn = instantiate_parameters
	(loop, analyze_scalar_evolution (loop, PHI_RESULT (phi)));

      if (!access_fn)
	{
	  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	    fprintf (vect_dump, "No Access function.");
	  return false;
	}

      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
        {
	  fprintf (vect_dump, "Access function of PHI: ");
	  print_generic_expr (vect_dump, access_fn, TDF_SLIM);
        }

      evolution_part = evolution_part_in_loop_num (access_fn, loop->num);
      
      if (evolution_part == NULL_TREE)
	return false;
  
      /* FORNOW: We do not transform initial conditions of IVs 
	 which evolution functions are a polynomial of degree >= 2.  */

      if (tree_is_chrec (evolution_part))
	return false;  
    }

  return true;
}


/* Function vect_get_loop_niters.

   Determine how many iterations the loop is executed.
   If an expression that represents the number of iterations
   can be constructed, place it in NUMBER_OF_ITERATIONS.
   Return the loop exit condition.  */

/* APPLE LOCAL loops-to-memset */
tree
vect_get_loop_niters (struct loop *loop, tree *number_of_iterations)
{
  tree niters;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "=== get_loop_niters ===");

  niters = number_of_iterations_in_loop (loop);

  if (niters != NULL_TREE
      && niters != chrec_dont_know)
    {
      *number_of_iterations = niters;

      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	{
	  fprintf (vect_dump, "==> get_loop_niters:" );
	  print_generic_expr (vect_dump, *number_of_iterations, TDF_SLIM);
	}
    }

  return get_loop_exit_condition (loop);
}


/* Function vect_analyze_loop_form.

   Verify the following restrictions (some may be relaxed in the future):
   - it's an inner-most loop
   - number of BBs = 2 (which are the loop header and the latch)
   - the loop has a pre-header
   - the loop has a single entry and exit
   - the loop exit condition is simple enough, and the number of iterations
     can be analyzed (a countable loop).  */

static loop_vec_info
vect_analyze_loop_form (struct loop *loop)
{
  loop_vec_info loop_vinfo;
  tree loop_cond;
  tree number_of_iterations = NULL;
  bool rescan = false;
  LOC loop_loc;

  loop_loc = find_loop_location (loop);

  if (vect_print_dump_info (REPORT_DETAILS, loop_loc))
    fprintf (vect_dump, "=== vect_analyze_loop_form ===");

  if (loop->inner)
    {
      if (vect_print_dump_info (REPORT_OUTER_LOOPS, loop_loc))
        fprintf (vect_dump, "not vectorized: nested loop.");
      return NULL;
    }
  
  if (!loop->single_exit 
      || loop->num_nodes != 2
      || EDGE_COUNT (loop->header->preds) != 2
      || loop->num_entries != 1)
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
        {
          if (!loop->single_exit)
            fprintf (vect_dump, "not vectorized: multiple exits.");
          else if (loop->num_nodes != 2)
            fprintf (vect_dump, "not vectorized: too many BBs in loop.");
          else if (EDGE_COUNT (loop->header->preds) != 2)
            fprintf (vect_dump, "not vectorized: too many incoming edges.");
          else if (loop->num_entries != 1)
            fprintf (vect_dump, "not vectorized: too many entries.");
        }

      return NULL;
    }

  /* We assume that the loop exit condition is at the end of the loop. i.e,
     that the loop is represented as a do-while (with a proper if-guard
     before the loop if needed), where the loop header contains all the
     executable statements, and the latch is empty.  */
  if (!empty_block_p (loop->latch))
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
        fprintf (vect_dump, "not vectorized: unexpectd loop form.");
      return NULL;
    }

  /* Make sure we have a preheader basic block.  */
  if (!loop->pre_header)
    {
      rescan = true;
      loop_split_edge_with (loop_preheader_edge (loop), NULL);
    }
    
  /* Make sure there exists a single-predecessor exit bb:  */
  if (EDGE_COUNT (loop->exit_edges[0]->dest->preds) != 1)
    {
      rescan = true;
      loop_split_edge_with (loop->exit_edges[0], NULL);
    }
    
  if (rescan)
    {
      flow_loop_scan (loop, LOOP_ALL);
      /* Flow loop scan does not update loop->single_exit field.  */
      loop->single_exit = loop->exit_edges[0];
    }

  if (empty_block_p (loop->header))
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
        fprintf (vect_dump, "not vectorized: empty loop.");
      return NULL;
    }

  loop_cond = vect_get_loop_niters (loop, &number_of_iterations);
  if (!loop_cond)
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
	fprintf (vect_dump, "not vectorized: complicated exit condition.");
      return NULL;
    }
  
  if (!number_of_iterations) 
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
	fprintf (vect_dump, 
		 "not vectorized: number of iterations cannot be computed.");
      return NULL;
    }

  if (chrec_contains_undetermined (number_of_iterations))
    {
      if (vect_print_dump_info (REPORT_BAD_FORM_LOOPS, loop_loc))
        fprintf (vect_dump, "Infinite number of iterations.");
      return false;
    }

  loop_vinfo = new_loop_vec_info (loop);
  LOOP_VINFO_NITERS (loop_vinfo) = number_of_iterations;

  if (!LOOP_VINFO_NITERS_KNOWN_P (loop_vinfo))
    {
      if (vect_print_dump_info (REPORT_DETAILS, loop_loc))
        {
          fprintf (vect_dump, "Symbolic number of iterations is ");
          print_generic_expr (vect_dump, number_of_iterations, TDF_DETAILS);
        }
    }
  else
  if (LOOP_VINFO_INT_NITERS (loop_vinfo) == 0)
    {
      if (vect_print_dump_info (REPORT_UNVECTORIZED_LOOPS, loop_loc))
        fprintf (vect_dump, "not vectorized: number of iterations = 0.");
      return NULL;
    }

  LOOP_VINFO_EXIT_COND (loop_vinfo) = loop_cond;
  LOOP_VINFO_LOC (loop_vinfo) = loop_loc;

  return loop_vinfo;
}


/* Function vect_analyze_loop.

   Apply a set of analyses on LOOP, and create a loop_vec_info struct
   for it. The different analyses will record information in the
   loop_vec_info struct.  */

static loop_vec_info
vect_analyze_loop (struct loop *loop)
{
  bool ok;
  loop_vec_info loop_vinfo;

  if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
    fprintf (vect_dump, "===== analyze_loop_nest =====");

  /* Check the CFG characteristics of the loop (nesting, entry/exit, etc.  */

  loop_vinfo = vect_analyze_loop_form (loop);
  if (!loop_vinfo)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "bad loop form.");
      return NULL;
    }

  /* Find all data references in the loop (which correspond to vdefs/vuses)
     and analyze their evolution in the loop.

     FORNOW: Handle only simple, array references, which
     alignment can be forced, and aligned pointer-references.  */

  ok = vect_analyze_data_refs (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad data references.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* Data-flow analysis to detect stmts that do not need to be vectorized.  */

  ok = vect_mark_stmts_to_be_vectorized (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "unexpected pattern.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* Check that all cross-iteration scalar data-flow cycles are OK.
     Cross-iteration cycles caused by virtual phis are analyzed separately.  */

  ok = vect_analyze_scalar_cycles (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad scalar cycle.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* APPLE LOCAL begin AV data dependence. -dpatel */
  /* Analyze operations before data dependence.  */

  /* Scan all the operations in the loop and make sure they are
     vectorizable.  */

  ok = vect_analyze_operations (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad data access.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* Analyze data dependences between the data-refs in the loop. 
     FORNOW: fail at the first data dependence that we encounter.  */

  ok = vect_analyze_data_ref_dependences (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad data alignment.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* Analyze the access patterns of the data-refs in the loop (consecutive,
     complex, etc.). FORNOW: Only handle consecutive access pattern.  */

  ok = vect_analyze_data_ref_accesses (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad operation or unsupported loop bound.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* Analyze the alignment of the data-refs in the loop.
     FORNOW: Only aligned accesses are handled.  */

  ok = vect_analyze_data_refs_alignment (loop_vinfo);
  if (!ok)
    {
      if (vect_print_dump_info (REPORT_DETAILS, LOOP_LOC (loop_vinfo)))
	fprintf (vect_dump, "bad data dependence.");
      destroy_loop_vec_info (loop_vinfo);
      return NULL;
    }

  /* APPLE LOCAL end AV data dependence. -dpatel */
  
  LOOP_VINFO_VECTORIZABLE_P (loop_vinfo) = 1;

  return loop_vinfo;
}


/* Function need_imm_uses_for.

   Return whether we ought to include information for 'var'
   when calculating immediate uses.  For this pass we only want use
   information for non-virtual variables.  */

static bool
need_imm_uses_for (tree var)
{
  return is_gimple_reg (var);
}


/* Function vectorize_loops.
   
   Entry Point to loop vectorization phase.  */

void
vectorize_loops (struct loops *loops)
{
  /* APPLE LOCAL AV data dependence. -dpatel */
  unsigned int i;
  unsigned int num_vectorized_loops = 0;

  /* Fix the verbosity level if not defined explicitly by the user.  */
  vect_set_dump_settings ();

  /* Does the target support SIMD?  */
  /* FORNOW: until more sophisticated machine modelling is in place.  */
  if (!UNITS_PER_SIMD_WORD)
    {
      if (vect_print_dump_info (REPORT_DETAILS, UNKNOWN_LOC))
	fprintf (vect_dump, "vectorizer: target vector size is not defined.");
      return;
    }

#ifdef ENABLE_CHECKING
  verify_loop_closed_ssa ();
#endif

  compute_immediate_uses (TDFA_USE_OPS, need_imm_uses_for);

  /*  ----------- Analyze loops. -----------  */

  /* If some loop was duplicated, it gets bigger number 
     than all previously defined loops. This fact allows us to run 
     only over initial loops skipping newly generated ones.  */
  loops_num = loops->num;
  for (i = 1; i < loops_num; i++)
    {
      loop_vec_info loop_vinfo;
      struct loop *loop = loops->parray[i];

      if (!loop)
        continue;

      loop_vinfo = vect_analyze_loop (loop);
      loop->aux = loop_vinfo;

      if (!loop_vinfo || !LOOP_VINFO_VECTORIZABLE_P (loop_vinfo))
	continue;

      vect_transform_loop (loop_vinfo, loops); 
      num_vectorized_loops++;
    }

  if (vect_print_dump_info (REPORT_VECTORIZED_LOOPS, UNKNOWN_LOC))
    fprintf (vect_dump, "vectorized %u loops in function.\n",
	     num_vectorized_loops);

  /*  ----------- Finalize. -----------  */

  free_df ();
  for (i = 1; i < loops_num; i++)
    {
      struct loop *loop = loops->parray[i];
      loop_vec_info loop_vinfo;

      if (!loop)
	continue;
      loop_vinfo = loop->aux;
      destroy_loop_vec_info (loop_vinfo);
      loop->aux = NULL;
    }

  rewrite_into_ssa (false);
  rewrite_into_loop_closed_ssa (); /* FORNOW */
  bitmap_clear (vars_to_rename);
}
