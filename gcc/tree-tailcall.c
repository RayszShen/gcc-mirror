/* Tail call optimization on trees.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "function.h"
#include "tree-flow.h"
#include "tree-dump.h"
#include "diagnostic.h"
#include "except.h"
#include "tree-pass.h"
#include "flags.h"
#include "langhooks.h"

/* The file implements the tail recursion elimination.  It is also used to
   analyze the tail calls in general, passing the results to the rtl level
   where they are used for sibcall optimization.

   In addition to the standard tail recursion elimination, we handle the most
   trivial cases of making the call tail recursive by creating accumulators.
   For example the following function

   int sum (int n)
   {
     if (n > 0)
       return n + sum (n - 1);
     else
       return 0;
   }

   is transformed into

   int sum (int n)
   {
     int acc = 0;

     while (n > 0)
       acc += n--;

     return acc;
   }

   To do this, we maintain two accumulators (a_acc and m_acc) that indicate 
   when we reach the return x statement, we should return a_acc + x * m_acc
   instead.  They are initially initialized to 0 and 1, respectively,
   so the semantics of the function is obviously preserved.  If we are
   guaranteed that the value of the accumulator never change, we
   omit the accumulator.

   There are three cases how the function may exit.  The first one is
   handled in adjust_return_value, the other two in adjust_accumulator_values
   (the second case is actually a special case of the third one and we
   present it separately just for clarity):

   1) Just return x, where x is not in any of the remaining special shapes.
      We rewrite this to a gimple equivalent of return m_acc * x + a_acc.
      
   2) return f (...), where f is the current function, is rewritten in a
      classical tail-recursion elimination way, into assignment of arguments
      and jump to the start of the function.  Values of the accumulators
      are unchanged.
	       
   3) return a + m * f(...), where a and m do not depend on call to f.
      To preserve the semantics described before we want this to be rewritten
      in such a way that we finally return

      a_acc + (a + m * f(...)) * m_acc = (a_acc + a * m_acc) + (m * m_acc) * f(...).

      I.e. we increase a_acc by a * m_acc, multiply m_acc by m and
      eliminate the tail call to f.  Special cases when the value is just
      added or just multiplied are obtained by setting a = 0 or m = 1.

   TODO -- it is possible to do similar tricks for other operations.  */

/* A structure that describes the tailcall.  */

struct tailcall
{
  /* The block in that the call occur.  */
  basic_block call_block;

  /* The iterator pointing to the call statement.  */
  block_stmt_iterator call_bsi;

  /* True if it is a call to the current function.  */
  bool tail_recursion;

  /* The return value of the caller is mult * f + add, where f is the return
     value of the call.  */
  tree mult, add;

  /* Next tailcall in the chain.  */
  struct tailcall *next;
};

/* The variables holding the value of multiplicative and additive
   accumulator.  */
static tree m_acc, a_acc;

static bool suitable_for_tail_opt_p (void);
static bool optimize_tail_call (struct tailcall *, bool);
static void eliminate_tail_call (struct tailcall *);
static void find_tail_calls (basic_block, struct tailcall **);

/* Returns false when the function is not suitable for tail call optimization
   from some reason (e.g. if it takes variable number of arguments).  */

static bool
suitable_for_tail_opt_p (void)
{
  int i;

  if (current_function_stdarg)
    return false;

  /* No local variable should be call-clobbered.  We ignore any kind
     of memory tag, as these are not real variables.  */
  for (i = 0; i < (int) VARRAY_ACTIVE_SIZE (referenced_vars); i++)
    {
      tree var = VARRAY_TREE (referenced_vars, i);

      if (!(TREE_STATIC (var) || DECL_EXTERNAL (var))
	  && var_ann (var)->mem_tag_kind == NOT_A_TAG
	  && is_call_clobbered (var))
	return false;
    }

  return true;
}
/* Returns false when the function is not suitable for tail call optimization
   from some reason (e.g. if it takes variable number of arguments).
   This test must pass in addition to suitable_for_tail_opt_p in order to make
   tail call discovery happen.  */

