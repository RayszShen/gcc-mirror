/* Data flow functions for trees.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@redhat.com>

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "hashtab.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "output.h"
#include "errors.h"
#include "expr.h"
#include "ggc.h"
#include "flags.h"

/* This should be eventually be generalized to other languages, but
   this would require a shared function-as-trees infrastructure.  */
#include "c-common.h"
#include "c-tree.h"

#include "tree-optimize.h"
#include "tree-simple.h"
#include "tree-flow.h"
#include "tree-inline.h"


/* Local declarations.  */
struct clobber_data_d
{
  basic_block bb;
  tree parent_stmt;
  tree parent_expr;
};


/* DFA Statistics.  */
struct dfa_stats_d
{
  unsigned long num_tree_refs;
  unsigned long num_tree_anns;
  unsigned long num_ref_list_nodes;
  unsigned long num_defs;
  unsigned long num_uses;
  unsigned long num_phis;
  unsigned long num_phi_args;
  unsigned long max_num_phi_args;
  unsigned long num_ephis;
  unsigned long num_euses;
  unsigned long num_ekills;
  unsigned long num_may_alias;
  unsigned long max_num_may_alias;
  unsigned long num_alias_imm_rdefs;
  unsigned long max_num_alias_imm_rdefs;
};


/* Data and functions shared with tree-ssa.c.  */
struct dfa_counts_d dfa_counts;
extern void tree_find_refs		PARAMS ((void));
extern FILE *tree_ssa_dump_file;
extern int tree_ssa_dump_flags;


/* Local functions.  */
static void find_refs_in_expr		PARAMS ((tree *, HOST_WIDE_INT,
      						 basic_block, tree, tree));
static void add_referenced_var		PARAMS ((tree));
static void dump_if_different		PARAMS ((FILE *, const char * const,
     						 unsigned long, unsigned long));
static void collect_dfa_stats		PARAMS ((struct dfa_stats_d *));
static tree collect_dfa_stats_r		PARAMS ((tree *, int *, void *));
static void count_tree_refs		PARAMS ((struct dfa_stats_d *,
      						 ref_list));
static void count_ref_list_nodes	PARAMS ((struct dfa_stats_d *,
      						 ref_list));
static tree clobber_vars_r		PARAMS ((tree *, int *, void *));
static void compute_may_aliases		PARAMS ((void));
static void find_may_aliases_for	PARAMS ((tree));
static void add_may_alias		PARAMS ((tree, tree));
static bool may_alias_p			PARAMS ((tree, tree));
static bool is_visible_to		PARAMS ((tree, tree));


/* Global declarations.  */

/* Array of all variables referenced in the function.  */
unsigned long num_referenced_vars;
varray_type referenced_vars;

/* Next unique reference ID to be assigned by create_ref().  */
unsigned long next_tree_ref_id;

/* Artificial variable used to model the effects of function calls on
   every variable that they may use and define.  Calls to non-const and
   non-pure functions are assumed to use and clobber this variable.  The
   SSA builder will then consider this variable to be an alias for every
   global variable and every local that has had its address taken.  */
tree global_var;

/* Reference types.  */
const HOST_WIDE_INT V_DEF	= 1 << 0;
const HOST_WIDE_INT V_USE	= 1 << 1;
const HOST_WIDE_INT V_PHI	= 1 << 2;
const HOST_WIDE_INT E_PHI	= 1 << 3;
const HOST_WIDE_INT E_USE	= 1 << 4;
const HOST_WIDE_INT E_KILL	= 1 << 5;

/* Reference type modifiers.  */
const HOST_WIDE_INT M_DEFAULT	= 1 << 7;
const HOST_WIDE_INT M_CLOBBER	= 1 << 8;
const HOST_WIDE_INT M_MAY	= 1 << 9;
const HOST_WIDE_INT M_PARTIAL	= 1 << 10;
const HOST_WIDE_INT M_INITIAL	= 1 << 11;
const HOST_WIDE_INT M_RELOCATE	= 1 << 12;
const HOST_WIDE_INT M_VOLATILE	= 1 << 13;
const HOST_WIDE_INT M_ADDRESSOF	= 1 << 14;

/* Look for variable references in every block of the flowgraph.  */

void
tree_find_refs ()
{
  basic_block bb;

  /* Traverse every block in the function looking for variable references.  */
  FOR_EACH_BB (bb)
    {
      tree t = bb->head_tree;

      if (bb_empty_p (bb))
	continue;

      while (t)
	{
	  /* Some basic blocks are composed exclusively of expressions
	     (e.g., FOR_* and DO_COND nodes), these are handled when
	     find_refs_in_stmt processes their entry node.  */
	  if (statement_code_p (TREE_CODE (t)))
	    {
	      find_refs_in_stmt (t, bb);
	      if (t == bb->end_tree || is_ctrl_stmt (t))
		break;
	    }

	  t = TREE_CHAIN (t);
	}
    }

  compute_may_aliases ();
}


/* Walk T looking for variable references.  BB is the basic block that
   contains T.  */

void
find_refs_in_stmt (t, bb)
     tree t;
     basic_block bb;
{
  enum tree_code code;

  if (t == NULL || t == error_mark_node)
    return;

  code = TREE_CODE (t);
  switch (code)
    {
    case EXPR_STMT:
      find_refs_in_expr (&EXPR_STMT_EXPR (t), V_USE, bb, t, EXPR_STMT_EXPR (t));
      break;

    case IF_STMT:
      find_refs_in_expr (&IF_COND (t), V_USE, bb, t, IF_COND (t));
      break;
      
    case SWITCH_STMT:
      find_refs_in_expr (&SWITCH_COND (t), V_USE, bb, t, SWITCH_COND (t));
      break;

    case WHILE_STMT:
      find_refs_in_expr (&WHILE_COND (t), V_USE, bb, t, WHILE_COND (t));
      break;

    case FOR_STMT:
      /* Grr, the FOR_INIT_STMT node of a FOR_STMT is also a statement,
	 which is handled by the main loop in tree_find_tree_refs.  */
      if (for_cond_bb (bb))
	find_refs_in_expr (&FOR_COND (t), V_USE, for_cond_bb (bb), t,
			   FOR_COND (t));

      if (for_expr_bb (bb))
	find_refs_in_expr (&FOR_EXPR (t), V_USE, for_expr_bb (bb), t,
			   FOR_EXPR (t));
      break;

    case DO_STMT:
      if (do_cond_bb (bb))
	find_refs_in_expr (&DO_COND (t), V_USE, do_cond_bb (bb), t,
	                   DO_COND (t));
      break;

    case ASM_STMT:
      find_refs_in_expr (&ASM_INPUTS (t), V_USE, bb, t, ASM_INPUTS (t));
      find_refs_in_expr (&ASM_OUTPUTS (t), V_DEF | M_CLOBBER, bb, t,
	                 ASM_OUTPUTS (t));
      find_refs_in_expr (&ASM_CLOBBERS (t), V_DEF | M_CLOBBER, bb, t,
	                 ASM_CLOBBERS (t));
      break;

    case RETURN_STMT:
      find_refs_in_expr (&RETURN_STMT_EXPR (t), V_USE, bb, t, 
	                 RETURN_STMT_EXPR (t));
      break;

    case GOTO_STMT:
      find_refs_in_expr (&GOTO_DESTINATION (t), V_USE, bb, t,
	                 GOTO_DESTINATION (t));
      break;

    case DECL_STMT:
      if (TREE_CODE (DECL_STMT_DECL (t)) == VAR_DECL
	  && DECL_INITIAL (DECL_STMT_DECL (t)))
	{
	  find_refs_in_expr (&DECL_INITIAL (DECL_STMT_DECL (t)), V_USE, bb, t,
	                     DECL_INITIAL (DECL_STMT_DECL (t)));
	  find_refs_in_expr (&DECL_STMT_DECL (t), V_DEF | M_INITIAL, bb, t,
			     DECL_STMT_DECL (t));
	}
      break;

    /* FIXME  CLEANUP_STMTs are not simplified.  Clobber everything.  */
    case CLEANUP_STMT:
      {
	struct clobber_data_d clobber_data;

	clobber_data.bb = bb;
	clobber_data.parent_stmt = t;
	clobber_data.parent_expr = CLEANUP_DECL (t);
	walk_tree (&CLEANUP_DECL (t), clobber_vars_r, &clobber_data, NULL);

	clobber_data.parent_expr = CLEANUP_EXPR (t);
	walk_tree (&CLEANUP_EXPR (t), clobber_vars_r, &clobber_data, NULL);
	break;
      }

    case LABEL_STMT:
      find_refs_in_expr (&LABEL_STMT_LABEL (t), V_USE, bb, t,
			 LABEL_STMT_LABEL (t));
      break;

    case STMT_EXPR:
      find_refs_in_stmt (STMT_EXPR_STMT (t), bb);
      break;

    case CONTINUE_STMT:
    case CASE_LABEL:
    case BREAK_STMT:
    case COMPOUND_STMT:
    case SCOPE_STMT:
    case FILE_STMT:
      break;				/* Nothing to do.  */

    default:
      {
	prep_stmt (t);
	error ("unhandled statement node in find_refs_in_stmt():");
	fprintf (stderr, "\n");
	tree_debug_bb (bb);
	fprintf (stderr, "\n");
	debug_tree (t);
	fprintf (stderr, "\n");
	abort ();
      }
    }
}


