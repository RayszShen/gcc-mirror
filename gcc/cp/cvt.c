/* Language-level data type conversion for GNU C++.
   Copyright (C) 1987, 88, 92, 93, 94, 95, 1996 Free Software Foundation, Inc.
   Hacked by Michael Tiemann (tiemann@cygnus.com)

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


/* This file contains the functions for converting C expressions
   to different data types.  The only entry point is `convert'.
   Every language front end must have a `convert' function
   but what kind of conversions it does will depend on the language.  */

#include "config.h"
#include <stdio.h>
#include "tree.h"
#include "flags.h"
#include "cp-tree.h"
#include "class.h"
#include "convert.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

extern tree static_aggregates;

static tree build_thunk PROTO((tree, tree));
static tree convert_fn_ptr PROTO((tree, tree));
static tree cp_convert_to_pointer PROTO((tree, tree));
static tree convert_to_pointer_force PROTO((tree, tree));
static tree build_up_reference PROTO((tree, tree, int, int));
static tree build_type_conversion_1 PROTO((tree, tree, tree, tree,
					   int));

/* Change of width--truncation and extension of integers or reals--
   is represented with NOP_EXPR.  Proper functioning of many things
   assumes that no other conversions can be NOP_EXPRs.

   Conversion between integer and pointer is represented with CONVERT_EXPR.
   Converting integer to real uses FLOAT_EXPR
   and real to integer uses FIX_TRUNC_EXPR.

   Here is a list of all the functions that assume that widening and
   narrowing is always done with a NOP_EXPR:
     In convert.c, convert_to_integer.
     In c-typeck.c, build_binary_op_nodefault (boolean ops),
        and truthvalue_conversion.
     In expr.c: expand_expr, for operands of a MULT_EXPR.
     In fold-const.c: fold.
     In tree.c: get_narrower and get_unwidened.

   C++: in multiple-inheritance, converting between pointers may involve
   adjusting them by a delta stored within the class definition.  */

/* Subroutines of `convert'.  */

/* Build a thunk.  What it is, is an entry point that when called will
   adjust the this pointer (the first argument) by offset, and then
   goto the real address of the function given by REAL_ADDR that we
   would like called.  What we return is the address of the thunk.  */

static tree
build_thunk (offset, real_addr)
     tree offset, real_addr;
{
  if (TREE_CODE (real_addr) != ADDR_EXPR
      || TREE_CODE (TREE_OPERAND (real_addr, 0)) != FUNCTION_DECL)
    {
      sorry ("MI pointer to member conversion too complex");
      return error_mark_node;
    }
  sorry ("MI pointer to member conversion too complex");
  return error_mark_node;
}

/* Convert a `pointer to member' (POINTER_TYPE to METHOD_TYPE) into
   another `pointer to method'.  This may involved the creation of
   a thunk to handle the this offset calculation.  */

static tree
convert_fn_ptr (type, expr)
     tree type, expr;
{
#if 0				/* We don't use thunks for pmfs.  */
  if (flag_vtable_thunks)
    {
      tree intype = TREE_TYPE (expr);
      tree binfo = get_binfo (TYPE_METHOD_BASETYPE (TREE_TYPE (intype)),
			      TYPE_METHOD_BASETYPE (TREE_TYPE (type)), 1);
      if (binfo == error_mark_node)
	{
	  error ("  in pointer to member conversion");
	  return error_mark_node;
	}
      if (binfo == NULL_TREE)
	{
	  /* ARM 4.8 restriction.  */
	  error ("invalid pointer to member conversion");
	  return error_mark_node;
	}

      if (BINFO_OFFSET_ZEROP (binfo))
	return build1 (NOP_EXPR, type, expr);
      return build1 (NOP_EXPR, type, build_thunk (BINFO_OFFSET (binfo), expr));
    }
  else
#endif
    return build_ptrmemfunc (type, expr, 1);
}

/* if converting pointer to pointer
     if dealing with classes, check for derived->base or vice versa
     else if dealing with method pointers, delegate
     else convert blindly
   else if converting class, pass off to build_type_conversion
   else try C-style pointer conversion  */