static bool
suitable_for_tail_call_opt_p (void)
{
  /* alloca (until we have stack slot life analysis) inhibits
     sibling call optimizations, but not tail recursion.  */
  if (current_function_calls_alloca)
    return false;

  /* If we are using sjlj exceptions, we may need to add a call to
     _Unwind_SjLj_Unregister at exit of the function.  Which means
     that we cannot do any sibcall transformations.  */
  if (USING_SJLJ_EXCEPTIONS && current_function_has_exception_handlers ())
    return false;

  /* Any function that calls setjmp might have longjmp called from
     any called function.  ??? We really should represent this
     properly in the CFG so that this needn't be special cased.  */
  if (current_function_calls_setjmp)
    return false;

  return true;
}

/* Checks whether the expression EXPR in stmt AT is independent of the
   statement pointed by BSI (in a sense that we already know EXPR's value
   at BSI).  We use the fact that we are only called from the chain of
   basic blocks that have only single successor.  Returns the expression
   containing the value of EXPR at BSI.  */

static tree
independent_of_stmt_p (tree expr, tree at, block_stmt_iterator bsi)
{
  basic_block bb, call_bb, at_bb;
  edge e;
  edge_iterator ei;

  if (is_gimple_min_invariant (expr))
    return expr;

  if (TREE_CODE (expr) != SSA_NAME)
    return NULL_TREE;

  /* Mark the blocks in the chain leading to the end.  */
  at_bb = bb_for_stmt (at);
  call_bb = bb_for_stmt (bsi_stmt (bsi));
  for (bb = call_bb; bb != at_bb; bb = EDGE_SUCC (bb, 0)->dest)
    bb->aux = &bb->aux;
  bb->aux = &bb->aux;

  while (1)
    { 
      at = SSA_NAME_DEF_STMT (expr);
      bb = bb_for_stmt (at);

      /* The default definition or defined before the chain.  */
      if (!bb || !bb->aux)
	break;

      if (bb == call_bb)
	{
	  for (; !bsi_end_p (bsi); bsi_next (&bsi))
	    if (bsi_stmt (bsi) == at)
	      break;

	  if (!bsi_end_p (bsi))
	    expr = NULL_TREE;
	  break;
	}

      if (TREE_CODE (at) != PHI_NODE)
	{
	  expr = NULL_TREE;
	  break;
	}

      FOR_EACH_EDGE (e, ei, bb->preds)
	if (e->src->aux)
	  break;
      gcc_assert (e);

      expr = PHI_ARG_DEF_FROM_EDGE (at, e);
      if (TREE_CODE (expr) != SSA_NAME)
	{
	  /* The value is a constant.  */
	  break;
	}
    }

  /* Unmark the blocks.  */
  for (bb = call_bb; bb != at_bb; bb = EDGE_SUCC (bb, 0)->dest)
    bb->aux = NULL;
  bb->aux = NULL;

  return expr;
}

/* Simulates the effect of an assignment of ASS in STMT on the return value
   of the tail recursive CALL passed in ASS_VAR.  M and A are the
   multiplicative and the additive factor for the real return value.  */

static bool
process_assignment (tree ass, tree stmt, block_stmt_iterator call, tree *m,
		    tree *a, tree *ass_var)
{
  tree op0, op1, non_ass_var;
  tree dest = TREE_OPERAND (ass, 0);
  tree src = TREE_OPERAND (ass, 1);
  enum tree_code code = TREE_CODE (src);
  tree src_var = src;

  /* See if this is a simple copy operation of an SSA name to the function
     result.  In that case we may have a simple tail call.  Ignore type
     conversions that can never produce extra code between the function
     call and the function return.  */
  STRIP_NOPS (src_var);
  if (TREE_CODE (src_var) == SSA_NAME)
    {
      if (src_var != *ass_var)
	return false;

      *ass_var = dest;
      return true;
    }

  if (TREE_CODE_CLASS (code) != tcc_binary)
    return false;

  /* Accumulator optimizations will reverse the order of operations.
     We can only do that for floating-point types if we're assuming
     that addition and multiplication are associative.  */
  if (!flag_unsafe_math_optimizations)
    if (FLOAT_TYPE_P (TREE_TYPE (DECL_RESULT (current_function_decl))))
      return false;

  /* We only handle the code like

     x = call ();
     y = m * x;
     z = y + a;
     return z;

     TODO -- Extend it for cases where the linear transformation of the output
     is expressed in a more complicated way.  */

  op0 = TREE_OPERAND (src, 0);
  op1 = TREE_OPERAND (src, 1);

  if (op0 == *ass_var
      && (non_ass_var = independent_of_stmt_p (op1, stmt, call)))
    ;
  else if (op1 == *ass_var
	   && (non_ass_var = independent_of_stmt_p (op0, stmt, call)))
    ;
  else
    return false;

  switch (code)
    {
    case PLUS_EXPR:
      /* There should be no previous addition.  TODO -- it should be fairly
	 straightforward to lift this restriction -- just allow storing
	 more complicated expressions in *A, and gimplify it in
	 adjust_accumulator_values.  */
      if (*a)
	return false;
      *a = non_ass_var;
      *ass_var = dest;
      return true;

    case MULT_EXPR:
      /* Similar remark applies here.  Handling multiplication after addition
	 is just slightly more complicated -- we need to multiply both *A and
	 *M.  */
      if (*a || *m)
	return false;
      *m = non_ass_var;
      *ass_var = dest;
      return true;

      /* TODO -- Handle other codes (NEGATE_EXPR, MINUS_EXPR).  */

    default:
      return false;
    }
}

