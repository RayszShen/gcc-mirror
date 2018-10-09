/* Preamble and helpers for the autogenerated gimple-match.c file.
   Copyright (C) 2014-2018 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "ssa.h"
#include "cgraph.h"
#include "fold-const.h"
#include "fold-const-call.h"
#include "stor-layout.h"
#include "gimple-fold.h"
#include "calls.h"
#include "tree-dfa.h"
#include "builtins.h"
#include "gimple-match.h"
#include "tree-pass.h"
#include "internal-fn.h"
#include "case-cfn-macros.h"
#include "gimplify.h"
#include "optabs-tree.h"
#include "tree-eh.h"


/* Forward declarations of the private auto-generated matchers.
   They expect valueized operands in canonical order and do not
   perform simplification of all-constant operands.  */
static bool gimple_simplify (gimple_match_op *, gimple_seq *, tree (*)(tree),
			     code_helper, tree, tree);
static bool gimple_simplify (gimple_match_op *, gimple_seq *, tree (*)(tree),
			     code_helper, tree, tree, tree);
static bool gimple_simplify (gimple_match_op *, gimple_seq *, tree (*)(tree),
			     code_helper, tree, tree, tree, tree);
static bool gimple_simplify (gimple_match_op *, gimple_seq *, tree (*)(tree),
			     code_helper, tree, tree, tree, tree, tree);
static bool gimple_simplify (gimple_match_op *, gimple_seq *, tree (*)(tree),
			     code_helper, tree, tree, tree, tree, tree, tree);

const unsigned int gimple_match_op::MAX_NUM_OPS;

/* Return whether T is a constant that we'll dispatch to fold to
   evaluate fully constant expressions.  */

static inline bool
constant_for_folding (tree t)
{
  return (CONSTANT_CLASS_P (t)
	  /* The following is only interesting to string builtins.  */
	  || (TREE_CODE (t) == ADDR_EXPR
	      && TREE_CODE (TREE_OPERAND (t, 0)) == STRING_CST));
}

/* Try to convert conditional operation ORIG_OP into an IFN_COND_*
   operation.  Return true on success, storing the new operation in NEW_OP.  */

static bool
convert_conditional_op (gimple_match_op *orig_op,
			gimple_match_op *new_op)
{
  internal_fn ifn;
  if (orig_op->code.is_tree_code ())
    ifn = get_conditional_internal_fn ((tree_code) orig_op->code);
  else
    {
      combined_fn cfn = orig_op->code;
      if (!internal_fn_p (cfn))
	return false;
      ifn = get_conditional_internal_fn (as_internal_fn (cfn));
    }
  if (ifn == IFN_LAST)
    return false;
  unsigned int num_ops = orig_op->num_ops;
  new_op->set_op (as_combined_fn (ifn), orig_op->type, num_ops + 2);
  new_op->ops[0] = orig_op->cond.cond;
  for (unsigned int i = 0; i < num_ops; ++i)
    new_op->ops[i + 1] = orig_op->ops[i];
  tree else_value = orig_op->cond.else_value;
  if (!else_value)
    else_value = targetm.preferred_else_value (ifn, orig_op->type,
					       num_ops, orig_op->ops);
  new_op->ops[num_ops + 1] = else_value;
  return true;
}

/* RES_OP is the result of a simplification.  If it is conditional,
   try to replace it with the equivalent UNCOND form, such as an
   IFN_COND_* call or a VEC_COND_EXPR.  Also try to resimplify the
   result of the replacement if appropriate, adding any new statements to
   SEQ and using VALUEIZE as the valueization function.  Return true if
   this resimplification occurred and resulted in at least one change.  */

static bool
maybe_resimplify_conditional_op (gimple_seq *seq, gimple_match_op *res_op,
				 tree (*valueize) (tree))
{
  if (!res_op->cond.cond)
    return false;

  if (!res_op->cond.else_value
      && res_op->code.is_tree_code ())
    {
      /* The "else" value doesn't matter.  If the "then" value is a
	 gimple value, just use it unconditionally.  This isn't a
	 simplification in itself, since there was no operation to
	 build in the first place.  */
      if (gimple_simplified_result_is_gimple_val (res_op))
	{
	  res_op->cond.cond = NULL_TREE;
	  return false;
	}

      /* Likewise if the operation would not trap.  */
      bool honor_trapv = (INTEGRAL_TYPE_P (res_op->type)
			  && TYPE_OVERFLOW_TRAPS (res_op->type));
      if (!operation_could_trap_p ((tree_code) res_op->code,
				   FLOAT_TYPE_P (res_op->type),
				   honor_trapv, res_op->op_or_null (1)))
	{
	  res_op->cond.cond = NULL_TREE;
	  return false;
	}
    }

  /* If the "then" value is a gimple value and the "else" value matters,
     create a VEC_COND_EXPR between them, then see if it can be further
     simplified.  */
  gimple_match_op new_op;
  if (res_op->cond.else_value
      && VECTOR_TYPE_P (res_op->type)
      && gimple_simplified_result_is_gimple_val (res_op))
    {
      new_op.set_op (VEC_COND_EXPR, res_op->type,
		     res_op->cond.cond, res_op->ops[0],
		     res_op->cond.else_value);
      *res_op = new_op;
      return gimple_resimplify3 (seq, res_op, valueize);
    }

  /* Otherwise try rewriting the operation as an IFN_COND_* call.
     Again, this isn't a simplification in itself, since it's what
     RES_OP already described.  */
  if (convert_conditional_op (res_op, &new_op))
    *res_op = new_op;

  return false;
}