static tree
cp_convert_to_pointer (type, expr)
     tree type, expr;
{
  register tree intype = TREE_TYPE (expr);
  register enum tree_code form;

  if (IS_AGGR_TYPE (intype))
    {
      tree rval;

      intype = complete_type (intype);
      if (TYPE_SIZE (intype) == NULL_TREE)
	{
	  cp_error ("can't convert from incomplete type `%T' to `%T'",
		    intype, type);
	  return error_mark_node;
	}

      rval = build_type_conversion (CONVERT_EXPR, type, expr, 1);
      if (rval)
	{
	  if (rval == error_mark_node)
	    cp_error ("conversion of `%E' from `%T' to `%T' is ambiguous",
		      expr, intype, type);
	  return rval;
	}
    }

  if (TYPE_PTRMEMFUNC_P (type))
    type = TYPE_PTRMEMFUNC_FN_TYPE (type);

  /* Handle anachronistic conversions from (::*)() to cv void* or (*)().  */
  if (TREE_CODE (type) == POINTER_TYPE
      && (TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE
	  || TYPE_MAIN_VARIANT (TREE_TYPE (type)) == void_type_node))
    {
      /* Allow an implicit this pointer for pointer to member
	 functions.  */
      if (TYPE_PTRMEMFUNC_P (intype))
	{
	  tree decl, basebinfo;
	  tree fntype = TREE_TYPE (TYPE_PTRMEMFUNC_FN_TYPE (intype));
	  tree t = TYPE_METHOD_BASETYPE (fntype);

	  if (current_class_type == 0
	      || get_base_distance (t, current_class_type, 0, &basebinfo)
	      == -1)
	    {
	      decl = build1 (NOP_EXPR, t, error_mark_node);
	    }
	  else if (current_class_ptr == 0)
	    decl = build1 (NOP_EXPR, t, error_mark_node);
	  else
	    decl = current_class_ref;

	  expr = build (OFFSET_REF, fntype, decl, expr);
	}

      if (TREE_CODE (expr) == OFFSET_REF
	  && TREE_CODE (TREE_TYPE (expr)) == METHOD_TYPE)
	expr = resolve_offset_ref (expr);
      if (TREE_CODE (TREE_TYPE (expr)) == METHOD_TYPE)
	expr = build_addr_func (expr);
      if (TREE_CODE (TREE_TYPE (expr)) == POINTER_TYPE)
	{
	  if (TREE_CODE (TREE_TYPE (TREE_TYPE (expr))) == METHOD_TYPE)
	    if (pedantic || warn_pmf2ptr)
	      cp_pedwarn ("converting from `%T' to `%T'", TREE_TYPE (expr),
			  type);
	  return build1 (NOP_EXPR, type, expr);
	}
      intype = TREE_TYPE (expr);
    }

  if (TYPE_PTRMEMFUNC_P (intype))
    intype = TYPE_PTRMEMFUNC_FN_TYPE (intype);

  form = TREE_CODE (intype);

  if (form == POINTER_TYPE || form == REFERENCE_TYPE)
    {
      intype = TYPE_MAIN_VARIANT (intype);

      if (TYPE_MAIN_VARIANT (type) != intype
	  && TREE_CODE (TREE_TYPE (type)) == RECORD_TYPE
	  && IS_AGGR_TYPE (TREE_TYPE (type))
	  && IS_AGGR_TYPE (TREE_TYPE (intype))
	  && TREE_CODE (TREE_TYPE (intype)) == RECORD_TYPE)
	{
	  enum tree_code code = PLUS_EXPR;
	  tree binfo = get_binfo (TREE_TYPE (type), TREE_TYPE (intype), 1);
	  if (binfo == error_mark_node)
	    return error_mark_node;
	  if (binfo == NULL_TREE)
	    {
	      binfo = get_binfo (TREE_TYPE (intype), TREE_TYPE (type), 1);
	      if (binfo == error_mark_node)
		return error_mark_node;
	      code = MINUS_EXPR;
	    }
	  if (binfo)
	    {
	      if (TYPE_USES_VIRTUAL_BASECLASSES (TREE_TYPE (type))
		  || TYPE_USES_VIRTUAL_BASECLASSES (TREE_TYPE (intype))
		  || ! BINFO_OFFSET_ZEROP (binfo))
		{
		  /* Need to get the path we took.  */
		  tree path;

		  if (code == PLUS_EXPR)
		    get_base_distance (TREE_TYPE (type), TREE_TYPE (intype), 0, &path);
		  else
		    get_base_distance (TREE_TYPE (intype), TREE_TYPE (type), 0, &path);
		  return build_vbase_path (code, type, expr, path, 0);
		}
	    }
	}
      if (TREE_CODE (TREE_TYPE (intype)) == METHOD_TYPE
	  && TREE_CODE (type) == POINTER_TYPE
	  && TREE_CODE (TREE_TYPE (type)) == METHOD_TYPE)
	return convert_fn_ptr (type, expr);

      if (TREE_CODE (TREE_TYPE (type)) == OFFSET_TYPE
	  && TREE_CODE (TREE_TYPE (intype)) == OFFSET_TYPE)
	{
	  tree b1 = TYPE_OFFSET_BASETYPE (TREE_TYPE (type));
	  tree b2 = TYPE_OFFSET_BASETYPE (TREE_TYPE (intype));
	  tree binfo = get_binfo (b2, b1, 1);
	  enum tree_code code = PLUS_EXPR;

	  if (binfo == NULL_TREE)
	    {
	      binfo = get_binfo (b1, b2, 1);
	      code = MINUS_EXPR;
	    }

	  if (binfo == error_mark_node)
	    return error_mark_node;
	  if (binfo && ! TREE_VIA_VIRTUAL (binfo))
	    expr = size_binop (code, expr, BINFO_OFFSET (binfo));
	}

      if (TREE_CODE (TREE_TYPE (intype)) == METHOD_TYPE
	  || (TREE_CODE (type) == POINTER_TYPE
	      && TREE_CODE (TREE_TYPE (type)) == METHOD_TYPE))
	{
	  cp_error ("cannot convert `%E' from type `%T' to type `%T'",
		    expr, intype, type);
	  return error_mark_node;
	}

      return build1 (NOP_EXPR, type, expr);
    }

  my_friendly_assert (form != OFFSET_TYPE, 186);

  if (TYPE_LANG_SPECIFIC (intype)
      && (IS_SIGNATURE_POINTER (intype) || IS_SIGNATURE_REFERENCE (intype)))
    return convert_to_pointer (type, build_optr_ref (expr));

  if (integer_zerop (expr))
    {
      if (TREE_CODE (TREE_TYPE (type)) == METHOD_TYPE)
	return build_ptrmemfunc (type, expr, 0);
      expr = build_int_2 (0, 0);
      TREE_TYPE (expr) = type;
      return expr;
    }

  if (INTEGRAL_CODE_P (form))
    {
      if (type_precision (intype) == POINTER_SIZE)
	return build1 (CONVERT_EXPR, type, expr);
      expr = cp_convert (type_for_size (POINTER_SIZE, 0), expr);
      /* Modes may be different but sizes should be the same.  */
      if (GET_MODE_SIZE (TYPE_MODE (TREE_TYPE (expr)))
	  != GET_MODE_SIZE (TYPE_MODE (type)))
	/* There is supposed to be some integral type
	   that is the same width as a pointer.  */
	abort ();
      return convert_to_pointer (type, expr);
    }

  cp_error ("cannot convert `%E' from type `%T' to type `%T'",
	    expr, intype, type);
  return error_mark_node;
}

/* Like convert, except permit conversions to take place which
   are not normally allowed due to access restrictions
   (such as conversion from sub-type to private super-type).  */

static tree
convert_to_pointer_force (type, expr)
     tree type, expr;
{
  register tree intype = TREE_TYPE (expr);
  register enum tree_code form = TREE_CODE (intype);
  
  if (integer_zerop (expr))
    {
      expr = build_int_2 (0, 0);
      TREE_TYPE (expr) = type;
      return expr;
    }

  /* Convert signature pointer/reference to `void *' first.  */
  if (form == RECORD_TYPE
      && (IS_SIGNATURE_POINTER (intype) || IS_SIGNATURE_REFERENCE (intype)))
    {
      expr = build_optr_ref (expr);
      intype = TREE_TYPE (expr);
      form = TREE_CODE (intype);
    }

  if (form == POINTER_TYPE)
    {
      intype = TYPE_MAIN_VARIANT (intype);

      if (TYPE_MAIN_VARIANT (type) != intype
	  && TREE_CODE (TREE_TYPE (type)) == RECORD_TYPE
	  && IS_AGGR_TYPE (TREE_TYPE (type))
	  && IS_AGGR_TYPE (TREE_TYPE (intype))
	  && TREE_CODE (TREE_TYPE (intype)) == RECORD_TYPE)
	{
	  enum tree_code code = PLUS_EXPR;
	  tree path;
	  int distance = get_base_distance (TREE_TYPE (type),
					    TREE_TYPE (intype), 0, &path);
	  if (distance == -2)
	    {
	    ambig:
	      cp_error ("type `%T' is ambiguous baseclass of `%s'",
			TREE_TYPE (type),
			TYPE_NAME_STRING (TREE_TYPE (intype)));
	      return error_mark_node;
	    }
	  if (distance == -1)
	    {
	      distance = get_base_distance (TREE_TYPE (intype),
					    TREE_TYPE (type), 0, &path);
	      if (distance == -2)
		goto ambig;
	      if (distance < 0)
		/* Doesn't need any special help from us.  */
		return build1 (NOP_EXPR, type, expr);

	      code = MINUS_EXPR;
	    }
	  return build_vbase_path (code, type, expr, path, 0);
	}
    }

  return cp_convert_to_pointer (type, expr);
}

/* We are passing something to a function which requires a reference.
   The type we are interested in is in TYPE. The initial
   value we have to begin with is in ARG.

   FLAGS controls how we manage access checking.
   DIRECT_BIND in FLAGS controls how any temporaries are generated.  */