/* Recursively scan the expression tree pointed by EXPR_P looking for
   variable references.
   
   REF_TYPE indicates what type of reference should be created.

   BB, PARENT_STMT and PARENT_EXPR are the block, statement and expression
      trees containing *EXPR_P.
      
   NOTE: PARENT_EXPR is the root node of the expression tree hanging off of
      PARENT_STMT.  For instance, given the statement 'a = b + c;', the
      parent expression for all references inside the statement is the '='
      node.  */

static void
find_refs_in_expr (expr_p, ref_type, bb, parent_stmt, parent_expr)
     tree *expr_p;
     HOST_WIDE_INT ref_type;
     basic_block bb;
     tree parent_stmt;
     tree parent_expr;
{
  enum tree_code code;
  char class;
  tree expr = *expr_p;
  struct clobber_data_d clobber_data;

  if (expr == NULL || expr == error_mark_node)
    return;

  code = TREE_CODE (expr);
  class = TREE_CODE_CLASS (code);

  /* Expressions that make no memory references.  */
  if (class == 'c'
      || class == 't'
      || class == 'b'
      || code == RESULT_DECL
      || code == FUNCTION_DECL
      || code == LABEL_DECL)
    return;

  /* If this reference is associated with a non SIMPLE expression, then we
     mark the parent expression non SIMPLE and recursively clobber every
     variable referenced by PARENT_EXPR.  */
  if (parent_expr && tree_flags (expr) & TF_NOT_SIMPLE)
    {
      set_tree_flag (parent_expr, TF_NOT_SIMPLE);
      clobber_data.bb = bb;
      clobber_data.parent_expr = parent_expr;
      clobber_data.parent_stmt = parent_stmt;
      walk_tree (&parent_expr, clobber_vars_r, &clobber_data, NULL);
      return;
    }

  /* If we found a _DECL node, create a reference to it and return.  */
  if (code == VAR_DECL || code == PARM_DECL)
    {
      create_ref (expr, ref_type, bb, parent_stmt, parent_expr, expr_p, true);

      /* If we just created a V_DEF reference for a pointer variable 'p',
	 we have to clobber the associated '*p' variable, because now 'p'
	 is pointing to a different memory location.  */
      if (ref_type & V_DEF
	  && POINTER_TYPE_P (TREE_TYPE (expr))
	  && indirect_var (expr))
	create_ref (indirect_var (expr), ref_type | M_RELOCATE, bb,
		    parent_stmt, parent_expr, NULL, true);

      return;
    }

  /* Pointer dereferences are considered references to the virtual variable
     represented by the INDIRECT_REF node (called INDIRECT_VAR).  Since
     INDIRECT_REF trees are not always shared like VAR_DECL nodes, this
     pass will think that two INDIRECT_REFs to the same _DECL are different
     references.  For instance,

	    1	p = &a;
	    2	*p = 5;
	    3	b = *p + 3;

     The INDIRECT_REF(p) at line 3 and the one at line 2 are two different
     tree nodes in the intermediate representation of the program.
     So, create_ref() will create two tree_ref objects that point to two
     different variables.  Therefore, the SSA pass will never link the use
     of *p at line 3 with the definition of *p at line 2.

     One way of fixing this problem would be to create the references with
     the pointed-to variable.  However, this is also wrong because now the
     SSA pass will think that the definition of *p at line 2 is killing the
     definition of p at line 1.

     The way we deal with this situation is to store in the VAR_DECL node
     for 'p' a pointer to the first INDIRECT_REF(p) node that we find.
     The first time we call create_ref with INDIRECT_REF(p), we use the
     given INDIRECT_REF tree as the variable associated with the reference.
     Subsequent calls to create_ref with INDIRECT_REF(p) nodes, will use
     the first INDIRECT_REF node found.  */
  if (code == INDIRECT_REF)
    {
      tree ptr = TREE_OPERAND (expr, 0);
      tree ptr_sym = get_base_symbol (ptr);

      /* Create a V_USE reference for the pointer variable itself.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 0), V_USE, bb, parent_stmt,
	                 parent_expr);

      /* If this is the first INDIRECT_REF node we find for PTR, set EXPR
	 to be the indirect variable used to represent all dereferences of
	 PTR.  */
      if (indirect_var (ptr_sym) == NULL)
	set_indirect_var (ptr_sym, expr);

      create_ref (indirect_var (ptr_sym), ref_type, bb, parent_stmt,
		  parent_expr, expr_p, true);

      return;
    }

  /* For array references we default to treating accesses to individual
     elements of the array as if they were to the whole array.  Create a
     partial reference for the base symbol of the array (its LHS) and
     recurse into the RHS look for variables used as index expressions.

     FIXME Further analysis is needed to get proper array-SSA information.  */
  if (code == ARRAY_REF)
    {
      /* Change the reference type to a partial def/use when processing
	 the LHS of the reference.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 0), ref_type | M_PARTIAL, bb,
	                 parent_stmt, parent_expr);

      /* References on the RHS of the array are always used as indices.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 1), V_USE, bb, parent_stmt,
			 parent_expr);
      return;
    }

  /* Similarly to arrays, references to compound variables (complex types
     and structures/unions) are globbed.  Create a partial reference for
     the base symbol of the reference.

     FIXME This means that

     			a.x = 6;
			a.y = 7;
			foo (a.x, a.y);

	   will not be constant propagated because the two partial
	   definitions to 'a' will kill each other.  SSA needs to be
	   enhanced to deal with this case.  */
  if (code == IMAGPART_EXPR || code == REALPART_EXPR || code == COMPONENT_REF)
    {
      /* Modify the reference to be a partial reference of the LHS of the
	 expression.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 0), ref_type | M_PARTIAL, bb,
	                 parent_stmt, parent_expr);
      return;
    }

  /* Assignments.  These are the only expressions that create V_DEF
     references besides DECL_STMTs.  */
  if (code == INIT_EXPR || code == MODIFY_EXPR)
    {
      find_refs_in_expr (&TREE_OPERAND (expr, 1), V_USE, bb, parent_stmt,
			 parent_expr);
      find_refs_in_expr (&TREE_OPERAND (expr, 0), V_DEF, bb, parent_stmt,
			 parent_expr);
      return;
    }

  /* Function calls.  Create a V_USE reference for every argument in the
     call.  If the callee is neither pure nor const, create a use and a def
     of GLOBAL_VAR.  Definitions of this variable will reach uses of every
     call clobbered variable in the function.  Uses of GLOBAL_VAR will be
     reached by definitions of call clobbered variables.  This is used to
     model the effects that the called function may have on local and
     global variables that might be visible to it.  */
  if (code == CALL_EXPR)
    {
      tree callee;

      /* Find references in the call address.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 0), V_USE, bb, parent_stmt,
			 parent_expr);

      /* Find references in the argument list.  */
      find_refs_in_expr (&TREE_OPERAND (expr, 1), V_USE, bb, parent_stmt,
			 parent_expr);

      /* See if the call might clobber local and/or global variables.  If
	 the called function is pure or const, then we can safely ignore
	 it.  */
      callee = get_callee_fndecl (expr);
      if (callee
	  && (DECL_IS_PURE (callee)
	      || (TREE_READONLY (callee)
		  && ! TREE_THIS_VOLATILE (callee))))
	return;

      /* Create a may-use followed by a clobbering definition of GLOBAL_VAR.  */
      create_ref (global_var, V_USE | M_MAY, bb, parent_stmt, parent_expr,
		  NULL, true);
      create_ref (global_var, V_DEF | M_CLOBBER, bb, parent_stmt, parent_expr,
		  NULL, true);

      return;
    }

  /* ADDR_EXPR nodes create an address-of use of their operand.  This means
     that the variable is not read, but its address is needed.  */
  if (code == ADDR_EXPR)
    {
      find_refs_in_expr (&TREE_OPERAND (expr, 0), V_USE|M_ADDRESSOF, bb,
			 parent_stmt, parent_expr);
      return;
    }

  /* Lists.  */
  if (code == TREE_LIST)
    {
      tree op;

      for (op = expr; op; op = TREE_CHAIN (op))
	find_refs_in_expr (&TREE_VALUE (op), ref_type, bb, parent_stmt,
	    parent_expr);
      return;
    }

  /* Unary expressions.  */
  if (class == '1'
      || code == EXPR_WITH_FILE_LOCATION
      || code == VA_ARG_EXPR)
    {
      find_refs_in_expr (&TREE_OPERAND (expr, 0), ref_type, bb, parent_stmt,
			 parent_expr);
      return;
    }

  /* Binary expressions.  */
  if (class == '2'
      || class == '<'
      || code == TRUTH_AND_EXPR
      || code == TRUTH_OR_EXPR
      || code == TRUTH_XOR_EXPR
      || code == COMPOUND_EXPR
      || code == CONSTRUCTOR)
    {
      find_refs_in_expr (&TREE_OPERAND (expr, 0), ref_type, bb, parent_stmt,
			 parent_expr);
      find_refs_in_expr (&TREE_OPERAND (expr, 1), ref_type, bb, parent_stmt,
			 parent_expr);
      return;
    }

  /* If we get here, something has gone wrong.  */
  prep_stmt (parent_stmt);
  error ("unhandled expression in find_refs_in_expr():");
  debug_tree (expr);
  fputs ("\n", stderr);
  abort ();
}


/* Create and return an empty list of references.  */

ref_list
create_ref_list ()
{
  ref_list list = xmalloc (sizeof (struct ref_list_priv));
  list->first = list->last = NULL;
  return list;
}

/* Free the nodes in LIST, but keep the empty list around.  
   (i.e., empty the list).  */

void 
empty_ref_list (list)
     ref_list list;
{
  struct ref_list_node *node;

  if (list == NULL)
    return;

  for (node = list->first; node; )
    {
      struct ref_list_node *tmp;
      tmp = node;
      node = node->next;
      free (tmp);
    }
  list->first = list->last = NULL;
}


/* Delete LIST, including the list itself.
   (i.e., destroy the list).  */

void
delete_ref_list (list)
     ref_list list;
{
  struct ref_list_node *node;

  if (list == NULL)
    return;

  for (node = list->first; node; )
    {
      struct ref_list_node *tmp;
      tmp = node;
      node = node->next;
      free (tmp);
    }
  free (list);
}

/* Remove REF from LIST.  */

void 
remove_ref_from_list (list, ref)
     ref_list list;
     tree_ref ref;
{
  struct ref_list_node *tmp;

  if (!list || !list->first || !list->last)
    return;

  tmp = find_list_node (list, ref);

  if (tmp == list->first)
    list->first = tmp->next;
  if (tmp == list->last)
    list->last = tmp->prev;
  if (tmp->next)
    tmp->next->prev = tmp->prev;
  if (tmp->prev)
    tmp->prev->next = tmp->next;

  free (tmp);
  return;
}


/* Add REF to the beginning of LIST.  */

void
add_ref_to_list_begin (list, ref)
     ref_list list;
     tree_ref ref;
{
  struct ref_list_node *node = xmalloc (sizeof (struct ref_list_node));
  node->ref = ref;
  if (list->first == NULL)
    {
      node->prev = node->next = NULL;
      list->first = list->last = node;
      return;
    }
  node->prev = NULL;
  node->next = list->first;
  list->first->prev = node;
  list->first = node;
}


/* Add REF to the end of LIST.  */

void
add_ref_to_list_end (list, ref)
     ref_list list;
     tree_ref ref;
{
  struct ref_list_node *node = xmalloc (sizeof (struct ref_list_node));
  node->ref = ref;

  if (list->first == NULL)
    {
      node->prev = node->next = NULL;
      list->first = list->last = node;
      return;
    }
  node->prev = list->last;
  node->next = NULL;
  list->last->next = node;
  list->last = node;
}

/* Add the contents of the list TOADD to the list LIST, at the beginning of
   LIST. */ 
void 
add_list_to_list_begin (list, toadd)
     ref_list list;
     ref_list toadd;
{
  struct ref_list_node *tmp;
  tree_ref tempref;

  FOR_EACH_REF (tempref, tmp, toadd)
  {
    add_ref_to_list_begin (list, tempref);
  }
}

/* Add the contents of the list TOADD to the list LIST, at the end of LIST. */
void
add_list_to_list_end (list, toadd)
     ref_list list;
     ref_list toadd;
{
  struct ref_list_node *tmp;
  tree_ref tempref;
  
  FOR_EACH_REF (tempref, tmp, toadd)
  {
    add_ref_to_list_end (list, tempref);
  }
}

/* Find the list container for reference REF in LIST.  */

struct ref_list_node *
find_list_node (list, ref)
     ref_list list;
     tree_ref ref;
{
  struct ref_list_node *node = NULL;

  if (list->first == NULL)
    return NULL;

  if (ref == list->first->ref)
    node = list->first;
  else if (ref == list->last->ref)
    node = list->last;
  else
    {
      for (node = list->first; node; node = node->next)
	if (node->ref == ref)
	  break;
    }

  return node;
}


/* Add REF after NODE.  */

void
add_ref_to_list_after (list, node, ref)
     ref_list list;
     struct ref_list_node *node;
     tree_ref ref;
{
  if (node == list->last)
    add_ref_to_list_end (list, ref);
  else if (node == list->first)
    add_ref_to_list_begin (list, ref);
  else
    {
      struct ref_list_node *new = xmalloc (sizeof (struct ref_list_node));
      new->ref = ref;
      new->prev = node;
      new->next = node->next;
      node->next = new;
    }
}


/* Create references and associations to variables and basic blocks.  */

/* Create a new variable reference for variable VAR.

   REF_TYPE is the type of reference to create (V_DEF, V_USE, V_PHI, etc).
    
   BB, PARENT_STMT, PARENT_EXPR and OPERAND_P give the exact location of the
      reference.  PARENT_STMT, PARENT_EXPR and OPERAND_P can be NULL in the
      case of artificial references (PHI nodes, default definitions, etc).

   ADD_TO_BB should be true if the caller wants the reference to be added
      to the list of references for BB (i.e., bb_refs (BB)).  In that case,
      the reference is added at the end of the list.
      
      This is a problem for certain types of references like V_PHI or
      default defs that need to be added in specific places within the list
      of references for the BB.  If ADD_TO_BB is false, the caller is
      responsible for the placement of the newly created reference.  */

tree_ref
create_ref (var, ref_type, bb, parent_stmt, parent_expr, operand_p, add_to_bb)
     tree var;
     HOST_WIDE_INT ref_type;
     basic_block bb;
     tree parent_stmt;
     tree parent_expr;
     tree *operand_p;
     int add_to_bb;
{
  tree_ref ref;

#if defined ENABLE_CHECKING
  if (bb == NULL)
    abort ();

  if (ref_type & (V_DEF | V_USE | V_PHI)
      && TREE_CODE_CLASS (TREE_CODE (var)) != 'd'
      && TREE_CODE (var) != INDIRECT_REF)
    abort ();
#endif

  /* Set the M_VOLATILE modifier if the reference is to a volatile
     variable.  */
  if (var && TREE_THIS_VOLATILE (var))
    ref_type |= M_VOLATILE;

  ref = (tree_ref) ggc_alloc (sizeof (*ref));
  memset ((void *) ref, 0, sizeof (*ref));

  ref->common.id = next_tree_ref_id++;
  ref->common.var = var;
  ref->common.type = ref_type;
  ref->common.stmt = parent_stmt;
  ref->common.bb = bb;
  ref->common.expr = parent_expr;
  ref->common.operand_p = operand_p;
  ref->common.orig_operand = (operand_p) ? *operand_p : NULL;

  /* Create containers according to the type of reference.  */
  if (ref_type & (V_DEF | V_PHI))
    {
      ref->vdef.imm_uses = create_ref_list ();
      ref->vdef.reached_uses = create_ref_list ();
      if (ref_type & V_PHI)
	{
	  unsigned num;
	  edge in;

	  /* Count the number of incoming edges.  */
	  for (in = bb->pred, num = 0; in; in = in->pred_next)
	    num++;

	  VARRAY_GENERIC_PTR_INIT (ref->vphi.phi_args, num, "phi_args");
	}
    }
  else if (ref_type & V_USE)
    ref->vuse.rdefs = create_ref_list ();
  else if (ref_type & E_PHI)
    {
      varray_type temp;
      VARRAY_GENERIC_PTR_INIT (temp, 
			       last_basic_block, "ephi_chain");
      set_exprphi_phi_args (ref, temp);
      set_exprphi_processed (ref, BITMAP_XMALLOC ());
      set_exprphi_downsafe (ref, true);
      set_exprphi_canbeavail (ref, true);
      set_exprphi_later (ref, true);
      set_exprphi_extraneous (ref, true);
    }

  if (var)
    {
      /* Add the variable to the list of variables referenced in this
	 function.  But only for actual variable defs or uses in the code.  */
      if ((ref_type & (V_DEF | V_USE))
	  && (TREE_CODE_CLASS (TREE_CODE (var)) == 'd'
	      || TREE_CODE (var) == INDIRECT_REF))
	add_referenced_var (var);

      /* Add this reference to the list of references for the variable.  */
      add_tree_ref (var, ref);

      /* Add this reference to the list of references for the containing
	 statement.  */
      if (parent_stmt)
	add_tree_ref (parent_stmt, ref);

      /* Ditto for the expression containing this reference.  NOTE: This is
	 only valid for unshared tree expressions, which are only
	 guaranteed in SIMPLE form.  */
      if (parent_expr)
	add_tree_ref (parent_expr, ref);
    }

  /* If requested, add this reference to the list of references for the basic
     block.  */
  if (add_to_bb)
    add_ref_to_list_end (bb_refs (bb), ref);

  /* If this is an unmodified V_DEF reference, then this reference
     represents the output of its parent expression (i.e., the reference is
     the LHS of an assignment expression).  In this case, tell the parent
     expression about it.

     This is useful for algorithms like constant propagation when
     evaluating expressions, the output reference for the expression is
     where the lattice value of the expression can be stored.  */
  if (ref_type == V_DEF)
    {
#if defined ENABLE_CHECKING
      if (TREE_CODE (parent_expr) != MODIFY_EXPR
	  && TREE_CODE (parent_expr) != INIT_EXPR)
	abort ();
#endif
      set_output_ref (parent_expr, ref);
    }


  return ref;
}


/* Add a new argument to PHI for definition DEF reaching in via edge E.  */

void
add_phi_arg (phi, def, e)
     tree_ref phi;
     tree_ref def;
     edge e;
{
  phi_node_arg arg;

  arg = (phi_node_arg) ggc_alloc (sizeof (*arg));
  memset ((void *) arg, 0, sizeof (*arg));
  dfa_counts.num_phi_args++;

  arg->def = def;
  arg->e = e;

  VARRAY_PUSH_GENERIC_PTR (phi->vphi.phi_args, (PTR)arg);
}


/* Add a unique copy of variable VAR to the list of referenced variables.  */

static void
add_referenced_var (var)
     tree var;
{
  if (tree_flags (var) & TF_REFERENCED)
    return;

  /* The variable has not been referenced yet.  Mark it referenced and add it
     to the list.  */
  set_tree_flag (var, TF_REFERENCED);
  VARRAY_PUSH_TREE (referenced_vars, var);
  num_referenced_vars = VARRAY_ACTIVE_SIZE (referenced_vars);
}


/* Manage annotations.  */

/* Create a new annotation for tree T.  */

tree_ann
create_tree_ann (t)
     tree t;
{
  tree_ann ann = (tree_ann) ggc_alloc (sizeof (*ann));
  memset ((void *) ann, 0, sizeof (*ann));
  ann->refs = create_ref_list ();
  t->common.aux = (void *) ann;
  return ann;
}


/* Remove the annotation for tree T.  */

void
remove_tree_ann (t)
     tree t;
{
  tree_ann ann = (tree_ann)t->common.aux;

  if (ann == NULL)
    return;

  ann->bb = NULL;
  delete_ref_list (ann->refs);
  ann->currdef = NULL;
  ann->compound_parent = NULL;
  ann->output_ref = NULL;
  t->common.aux = NULL;
}


/* Miscellaneous helpers.  */

/* Return 1 if the function may call itself.
   
   FIXME Currently this is very limited because we do not have call-graph
	 information.  */

int
function_may_recurse_p ()
{
  basic_block bb;

  /* If we only make calls to pure and/or builtin functions, then the
     function is not recursive.  */
  FOR_EACH_BB (bb)
    {
      struct ref_list_node *tmp;
      tree_ref ref;

      FOR_EACH_REF (ref, tmp, bb_refs (bb))
	if (ref_var (ref) == global_var
	      && (ref_type (ref) & (V_DEF | M_CLOBBER)))
	  return 1;
    }

  return 0;
}


/* Return the basic block containing the statement that declares DECL.  A
   NULL return value means that DECL is a global variable.  */

basic_block
find_declaration (decl)
     tree decl;
{
  basic_block bb;
  tree t;
  tree_ref first_ref;

  /* Start with the first reference of DECL and walk the flowgraph
     backwards looking for a node with the scope block declaring the
     original variable.  */
  if (!tree_refs (decl) || !tree_refs (decl)->first)
    return NULL;

  first_ref = tree_refs (decl)->first->ref;
  t = ref_stmt (first_ref);
  FOR_BB_BETWEEN (bb, bb_for_stmt (t), ENTRY_BLOCK_PTR, prev_bb)
    {
      if (TREE_CODE (bb->head_tree) == SCOPE_STMT
	  && SCOPE_STMT_BLOCK (bb->head_tree))
	{
	  tree block = SCOPE_STMT_BLOCK (bb->head_tree);
	  tree var;

	  for (var = BLOCK_VARS (block); var; var = TREE_CHAIN (var))
	    if (var == decl)
	      return bb;
	}
    }

  return NULL;
}



/* Debugging functions.  */

/* Display variable reference REF on stream OUTF.  PREFIX is a string that
   is prefixed to every line of output, and INDENT is the amount of left
   margin to leave.  If DETAILS is nonzero, the output is more verbose.  */

void
dump_ref (outf, prefix, ref, indent, details)
     FILE *outf;
     const char *prefix;
     tree_ref ref;
     int indent;
     int details;
{
  int lineno, bbix;
  const char *type;
  char *s_indent;

  if (ref == NULL)
    return;

  s_indent = (char *) alloca ((size_t) indent + 1);
  memset ((void *) s_indent, ' ', (size_t) indent);
  s_indent[indent] = '\0';

  lineno = (ref_stmt (ref)) ? STMT_LINENO (ref_stmt (ref)) : -1;

  bbix = (ref_bb (ref)) ? ref_bb (ref)->index : -1;

  type = ref_type_name (ref_type (ref));

  fprintf (outf, "%s%s%s(", s_indent, prefix, type);

  if (ref_var (ref))
    print_c_node (outf, ref_var (ref));
  else
    fprintf (outf, "nil");

  fprintf (outf, "): line %d, bb %d, id %lu, ", lineno, bbix, ref_id (ref));

  if (ref_expr (ref))
    print_c_node (outf, ref_expr (ref));
  else
    fprintf (outf, "<nil>");

  /* Dump specific contents for the different types of references.  */
  if (details)
    {
      if (ref_type (ref) & V_PHI)
	{
	  if (phi_args (ref))
	    {
	      fputs (" phi-args:\n", outf);
	      dump_phi_args (outf, prefix, phi_args (ref), indent + 4, 0);
	    }

	  if (imm_uses (ref))
	    {
	      fputs ("    immediate uses:\n", outf);
	      dump_ref_list (outf, prefix, imm_uses (ref), indent + 4, 0);
	    }
	}

      if ((ref_type (ref) & E_PHI) && exprphi_phi_args (ref))
	{
	  if (details)
	    fprintf (outf, " class:%d downsafe:%d can_be_avail:%d later:%d\n", 
		     exprref_class (ref), exprphi_downsafe (ref), 
		     exprphi_canbeavail (ref), exprphi_later (ref));
	  fputs (" expr-phi-args:\n", outf);
	  dump_ref_array (outf, prefix, exprphi_phi_args (ref), indent + 4, 1);
	}	

      if ((ref_type (ref) & V_DEF) && imm_uses (ref))
	{
	  fputs (" immediate uses:\n", outf);
	  dump_ref_list (outf, prefix, imm_uses (ref), indent + 4, 0);
	}

      if (ref_type (ref) & V_USE && imm_reaching_def (ref))
	{
	  fputs (" immediate reaching def:\n", outf);
	  dump_ref (outf, prefix, imm_reaching_def (ref), indent + 4, 0);
	}	  

      if ((ref_type (ref) & E_USE) && expruse_phiop (ref) == true)
	{
	  char *temp_indent;
	  fprintf (outf, " class:%d has_real_use:%d  operand defined by:\n", 
		   exprref_class (ref), expruse_has_real_use (ref));	  
	  temp_indent = (char *) alloca ((size_t) indent + 4 + 1);
	  memset ((void *) temp_indent, ' ', (size_t) indent + 4);
	  temp_indent[indent + 4] = '\0';
	  if (expruse_def (ref) == NULL)
	    fprintf (outf, "%snothing\n", temp_indent);
	  else
	    dump_ref (outf, prefix, expruse_def (ref), indent + 4, 0);
	}
    }

  fputc ('\n', outf);
}


/*  Display variable reference REF on stderr.  */

void
debug_ref (ref)
     tree_ref ref;
{
  dump_ref (stderr, "", ref, 0, 1);
}


/*  Display a list of variable references on stream OUTF. PREFIX is a
    string that is prefixed to every line of output, and INDENT is the
    amount of left margin to leave.  If DETAILS is nonzero, the output is
    more verbose.  */

void
dump_ref_list (outf, prefix, reflist, indent, details)
     FILE *outf;
     const char *prefix;
     ref_list reflist;
     int indent;
     int details;
{
  struct ref_list_node *tmp;
  tree_ref ref;

  if (reflist == NULL)
    return;

  FOR_EACH_REF (ref, tmp, reflist)
    dump_ref (outf, prefix, ref, indent, details);
}



/*  Display a list of variable references on stream OUTF. PREFIX is a
    string that is prefixed to every line of output, and INDENT is the
    amount of left margin to leave.  If DETAILS is nonzero, the output is
    more verbose.  */

void
dump_ref_array (outf, prefix, reflist, indent, details)
     FILE *outf;
     const char *prefix;
     varray_type reflist;
     int indent;
     int details;
{
  size_t i;

  if (reflist == NULL)
    return;

  for (i = 0; i < VARRAY_SIZE (reflist); i++)
    if (VARRAY_GENERIC_PTR (reflist, i))
      dump_ref (outf, prefix, VARRAY_GENERIC_PTR (reflist, i), 
		   indent, details);
}

/*  Dump REFLIST on stderr.  */

void
debug_ref_list (reflist)
     ref_list reflist;
{
  dump_ref_list (stderr, "", reflist, 0, 1);
}


/* Dump REFLIST on stderr.  */

void
debug_ref_array (reflist)
    varray_type reflist;
{
  dump_ref_array (stderr, "", reflist, 0, 1);
}


/* Dump the list of all the referenced variables in the current function.  */

void
dump_referenced_vars (file)
     FILE *file;
{
  size_t i;

  fprintf (file, "\nReferenced variables: %lu\n\n", num_referenced_vars);

  for (i = 0; i < num_referenced_vars; i++)
    {
      tree var = referenced_var (i);
      dump_variable (file, var);
      dump_ref_list (file, "", tree_refs (var), 4, 1);
      fputc ('\n', file);
    }
}


/* Dump the list of all the referenced variables to stderr.  */

void
debug_referenced_vars ()
{
  dump_referenced_vars (stderr);
}


/* Dump a variable and its may-aliases to FILE.  */

void
dump_variable (file, var)
     FILE *file;
     tree var;
{
  size_t num;

  fprintf (file, "Variable: ");
  print_c_node (file, var);
  
  num = num_may_alias (var);
  if (num > 0)
    {
      size_t i;

      fprintf (file, ", may-aliases: {");

      for (i = 0; i < num; i++)
	{
	  print_c_node (file, may_alias (var, i));
	  if (i < num - 1)
	    fprintf (file, ", ");
	}
      fprintf (file, "}\n");
    }
  else
    fprintf (file, "\n");
}


/* Dump a variable and its may-aliases to stderr.  */

void
debug_variable (var)
     tree var;
{
  dump_variable (stderr, var);
}


/* Dump the given array of phi arguments on stderr.  */

void
debug_phi_args (args)
     varray_type args;
{
  dump_phi_args (stderr, "", args, 0, 0);
}


/* Display the given array of PHI arguments definitions on stream OUTF.  PREFIX
   is a string that is prefixed to every line of output, and INDENT is the
   amount of left margin to leave.  If DETAILS is nonzero, the output is more
   verbose.  */

void
dump_phi_args (outf, prefix, args, indent, details)
     FILE *outf;
     const char *prefix;
     varray_type args;
     int indent;
     int details;
{
  size_t i;

  if (args == NULL)
    return;

  for (i = 0; i < VARRAY_SIZE (args); i++)
    {
      phi_node_arg arg = VARRAY_GENERIC_PTR (args, i);
      if (arg)
	dump_ref (outf, prefix, phi_arg_def (arg), indent, details);
      else
	fprintf (outf, "<nil>\n");
    }
}


/* Dump various DFA statistics to FILE.  */

#define SCALE(x) ((unsigned long) ((x) < 1024*10 \
		  ? (x) \
		  : ((x) < 1024*1024*10 \
		     ? (x) / 1024 \
		     : (x) / (1024*1024))))