/* Helper that matches and simplifies the toplevel result from
   a gimple_simplify run (where we don't want to build
   a stmt in case it's used in in-place folding).  Replaces
   RES_OP with a simplified and/or canonicalized result and
   returns whether any change was made.  */

bool
gimple_resimplify1 (gimple_seq *seq, gimple_match_op *res_op,
		    tree (*valueize)(tree))
{
  if (constant_for_folding (res_op->ops[0]))
    {
      tree tem = NULL_TREE;
      if (res_op->code.is_tree_code ())
	tem = const_unop (res_op->code, res_op->type, res_op->ops[0]);
      else
	tem = fold_const_call (combined_fn (res_op->code), res_op->type,
			       res_op->ops[0]);
      if (tem != NULL_TREE
	  && CONSTANT_CLASS_P (tem))
	{
	  if (TREE_OVERFLOW_P (tem))
	    tem = drop_tree_overflow (tem);
	  res_op->set_value (tem);
	  maybe_resimplify_conditional_op (seq, res_op, valueize);
	  return true;
	}
    }

  /* Limit recursion, there are cases like PR80887 and others, for
     example when value-numbering presents us with unfolded expressions
     that we are really not prepared to handle without eventual
     oscillation like ((_50 + 0) + 8) where _50 gets mapped to _50
     itself as available expression.  */
  static unsigned depth;
  if (depth > 10)
    {
      if (dump_file && (dump_flags & TDF_FOLDING))
	fprintf (dump_file, "Aborting expression simplification due to "
		 "deep recursion\n");
      return false;
    }

  ++depth;
  gimple_match_op res_op2 (*res_op);
  if (gimple_simplify (&res_op2, seq, valueize,
		       res_op->code, res_op->type, res_op->ops[0]))
    {
      --depth;
      *res_op = res_op2;
      return true;
    }
  --depth;

  if (maybe_resimplify_conditional_op (seq, res_op, valueize))
    return true;

  return false;
}

/* Helper that matches and simplifies the toplevel result from
   a gimple_simplify run (where we don't want to build
   a stmt in case it's used in in-place folding).  Replaces
   RES_OP with a simplified and/or canonicalized result and
   returns whether any change was made.  */

bool
gimple_resimplify2 (gimple_seq *seq, gimple_match_op *res_op,
		    tree (*valueize)(tree))
{
  if (constant_for_folding (res_op->ops[0])
      && constant_for_folding (res_op->ops[1]))
    {
      tree tem = NULL_TREE;
      if (res_op->code.is_tree_code ())
	tem = const_binop (res_op->code, res_op->type,
			   res_op->ops[0], res_op->ops[1]);
      else
	tem = fold_const_call (combined_fn (res_op->code), res_op->type,
			       res_op->ops[0], res_op->ops[1]);
      if (tem != NULL_TREE
	  && CONSTANT_CLASS_P (tem))
	{
	  if (TREE_OVERFLOW_P (tem))
	    tem = drop_tree_overflow (tem);
	  res_op->set_value (tem);
	  maybe_resimplify_conditional_op (seq, res_op, valueize);
	  return true;
	}
    }

  /* Canonicalize operand order.  */
  bool canonicalized = false;
  if (res_op->code.is_tree_code ()
      && (TREE_CODE_CLASS ((enum tree_code) res_op->code) == tcc_comparison
	  || commutative_tree_code (res_op->code))
      && tree_swap_operands_p (res_op->ops[0], res_op->ops[1]))
    {
      std::swap (res_op->ops[0], res_op->ops[1]);
      if (TREE_CODE_CLASS ((enum tree_code) res_op->code) == tcc_comparison)
	res_op->code = swap_tree_comparison (res_op->code);
      canonicalized = true;
    }

  /* Limit recursion, see gimple_resimplify1.  */
  static unsigned depth;
  if (depth > 10)
    {
      if (dump_file && (dump_flags & TDF_FOLDING))
	fprintf (dump_file, "Aborting expression simplification due to "
		 "deep recursion\n");
      return false;
    }

  ++depth;
  gimple_match_op res_op2 (*res_op);
  if (gimple_simplify (&res_op2, seq, valueize,
		       res_op->code, res_op->type,
		       res_op->ops[0], res_op->ops[1]))
    {
      --depth;
      *res_op = res_op2;
      return true;
    }
  --depth;

  if (maybe_resimplify_conditional_op (seq, res_op, valueize))
    return true;

  return canonicalized;
}