static tree
build_up_reference (type, arg, flags, checkconst)
     tree type, arg;
     int flags, checkconst;
{
  tree rval;
  tree argtype = TREE_TYPE (arg);
  tree target_type = TREE_TYPE (type);

  my_friendly_assert (TREE_CODE (type) == REFERENCE_TYPE, 187);

  if ((flags & DIRECT_BIND) && ! real_lvalue_p (arg))
    {
      tree targ = arg;
      if (toplevel_bindings_p ())
	arg = get_temp_name (argtype, 1);
      else
	{
	  arg = pushdecl (build_decl (VAR_DECL, NULL_TREE, argtype));
	  DECL_ARTIFICIAL (arg) = 1;
	}
      DECL_INITIAL (arg) = targ;
      cp_finish_decl (arg, targ, NULL_TREE, 0, LOOKUP_ONLYCONVERTING);
    }
  else if (!(flags & DIRECT_BIND) && ! lvalue_p (arg))
    {
      tree slot = build_decl (VAR_DECL, NULL_TREE, argtype);
      arg = build (TARGET_EXPR, argtype, slot, arg, NULL_TREE, NULL_TREE);
      TREE_SIDE_EFFECTS (arg) = 1;
    }

  /* If we had a way to wrap this up, and say, if we ever needed it's
     address, transform all occurrences of the register, into a memory
     reference we could win better.  */
  rval = build_unary_op (ADDR_EXPR, arg, 1);
  if ((flags & LOOKUP_PROTECT)
      && TYPE_MAIN_VARIANT (argtype) != TYPE_MAIN_VARIANT (target_type)
      && IS_AGGR_TYPE (argtype)
      && IS_AGGR_TYPE (target_type))
    {
      /* We go through get_binfo for the access control.  */
      tree binfo = get_binfo (target_type, argtype, 1);
      if (binfo == error_mark_node)
	return error_mark_node;
      if (binfo == NULL_TREE)
	return error_not_base_type (target_type, argtype);
      rval = convert_pointer_to_real (binfo, rval);
    }
  else
    rval
      = convert_to_pointer_force (build_pointer_type (target_type), rval);
  rval = build1 (NOP_EXPR, type, rval);
  TREE_CONSTANT (rval) = TREE_CONSTANT (TREE_OPERAND (rval, 0));
  return rval;
}

/* For C++: Only need to do one-level references, but cannot
   get tripped up on signed/unsigned differences.

   DECL is either NULL_TREE or the _DECL node for a reference that is being
   initialized.  It can be error_mark_node if we don't know the _DECL but
   we know it's an initialization.  */

tree
convert_to_reference (reftype, expr, convtype, flags, decl)
     tree reftype, expr;
     int convtype, flags;
     tree decl;
{
  register tree type = TYPE_MAIN_VARIANT (TREE_TYPE (reftype));
  register tree intype = TREE_TYPE (expr);
  tree rval = NULL_TREE;
  tree rval_as_conversion = NULL_TREE;
  int i;

  if (TREE_CODE (intype) == REFERENCE_TYPE)
    my_friendly_abort (364);

  intype = TYPE_MAIN_VARIANT (intype);

  i = comp_target_types (type, intype, 0);

  if (i <= 0 && (convtype & CONV_IMPLICIT) && IS_AGGR_TYPE (intype)
      && ! (flags & LOOKUP_NO_CONVERSION))
    {
      /* Look for a user-defined conversion to lvalue that we can use.  */

      if (flag_ansi_overloading)
	rval_as_conversion
	  = build_type_conversion (CONVERT_EXPR, reftype, expr, 1);
      else
	rval_as_conversion = build_type_conversion (CONVERT_EXPR, type, expr, 1);

      if (rval_as_conversion && rval_as_conversion != error_mark_node
	  && real_lvalue_p (rval_as_conversion))
	{
	  expr = rval_as_conversion;
	  rval_as_conversion = NULL_TREE;
	  intype = type;
	  i = 1;
	}
    }

  if (((convtype & CONV_STATIC) && i == -1)
      || ((convtype & CONV_IMPLICIT) && i == 1))
    {
      if (flags & LOOKUP_COMPLAIN)
	{
	  tree ttl = TREE_TYPE (reftype);
	  tree ttr;
	  
	  {
	    int r = TREE_READONLY (expr);
	    int v = TREE_THIS_VOLATILE (expr);
	    ttr = cp_build_type_variant (TREE_TYPE (expr), r, v);
	  }

	  if (! real_lvalue_p (expr) && ! TYPE_READONLY (ttl))
	    {
	      if (decl)
		/* Ensure semantics of [dcl.init.ref] */
		cp_pedwarn ("initialization of non-const reference `%#T' from rvalue `%T'",
			    reftype, intype);
	      else
		cp_pedwarn ("conversion to non-const `%T' from rvalue `%T'",
			    reftype, intype);
	    }
	  else if (! (convtype & CONV_CONST))
	    {
	      if (! TYPE_READONLY (ttl) && TYPE_READONLY (ttr))
		cp_pedwarn ("conversion from `%T' to `%T' discards const",
			    ttr, reftype);
	      else if (! TYPE_VOLATILE (ttl) && TYPE_VOLATILE (ttr))
		cp_pedwarn ("conversion from `%T' to `%T' discards volatile",
			    ttr, reftype);
	    }
	}

      return build_up_reference (reftype, expr, flags,
				 ! (convtype & CONV_CONST));
    }
  else if ((convtype & CONV_REINTERPRET) && lvalue_p (expr))
    {
      /* When casting an lvalue to a reference type, just convert into
	 a pointer to the new type and deference it.  This is allowed
	 by San Diego WP section 5.2.9 paragraph 12, though perhaps it
	 should be done directly (jason).  (int &)ri ---> *(int*)&ri */

      /* B* bp; A& ar = (A&)bp; is valid, but it's probably not what they
         meant.  */
      if (TREE_CODE (intype) == POINTER_TYPE
	  && (comptypes (TREE_TYPE (intype), type, -1)))
	cp_warning ("casting `%T' to `%T' does not dereference pointer",
		    intype, reftype);
	  
      rval = build_unary_op (ADDR_EXPR, expr, 0);
      if (rval != error_mark_node)
	rval = convert_force (build_pointer_type (TREE_TYPE (reftype)), rval, 0);
      if (rval != error_mark_node)
	rval = build1 (NOP_EXPR, reftype, rval);
    }
  else if (flag_ansi_overloading)
    {
      rval = convert_for_initialization (NULL_TREE, type, expr, flags,
					 "converting", 0, 0);
      if (rval == error_mark_node)
	return error_mark_node;
      rval = build_up_reference (reftype, rval, flags, 1);

      if (rval && ! TYPE_READONLY (TREE_TYPE (reftype)))
	cp_pedwarn ("initializing non-const `%T' with `%T' will use a temporary",
		    reftype, intype);
    }
  else
    {
      tree rval_as_ctor = NULL_TREE;
      
      if (rval_as_conversion)
	{
	  if (rval_as_conversion == error_mark_node)
	    {
	      cp_error ("conversion from `%T' to `%T' is ambiguous",
			intype, reftype);
	      return error_mark_node;
	    }
	  rval_as_conversion = build_up_reference (reftype, rval_as_conversion,
						   flags, 1);
	}
      
      /* Definitely need to go through a constructor here.  */
      if (TYPE_HAS_CONSTRUCTOR (type)
	  && ! CLASSTYPE_ABSTRACT_VIRTUALS (type)
	  && (rval = build_method_call
	      (NULL_TREE, ctor_identifier,
	       build_expr_list (NULL_TREE, expr), TYPE_BINFO (type),
	       LOOKUP_NO_CONVERSION|LOOKUP_SPECULATIVELY
	       | LOOKUP_ONLYCONVERTING)))
	{
	  tree init;

	  if (toplevel_bindings_p ())
	    {
	      tree t = get_temp_name (type, toplevel_bindings_p ());
	      init = build_method_call (t, ctor_identifier,
					build_expr_list (NULL_TREE, expr),
					TYPE_BINFO (type),
					LOOKUP_NORMAL|LOOKUP_NO_CONVERSION
					| LOOKUP_ONLYCONVERTING);

	      if (init == error_mark_node)
		return error_mark_node;

	      make_decl_rtl (t, NULL_PTR, 1);
	      static_aggregates = perm_tree_cons (expr, t, static_aggregates);
	      rval = build_unary_op (ADDR_EXPR, t, 0);
	    }
	  else
	    {
	      init = build_method_call (NULL_TREE, ctor_identifier,
					build_expr_list (NULL_TREE, expr),
					TYPE_BINFO (type),
					LOOKUP_NORMAL|LOOKUP_NO_CONVERSION
					|LOOKUP_ONLYCONVERTING);

	      if (init == error_mark_node)
		return error_mark_node;

	      rval = build_cplus_new (type, init);
	      rval = build_up_reference (reftype, rval, flags, 1);
	    }
	  rval_as_ctor = rval;
	}

      if (rval_as_ctor && rval_as_conversion)
	{
	  cp_error ("ambiguous conversion from `%T' to `%T'; both user-defined conversion and constructor apply",
		    intype, reftype);
	  return error_mark_node;
	}
      else if (rval_as_ctor)
	rval = rval_as_ctor;
      else if (rval_as_conversion)
	rval = rval_as_conversion;
      else if (! IS_AGGR_TYPE (type) && ! IS_AGGR_TYPE (intype))
	{
	  rval = cp_convert (type, expr);
	  if (rval == error_mark_node)
	    return error_mark_node;
	  
	  rval = build_up_reference (reftype, rval, flags, 1);
	}

      if (rval && ! TYPE_READONLY (TREE_TYPE (reftype)))
	cp_pedwarn ("initializing non-const `%T' with `%T' will use a temporary",
		    reftype, intype);
    }

  if (rval)
    {
      /* If we found a way to convert earlier, then use it.  */
      return rval;
    }

  my_friendly_assert (TREE_CODE (intype) != OFFSET_TYPE, 189);

  if (flags & LOOKUP_COMPLAIN)
    cp_error ("cannot convert type `%T' to type `%T'", intype, reftype);

  if (flags & LOOKUP_SPECULATIVELY)
    return NULL_TREE;

  return error_mark_node;
}