#define LABEL(x) ((x) < 1024*10 ? 'b' : ((x) < 1024*1024*10 ? 'k' : 'M'))
#define PERCENT(x,y) ((float)(x) * 100.0 / (float)(y))

void
dump_dfa_stats (file)
     FILE *file;
{
  struct dfa_stats_d dfa_stats;
  unsigned long size, total = 0;
  const char * const fmt_str   = "%-30s%-13s%12s\n";
  const char * const fmt_str_1 = "%-30s%13lu%11lu%c\n";
  const char * const fmt_str_2 = "%-30s%6lu (%3.0f%%)\n";
  const char * const fmt_str_3 = "%-43s%11lu%c\n";

  collect_dfa_stats (&dfa_stats);

  fprintf (file, "\nDFA Statistics for %s\n\n", get_name (current_function_decl));

  fprintf (file, "---------------------------------------------------------\n");
  fprintf (file, fmt_str, "", "  Number of  ", "Memory");
  fprintf (file, fmt_str, "", "  instances  ", "used ");
  fprintf (file, "---------------------------------------------------------\n");

  size = num_referenced_vars * sizeof (tree);
  total += size;
  fprintf (file, fmt_str_1, "Referenced variables", num_referenced_vars, 
	   SCALE (size), LABEL (size));

  size = dfa_stats.num_tree_anns * sizeof (struct tree_ann_d);
  total += size;
  fprintf (file, fmt_str_1, "Trees annotated", dfa_stats.num_tree_anns,
	   SCALE (size), LABEL (size));

  size = dfa_stats.num_ref_list_nodes * sizeof (struct ref_list_node);
  total += size;
  fprintf (file, fmt_str_1, "ref_list nodes", dfa_stats.num_ref_list_nodes,
	   SCALE (size), LABEL (size));

  size = dfa_stats.num_phi_args * sizeof (struct phi_node_arg_d);
  total += size;
  fprintf (file, fmt_str_1, "PHI arguments", dfa_stats.num_phi_args,
	   SCALE (size), LABEL (size));

  size = dfa_stats.num_may_alias * sizeof (tree); 
  total += size;
  fprintf (file, fmt_str_1, "may-aliases", dfa_stats.num_may_alias,
	   SCALE (size), LABEL (size));

  size = dfa_stats.num_alias_imm_rdefs * sizeof (tree_ref);
  total += size;
  fprintf (file, fmt_str_1, "SSA links for may-aliases",
	   dfa_stats.num_alias_imm_rdefs, SCALE (size), LABEL (size));

  size = dfa_stats.num_tree_refs * sizeof (union tree_ref_d);
  total += size;
  fprintf (file, fmt_str_1, "Variable references", dfa_stats.num_tree_refs,
	   SCALE (size), LABEL (size));

  if (dfa_stats.num_tree_refs == 0)
    dfa_stats.num_tree_refs = 1;

  if (dfa_stats.num_defs)
    fprintf (file, fmt_str_2, "    V_DEF", dfa_stats.num_defs,
	     PERCENT (dfa_stats.num_defs, dfa_stats.num_tree_refs));

  if (dfa_stats.num_uses)
    fprintf (file, fmt_str_2, "    V_USE", dfa_stats.num_uses,
	     PERCENT (dfa_stats.num_uses, dfa_stats.num_tree_refs));

  if (dfa_stats.num_phis)
    fprintf (file, fmt_str_2, "    V_PHI", dfa_stats.num_phis,
	     PERCENT (dfa_stats.num_phis, dfa_stats.num_tree_refs));

  if (dfa_stats.num_ephis)
    fprintf (file, fmt_str_2, "    E_PHI", dfa_stats.num_ephis,
	     PERCENT (dfa_stats.num_ephis, dfa_stats.num_tree_refs));

  if (dfa_stats.num_euses)
    fprintf (file, fmt_str_2, "    E_USE", dfa_stats.num_euses,
	     PERCENT (dfa_stats.num_euses, dfa_stats.num_tree_refs));

  if (dfa_stats.num_ekills)
    fprintf (file, fmt_str_2, "    E_KILL", dfa_stats.num_ekills,
	     PERCENT (dfa_stats.num_ekills, dfa_stats.num_tree_refs));

  fprintf (file, "---------------------------------------------------------\n");
  fprintf (file, fmt_str_3, "Total memory used by DFA/SSA data", SCALE (total),
	   LABEL (total));
  fprintf (file, "---------------------------------------------------------\n");
  fprintf (file, "\n");

  if (dfa_stats.num_phis)
    fprintf (file, "Average number of PHI arguments per PHI node: %.1f (max: %lu)\n",
	     (float) dfa_stats.num_phi_args / (float) dfa_stats.num_phis,
	     dfa_stats.max_num_phi_args);

  if (dfa_stats.num_may_alias)
    fprintf (file, "Average number of may-aliases per variable: %.1f (max: %lu)\n",
	     (float) dfa_stats.num_may_alias / (float) num_referenced_vars,
	     dfa_stats.max_num_may_alias);
  
  if (dfa_stats.num_alias_imm_rdefs)
    fprintf (file, "Average number of SSA links for may-aliases per reference: %.1f (max: %lu)\n",
	     (float) dfa_stats.num_alias_imm_rdefs
	     / (float) dfa_stats.num_tree_refs,
	     dfa_stats.max_num_alias_imm_rdefs);
  
  fprintf (file, "\n");

  dump_if_different (file, "Discrepancy in variable references: %ld\n",
		     next_tree_ref_id, dfa_stats.num_tree_refs);

  dump_if_different (file, "Discrepancy in PHI arguments: %ld\n",
		     dfa_counts.num_phi_args, dfa_stats.num_phi_args);

  dump_if_different (file, "Discrepancy in may-aliases: %ld\n",
		     dfa_counts.num_may_alias, dfa_stats.num_may_alias);

  dump_if_different (file,
		     "Discrepancy in SSA links for may-aliases: %ld\n",
		     dfa_counts.num_alias_imm_rdefs,
		     dfa_stats.num_alias_imm_rdefs);

  fprintf (file, "\n");
}