/* Helper that matches and simplifies the toplevel result from
   a gimple_simplify run (where we don't want to build
   a stmt in case it's used in in-place folding).  Replaces
   RES_OP with a simplified and/or canonicalized result and
   returns whether any change was made.  */

bool
gimple_resimplify3 (gimple_seq *seq, gimple_match_op *res_op,
		    tree (*valueize)(tree))
{
  if (constant_for_folding (res_op->ops[0])
      && constant_for_folding (res_op->ops[1])
      && constant_for_folding (res_op->ops[2]))
    {
      tree tem = NULL_TREE;
      if (res_op->code.is_tree_code ())
	tem = fold_ternary/*_to_constant*/ (res_op->code, res_op->type,
					    res_op->ops[0], res_op->ops[1],
					    res_op->ops[2]);
      else
	tem = fold_const_call (combined_fn (res_op->code), res_op->type,
			       res_op->ops[0], res_op->ops[1], res_op->ops[2]);
      if (tem != NULL_TREE
	  && CONSTANT_CLASS_P (tem))
	{
	  if (TREE_OVERFLOW_P (tem))
	    tem = drop_tree_overflow (tem);
	  res_op->set_value (tem);
	  maybe_resimplify_conditional_op (seq, res_op, valueize);
	  return true;
	}
    }

  /* Canonicalize operand order.  */
  bool canonicalized = false;
  if (res_op->code.is_tree_code ()
      && commutative_ternary_tree_code (res_op->code)
      && tree_swap_operands_p (res_op->ops[0], res_op->ops[1]))
    {
      std::swap (res_op->ops[0], res_op->ops[1]);
      canonicalized = true;
    }

  /* Limit recursion, see gimple_resimplify1.  */
  static unsigned depth;
  if (depth > 10)
    {
      if (dump_file && (dump_flags & TDF_FOLDING))
	fprintf (dump_file, "Aborting expression simplification due to "
		 "deep recursion\n");
      return false;
    }

  ++depth;
  gimple_match_op res_op2 (*res_op);
  if (gimple_simplify (&res_op2, seq, valueize,
		       res_op->code, res_op->type,
		       res_op->ops[0], res_op->ops[1], res_op->ops[2]))
    {
      --depth;
      *res_op = res_op2;
      return true;
    }
  --depth;

  if (maybe_resimplify_conditional_op (seq, res_op, valueize))
    return true;

  return canonicalized;
}

/* Helper that matches and simplifies the toplevel result from
   a gimple_simplify run (where we don't want to build
   a stmt in case it's used in in-place folding).  Replaces
   RES_OP with a simplified and/or canonicalized result and
   returns whether any change was made.  */

bool
gimple_resimplify4 (gimple_seq *seq, gimple_match_op *res_op,
		    tree (*valueize)(tree))
{
  /* No constant folding is defined for four-operand functions.  */

  /* Limit recursion, see gimple_resimplify1.  */
  static unsigned depth;
  if (depth > 10)
    {
      if (dump_file && (dump_flags & TDF_FOLDING))
	fprintf (dump_file, "Aborting expression simplification due to "
		 "deep recursion\n");
      return false;
    }

  ++depth;
  gimple_match_op res_op2 (*res_op);
  if (gimple_simplify (&res_op2, seq, valueize,
		       res_op->code, res_op->type,
		       res_op->ops[0], res_op->ops[1], res_op->ops[2],
		       res_op->ops[3]))
    {
      --depth;
      *res_op = res_op2;
      return true;
    }
  --depth;

  if (maybe_resimplify_conditional_op (seq, res_op, valueize))
    return true;

  return false;
}

/* Helper that matches and simplifies the toplevel result from
   a gimple_simplify run (where we don't want to build
   a stmt in case it's used in in-place folding).  Replaces
   RES_OP with a simplified and/or canonicalized result and
   returns whether any change was made.  */

bool
gimple_resimplify5 (gimple_seq *seq, gimple_match_op *res_op,
		    tree (*valueize)(tree))
{
  /* No constant folding is defined for five-operand functions.  */

  gimple_match_op res_op2 (*res_op);
  if (gimple_simplify (&res_op2, seq, valueize,
		       res_op->code, res_op->type,
		       res_op->ops[0], res_op->ops[1], res_op->ops[2],
		       res_op->ops[3], res_op->ops[4]))
    {
      *res_op = res_op2;
      return true;
    }

  if (maybe_resimplify_conditional_op (seq, res_op, valueize))
    return true;

  return false;
}