/* Propagate VAR through phis on edge E.  */

static tree
propagate_through_phis (tree var, edge e)
{
  basic_block dest = e->dest;
  tree phi;

  for (phi = phi_nodes (dest); phi; phi = PHI_CHAIN (phi))
    if (PHI_ARG_DEF_FROM_EDGE (phi, e) == var)
      return PHI_RESULT (phi);

  return var;
}

/* Finds tailcalls falling into basic block BB. The list of found tailcalls is
   added to the start of RET.  */

static void
find_tail_calls (basic_block bb, struct tailcall **ret)
{
  tree ass_var, ret_var, stmt, func, param, args, call = NULL_TREE;
  block_stmt_iterator bsi, absi;
  bool tail_recursion;
  struct tailcall *nw;
  edge e;
  tree m, a;
  basic_block abb;
  stmt_ann_t ann;

  if (EDGE_COUNT (bb->succs) > 1)
    return;

  for (bsi = bsi_last (bb); !bsi_end_p (bsi); bsi_prev (&bsi))
    {
      stmt = bsi_stmt (bsi);

      /* Ignore labels.  */
      if (TREE_CODE (stmt) == LABEL_EXPR)
	continue;

      get_stmt_operands (stmt);

      /* Check for a call.  */
      if (TREE_CODE (stmt) == MODIFY_EXPR)
	{
	  ass_var = TREE_OPERAND (stmt, 0);
	  call = TREE_OPERAND (stmt, 1);
	  if (TREE_CODE (call) == WITH_SIZE_EXPR)
	    call = TREE_OPERAND (call, 0);
	}
      else
	{
	  ass_var = NULL_TREE;
	  call = stmt;
	}

      if (TREE_CODE (call) == CALL_EXPR)
	break;

      /* If the statement has virtual or volatile operands, fail.  */
      ann = stmt_ann (stmt);
      if (NUM_V_MAY_DEFS (V_MAY_DEF_OPS (ann))
          || NUM_V_MUST_DEFS (V_MUST_DEF_OPS (ann))
	  || NUM_VUSES (VUSE_OPS (ann))
	  || ann->has_volatile_ops)
	return;
    }

  if (bsi_end_p (bsi))
    {
      edge_iterator ei;
      /* Recurse to the predecessors.  */
      FOR_EACH_EDGE (e, ei, bb->preds)
	find_tail_calls (e->src, ret);

      return;
    }

  /* We found the call, check whether it is suitable.  */
  tail_recursion = false;
  func = get_callee_fndecl (call);
  if (func == current_function_decl)
    {
      for (param = DECL_ARGUMENTS (func), args = TREE_OPERAND (call, 1);
	   param && args;
	   param = TREE_CHAIN (param), args = TREE_CHAIN (args))
	{
	  tree arg = TREE_VALUE (args);
	  if (param != arg
	      /* Make sure there are no problems with copying.  Note we must
	         have a copyable type and the two arguments must have reasonably
	         equivalent types.  The latter requirement could be relaxed if
	         we emitted a suitable type conversion statement.  */
	      && (!is_gimple_reg_type (TREE_TYPE (param))
		  || !lang_hooks.types_compatible_p (TREE_TYPE (param),
						     TREE_TYPE (arg))))
	    break;
	}
      if (!args && !param)
	tail_recursion = true;
    }

  /* Now check the statements after the call.  None of them has virtual
     operands, so they may only depend on the call through its return
     value.  The return value should also be dependent on each of them,
     since we are running after dce.  */
  m = NULL_TREE;
  a = NULL_TREE;

  abb = bb;
  absi = bsi;
  while (1)
    {
      bsi_next (&absi);

      while (bsi_end_p (absi))
	{
	  ass_var = propagate_through_phis (ass_var, EDGE_SUCC (abb, 0));
	  abb = EDGE_SUCC (abb, 0)->dest;
	  absi = bsi_start (abb);
	}

      stmt = bsi_stmt (absi);

      if (TREE_CODE (stmt) == LABEL_EXPR)
	continue;

      if (TREE_CODE (stmt) == RETURN_EXPR)
	break;

      if (TREE_CODE (stmt) != MODIFY_EXPR)
	return;

      if (!process_assignment (stmt, stmt, bsi, &m, &a, &ass_var))
	return;
    }

  /* See if this is a tail call we can handle.  */
  ret_var = TREE_OPERAND (stmt, 0);
  if (ret_var
      && TREE_CODE (ret_var) == MODIFY_EXPR)
    {
      tree ret_op = TREE_OPERAND (ret_var, 1);
      STRIP_NOPS (ret_op);
      if (!tail_recursion
	  && TREE_CODE (ret_op) != SSA_NAME)
	return;

      if (!process_assignment (ret_var, stmt, bsi, &m, &a, &ass_var))
	return;
      ret_var = TREE_OPERAND (ret_var, 0);
    }

  /* We may proceed if there either is no return value, or the return value
     is identical to the call's return.  */
  if (ret_var
      && (ret_var != ass_var))
    return;

  /* If this is not a tail recursive call, we cannot handle addends or
     multiplicands.  */
  if (!tail_recursion && (m || a))
    return;

  nw = xmalloc (sizeof (struct tailcall));

  nw->call_block = bb;
  nw->call_bsi = bsi;

  nw->tail_recursion = tail_recursion;

  nw->mult = m;
  nw->add = a;

  nw->next = *ret;
  *ret = nw;
}