static void
dump_if_different (file, fmt_str, expected, counted)
     FILE *file;
     const char * const fmt_str;
     unsigned long expected;
     unsigned long counted;
{
  if (expected != counted)
    {
      fprintf (file, fmt_str, labs (expected - counted));
      fprintf (file, "\texpected: %lu\n", expected);
      fprintf (file, "\tcounted:  %lu\n", counted);
    }
}


/* Dump DFA statistics on stderr.  */

void
debug_dfa_stats ()
{
  dump_dfa_stats (stderr);
}


/* Collect DFA statistics into *DFA_STATS_P.  */

static void
collect_dfa_stats (dfa_stats_p)
     struct dfa_stats_d *dfa_stats_p;
{
  htab_t htab;
  tree first_stmt;
  basic_block bb;

  if (dfa_stats_p == NULL)
    abort ();

  memset ((void *)dfa_stats_p, 0, sizeof (struct dfa_stats_d));

  /* Walk all the trees in the function counting references.  */
  first_stmt = BASIC_BLOCK (0)->head_tree;
  htab = htab_create (30, htab_hash_pointer, htab_eq_pointer, NULL);
  walk_tree (&first_stmt, collect_dfa_stats_r, (void *) dfa_stats_p,
             (void *) htab);

  /* Also look into GLOBAL_VAR (which is not actually part of the program).  */
  walk_tree (&global_var, collect_dfa_stats_r, (void *) dfa_stats_p, NULL);

