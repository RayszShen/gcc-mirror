/* Convert language-specific tree expression to rtl instructions,
   for GNU compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   2000 Free Software Foundation, Inc.

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
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "expr.h"
#include "cp-tree.h"
#include "toplev.h"
#include "except.h"
#include "tm_p.h"

static rtx cplus_expand_expr PARAMS ((tree, rtx, enum machine_mode,
				    enum expand_modifier));

/* Hook used by output_constant to expand language-specific
   constants.  */

tree
cplus_expand_constant (cst)
     tree cst;
{
  switch (TREE_CODE (cst))
    {
    case PTRMEM_CST:
      {
	tree type = TREE_TYPE (cst);
	tree member;
      
	/* Find the member.  */
	member = PTRMEM_CST_MEMBER (cst);

	if (TREE_CODE (member) == FIELD_DECL) 
	  {
	    /* Find the offset for the field.  */
	    tree offset = byte_position (member);

	    if (flag_new_abi)
	      /* Under the new ABI, we use -1 to represent the NULL
		 pointer; non-NULL values simply contain the offset of
		 the data member.  */
	      ;
	    else
	      /* We offset all pointer to data members by 1 so that we
		 can distinguish between a null pointer to data member
		 and the first data member of a structure.  */
	      offset = size_binop (PLUS_EXPR, offset, size_one_node);

	    cst = fold (build1 (NOP_EXPR, type, offset));
	  }
	else
	  {
	    tree delta, idx, pfn, delta2;

	    expand_ptrmemfunc_cst (cst, &delta, &idx, &pfn, &delta2);
	    cst = build_ptrmemfunc1 (type, delta, idx, pfn, delta2);
	  }
      }
      break;

    default:
      /* There's nothing to do.  */
      break;
    }

  return cst;
}

/* Hook used by expand_expr to expand language-specific tree codes.  */

static rtx
cplus_expand_expr (exp, target, tmode, modifier)
     tree exp;
     rtx target;
     enum machine_mode tmode;
     enum expand_modifier modifier;
{
  tree type = TREE_TYPE (exp);
  register enum machine_mode mode = TYPE_MODE (type);
  register enum tree_code code = TREE_CODE (exp);
  int ignore = target == const0_rtx;

  if (ignore)
    target = 0;

  /* No sense saving up arithmetic to be done
     if it's all in the wrong mode to form part of an address.
     And force_operand won't know whether to sign-extend or zero-extend.  */

  if (mode != Pmode && modifier == EXPAND_SUM)
    modifier = EXPAND_NORMAL;

  switch (code)
    {
    case PTRMEM_CST:
      return expand_expr (cplus_expand_constant (exp),
			  target, tmode, modifier);

    case OFFSET_REF:
      {
	return expand_expr (default_conversion (resolve_offset_ref (exp)),
			    target, tmode, EXPAND_NORMAL);
      }

    case THROW_EXPR:
      expand_expr (TREE_OPERAND (exp, 0), const0_rtx, VOIDmode, 0);
      expand_internal_throw ();
      return NULL;

    case EMPTY_CLASS_EXPR:
      /* We don't need to generate any code for an empty class.  */
      return const0_rtx;

    default:
      return c_expand_expr (exp, target, tmode, modifier);
    }
  my_friendly_abort (40);
  /* NOTREACHED */
  return NULL;
}

void
init_cplus_expand ()
{
  lang_expand_expr = cplus_expand_expr;
  lang_expand_constant = cplus_expand_constant;
}

/* If DECL had its rtl moved from where callers expect it
   to be, fix it up.  RESULT is the nominal rtl for the RESULT_DECL,
   which may be a pseudo instead of a hard register.  */

void
fixup_result_decl (decl, result)
     tree decl;
     rtx result;
{
  if (REG_P (result))
    {
      if (REGNO (result) >= FIRST_PSEUDO_REGISTER)
	{
	  rtx real_decl_result;

#ifdef FUNCTION_OUTGOING_VALUE
	  real_decl_result
	    = FUNCTION_OUTGOING_VALUE (TREE_TYPE (decl), current_function_decl);
#else
	  real_decl_result
	    = FUNCTION_VALUE (TREE_TYPE (decl), current_function_decl);
#endif
	  REG_FUNCTION_VALUE_P (real_decl_result) = 1;
	  result = real_decl_result;
	}
      store_expr (decl, result, 0);
      emit_insn (gen_rtx (USE, VOIDmode, result));
    }
}

int
extract_init (decl, init)
     tree decl ATTRIBUTE_UNUSED, init ATTRIBUTE_UNUSED;
{
  return 0;
}