/* If in GIMPLE the operation described by RES_OP should be single-rhs,
   build a GENERIC tree for that expression and update RES_OP accordingly.  */

void
maybe_build_generic_op (gimple_match_op *res_op)
{
  tree_code code = (tree_code) res_op->code;
  tree val;
  switch (code)
    {
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case VIEW_CONVERT_EXPR:
      val = build1 (code, res_op->type, res_op->ops[0]);
      res_op->set_value (val);
      break;
    case BIT_FIELD_REF:
      val = build3 (code, res_op->type, res_op->ops[0], res_op->ops[1],
		    res_op->ops[2]);
      REF_REVERSE_STORAGE_ORDER (val) = res_op->reverse;
      res_op->set_value (val);
      break;
    default:;
    }
}

tree (*mprts_hook) (gimple_match_op *);

/* Try to build RES_OP, which is known to be a call to FN.  Return null
   if the target doesn't support the function.  */

static gcall *
build_call_internal (internal_fn fn, gimple_match_op *res_op)
{
  if (direct_internal_fn_p (fn))
    {
      tree_pair types = direct_internal_fn_types (fn, res_op->type,
						  res_op->ops);
      if (!direct_internal_fn_supported_p (fn, types, OPTIMIZE_FOR_BOTH))
	return NULL;
    }
  return gimple_build_call_internal (fn, res_op->num_ops,
				     res_op->op_or_null (0),
				     res_op->op_or_null (1),
				     res_op->op_or_null (2),
				     res_op->op_or_null (3),
				     res_op->op_or_null (4));
}

/* Push the exploded expression described by RES_OP as a statement to
   SEQ if necessary and return a gimple value denoting the value of the
   expression.  If RES is not NULL then the result will be always RES
   and even gimple values are pushed to SEQ.  */

tree
maybe_push_res_to_seq (gimple_match_op *res_op, gimple_seq *seq, tree res)
{
  tree *ops = res_op->ops;
  unsigned num_ops = res_op->num_ops;

  /* The caller should have converted conditional operations into an UNCOND
     form and resimplified as appropriate.  The conditional form only
     survives this far if that conversion failed.  */
  if (res_op->cond.cond)
    return NULL_TREE;

  if (res_op->code.is_tree_code ())
    {
      if (!res
	  && gimple_simplified_result_is_gimple_val (res_op))
	return ops[0];
      if (mprts_hook)
	{
	  tree tem = mprts_hook (res_op);
	  if (tem)
	    return tem;
	}
    }

  if (!seq)
    return NULL_TREE;

  /* Play safe and do not allow abnormals to be mentioned in
     newly created statements.  */
  for (unsigned int i = 0; i < num_ops; ++i)
    if (TREE_CODE (ops[i]) == SSA_NAME
	&& SSA_NAME_OCCURS_IN_ABNORMAL_PHI (ops[i]))
      return NULL_TREE;

  if (num_ops > 0 && COMPARISON_CLASS_P (ops[0]))
    for (unsigned int i = 0; i < 2; ++i)
      if (TREE_CODE (TREE_OPERAND (ops[0], i)) == SSA_NAME
	  && SSA_NAME_OCCURS_IN_ABNORMAL_PHI (TREE_OPERAND (ops[0], i)))
	return NULL_TREE;

  if (res_op->code.is_tree_code ())
    {
      if (!res)
	{
	  if (gimple_in_ssa_p (cfun))
	    res = make_ssa_name (res_op->type);
	  else
	    res = create_tmp_reg (res_op->type);
	}
      maybe_build_generic_op (res_op);
      gimple *new_stmt = gimple_build_assign (res, res_op->code,
					      res_op->op_or_null (0),
					      res_op->op_or_null (1),
					      res_op->op_or_null (2));
      gimple_seq_add_stmt_without_update (seq, new_stmt);
      return res;
    }
  else
    {
      gcc_assert (num_ops != 0);
      combined_fn fn = res_op->code;
      gcall *new_stmt = NULL;
      if (internal_fn_p (fn))
	{
	  /* Generate the given function if we can.  */
	  internal_fn ifn = as_internal_fn (fn);
	  new_stmt = build_call_internal (ifn, res_op);
	  if (!new_stmt)
	    return NULL_TREE;
	}
      else
	{
	  /* Find the function we want to call.  */
	  tree decl = builtin_decl_implicit (as_builtin_fn (fn));
	  if (!decl)
	    return NULL;

	  /* We can't and should not emit calls to non-const functions.  */
	  if (!(flags_from_decl_or_type (decl) & ECF_CONST))
	    return NULL;

	  new_stmt = gimple_build_call (decl, num_ops,
					res_op->op_or_null (0),
					res_op->op_or_null (1),
					res_op->op_or_null (2),
					res_op->op_or_null (3),
					res_op->op_or_null (4));
	}
      if (!res)
	{
	  if (gimple_in_ssa_p (cfun))
	    res = make_ssa_name (res_op->type);
	  else
	    res = create_tmp_reg (res_op->type);
	}
      gimple_call_set_lhs (new_stmt, res);
      gimple_seq_add_stmt_without_update (seq, new_stmt);
      return res;
    }
}