  FOR_EACH_BB (bb)
    count_tree_refs (dfa_stats_p, bb_refs (bb));
}


/* Callback for walk_tree to collect DFA statistics for a tree and its
   children.  */

static tree
collect_dfa_stats_r (tp, walk_subtrees, data)
     tree *tp;
     int *walk_subtrees ATTRIBUTE_UNUSED;
     void *data;
{
  tree_ann ann;
  tree t = *tp;
  struct dfa_stats_d *dfa_stats_p = (struct dfa_stats_d *)data;

  ann = tree_annotation (t);
  if (ann)
    {
      dfa_stats_p->num_tree_anns++;
      count_ref_list_nodes (dfa_stats_p, ann->refs);
      if (ann->may_aliases)
	{
	  size_t num = VARRAY_ACTIVE_SIZE (ann->may_aliases);
	  dfa_stats_p->num_may_alias += num;
	  if (num > dfa_stats_p->max_num_may_alias)
	    dfa_stats_p->max_num_may_alias = num;
	}
    }

  return NULL;
}


/* Update DFA_STATS_P with the number of tree_ref objects in LIST.  */

static void
count_tree_refs (dfa_stats_p, list)
     struct dfa_stats_d *dfa_stats_p;
     ref_list list;
{
  tree_ref ref;
  struct ref_list_node *tmp;

  FOR_EACH_REF (ref, tmp, list)
    {
      dfa_stats_p->num_tree_refs++;

      if (ref->vref.alias_imm_rdefs)
	{
	  size_t num = num_may_alias (ref_var (ref));
	  dfa_stats_p->num_alias_imm_rdefs += num;
	  if (num > dfa_stats_p->max_num_alias_imm_rdefs)
	    dfa_stats_p->max_num_alias_imm_rdefs = num;
	}

      if (ref_type (ref) & V_DEF)
	{
	  dfa_stats_p->num_defs++;
	  count_ref_list_nodes (dfa_stats_p, imm_uses (ref));
	  count_ref_list_nodes (dfa_stats_p, reached_uses (ref));
	}
      else if (ref_type (ref) & V_USE)
	{
	  dfa_stats_p->num_uses++;
	  count_ref_list_nodes (dfa_stats_p, reaching_defs (ref));
	}
      else if (ref_type (ref) & V_PHI)
	{
	  unsigned int num = num_phi_args (ref);
	  dfa_stats_p->num_phis++;
	  dfa_stats_p->num_phi_args += num;
	  if (num > dfa_stats_p->max_num_phi_args)
	    dfa_stats_p->max_num_phi_args = num;
	}
      else if (ref_type (ref) & E_PHI)
	dfa_stats_p->num_ephis++;
      else if (ref_type (ref) & E_USE)
	dfa_stats_p->num_euses++;
      else if (ref_type (ref) & E_KILL)
	dfa_stats_p->num_ekills++;
    }
}