/* We are using a reference VAL for its value. Bash that reference all the
   way down to its lowest form.  */

tree
convert_from_reference (val)
     tree val;
{
  tree type = TREE_TYPE (val);

  if (TREE_CODE (type) == OFFSET_TYPE)
    type = TREE_TYPE (type);
  if (TREE_CODE (type) == REFERENCE_TYPE)
    return build_indirect_ref (val, NULL_PTR);
  return val;
}

/* See if there is a constructor of type TYPE which will convert
   EXPR.  The reference manual seems to suggest (8.5.6) that we need
   not worry about finding constructors for base classes, then converting
   to the derived class.

   MSGP is a pointer to a message that would be an appropriate error
   string.  If MSGP is NULL, then we are not interested in reporting
   errors.  */

tree
convert_to_aggr (type, expr, msgp, protect)
     tree type, expr;
     char **msgp;
     int protect;
{
  tree basetype = type;
  tree name = TYPE_IDENTIFIER (basetype);
  tree function, fndecl, fntype, parmtypes, parmlist, result;
#if 0
  /* See code below that used this.  */
  tree method_name;
#endif
  tree access;
  int can_be_private, can_be_protected;

  if (! TYPE_HAS_CONSTRUCTOR (basetype))
    {
      if (msgp)
	*msgp = "type `%s' does not have a constructor";
      return error_mark_node;
    }

  access = access_public_node;
  can_be_private = 0;
  can_be_protected = IDENTIFIER_CLASS_VALUE (name) || name == current_class_name;

  parmlist = build_expr_list (NULL_TREE, expr);
  parmtypes = scratch_tree_cons (NULL_TREE, TREE_TYPE (expr), void_list_node);

  if (TYPE_USES_VIRTUAL_BASECLASSES (basetype))
    {
      parmtypes = expr_tree_cons (NULL_TREE, integer_type_node, parmtypes);
      parmlist = scratch_tree_cons (NULL_TREE, integer_one_node, parmlist);
    }

  /* The type of the first argument will be filled in inside the loop.  */
  parmlist = expr_tree_cons (NULL_TREE, integer_zero_node, parmlist);
  parmtypes = scratch_tree_cons (NULL_TREE, build_pointer_type (basetype), parmtypes);

  /* No exact conversion was found.  See if an approximate
     one will do.  */
  fndecl = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (basetype), 0);

  {
    int saw_private = 0;
    int saw_protected = 0;
    struct candidate *candidates
      = (struct candidate *) alloca ((decl_list_length (fndecl)+1) * sizeof (struct candidate));
    struct candidate *cp = candidates;

    while (fndecl)
      {
	function = fndecl;
	cp->h_len = 2;
	cp->harshness = (struct harshness_code *)
	  alloca (3 * sizeof (struct harshness_code));

	compute_conversion_costs (fndecl, parmlist, cp, 2);
	if ((cp->h.code & EVIL_CODE) == 0)
	  {
	    cp->u.field = fndecl;
	    if (protect)
	      {
		if (TREE_PRIVATE (fndecl))
		  access = access_private_node;
		else if (TREE_PROTECTED (fndecl))
		  access = access_protected_node;
		else
		  access = access_public_node;
	      }
	    else
	      access = access_public_node;

	    if (access == access_private_node
		? (basetype == current_class_type
		   || is_friend (basetype, cp->function)
		   || purpose_member (basetype, DECL_ACCESS (fndecl)))
		: access == access_protected_node
		? (can_be_protected
		   || purpose_member (basetype, DECL_ACCESS (fndecl)))
		: 1)
	      {
		if (cp->h.code <= TRIVIAL_CODE)
		  goto found_and_ok;
		cp++;
	      }
	    else
	      {
		if (access == access_private_node)
		  saw_private = 1;
		else
		  saw_protected = 1;
	      }
	  }
	fndecl = DECL_CHAIN (fndecl);
      }
    if (cp - candidates)
      {
	/* Rank from worst to best.  Then cp will point to best one.
	   Private fields have their bits flipped.  For unsigned
	   numbers, this should make them look very large.
	   If the best alternate has a (signed) negative value,
	   then all we ever saw were private members.  */
	if (cp - candidates > 1)
	  qsort (candidates,	/* char *base */
		 cp - candidates, /* int nel */
		 sizeof (struct candidate), /* int width */
		 (int (*) PROTO((const void *, const void *))) rank_for_overload); /* int (*compar)() */

	--cp;
	if (cp->h.code & EVIL_CODE)
	  {
	    if (msgp)
	      *msgp = "ambiguous type conversion possible for `%s'";
	    return error_mark_node;
	  }

	function = cp->function;
	fndecl = cp->u.field;
	goto found_and_ok;
      }
    else if (msgp)
      {
	if (saw_private)
	  {
	    if (saw_protected)
	      *msgp = "only private and protected conversions apply";
	    else
	      *msgp = "only private conversions apply";
	  }
	else if (saw_protected)
	  *msgp = "only protected conversions apply";
	else
	  *msgp = "no appropriate conversion to type `%s'";
      }
    return error_mark_node;
  }
  /* NOTREACHED */

 found:
  if (access == access_private_node)
    if (! can_be_private)
      {
	if (msgp)
	  *msgp = TREE_PRIVATE (fndecl)
	    ? "conversion to type `%s' is private"
	    : "conversion to type `%s' is from private base class";
	return error_mark_node;
      }
  if (access == access_protected_node)
    if (! can_be_protected)
      {
	if (msgp)
	  *msgp = TREE_PRIVATE (fndecl)
	    ? "conversion to type `%s' is protected"
	    : "conversion to type `%s' is from protected base class";
	return error_mark_node;
      }
  function = fndecl;
 found_and_ok:

  /* It will convert, but we don't do anything about it yet.  */
  if (msgp == 0)
    return NULL_TREE;

  fntype = TREE_TYPE (function);

  parmlist = convert_arguments (NULL_TREE, TYPE_ARG_TYPES (fntype),
				parmlist, NULL_TREE, LOOKUP_NORMAL);

  result = build_call (function, TREE_TYPE (fntype), parmlist);
  return result;
}