/* Public API overloads follow for operation being tree_code or
   built_in_function and for one to three operands or arguments.
   They return NULL_TREE if nothing could be simplified or
   the resulting simplified value with parts pushed to SEQ.
   If SEQ is NULL then if the simplification needs to create
   new stmts it will fail.  If VALUEIZE is non-NULL then all
   SSA names will be valueized using that hook prior to
   applying simplifications.  */

/* Unary ops.  */

tree
gimple_simplify (enum tree_code code, tree type,
		 tree op0,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (op0))
    {
      tree res = const_unop (code, type, op0);
      if (res != NULL_TREE
	  && CONSTANT_CLASS_P (res))
	return res;
    }

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, code, type, op0))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Binary ops.  */

tree
gimple_simplify (enum tree_code code, tree type,
		 tree op0, tree op1,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (op0) && constant_for_folding (op1))
    {
      tree res = const_binop (code, type, op0, op1);
      if (res != NULL_TREE
	  && CONSTANT_CLASS_P (res))
	return res;
    }

  /* Canonicalize operand order both for matching and fallback stmt
     generation.  */
  if ((commutative_tree_code (code)
       || TREE_CODE_CLASS (code) == tcc_comparison)
      && tree_swap_operands_p (op0, op1))
    {
      std::swap (op0, op1);
      if (TREE_CODE_CLASS (code) == tcc_comparison)
	code = swap_tree_comparison (code);
    }

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, code, type, op0, op1))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Ternary ops.  */

tree
gimple_simplify (enum tree_code code, tree type,
		 tree op0, tree op1, tree op2,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (op0) && constant_for_folding (op1)
      && constant_for_folding (op2))
    {
      tree res = fold_ternary/*_to_constant */ (code, type, op0, op1, op2);
      if (res != NULL_TREE
	  && CONSTANT_CLASS_P (res))
	return res;
    }

  /* Canonicalize operand order both for matching and fallback stmt
     generation.  */
  if (commutative_ternary_tree_code (code)
      && tree_swap_operands_p (op0, op1))
    std::swap (op0, op1);

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, code, type, op0, op1, op2))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Builtin or internal function with one argument.  */

tree
gimple_simplify (combined_fn fn, tree type,
		 tree arg0,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (arg0))
    {
      tree res = fold_const_call (fn, type, arg0);
      if (res && CONSTANT_CLASS_P (res))
	return res;
    }

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, fn, type, arg0))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Builtin or internal function with two arguments.  */

tree
gimple_simplify (combined_fn fn, tree type,
		 tree arg0, tree arg1,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (arg0)
      && constant_for_folding (arg1))
    {
      tree res = fold_const_call (fn, type, arg0, arg1);
      if (res && CONSTANT_CLASS_P (res))
	return res;
    }

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, fn, type, arg0, arg1))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Builtin or internal function with three arguments.  */

tree
gimple_simplify (combined_fn fn, tree type,
		 tree arg0, tree arg1, tree arg2,
		 gimple_seq *seq, tree (*valueize)(tree))
{
  if (constant_for_folding (arg0)
      && constant_for_folding (arg1)
      && constant_for_folding (arg2))
    {
      tree res = fold_const_call (fn, type, arg0, arg1, arg2);
      if (res && CONSTANT_CLASS_P (res))
	return res;
    }

  gimple_match_op res_op;
  if (!gimple_simplify (&res_op, seq, valueize, fn, type, arg0, arg1, arg2))
    return NULL_TREE;
  return maybe_push_res_to_seq (&res_op, seq);
}

/* Helper for gimple_simplify valueizing OP using VALUEIZE and setting
   VALUEIZED to true if valueization changed OP.  */

static inline tree
do_valueize (tree op, tree (*valueize)(tree), bool &valueized)
{
  if (valueize && TREE_CODE (op) == SSA_NAME)
    {
      tree tem = valueize (op);
      if (tem && tem != op)
	{
	  op = tem;
	  valueized = true;
	}
    }
  return op;
}