/* Count the number of nodes in a ref_list container.  */

static void
count_ref_list_nodes (dfa_stats_p, list)
     struct dfa_stats_d *dfa_stats_p;
     ref_list list;
{
  tree_ref ref;
  struct ref_list_node *tmp;

  FOR_EACH_REF (ref, tmp, list)
    dfa_stats_p->num_ref_list_nodes++;
}


/* Return the reference type as a string.  */

const char *
ref_type_name (type)
     HOST_WIDE_INT type;
{
#define max 80
  static char str[max];

#if defined ENABLE_CHECKING
  if (!validate_ref_type (type))
    abort ();
#endif

  strncpy (str, type & V_DEF ? "V_DEF"
	        : type & V_USE ? "V_USE"
	        : type & V_PHI ? "V_PHI"
	        : type & E_PHI ? "E_PHI"
	        : type & E_USE ? "E_USE"
	        : type & E_KILL ? "E_KILL"
	        : "???",
	   max);

  if (type & M_DEFAULT)
    strncat (str, "/default", max - strlen (str));

  if (type & M_MAY)
    strncat (str, "/may", max - strlen (str));

  if (type & M_PARTIAL)
    strncat (str, "/partial", max - strlen (str));

  if (type & M_CLOBBER)
    strncat (str, "/clobber", max - strlen (str));

  if (type & M_INITIAL)
    strncat (str, "/initial", max - strlen (str));

  if (type & M_VOLATILE)
    strncat (str, "/volatile", max - strlen (str));

  if (type & M_RELOCATE)
    strncat (str, "/relocate", max - strlen (str));

  if (type & M_ADDRESSOF)
    strncat (str, "/addressof", max - strlen (str));

  return str;
}