/* Adjust the accumulator values according to A and M after BSI, and update
   the phi nodes on edge BACK.  */

static void
adjust_accumulator_values (block_stmt_iterator bsi, tree m, tree a, edge back)
{
  tree stmt, var, phi, tmp;
  tree ret_type = TREE_TYPE (DECL_RESULT (current_function_decl));
  tree a_acc_arg = a_acc, m_acc_arg = m_acc;

  if (a)
    {
      if (m_acc)
	{
	  if (integer_onep (a))
	    var = m_acc;
	  else
	    {
	      stmt = build (MODIFY_EXPR, ret_type, NULL_TREE,
			    build (MULT_EXPR, ret_type, m_acc, a));

	      tmp = create_tmp_var (ret_type, "acc_tmp");
	      add_referenced_tmp_var (tmp);

	      var = make_ssa_name (tmp, stmt);
	      TREE_OPERAND (stmt, 0) = var;
	      bsi_insert_after (&bsi, stmt, BSI_NEW_STMT);
	    }
	}
      else
	var = a;

      stmt = build (MODIFY_EXPR, ret_type, NULL_TREE,
		    build (PLUS_EXPR, ret_type, a_acc, var));
      var = make_ssa_name (SSA_NAME_VAR (a_acc), stmt);
      TREE_OPERAND (stmt, 0) = var;
      bsi_insert_after (&bsi, stmt, BSI_NEW_STMT);
      a_acc_arg = var;
    }

  if (m)
    {
      stmt = build (MODIFY_EXPR, ret_type, NULL_TREE,
		    build (MULT_EXPR, ret_type, m_acc, m));
      var = make_ssa_name (SSA_NAME_VAR (m_acc), stmt);
      TREE_OPERAND (stmt, 0) = var;
      bsi_insert_after (&bsi, stmt, BSI_NEW_STMT);
      m_acc_arg = var;
    }

  if (a_acc)
    {
      for (phi = phi_nodes (back->dest); phi; phi = PHI_CHAIN (phi))
	if (PHI_RESULT (phi) == a_acc)
	  break;

      add_phi_arg (&phi, a_acc_arg, back);
    }

  if (m_acc)
    {
      for (phi = phi_nodes (back->dest); phi; phi = PHI_CHAIN (phi))
	if (PHI_RESULT (phi) == m_acc)
	  break;

      add_phi_arg (&phi, m_acc_arg, back);
    }
}