/* If RES_OP is a call to a conditional internal function, try simplifying
   the associated unconditional operation and using the result to build
   a new conditional operation.  For example, if RES_OP is:

     IFN_COND_ADD (COND, A, B, ELSE)

   try simplifying (plus A B) and using the result to build a replacement
   for the whole IFN_COND_ADD.

   Return true if this approach led to a simplification, otherwise leave
   RES_OP unchanged (and so suitable for other simplifications).  When
   returning true, add any new statements to SEQ and use VALUEIZE as the
   valueization function.

   RES_OP is known to be a call to IFN.  */

static bool
try_conditional_simplification (internal_fn ifn, gimple_match_op *res_op,
				gimple_seq *seq, tree (*valueize) (tree))
{
  code_helper op;
  tree_code code = conditional_internal_fn_code (ifn);
  if (code != ERROR_MARK)
    op = code;
  else
    {
      ifn = get_unconditional_internal_fn (ifn);
      if (ifn == IFN_LAST)
	return false;
      op = as_combined_fn (ifn);
    }

  unsigned int num_ops = res_op->num_ops;
  gimple_match_op cond_op (gimple_match_cond (res_op->ops[0],
					      res_op->ops[num_ops - 1]),
			   op, res_op->type, num_ops - 2);
  for (unsigned int i = 1; i < num_ops - 1; ++i)
    cond_op.ops[i - 1] = res_op->ops[i];
  switch (num_ops - 2)
    {
    case 2:
      if (!gimple_resimplify2 (seq, &cond_op, valueize))
	return false;
      break;
    case 3:
      if (!gimple_resimplify3 (seq, &cond_op, valueize))
	return false;
      break;
    default:
      gcc_unreachable ();
    }
  *res_op = cond_op;
  maybe_resimplify_conditional_op (seq, res_op, valueize);
  return true;
}

/* The main STMT based simplification entry.  It is used by the fold_stmt
   and the fold_stmt_to_constant APIs.  */