/* Return true if TYPE is a valid reference type.  */

bool
validate_ref_type (type)
     HOST_WIDE_INT type;
{
  type &= ~M_VOLATILE;

  if (type & V_DEF)
    {
      type &= ~(M_DEFAULT | M_MAY | M_PARTIAL | M_CLOBBER | M_INITIAL
	        | M_RELOCATE);
      return type == V_DEF;
    }
  else if (type & V_USE)
    {
      type &= ~(M_MAY | M_PARTIAL | M_ADDRESSOF);
      return type == V_USE;
    }
  else if (type & V_PHI)
    {
      return type == V_PHI;
    }
  else if (type & E_PHI)
    {
      return type == E_PHI;
    }
  else if (type & E_USE)
    {
      return type == E_USE;
    }
  else if (type & E_KILL)
    {
      return type == E_KILL;
    }

  return false;
}

/* Callback for walk_tree.  Create a may-def/may-use reference for every _DECL
   and compound reference found under *TP.  */

static tree
clobber_vars_r (tp, walk_subtrees, data)
     tree *tp;
     int *walk_subtrees ATTRIBUTE_UNUSED;
     void *data;
{
  enum tree_code code = TREE_CODE (*tp);
  struct clobber_data_d *clobber = (struct clobber_data_d *)data;

  /* Create may-use and clobber references for every *_DECL in sight.  */
  if (code == VAR_DECL || code == PARM_DECL)
    {
      create_ref (*tp, V_USE | M_MAY, clobber->bb, clobber->parent_stmt,
		  clobber->parent_expr, NULL, true);
      create_ref (*tp, V_DEF | M_CLOBBER, clobber->bb, clobber->parent_stmt,
		  clobber->parent_expr, NULL, true);
    }

  return NULL;
}


/* Compute may-alias information for every variable referenced in the
   program.  FIXME this computes a bigger set than necessary.

   This function also inserts default definitions for all the variables
   referenced in the function. This allows the identification of variables
   that have been used without a preceding definition.  It also allows for
   default values to be assumed by transformations like constant
   propagation.  */