/* Adjust value of the return at the end of BB according to M and A
   accumulators.  */

static void
adjust_return_value (basic_block bb, tree m, tree a)
{
  tree ret_stmt = last_stmt (bb), ret_var, var, stmt, tmp;
  tree ret_type = TREE_TYPE (DECL_RESULT (current_function_decl));
  block_stmt_iterator bsi = bsi_last (bb);

  gcc_assert (TREE_CODE (ret_stmt) == RETURN_EXPR);

  ret_var = TREE_OPERAND (ret_stmt, 0);
  if (!ret_var)
    return;

  if (TREE_CODE (ret_var) == MODIFY_EXPR)
    {
      ret_var->common.ann = (tree_ann_t) stmt_ann (ret_stmt);
      bsi_replace (&bsi, ret_var, true);
      SSA_NAME_DEF_STMT (TREE_OPERAND (ret_var, 0)) = ret_var;
      ret_var = TREE_OPERAND (ret_var, 0);
      ret_stmt = build1 (RETURN_EXPR, TREE_TYPE (ret_stmt), ret_var);
      bsi_insert_after (&bsi, ret_stmt, BSI_NEW_STMT);
    }

  if (m)
    {
      stmt = build (MODIFY_EXPR, ret_type, NULL_TREE,
		    build (MULT_EXPR, ret_type, m_acc, ret_var));

      tmp = create_tmp_var (ret_type, "acc_tmp");
      add_referenced_tmp_var (tmp);

      var = make_ssa_name (tmp, stmt);
      TREE_OPERAND (stmt, 0) = var;
      bsi_insert_before (&bsi, stmt, BSI_SAME_STMT);
    }
  else
    var = ret_var;

  if (a)
    {
      stmt = build (MODIFY_EXPR, ret_type, NULL_TREE,
		    build (PLUS_EXPR, ret_type, a_acc, var));

      tmp = create_tmp_var (ret_type, "acc_tmp");
      add_referenced_tmp_var (tmp);

      var = make_ssa_name (tmp, stmt);
      TREE_OPERAND (stmt, 0) = var;
      bsi_insert_before (&bsi, stmt, BSI_SAME_STMT);
    }

  TREE_OPERAND (ret_stmt, 0) = var;
  modify_stmt (ret_stmt);
}

/* Eliminates tail call described by T.  TMP_VARS is a list of
   temporary variables used to copy the function arguments.  */