bool
gimple_simplify (gimple *stmt, gimple_match_op *res_op, gimple_seq *seq,
		 tree (*valueize)(tree), tree (*top_valueize)(tree))
{
  switch (gimple_code (stmt))
    {
    case GIMPLE_ASSIGN:
      {
	enum tree_code code = gimple_assign_rhs_code (stmt);
	tree type = TREE_TYPE (gimple_assign_lhs (stmt));
	switch (gimple_assign_rhs_class (stmt))
	  {
	  case GIMPLE_SINGLE_RHS:
	    if (code == REALPART_EXPR
		|| code == IMAGPART_EXPR
		|| code == VIEW_CONVERT_EXPR)
	      {
		tree op0 = TREE_OPERAND (gimple_assign_rhs1 (stmt), 0);
		bool valueized = false;
		op0 = do_valueize (op0, top_valueize, valueized);
		res_op->set_op (code, type, op0);
		return (gimple_resimplify1 (seq, res_op, valueize)
			|| valueized);
	      }
	    else if (code == BIT_FIELD_REF)
	      {
		tree rhs1 = gimple_assign_rhs1 (stmt);
		tree op0 = TREE_OPERAND (rhs1, 0);
		bool valueized = false;
		op0 = do_valueize (op0, top_valueize, valueized);
		res_op->set_op (code, type, op0,
				TREE_OPERAND (rhs1, 1),
				TREE_OPERAND (rhs1, 2),
				REF_REVERSE_STORAGE_ORDER (rhs1));
		if (res_op->reverse)
		  return valueized;
		return (gimple_resimplify3 (seq, res_op, valueize)
			|| valueized);
	      }
	    else if (code == SSA_NAME
		     && top_valueize)
	      {
		tree op0 = gimple_assign_rhs1 (stmt);
		tree valueized = top_valueize (op0);
		if (!valueized || op0 == valueized)
		  return false;
		res_op->set_op (TREE_CODE (op0), type, valueized);
		return true;
	      }
	    break;
	  case GIMPLE_UNARY_RHS:
	    {
	      tree rhs1 = gimple_assign_rhs1 (stmt);
	      bool valueized = false;
	      rhs1 = do_valueize (rhs1, top_valueize, valueized);
	      res_op->set_op (code, type, rhs1);
	      return (gimple_resimplify1 (seq, res_op, valueize)
		      || valueized);
	    }
	  case GIMPLE_BINARY_RHS:
	    {
	      tree rhs1 = gimple_assign_rhs1 (stmt);
	      tree rhs2 = gimple_assign_rhs2 (stmt);
	      bool valueized = false;
	      rhs1 = do_valueize (rhs1, top_valueize, valueized);
	      rhs2 = do_valueize (rhs2, top_valueize, valueized);
	      res_op->set_op (code, type, rhs1, rhs2);
	      return (gimple_resimplify2 (seq, res_op, valueize)
		      || valueized);
	    }
	  case GIMPLE_TERNARY_RHS:
	    {
	      bool valueized = false;
	      tree rhs1 = gimple_assign_rhs1 (stmt);
	      /* If this is a [VEC_]COND_EXPR first try to simplify an
		 embedded GENERIC condition.  */
	      if (code == COND_EXPR
		  || code == VEC_COND_EXPR)
		{
		  if (COMPARISON_CLASS_P (rhs1))
		    {
		      tree lhs = TREE_OPERAND (rhs1, 0);
		      tree rhs = TREE_OPERAND (rhs1, 1);
		      lhs = do_valueize (lhs, top_valueize, valueized);
		      rhs = do_valueize (rhs, top_valueize, valueized);
		      gimple_match_op res_op2 (res_op->cond, TREE_CODE (rhs1),
					       TREE_TYPE (rhs1), lhs, rhs);
		      if ((gimple_resimplify2 (seq, &res_op2, valueize)
			   || valueized)
			  && res_op2.code.is_tree_code ())
			{
			  valueized = true;
			  if (TREE_CODE_CLASS ((enum tree_code) res_op2.code)
			      == tcc_comparison)
			    rhs1 = build2 (res_op2.code, TREE_TYPE (rhs1),
					   res_op2.ops[0], res_op2.ops[1]);
			  else if (res_op2.code == SSA_NAME
				   || res_op2.code == INTEGER_CST
				   || res_op2.code == VECTOR_CST)
			    rhs1 = res_op2.ops[0];
			  else
			    valueized = false;
			}
		    }
		}
	      tree rhs2 = gimple_assign_rhs2 (stmt);
	      tree rhs3 = gimple_assign_rhs3 (stmt);
	      rhs1 = do_valueize (rhs1, top_valueize, valueized);
	      rhs2 = do_valueize (rhs2, top_valueize, valueized);
	      rhs3 = do_valueize (rhs3, top_valueize, valueized);
	      res_op->set_op (code, type, rhs1, rhs2, rhs3);
	      return (gimple_resimplify3 (seq, res_op, valueize)
		      || valueized);
	    }
	  default:
	    gcc_unreachable ();
	  }
	break;
      }

    case GIMPLE_CALL:
      /* ???  This way we can't simplify calls with side-effects.  */
      if (gimple_call_lhs (stmt) != NULL_TREE
	  && gimple_call_num_args (stmt) >= 1
	  && gimple_call_num_args (stmt) <= 5)
	{
	  bool valueized = false;
	  combined_fn cfn;
	  if (gimple_call_internal_p (stmt))
	    cfn = as_combined_fn (gimple_call_internal_fn (stmt));
	  else
	    {
	      tree fn = gimple_call_fn (stmt);
	      if (!fn)
		return false;

	      fn = do_valueize (fn, top_valueize, valueized);
	      if (TREE_CODE (fn) != ADDR_EXPR
		  || TREE_CODE (TREE_OPERAND (fn, 0)) != FUNCTION_DECL)
		return false;

	      tree decl = TREE_OPERAND (fn, 0);
	      if (DECL_BUILT_IN_CLASS (decl) != BUILT_IN_NORMAL
		  || !gimple_builtin_call_types_compatible_p (stmt, decl))
		return false;

	      cfn = as_combined_fn (DECL_FUNCTION_CODE (decl));
	    }

	  unsigned int num_args = gimple_call_num_args (stmt);
	  res_op->set_op (cfn, TREE_TYPE (gimple_call_lhs (stmt)), num_args);
	  for (unsigned i = 0; i < num_args; ++i)
	    {
	      tree arg = gimple_call_arg (stmt, i);
	      res_op->ops[i] = do_valueize (arg, top_valueize, valueized);
	    }
	  if (internal_fn_p (cfn)
	      && try_conditional_simplification (as_internal_fn (cfn),
						 res_op, seq, valueize))
	    return true;
	  switch (num_args)
	    {
	    case 1:
	      return (gimple_resimplify1 (seq, res_op, valueize)
		      || valueized);
	    case 2:
	      return (gimple_resimplify2 (seq, res_op, valueize)
		      || valueized);
	    case 3:
	      return (gimple_resimplify3 (seq, res_op, valueize)
		      || valueized);
	    case 4:
	      return (gimple_resimplify4 (seq, res_op, valueize)
		      || valueized);
	    case 5:
	      return (gimple_resimplify5 (seq, res_op, valueize)
		      || valueized);
	    default:
	     gcc_unreachable ();
	    }
	}
      break;

    case GIMPLE_COND:
      {
	tree lhs = gimple_cond_lhs (stmt);
	tree rhs = gimple_cond_rhs (stmt);
	bool valueized = false;
	lhs = do_valueize (lhs, top_valueize, valueized);
	rhs = do_valueize (rhs, top_valueize, valueized);
	res_op->set_op (gimple_cond_code (stmt), boolean_type_node, lhs, rhs);
	return (gimple_resimplify2 (seq, res_op, valueize)
		|| valueized);
      }

    default:
      break;
    }

  return false;
}