/* Call this when we know (for any reason) that expr is not, in fact,
   zero.  This routine is like convert_pointer_to, but it pays
   attention to which specific instance of what type we want to
   convert to.  This routine should eventually become
   convert_to_pointer after all references to convert_to_pointer
   are removed.  */

tree
convert_pointer_to_real (binfo, expr)
     tree binfo, expr;
{
  register tree intype = TREE_TYPE (expr);
  tree ptr_type;
  tree type, rval;

  if (TREE_CODE (binfo) == TREE_VEC)
    type = BINFO_TYPE (binfo);
  else if (IS_AGGR_TYPE (binfo))
    {
      type = binfo;
    }
  else
    {
      type = binfo;
      binfo = NULL_TREE;
    }

  ptr_type = cp_build_type_variant (type, TYPE_READONLY (TREE_TYPE (intype)),
				    TYPE_VOLATILE (TREE_TYPE (intype)));
  ptr_type = build_pointer_type (ptr_type);
  if (ptr_type == TYPE_MAIN_VARIANT (intype))
    return expr;

  if (intype == error_mark_node)
    return error_mark_node;

  my_friendly_assert (!integer_zerop (expr), 191);

  if (TREE_CODE (type) == RECORD_TYPE
      && TREE_CODE (TREE_TYPE (intype)) == RECORD_TYPE
      && type != TYPE_MAIN_VARIANT (TREE_TYPE (intype)))
    {
      tree path;
      int distance
	= get_base_distance (binfo, TYPE_MAIN_VARIANT (TREE_TYPE (intype)),
			     0, &path);

      /* This function shouldn't be called with unqualified arguments
	 but if it is, give them an error message that they can read.  */
      if (distance < 0)
	{
	  cp_error ("cannot convert a pointer of type `%T' to a pointer of type `%T'",
		    TREE_TYPE (intype), type);

	  if (distance == -2)
	    cp_error ("because `%T' is an ambiguous base class", type);
	  return error_mark_node;
	}

      return build_vbase_path (PLUS_EXPR, ptr_type, expr, path, 1);
    }
  rval = build1 (NOP_EXPR, ptr_type,
		 TREE_CODE (expr) == NOP_EXPR ? TREE_OPERAND (expr, 0) : expr);
  TREE_CONSTANT (rval) = TREE_CONSTANT (expr);
  return rval;
}

/* Call this when we know (for any reason) that expr is
   not, in fact, zero.  This routine gets a type out of the first
   argument and uses it to search for the type to convert to.  If there
   is more than one instance of that type in the expr, the conversion is
   ambiguous.  This routine should eventually go away, and all
   callers should use convert_to_pointer_real.  */

tree
convert_pointer_to (binfo, expr)
     tree binfo, expr;
{
  tree type;

  if (TREE_CODE (binfo) == TREE_VEC)
    type = BINFO_TYPE (binfo);
  else if (IS_AGGR_TYPE (binfo))
      type = binfo;
  else
      type = binfo;
  return convert_pointer_to_real (type, expr);
}

/* C++ conversions, preference to static cast conversions.  */

tree
cp_convert (type, expr)
     tree type, expr;
{
  return ocp_convert (type, expr, CONV_OLD_CONVERT, LOOKUP_NORMAL);
}

/* Conversion...

   FLAGS indicates how we should behave.  */

tree
ocp_convert (type, expr, convtype, flags)
     tree type, expr;
     int convtype, flags;
{
  register tree e = expr;
  register enum tree_code code = TREE_CODE (type);

  if (e == error_mark_node
      || TREE_TYPE (e) == error_mark_node)
    return error_mark_node;

  if (TREE_READONLY_DECL_P (e))
    e = decl_constant_value (e);

  if (IS_AGGR_TYPE (type) && (convtype & CONV_FORCE_TEMP))
    /* We need a new temporary; don't take this shortcut.  */;
  else if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (TREE_TYPE (e)))
    /* Trivial conversion: cv-qualifiers do not matter on rvalues.  */
    return fold (build1 (NOP_EXPR, type, e));
  
  if (code == VOID_TYPE && (convtype & CONV_STATIC))
    return build1 (CONVERT_EXPR, type, e);

#if 0
  /* This is incorrect.  A truncation can't be stripped this way.
     Extensions will be stripped by the use of get_unwidened.  */
  if (TREE_CODE (e) == NOP_EXPR)
    return cp_convert (type, TREE_OPERAND (e, 0));
#endif

  /* Just convert to the type of the member.  */
  if (code == OFFSET_TYPE)
    {
      type = TREE_TYPE (type);
      code = TREE_CODE (type);
    }

#if 0
  if (code == REFERENCE_TYPE)
    return fold (convert_to_reference (type, e, convtype, flags, NULL_TREE));
  else if (TREE_CODE (TREE_TYPE (e)) == REFERENCE_TYPE)
    e = convert_from_reference (e);