static void
eliminate_tail_call (struct tailcall *t)
{
  tree param, stmt, args, rslt, call;
  basic_block bb, first;
  edge e;
  tree phi;
  stmt_ann_t ann;
  v_may_def_optype v_may_defs;
  unsigned i;
  block_stmt_iterator bsi;

  stmt = bsi_stmt (t->call_bsi);
  get_stmt_operands (stmt);
  ann = stmt_ann (stmt);
  bb = t->call_block;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Eliminated tail recursion in bb %d : ",
	       bb->index);
      print_generic_stmt (dump_file, stmt, TDF_SLIM);
      fprintf (dump_file, "\n");
    }

  if (TREE_CODE (stmt) == MODIFY_EXPR)
    stmt = TREE_OPERAND (stmt, 1);

  first = EDGE_SUCC (ENTRY_BLOCK_PTR, 0)->dest;

  /* Remove the code after call_bsi that will become unreachable.  The
     possibly unreachable code in other blocks is removed later in
     cfg cleanup.  */
  bsi = t->call_bsi;
  bsi_next (&bsi);
  while (!bsi_end_p (bsi))
    {
      tree t = bsi_stmt (bsi);
      /* Do not remove the return statement, so that redirect_edge_and_branch
	 sees how the block ends.  */
      if (TREE_CODE (t) == RETURN_EXPR)
	break;

      bsi_remove (&bsi);
      release_defs (t);
    }

  /* Replace the call by a jump to the start of function.  */
  e = redirect_edge_and_branch (EDGE_SUCC (t->call_block, 0), first);
  gcc_assert (e);
  PENDING_STMT (e) = NULL_TREE;

  /* Add phi node entries for arguments.  Not every PHI node corresponds to
     a function argument (there may be PHI nodes for virtual definitions of the
     eliminated calls), so we search for a PHI corresponding to each argument
     rather than searching for which argument a PHI node corresponds to.  */
  
  for (param = DECL_ARGUMENTS (current_function_decl),
       args = TREE_OPERAND (stmt, 1);
       param;
       param = TREE_CHAIN (param),
       args = TREE_CHAIN (args))
    {
      
      for (phi = phi_nodes (first); phi; phi = PHI_CHAIN (phi))
	if (param == SSA_NAME_VAR (PHI_RESULT (phi)))
	  break;

      /* The phi node indeed does not have to be there, in case the operand is
	 invariant in the function.  */
      if (!phi)
	continue;

      add_phi_arg (&phi, TREE_VALUE (args), e);
    }

  /* Add phi nodes for the call clobbered variables.  */
  v_may_defs = V_MAY_DEF_OPS (ann);
  for (i = 0; i < NUM_V_MAY_DEFS (v_may_defs); i++)
    {
      param = SSA_NAME_VAR (V_MAY_DEF_RESULT (v_may_defs, i));
      for (phi = phi_nodes (first); phi; phi = PHI_CHAIN (phi))
	if (param == SSA_NAME_VAR (PHI_RESULT (phi)))
	  break;

      if (!phi)
	{
	  tree name = var_ann (param)->default_def;
	  tree new_name;

	  if (!name)
	    {
	      /* It may happen that the tag does not have a default_def in case
		 when all uses of it are dominated by a MUST_DEF.  This however
		 means that it is not necessary to add a phi node for this
		 tag.  */
	      continue;
	    }
	  new_name = make_ssa_name (param, SSA_NAME_DEF_STMT (name));

	  var_ann (param)->default_def = new_name;
	  phi = create_phi_node (name, first);
	  SSA_NAME_DEF_STMT (name) = phi;
	  add_phi_arg (&phi, new_name, EDGE_SUCC (ENTRY_BLOCK_PTR, 0));

	  /* For all calls the same set of variables should be clobbered.  This
	     means that there always should be the appropriate phi node except
	     for the first time we eliminate the call.  */
	  gcc_assert (EDGE_COUNT (first->preds) <= 2);
	}

      add_phi_arg (&phi, V_MAY_DEF_OP (v_may_defs, i), e);
    }

  /* Update the values of accumulators.  */
  adjust_accumulator_values (t->call_bsi, t->mult, t->add, e);

  call = bsi_stmt (t->call_bsi);
  if (TREE_CODE (call) == MODIFY_EXPR)
    {
      rslt = TREE_OPERAND (call, 0);

      /* Result of the call will no longer be defined.  So adjust the
	 SSA_NAME_DEF_STMT accordingly.  */
      SSA_NAME_DEF_STMT (rslt) = build_empty_stmt ();
    }

  bsi_remove (&t->call_bsi);
  release_defs (call);
}

/* Optimizes the tailcall described by T.  If OPT_TAILCALLS is true, also
   mark the tailcalls for the sibcall optimization.  */

static bool
optimize_tail_call (struct tailcall *t, bool opt_tailcalls)
{
  if (t->tail_recursion)
    {
      eliminate_tail_call (t);
      return true;
    }

  if (opt_tailcalls)
    {
      tree stmt = bsi_stmt (t->call_bsi);

      stmt = get_call_expr_in (stmt);
      CALL_EXPR_TAILCALL (stmt) = 1;
      if (dump_file && (dump_flags & TDF_DETAILS))
        {
	  fprintf (dump_file, "Found tail call ");
	  print_generic_expr (dump_file, stmt, dump_flags);
	  fprintf (dump_file, " in bb %i\n", t->call_block->index);
	}
    }

  return false;
}

/* Optimizes tail calls in the function, turning the tail recursion
   into iteration.  */