/* Helper for the autogenerated code, valueize OP.  */

inline tree
do_valueize (tree (*valueize)(tree), tree op)
{
  if (valueize && TREE_CODE (op) == SSA_NAME)
    {
      tree tem = valueize (op);
      if (tem)
	return tem;
    }
  return op;
}

/* Helper for the autogenerated code, get at the definition of NAME when
   VALUEIZE allows that.  */

inline gimple *
get_def (tree (*valueize)(tree), tree name)
{
  if (valueize && ! valueize (name))
    return NULL;
  return SSA_NAME_DEF_STMT (name);
}

/* Routine to determine if the types T1 and T2 are effectively
   the same for GIMPLE.  If T1 or T2 is not a type, the test
   applies to their TREE_TYPE.  */

static inline bool
types_match (tree t1, tree t2)
{
  if (!TYPE_P (t1))
    t1 = TREE_TYPE (t1);
  if (!TYPE_P (t2))
    t2 = TREE_TYPE (t2);

  return types_compatible_p (t1, t2);
}

/* Return if T has a single use.  For GIMPLE, we also allow any
   non-SSA_NAME (ie constants) and zero uses to cope with uses
   that aren't linked up yet.  */

static inline bool
single_use (tree t)
{
  return TREE_CODE (t) != SSA_NAME || has_zero_uses (t) || has_single_use (t);
}

/* Return true if math operations should be canonicalized,
   e.g. sqrt(sqrt(x)) -> pow(x, 0.25).  */

static inline bool
canonicalize_math_p ()
{
  return !cfun || (cfun->curr_properties & PROP_gimple_opt_math) == 0;
}

/* Return true if math operations that are beneficial only after
   vectorization should be canonicalized.  */

static inline bool
canonicalize_math_after_vectorization_p ()
{
  return !cfun || (cfun->curr_properties & PROP_gimple_lvec) != 0;
}

/* Return true if pow(cst, x) should be optimized into exp(log(cst) * x).
   As a workaround for SPEC CPU2017 628.pop2_s, don't do it if arg0
   is an exact integer, arg1 = phi_res +/- cst1 and phi_res = PHI <cst2, ...>
   where cst2 +/- cst1 is an exact integer, because then pow (arg0, arg1)
   will likely be exact, while exp (log (arg0) * arg1) might be not.
   Also don't do it if arg1 is phi_res above and cst2 is an exact integer.  */

static bool
optimize_pow_to_exp (tree arg0, tree arg1)
{
  gcc_assert (TREE_CODE (arg0) == REAL_CST);
  if (!real_isinteger (TREE_REAL_CST_PTR (arg0), TYPE_MODE (TREE_TYPE (arg0))))
    return true;

  if (TREE_CODE (arg1) != SSA_NAME)
    return true;

  gimple *def = SSA_NAME_DEF_STMT (arg1);
  gphi *phi = dyn_cast <gphi *> (def);
  tree cst1 = NULL_TREE;
  enum tree_code code = ERROR_MARK;
  if (!phi)
    {
      if (!is_gimple_assign (def))
	return true;
      code = gimple_assign_rhs_code (def);
      switch (code)
	{
	case PLUS_EXPR:
	case MINUS_EXPR:
	  break;
	default:
	  return true;
	}
      if (TREE_CODE (gimple_assign_rhs1 (def)) != SSA_NAME
	  || TREE_CODE (gimple_assign_rhs2 (def)) != REAL_CST)
	return true;

      cst1 = gimple_assign_rhs2 (def);

      phi = dyn_cast <gphi *> (SSA_NAME_DEF_STMT (gimple_assign_rhs1 (def)));
      if (!phi)
	return true;
    }

  tree cst2 = NULL_TREE;
  int n = gimple_phi_num_args (phi);
  for (int i = 0; i < n; i++)
    {
      tree arg = PHI_ARG_DEF (phi, i);
      if (TREE_CODE (arg) != REAL_CST)
	continue;
      else if (cst2 == NULL_TREE)
	cst2 = arg;
      else if (!operand_equal_p (cst2, arg, 0))
	return true;
    }

  if (cst1 && cst2)
    cst2 = const_binop (code, TREE_TYPE (cst2), cst2, cst1);
  if (cst2
      && TREE_CODE (cst2) == REAL_CST
      && real_isinteger (TREE_REAL_CST_PTR (cst2),
			 TYPE_MODE (TREE_TYPE (cst2))))
    return false;
  return true;
}