#endif

  if (TREE_CODE (e) == OFFSET_REF)
    e = resolve_offset_ref (e);

  if (INTEGRAL_CODE_P (code))
    {
      tree intype = TREE_TYPE (e);
      /* enum = enum, enum = int, enum = float, (enum)pointer are all
         errors.  */
      if (flag_int_enum_equivalence == 0
	  && TREE_CODE (type) == ENUMERAL_TYPE
	  && ((ARITHMETIC_TYPE_P (intype) && ! (convtype & CONV_STATIC))
	      || (TREE_CODE (intype) == POINTER_TYPE)))
	{
	  cp_pedwarn ("conversion from `%#T' to `%#T'", intype, type);

	  if (flag_pedantic_errors)
	    return error_mark_node;
	}
      if (IS_AGGR_TYPE (intype))
	{
	  tree rval;
	  rval = build_type_conversion (CONVERT_EXPR, type, e, 1);
	  if (rval)
	    return rval;
	  if (flags & LOOKUP_COMPLAIN)
	    cp_error ("`%#T' used where a `%T' was expected", intype, type);
	  if (flags & LOOKUP_SPECULATIVELY)
	    return NULL_TREE;
	  return error_mark_node;
	}
      if (code == BOOLEAN_TYPE)
	{
	  /* Common Ada/Pascal programmer's mistake.  We always warn
             about this since it is so bad.  */
	  if (TREE_CODE (expr) == FUNCTION_DECL)
	    cp_warning ("the address of `%D', will always be `true'", expr);
	  return truthvalue_conversion (e);
	}
      return fold (convert_to_integer (type, e));
    }
  if (code == POINTER_TYPE || code == REFERENCE_TYPE
      || TYPE_PTRMEMFUNC_P (type))
    return fold (cp_convert_to_pointer (type, e));
  if (code == REAL_TYPE || code == COMPLEX_TYPE)
    {
      if (IS_AGGR_TYPE (TREE_TYPE (e)))
	{
	  tree rval;
	  rval = build_type_conversion (CONVERT_EXPR, type, e, 1);
	  if (rval)
	    return rval;
	  else
	    if (flags & LOOKUP_COMPLAIN)
	      cp_error ("`%#T' used where a floating point value was expected",
			TREE_TYPE (e));
	}
      if (code == REAL_TYPE)
	return fold (convert_to_real (type, e));
      else if (code == COMPLEX_TYPE)
	return fold (convert_to_complex (type, e));
    }

  /* New C++ semantics:  since assignment is now based on
     memberwise copying,  if the rhs type is derived from the
     lhs type, then we may still do a conversion.  */
  if (IS_AGGR_TYPE_CODE (code))
    {
      tree dtype = TREE_TYPE (e);
      tree ctor = NULL_TREE;
      tree conversion = NULL_TREE;

      dtype = TYPE_MAIN_VARIANT (dtype);

      /* Conversion of object pointers or signature pointers/references
	 to signature pointers/references.  */

      if (TYPE_LANG_SPECIFIC (type)
	  && (IS_SIGNATURE_POINTER (type) || IS_SIGNATURE_REFERENCE (type)))
	{
	  tree constructor = build_signature_pointer_constructor (type, expr);
	  tree sig_ty = SIGNATURE_TYPE (type);
	  tree sig_ptr;

	  if (constructor == error_mark_node)
	    return error_mark_node;

	  sig_ptr = get_temp_name (type, 1);
	  DECL_INITIAL (sig_ptr) = constructor;
	  CLEAR_SIGNATURE (sig_ty);
	  cp_finish_decl (sig_ptr, constructor, NULL_TREE, 0, 0);
	  SET_SIGNATURE (sig_ty);
	  TREE_READONLY (sig_ptr) = 1;

	  return sig_ptr;
	}

      /* Conversion between aggregate types.  New C++ semantics allow
	 objects of derived type to be cast to objects of base type.
	 Old semantics only allowed this between pointers.

	 There may be some ambiguity between using a constructor
	 vs. using a type conversion operator when both apply.  */

      if (flag_ansi_overloading)
	{
	  ctor = e;
	  
	  if ((flags & LOOKUP_ONLYCONVERTING)
	      && ! (IS_AGGR_TYPE (dtype) && DERIVED_FROM_P (type, dtype)))
	    {
	      ctor = build_user_type_conversion (type, ctor, flags);
	      flags |= LOOKUP_NO_CONVERSION;
	    }
	  if (ctor)
	    ctor = build_method_call (NULL_TREE, ctor_identifier,
				      build_expr_list (NULL_TREE, ctor),
				      TYPE_BINFO (type), flags);
	  if (ctor)
	    return build_cplus_new (type, ctor);
	}
      else
	{
	  if (IS_AGGR_TYPE (dtype) && ! DERIVED_FROM_P (type, dtype)
	      && TYPE_HAS_CONVERSION (dtype))
	    conversion = build_type_conversion (CONVERT_EXPR, type, e, 1);

	  if (conversion == error_mark_node)
	    {
	      if (flags & LOOKUP_COMPLAIN)
		error ("ambiguous pointer conversion");
	      return conversion;
	    }

	  if (TYPE_HAS_CONSTRUCTOR (complete_type (type)))
	    ctor = build_method_call (NULL_TREE, ctor_identifier,
				      build_expr_list (NULL_TREE, e),
				      TYPE_BINFO (type),
				      (flags & LOOKUP_NORMAL)
				      | LOOKUP_SPECULATIVELY
				      | (flags & LOOKUP_ONLYCONVERTING)
				      | (flags & LOOKUP_NO_CONVERSION)
				      | (conversion ? LOOKUP_NO_CONVERSION : 0));

	  if (ctor == error_mark_node)
	    {
	      if (flags & LOOKUP_COMPLAIN)
		cp_error ("in conversion to type `%T'", type);
	      if (flags & LOOKUP_SPECULATIVELY)
		return NULL_TREE;
	      return error_mark_node;
	    }
      
	  if (conversion && ctor)
	    {
	      if (flags & LOOKUP_COMPLAIN)
		error ("both constructor and type conversion operator apply");
	      if (flags & LOOKUP_SPECULATIVELY)
		return NULL_TREE;
	      return error_mark_node;
	    }
	  else if (conversion)
	    return conversion;
	  else if (ctor)
	    {
	      ctor = build_cplus_new (type, ctor);
	      return ctor;
	    }
	}
    }

  /* If TYPE or TREE_TYPE (E) is not on the permanent_obstack,
     then it won't be hashed and hence compare as not equal,
     even when it is.  */
  if (code == ARRAY_TYPE
      && TREE_TYPE (TREE_TYPE (e)) == TREE_TYPE (type)
      && index_type_equal (TYPE_DOMAIN (TREE_TYPE (e)), TYPE_DOMAIN (type)))
    return e;

  if (flags & LOOKUP_COMPLAIN)
    cp_error ("conversion from `%T' to non-scalar type `%T' requested",
	      TREE_TYPE (expr), type);
  if (flags & LOOKUP_SPECULATIVELY)
    return NULL_TREE;
  return error_mark_node;
}

/* Create an expression whose value is that of EXPR,
   converted to type TYPE.  The TREE_TYPE of the value
   is always TYPE.  This function implements all reasonable
   conversions; callers should filter out those that are
   not permitted by the language being compiled.

   Most of this routine is from build_reinterpret_cast.

   The backend cannot call cp_convert (what was convert) because
   conversions to/from basetypes may involve memory references
   (vbases) and adding or subtracting small values (multiple
   inheritance), but it calls convert from the constant folding code
   on subtrees of already build trees after it has ripped them apart.

   Also, if we ever support range variables, we'll probably also have to
   do a little bit more work.  */

tree
convert (type, expr)
     tree type, expr;
{
  tree intype;

  if (type == error_mark_node || expr == error_mark_node)
    return error_mark_node;

  intype = TREE_TYPE (expr);

  if (POINTER_TYPE_P (type) && POINTER_TYPE_P (intype))
    {
      if (TREE_READONLY_DECL_P (expr))
	expr = decl_constant_value (expr);
      return fold (build1 (NOP_EXPR, type, expr));
    }

  return ocp_convert (type, expr, CONV_OLD_CONVERT,
		      LOOKUP_NORMAL|LOOKUP_NO_CONVERSION);
}

/* Like cp_convert, except permit conversions to take place which
   are not normally allowed due to access restrictions
   (such as conversion from sub-type to private super-type).  */