static void
compute_may_aliases ()
{
  unsigned long i;

  for (i = 0; i < num_referenced_vars; i++)
    {
      tree var = referenced_var (i);
      tree sym = get_base_symbol (var);

      /* Find aliases for pointer variables.  */
      if (POINTER_TYPE_P (TREE_TYPE (sym)))
	find_may_aliases_for (var);
    }
}


/* Find variables that PTR may be aliasing.  */

static void
find_may_aliases_for (ptr)
     tree ptr;
{
  unsigned long i;

  for (i = 0; i < num_referenced_vars; i++)
    {
      tree var = referenced_var (i);
      tree var_sym = get_base_symbol (var);

      if (may_alias_p (ptr, var_sym))
	{
	  add_may_alias (ptr, var_sym);
	  add_may_alias (var_sym, ptr);
	}
    }
}


/* Return true if PTR (an INDIRECT_REF tree) may alias VAR_SYM (a _DECL tree).
   FIXME  This returns true more often than it should.  */

static bool
may_alias_p (ptr, var_sym)
     tree ptr;
     tree var_sym;
{
  HOST_WIDE_INT ptr_alias_set, var_alias_set;
  tree ptr_sym = get_base_symbol (ptr);

  /* GLOBAL_VAR aliases every global variable and locals that have had
     its address taken.  */
  if (ptr == global_var
      && var_sym != global_var
      && (TREE_ADDRESSABLE (var_sym)
	  || decl_function_context (var_sym) == NULL))
    return true;

  /* Obvious reasons why PTR_SYM and VAR_SYM can't possibly alias
     each other.  */
  if (var_sym == ptr_sym
      || !POINTER_TYPE_P (TREE_TYPE (ptr_sym))
      || !TREE_ADDRESSABLE (var_sym)
      || DECL_ARTIFICIAL (var_sym)
      || !is_visible_to (var_sym, ptr_sym))
    return false;

  ptr_alias_set = get_alias_set (TREE_TYPE (ptr));
  var_alias_set = get_alias_set (TREE_TYPE (var_sym));

  return alias_sets_conflict_p (ptr_alias_set, var_alias_set);
}


/* Return true if SYM1 and SYM2 are visible to each other.  Visibility is
   determined based on the scopes where each variable is declared.  Both
   scopes must be the same or one must be enclosed in the other.  */

static bool
is_visible_to (sym1, sym2)
     tree sym1;
     tree sym2;
{
  basic_block bb, low_bb, high_bb;
  basic_block bb1 = find_declaration (sym1);
  basic_block bb2 = find_declaration (sym2);

  /* If either variable is global, find_declaration() will return NULL.  In
     which case, the variables are visible to each other.  */
  if (bb1 == NULL || bb2 == NULL)
    return true;

  /* If BB1 and BB2 are the same, then both variables can see each other.  */
  if (bb1 == bb2)
    return true;

  /* Now walk up and down the scope binding chains trying to reach BB1 from
     BB2 and vice-versa.  Note that we can't use dominator information to
     determine this.  It may happen that BB1 dominates BB2 and yet the two
     symbols are not visible to each other.  E.g.,

     		{
		  int p;
		  ...
		}
		{
		  int q;
		  ...
		}

     The only trick we can use is to start looking up from the basic block
     with a higher index value in the hopes that if they do contain each
     other, starting up from the higher numbered block will reach the other
     one.  */
  low_bb = (bb1->index > bb2->index) ? bb1 : bb2;
  high_bb = (bb1->index > bb2->index) ? bb2 : bb1;

  for (bb = high_bb; binding_scope (bb) != bb; bb = binding_scope (bb))
    if (bb == low_bb)
      return true;

  /* Rats.  The lower numbered basic block is deeper in the graph.  Do the
     opposite search now.  */
  for (bb = low_bb; binding_scope (bb) != bb; bb = binding_scope (bb))
    if (bb == high_bb)
      return true;

  /* These variables can possibly see each other.  */
  return false;
}


/* Return true if REF is a V_DEF reference for VAR.  This function handles
   relocations of pointers.  Relocating a pointer, clobbers any dereference
   of the pointer, but it does not affect any of its aliases.  For
   instance,

   	  1	p = &a			V_DEF(p) = &a
	  2	*p = 10			V_DEF(*p) = 10
	  3	p = ...			V_DEF(p) = ... => V_DEF/relocate(*p)
	  4	c = a + *p		V_DEF(c) = V_USE(a) + V_USE(*p)

   The definition of 'p' at line 2 clobbers '*p' because now 'p' points to
   a different location.  Therefore, the use of '*p' at line 3 cannot be
   reached by the assignment to '*p' at line 2.  However, relocating 'p'
   in this case should not modify 'a', therefore 'a' should still be
   reached by the assignment to '*p' at line 2.  */

bool
ref_defines (ref, var)
     tree_ref ref;
     tree var;
{
  tree rvar;

  if (!(ref_type (ref) & V_DEF))
    return false;
  
  rvar = ref_var (ref);

  /* If this is a relocating definition, then it only reaches VAR if both
     REF's variable and VAR are the same.  */
  if (ref_type (ref) & M_RELOCATE)
    return (rvar == var);

  /* Otherwise, REF is a definition for VAR if either VAR and REF's
     variable are the same or if VAR is an alias for REF's variable.  */
  return (rvar == var || get_alias_index (rvar, var) >= 0);
}


/* Return true if DEF is a killing definition for USE.  Note, this assumes
   that DEF reaches USE.  */ 

bool
is_killing_def (def, use)
     tree_ref def;
     tree_ref use;
{
  tree def_var = ref_var (def);
  tree use_var = ref_var (use);

  if (!(ref_type (def) & (V_DEF | V_PHI)) || !(ref_type (use) & V_USE))
    return false;

  /* Partial, potential and volatile definitions are no killers.  */
  if (ref_type (def) & (M_PARTIAL | M_VOLATILE | M_MAY))
    return false;

  /* Common case.  Both references are for the same variable.  */
  if (def_var == use_var)
    return true;

  /* If DEF_VAR may-alias USE, then DEF is not a killing definition for
     USE.  */
  if (get_alias_index (use_var, def_var) >= 0)
    return false;

#if 0
  /* If DEF_VAR must-alias USE_VAR, then DEF is a killing definition for
     USE.  */
  if (are_must_aliased (def_var, use_var))
    return true;
#endif

  return false;
}


/* Add ALIAS to the set of variables that may be aliasing VAR.  */

static void
add_may_alias (var, alias)
     tree var;
     tree alias;
{
  tree_ann ann;

#if defined ENABLE_CHECKING
  if (TREE_CODE (alias) != VAR_DECL
      && TREE_CODE (alias) != PARM_DECL
      && TREE_CODE (alias) != FUNCTION_DECL
      && TREE_CODE (alias) != INDIRECT_REF)
    abort ();
#endif

  ann = tree_annotation (var);
  if (ann->may_aliases == NULL)
    VARRAY_TREE_INIT (ann->may_aliases, 3, "may_aliases");
  
  VARRAY_PUSH_TREE (ann->may_aliases, alias);
  dfa_counts.num_may_alias++;
}


/* Return the index into the set of VAR1's aliases where VAR2 is located.
   Return -1 if VAR1 and VAR2 are not aliases.  */

int
get_alias_index (var1, var2)
     tree var1;
     tree var2;
{
  size_t i;

  for (i = 0; i < num_may_alias (var1); i++)
    if (may_alias (var1, i) == var2)
      return i;

  return -1;
}