static void
tree_optimize_tail_calls_1 (bool opt_tailcalls)
{
  edge e;
  bool phis_constructed = false;
  struct tailcall *tailcalls = NULL, *act, *next;
  bool changed = false;
  basic_block first = EDGE_SUCC (ENTRY_BLOCK_PTR, 0)->dest;
  tree stmt, param, ret_type, tmp, phi;
  edge_iterator ei;

  if (!suitable_for_tail_opt_p ())
    return;
  if (opt_tailcalls)
    opt_tailcalls = suitable_for_tail_call_opt_p ();

  FOR_EACH_EDGE (e, ei, EXIT_BLOCK_PTR->preds)
    {
      /* Only traverse the normal exits, i.e. those that end with return
	 statement.  */
      stmt = last_stmt (e->src);

      if (stmt
	  && TREE_CODE (stmt) == RETURN_EXPR)
	find_tail_calls (e->src, &tailcalls);
    }

  /* Construct the phi nodes and accumulators if necessary.  */
  a_acc = m_acc = NULL_TREE;
  for (act = tailcalls; act; act = act->next)
    {
      if (!act->tail_recursion)
	continue;

      if (!phis_constructed)
	{
	  /* Ensure that there is only one predecessor of the block.  */
	  if (EDGE_COUNT (first->preds) > 1)
	    first = split_edge (EDGE_SUCC (ENTRY_BLOCK_PTR, 0));

	  /* Copy the args if needed.  */
	  for (param = DECL_ARGUMENTS (current_function_decl);
	       param;
	       param = TREE_CHAIN (param))
	    if (var_ann (param)
		/* Also parameters that are only defined but never used need not
		   be copied.  */
		&& (var_ann (param)->default_def
		    && TREE_CODE (var_ann (param)->default_def) == SSA_NAME))
	    {
	      tree name = var_ann (param)->default_def;
	      tree new_name = make_ssa_name (param, SSA_NAME_DEF_STMT (name));
	      tree phi;

	      var_ann (param)->default_def = new_name;
	      phi = create_phi_node (name, first);
	      SSA_NAME_DEF_STMT (name) = phi;
	      add_phi_arg (&phi, new_name, EDGE_PRED (first, 0));
	    }
	  phis_constructed = true;
	}

      if (act->add && !a_acc)
	{
	  ret_type = TREE_TYPE (DECL_RESULT (current_function_decl));

	  tmp = create_tmp_var (ret_type, "add_acc");
	  add_referenced_tmp_var (tmp);

	  phi = create_phi_node (tmp, first);
	  add_phi_arg (&phi, build_int_cst (ret_type, 0), EDGE_PRED (first, 0));
	  a_acc = PHI_RESULT (phi);
	}

      if (act->mult && !m_acc)
	{
	  ret_type = TREE_TYPE (DECL_RESULT (current_function_decl));

	  tmp = create_tmp_var (ret_type, "mult_acc");
	  add_referenced_tmp_var (tmp);

	  phi = create_phi_node (tmp, first);
	  add_phi_arg (&phi, build_int_cst (ret_type, 1), EDGE_PRED (first, 0));
	  m_acc = PHI_RESULT (phi);
	}
    }

  for (; tailcalls; tailcalls = next)
    {
      next = tailcalls->next;
      changed |= optimize_tail_call (tailcalls, opt_tailcalls);
      free (tailcalls);
    }

  if (a_acc || m_acc)
    {
      /* Modify the remaining return statements.  */
      FOR_EACH_EDGE (e, ei, EXIT_BLOCK_PTR->preds)
	{
	  stmt = last_stmt (e->src);

	  if (stmt
	      && TREE_CODE (stmt) == RETURN_EXPR)
	    adjust_return_value (e->src, m_acc, a_acc);
	}
    }

  if (changed)
    {
      free_dominance_info (CDI_DOMINATORS);
      cleanup_tree_cfg ();
    }
}

static void
execute_tail_recursion (void)
{
  tree_optimize_tail_calls_1 (false);
}

static bool
gate_tail_calls (void)
{
  return flag_optimize_sibling_calls != 0;
}

static void
execute_tail_calls (void)
{
  tree_optimize_tail_calls_1 (true);
}

struct tree_opt_pass pass_tail_recursion = 
{
  "tailr",				/* name */
  NULL,					/* gate */
  execute_tail_recursion,		/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  PROP_cfg | PROP_ssa | PROP_alias,	/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func | TODO_verify_ssa,	/* todo_flags_finish */
  0					/* letter */
};

struct tree_opt_pass pass_tail_calls = 
{
  "tailc",				/* name */
  gate_tail_calls,			/* gate */
  execute_tail_calls,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  PROP_cfg | PROP_ssa | PROP_alias,	/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func | TODO_verify_ssa,	/* todo_flags_finish */
  0					/* letter */
};