tree
convert_force (type, expr, convtype)
     tree type;
     tree expr;
     int convtype;
{
  register tree e = expr;
  register enum tree_code code = TREE_CODE (type);

  if (code == REFERENCE_TYPE)
    return fold (convert_to_reference (type, e, CONV_C_CAST, LOOKUP_COMPLAIN,
				       NULL_TREE));
  else if (TREE_CODE (TREE_TYPE (e)) == REFERENCE_TYPE)
    e = convert_from_reference (e);

  if (code == POINTER_TYPE)
    return fold (convert_to_pointer_force (type, e));

  /* From typeck.c convert_for_assignment */
  if (((TREE_CODE (TREE_TYPE (e)) == POINTER_TYPE && TREE_CODE (e) == ADDR_EXPR
	&& TREE_CODE (TREE_TYPE (e)) == POINTER_TYPE
	&& TREE_CODE (TREE_TYPE (TREE_TYPE (e))) == METHOD_TYPE)
       || integer_zerop (e)
       || TYPE_PTRMEMFUNC_P (TREE_TYPE (e)))
      && TYPE_PTRMEMFUNC_P (type))
    {
      /* compatible pointer to member functions.  */
      return build_ptrmemfunc (TYPE_PTRMEMFUNC_FN_TYPE (type), e, 1);
    }

  return ocp_convert (type, e, CONV_C_CAST|convtype, LOOKUP_NORMAL);
}

/* Subroutine of build_type_conversion.  */

static tree
build_type_conversion_1 (xtype, basetype, expr, typename, for_sure)
     tree xtype, basetype;
     tree expr;
     tree typename;
     int for_sure;
{
  tree rval;
  int flags;

  if (for_sure == 0)
    flags = LOOKUP_PROTECT|LOOKUP_ONLYCONVERTING;
  else
    flags = LOOKUP_NORMAL|LOOKUP_ONLYCONVERTING;

  rval = build_method_call (expr, typename, NULL_TREE, NULL_TREE, flags);
  if (rval == error_mark_node)
    {
      if (for_sure == 0)
	return NULL_TREE;
      return error_mark_node;
    }

  if (IS_AGGR_TYPE (TREE_TYPE (rval)))
    return rval;

  if (warn_cast_qual
      && TREE_TYPE (xtype)
      && (TREE_READONLY (TREE_TYPE (TREE_TYPE (rval)))
	  > TREE_READONLY (TREE_TYPE (xtype))))
    warning ("user-defined conversion casting away `const'");
  return cp_convert (xtype, rval);
}

/* Convert an aggregate EXPR to type XTYPE.  If a conversion
   exists, return the attempted conversion.  This may
   return ERROR_MARK_NODE if the conversion is not
   allowed (references private members, etc).
   If no conversion exists, NULL_TREE is returned.

   If (FOR_SURE & 1) is non-zero, then we allow this type conversion
   to take place immediately.  Otherwise, we build a SAVE_EXPR
   which can be evaluated if the results are ever needed.

   Changes to this functions should be mirrored in user_harshness.

   FIXME: Ambiguity checking is wrong.  Should choose one by the implicit
   object parameter, or by the second standard conversion sequence if
   that doesn't do it.  This will probably wait for an overloading rewrite.
   (jason 8/9/95)  */

tree
build_type_conversion (code, xtype, expr, for_sure)
     enum tree_code code;
     tree xtype, expr;
     int for_sure;
{
  /* C++: check to see if we can convert this aggregate type
     into the required type.  */
  tree basetype;
  tree conv;
  tree winner = NULL_TREE;

  if (flag_ansi_overloading)
    return build_user_type_conversion
      (xtype, expr, for_sure ? LOOKUP_NORMAL : 0);

  if (expr == error_mark_node)
    return error_mark_node;

  basetype = TREE_TYPE (expr);
  if (TREE_CODE (basetype) == REFERENCE_TYPE)
    basetype = TREE_TYPE (basetype);

  basetype = TYPE_MAIN_VARIANT (basetype);
  if (! TYPE_LANG_SPECIFIC (basetype) || ! TYPE_HAS_CONVERSION (basetype))
    return NULL_TREE;

  /* Do we have an exact match?  */
  {
    tree typename = build_typename_overload (xtype);
    if (lookup_fnfields (TYPE_BINFO (basetype), typename, 0))
      return build_type_conversion_1 (xtype, basetype, expr, typename,
				      for_sure);
  }

  /* Nope; try looking for others.  */
  for (conv = lookup_conversions (basetype); conv; conv = TREE_CHAIN (conv))
    {
      tree cand = TREE_VALUE (conv);

      if (winner && winner == cand)
	continue;

      if (can_convert (xtype, TREE_TYPE (TREE_TYPE (cand))))
	{
	  if (winner)
	    {
	      if (for_sure)
		{
		  cp_error ("ambiguous conversion from `%T' to `%T'", basetype,
			    xtype);
		  cp_error ("  candidate conversions include `%D' and `%D'",
			    winner, cand);
		}
	      return NULL_TREE;
	    }
	  else
	    winner = cand;
	}
    }

  if (winner)
    return build_type_conversion_1 (xtype, basetype, expr,
				    DECL_NAME (winner), for_sure);

  return NULL_TREE;
}

/* Convert the given EXPR to one of a group of types suitable for use in an
   expression.  DESIRES is a combination of various WANT_* flags (q.v.)
   which indicates which types are suitable.  If COMPLAIN is 1, complain
   about ambiguity; otherwise, the caller will deal with it.  */

tree
build_expr_type_conversion (desires, expr, complain)
     int desires;
     tree expr;
     int complain;
{
  tree basetype = TREE_TYPE (expr);
  tree conv;
  tree winner = NULL_TREE;

  if (TREE_CODE (basetype) == OFFSET_TYPE)
    expr = resolve_offset_ref (expr);
  expr = convert_from_reference (expr);
  basetype = TREE_TYPE (expr);

  if (! IS_AGGR_TYPE (basetype))
    switch (TREE_CODE (basetype))
      {
      case INTEGER_TYPE:
	if ((desires & WANT_NULL) && TREE_CODE (expr) == INTEGER_CST
	    && integer_zerop (expr))
	  return expr;
	/* else fall through...  */

      case BOOLEAN_TYPE:
	return (desires & WANT_INT) ? expr : NULL_TREE;
      case ENUMERAL_TYPE:
	return (desires & WANT_ENUM) ? expr : NULL_TREE;
      case REAL_TYPE:
	return (desires & WANT_FLOAT) ? expr : NULL_TREE;
      case POINTER_TYPE:
	return (desires & WANT_POINTER) ? expr : NULL_TREE;
	
      case FUNCTION_TYPE:
      case ARRAY_TYPE:
	return (desires & WANT_POINTER) ? default_conversion (expr)
     	                                : NULL_TREE;
      default:
	return NULL_TREE;
      }

  if (! TYPE_HAS_CONVERSION (basetype))
    return NULL_TREE;

  for (conv = lookup_conversions (basetype); conv; conv = TREE_CHAIN (conv))
    {
      int win = 0;
      tree candidate;
      tree cand = TREE_VALUE (conv);

      if (winner && winner == cand)
	continue;

      candidate = TREE_TYPE (TREE_TYPE (cand));
      if (TREE_CODE (candidate) == REFERENCE_TYPE)
	candidate = TREE_TYPE (candidate);

      switch (TREE_CODE (candidate))
	{
	case BOOLEAN_TYPE:
	case INTEGER_TYPE:
	  win = (desires & WANT_INT); break;
	case ENUMERAL_TYPE:
	  win = (desires & WANT_ENUM); break;
	case REAL_TYPE:
	  win = (desires & WANT_FLOAT); break;
	case POINTER_TYPE:
	  win = (desires & WANT_POINTER); break;

	default:
	  break;
	}

      if (win)
	{
	  if (winner)
	    {
	      if (complain)
		{
		  cp_error ("ambiguous default type conversion from `%T'",
			    basetype);
		  cp_error ("  candidate conversions include `%D' and `%D'",
			    winner, cand);
		}
	      return error_mark_node;
	    }
	  else
	    winner = cand;
	}
    }

  if (winner)
    {
      tree type = TREE_TYPE (TREE_TYPE (winner));
      if (TREE_CODE (type) == REFERENCE_TYPE)
	type = TREE_TYPE (type);
      return build_type_conversion_1 (type, basetype, expr,
				      DECL_NAME (winner), 1);
    }

  return NULL_TREE;
}

/* Must convert two aggregate types to non-aggregate type.
   Attempts to find a non-ambiguous, "best" type conversion.

   Return 1 on success, 0 on failure.

   @@ What are the real semantics of this supposed to be??? */

int
build_default_binary_type_conversion (code, arg1, arg2)
     enum tree_code code;
     tree *arg1, *arg2;
{
  switch (code)
    {
    case MULT_EXPR:
    case TRUNC_DIV_EXPR:
    case CEIL_DIV_EXPR:
    case FLOOR_DIV_EXPR:
    case ROUND_DIV_EXPR:
    case EXACT_DIV_EXPR:
      *arg1 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg1, 0);
      *arg2 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg2, 0);
      break;

    case TRUNC_MOD_EXPR:
    case FLOOR_MOD_EXPR:
    case LSHIFT_EXPR:
    case RSHIFT_EXPR:
    case BIT_AND_EXPR:
    case BIT_XOR_EXPR:
    case BIT_IOR_EXPR:
      *arg1 = build_expr_type_conversion (WANT_INT | WANT_ENUM, *arg1, 0);
      *arg2 = build_expr_type_conversion (WANT_INT | WANT_ENUM, *arg2, 0);
      break;

    case PLUS_EXPR:
      {
	tree a1, a2, p1, p2;
	int wins;

	a1 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg1, 0);
	a2 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg2, 0);
	p1 = build_expr_type_conversion (WANT_POINTER, *arg1, 0);
	p2 = build_expr_type_conversion (WANT_POINTER, *arg2, 0);

	wins = (a1 && a2) + (a1 && p2) + (p1 && a2);

	if (wins > 1)
	  error ("ambiguous default type conversion for `operator +'");

	if (a1 && a2)
	  *arg1 = a1, *arg2 = a2;
	else if (a1 && p2)
	  *arg1 = a1, *arg2 = p2;
	else
	  *arg1 = p1, *arg2 = a2;
	break;
      }

    case MINUS_EXPR:
      {
	tree a1, a2, p1, p2;
	int wins;

	a1 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg1, 0);
	a2 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg2, 0);
	p1 = build_expr_type_conversion (WANT_POINTER, *arg1, 0);
	p2 = build_expr_type_conversion (WANT_POINTER, *arg2, 0);

	wins = (a1 && a2) + (p1 && p2) + (p1 && a2);

	if (wins > 1)
	  error ("ambiguous default type conversion for `operator -'");

	if (a1 && a2)
	  *arg1 = a1, *arg2 = a2;
	else if (p1 && p2)
	  *arg1 = p1, *arg2 = p2;
	else
	  *arg1 = p1, *arg2 = a2;
	break;
      }

    case GT_EXPR:
    case LT_EXPR:
    case GE_EXPR:
    case LE_EXPR:
    case EQ_EXPR:
    case NE_EXPR:
      {
	tree a1, a2, p1, p2;
	int wins;

	a1 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg1, 0);
	a2 = build_expr_type_conversion (WANT_ARITH | WANT_ENUM, *arg2, 0);
	p1 = build_expr_type_conversion (WANT_POINTER | WANT_NULL, *arg1, 0);
	p2 = build_expr_type_conversion (WANT_POINTER | WANT_NULL, *arg2, 0);

	wins = (a1 && a2) + (p1 && p2);

	if (wins > 1)
	  cp_error ("ambiguous default type conversion for `%O'", code);

	if (a1 && a2)
	  *arg1 = a1, *arg2 = a2;
	else
	  *arg1 = p1, *arg2 = p2;
	break;
      }

    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
      *arg1 = cp_convert (boolean_type_node, *arg1);
      *arg2 = cp_convert (boolean_type_node, *arg2);
      break;

    default:
      *arg1 = NULL_TREE;
      *arg2 = NULL_TREE;
    }

  if (*arg1 == error_mark_node || *arg2 == error_mark_node)
    cp_error ("ambiguous default type conversion for `%O'", code);

  if (*arg1 && *arg2)
    return 1;

  return 0;
}

/* Implements integral promotion (4.1) and float->double promotion.  */

tree
type_promotes_to (type)
     tree type;
{
  int constp, volatilep;

  if (type == error_mark_node)
    return error_mark_node;

  constp = TYPE_READONLY (type);
  volatilep = TYPE_VOLATILE (type);
  type = TYPE_MAIN_VARIANT (type);

  /* bool always promotes to int (not unsigned), even if it's the same
     size.  */
  if (type == boolean_type_node)
    type = integer_type_node;

  /* Normally convert enums to int, but convert wide enums to something
     wider.  */
  else if (TREE_CODE (type) == ENUMERAL_TYPE
	   || type == wchar_type_node)
    {
      int precision = MAX (TYPE_PRECISION (type),
			   TYPE_PRECISION (integer_type_node));
      tree totype = type_for_size (precision, 0);
      if (TREE_UNSIGNED (type)
	  && ! int_fits_type_p (TYPE_MAX_VALUE (type), totype))
	type = type_for_size (precision, 1);
      else
	type = totype;
    }
  else if (C_PROMOTING_INTEGER_TYPE_P (type))
    {
      /* Retain unsignedness if really not getting bigger.  */
      if (TREE_UNSIGNED (type)
	  && TYPE_PRECISION (type) == TYPE_PRECISION (integer_type_node))
	type = unsigned_type_node;
      else
	type = integer_type_node;
    }
  else if (type == float_type_node)
    type = double_type_node;

  return cp_build_type_variant (type, constp, volatilep);
}


/* The routines below this point are carefully written to conform to
   the standard.  They use the same terminology, and follow the rules
   closely.  Although they are used only in pt.c at the moment, they
   should presumably be used everywhere in the future.  */

/* Attempt to perform qualification conversions on EXPR to convert it
   to TYPE.  Return the resulting expression, or error_mark_node if
   the conversion was impossible.  */

tree 
perform_qualification_conversions (type, expr)
     tree type;
     tree expr;
{
  if (comp_ptr_ttypes (type, TREE_TYPE(expr)))
    return build1 (NOP_EXPR, type, expr);
  else
    return error_mark_node;
}
