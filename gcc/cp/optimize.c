/* Perform optimizations on tree structure.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004
   Free Software Foundation, Inc.
   Written by Mark Michell (mark@codesourcery.com).

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "cp-tree.h"
#include "rtl.h"
#include "insn-config.h"
#include "input.h"
#include "integrate.h"
#include "toplev.h"
#include "varray.h"
#include "params.h"
#include "hashtab.h"
#include "debug.h"
#include "tree-inline.h"
#include "flags.h"
#include "langhooks.h"
#include "diagnostic.h"
#include "tree-dump.h"
#include "tree-gimple.h"

/* Prototypes.  */

static void update_cloned_parm (tree, tree);
/* APPLE LOCAL begin structor thunks */
static int maybe_alias_body (tree fn, tree clone);
static int maybe_thunk_body (tree fn);
/* APPLE LOCAL end structor thunks */

/* CLONED_PARM is a copy of CLONE, generated for a cloned constructor
   or destructor.  Update it to ensure that the source-position for
   the cloned parameter matches that for the original, and that the
   debugging generation code will be able to find the original PARM.  */

static void
update_cloned_parm (tree parm, tree cloned_parm)
{
  DECL_ABSTRACT_ORIGIN (cloned_parm) = parm;

  /* We may have taken its address.  */
  TREE_ADDRESSABLE (cloned_parm) = TREE_ADDRESSABLE (parm);

  /* The definition might have different constness.  */
  TREE_READONLY (cloned_parm) = TREE_READONLY (parm);
  
  TREE_USED (cloned_parm) = TREE_USED (parm);
  
  /* The name may have changed from the declaration.  */
  DECL_NAME (cloned_parm) = DECL_NAME (parm);
  DECL_SOURCE_LOCATION (cloned_parm) = DECL_SOURCE_LOCATION (parm);
}

/* APPLE LOCAL begin structor thunks */
/* FN is a constructor or destructor, and there are FUNCTION_DECLs cloned from it nearby.
   If the clone and the original funciton have identical parameter lists,
   it is a fully-degenerate (does absolutely nothing) thunk.
   Make the clone an alias for the original function label.  */
static int
maybe_alias_body (tree fn ATTRIBUTE_UNUSED, tree clone ATTRIBUTE_UNUSED)
{
  extern FILE *asm_out_file ATTRIBUTE_UNUSED;

#ifdef ASM_MAYBE_ALIAS_BODY
  ASM_MAYBE_ALIAS_BODY (asm_out_file, fn, clone);
#endif
  return 0;
}

/* FN is a constructor or destructor, and there are FUNCTION_DECLs
   cloned from it nearby.  Instead of cloning this body, leave it
   alone and create tiny one-call bodies for the cloned
   FUNCTION_DECLs.  These clones are sibcall candidates, and their
   resulting code will be very thunk-esque.  */
static int
maybe_thunk_body (tree fn)
{
  tree call, clone, expr_stmt, fn_parm, fn_parm_typelist, last_arg, start;
  int parmno, vtt_parmno;

  if (flag_apple_kext || flag_clone_structors)
    return 0;

  /* If we've already seen this structor, avoid re-processing it.  */
  if (TREE_ASM_WRITTEN (fn))
    return 1;

  /* If function accepts variable arguments, give up.  */
  last_arg = tree_last (TYPE_ARG_TYPES (TREE_TYPE (fn)));
  if ( ! VOID_TYPE_P (TREE_VALUE (last_arg)))
       return 0;

  /* If constructor expects vector (AltiVec) arguments, give up.  */
  for (fn_parm = DECL_ARGUMENTS( fn); fn_parm; fn_parm = TREE_CHAIN (fn_parm))
    if (TREE_CODE (fn_parm) == VECTOR_TYPE)
      return 0;

  /* If we don't see a clone, nothing to do.  */
  clone = TREE_CHAIN (fn);
  if (!clone || ! DECL_CLONED_FUNCTION_P (clone))
    return 0;

  /* This is only a win if there are two or more clones.  */
  if ( ! TREE_CHAIN (clone))
    return 0;

  /* Only thunk-ify non-trivial structors.  */
  if (DECL_ESTIMATED_INSNS (fn) < 5)
     return 0;

  /* If we got this far, we've decided to turn the clones into thunks.  */

  /* We're going to generate code for fn, so it is no longer "abstract."  */
  /* APPLE LOCAL begin 3271957 and 3262497 */
  /* Leave 'abstract' bit set for unified constructs and destructors when 
     -gused is used.  */
  if (!(flag_debug_only_used_symbols
	&& DECL_DESTRUCTOR_P (fn)
	&& DECL_MAYBE_IN_CHARGE_DESTRUCTOR_P (fn))
      && !(flag_debug_only_used_symbols
	   && DECL_CONSTRUCTOR_P (fn)
	   && DECL_MAYBE_IN_CHARGE_CONSTRUCTOR_P (fn))
      )
    DECL_ABSTRACT (fn) = 0;
  /* APPLE LOCAL end 3271957 and 3262497 */

  /* Find the vtt_parm, if present.  */
  for (vtt_parmno = -1, parmno = 0, fn_parm = DECL_ARGUMENTS (fn);
       fn_parm;
       ++parmno, fn_parm = TREE_CHAIN (fn_parm))
    {
      if (DECL_ARTIFICIAL (fn_parm) && DECL_NAME (fn_parm) == vtt_parm_identifier)
	{
	  vtt_parmno = parmno;	/* Compensate for removed in_charge parameter.  */
	  break;
	}
    }

  /* We know that any clones immediately follow FN in the TYPE_METHODS
     list.  */
  for (clone = start = TREE_CHAIN (fn);
       clone && DECL_CLONED_FUNCTION_P (clone);
       clone = TREE_CHAIN (clone))
    {
      tree clone_parm, parmlist;

      /* If the clone and original parmlists are identical, turn the clone into an alias.  */
      if (maybe_alias_body (fn, clone))
	continue;

      /* If we've already generated a body for this clone, avoid duplicating it.
	 (Is it possible for a clone-list to grow after we first see it?)  */
      if (DECL_SAVED_TREE (clone) || TREE_ASM_WRITTEN (clone))
	continue;

      /* Start processing the function.  */
      push_to_top_level ();
      start_function (NULL_TREE, clone, NULL_TREE, SF_PRE_PARSED);

      /* Walk parameter lists together, creating parameter list for call to original function.  */
      for (parmno = 0,
	     parmlist = NULL,
	     fn_parm = DECL_ARGUMENTS (fn),
	     fn_parm_typelist = TYPE_ARG_TYPES (TREE_TYPE (fn)),
	     clone_parm = DECL_ARGUMENTS (clone);
	   fn_parm;
	   ++parmno,
	     fn_parm = TREE_CHAIN (fn_parm))
	{
	  if (parmno == vtt_parmno && ! DECL_HAS_VTT_PARM_P (clone))
	    {
	      tree typed_null_pointer_node = copy_node (null_pointer_node);
	      my_friendly_assert (fn_parm_typelist, 0);
	      /* Clobber actual parameter with formal parameter type.  */
	      TREE_TYPE (typed_null_pointer_node) = TREE_VALUE (fn_parm_typelist);
	      parmlist = tree_cons (NULL, typed_null_pointer_node, parmlist);
	    }
	  else if (parmno == 1 && DECL_HAS_IN_CHARGE_PARM_P (fn))
	    {
	      tree in_charge = copy_node (in_charge_arg_for_name (DECL_NAME (clone)));
	      parmlist = tree_cons (NULL, in_charge, parmlist);
	    }
	  /* Map other parameters to their equivalents in the cloned
	     function.  */
	  else
	    {
	      my_friendly_assert (clone_parm, 0);
	      DECL_ABSTRACT_ORIGIN (clone_parm) = NULL;
	      parmlist = tree_cons (NULL, clone_parm, parmlist);
	      clone_parm = TREE_CHAIN (clone_parm);
	    }
	  if (fn_parm_typelist)
	    fn_parm_typelist = TREE_CHAIN (fn_parm_typelist);
	}

      /* We built this list backwards; fix now.  */
      parmlist = nreverse (parmlist);
      mark_used (fn);
      call = build_function_call (fn, parmlist);
      expr_stmt = build_stmt (EXPR_STMT, call);
      add_stmt (expr_stmt);

      /* Now, expand this function into RTL, if appropriate.  */
      finish_function (0);
      DECL_ABSTRACT_ORIGIN (clone) = NULL;
      expand_body (clone);
      pop_from_top_level ();
    }
  return 1;
}
/* APPLE LOCAL end structor thunks */

/* FN is a function that has a complete body.  Clone the body as
   necessary.  Returns nonzero if there's no longer any need to
   process the main body.  */

bool
maybe_clone_body (tree fn)
{
  tree clone;

  /* We only clone constructors and destructors.  */
  if (!DECL_MAYBE_IN_CHARGE_CONSTRUCTOR_P (fn)
      && !DECL_MAYBE_IN_CHARGE_DESTRUCTOR_P (fn))
    return 0;

  /* Emit the DWARF1 abstract instance.  */
  (*debug_hooks->deferred_inline_function) (fn);

  /* We know that any clones immediately follow FN in the TYPE_METHODS
     list.  */
  for (clone = TREE_CHAIN (fn);
       clone && DECL_CLONED_FUNCTION_P (clone);
       clone = TREE_CHAIN (clone))
    {
      tree parm;
      tree clone_parm;
      /* APPLE LOCAL structor thunks */

      /* Update CLONE's source position information to match FN's.  */
      DECL_SOURCE_LOCATION (clone) = DECL_SOURCE_LOCATION (fn);
      DECL_INLINE (clone) = DECL_INLINE (fn);
      DECL_DECLARED_INLINE_P (clone) = DECL_DECLARED_INLINE_P (fn);
      DECL_COMDAT (clone) = DECL_COMDAT (fn);
      DECL_WEAK (clone) = DECL_WEAK (fn);
      DECL_ONE_ONLY (clone) = DECL_ONE_ONLY (fn);
      DECL_SECTION_NAME (clone) = DECL_SECTION_NAME (fn);
      DECL_USE_TEMPLATE (clone) = DECL_USE_TEMPLATE (fn);
      DECL_EXTERNAL (clone) = DECL_EXTERNAL (fn);
      DECL_INTERFACE_KNOWN (clone) = DECL_INTERFACE_KNOWN (fn);
      DECL_NOT_REALLY_EXTERN (clone) = DECL_NOT_REALLY_EXTERN (fn);
      TREE_PUBLIC (clone) = TREE_PUBLIC (fn);
      DECL_VISIBILITY (clone) = DECL_VISIBILITY (fn);

      /* Adjust the parameter names and locations.  */
      parm = DECL_ARGUMENTS (fn);
      clone_parm = DECL_ARGUMENTS (clone);
      /* Update the `this' parameter, which is always first.  */
      update_cloned_parm (parm, clone_parm);
      parm = TREE_CHAIN (parm);
      clone_parm = TREE_CHAIN (clone_parm);
      if (DECL_HAS_IN_CHARGE_PARM_P (fn))
	parm = TREE_CHAIN (parm);
      if (DECL_HAS_VTT_PARM_P (fn))
	parm = TREE_CHAIN (parm);
      if (DECL_HAS_VTT_PARM_P (clone))
	clone_parm = TREE_CHAIN (clone_parm);
      for (; parm;
	   parm = TREE_CHAIN (parm), clone_parm = TREE_CHAIN (clone_parm))
	/* Update this parameter.  */
	update_cloned_parm (parm, clone_parm);
      /* APPLE LOCAL structor thunks */
    }

  /* APPLE LOCAL begin structor thunks */
  /* If we decide to turn clones into thunks, they will branch to fn.
     Must have original function available to call.  */
  if (maybe_thunk_body (fn))
    return 0;
  /* APPLE LOCAL end structor thunks */

  /* APPLE LOCAL begin structor thunks */
  /* We know that any clones immediately follow FN in the TYPE_METHODS
     list.  */
  for (clone = TREE_CHAIN (fn);
       clone && DECL_CLONED_FUNCTION_P (clone);
       clone = TREE_CHAIN (clone))
    {
      tree parm;
      tree clone_parm;
      int parmno;
      splay_tree decl_map;
   /* APPLE LOCAL end structor thunks */

      /* Start processing the function.  */
      push_to_top_level ();
      start_function (NULL_TREE, clone, NULL_TREE, SF_PRE_PARSED);

      /* Remap the parameters.  */
      decl_map = splay_tree_new (splay_tree_compare_pointers, NULL, NULL);
      for (parmno = 0,
	     parm = DECL_ARGUMENTS (fn),
	     clone_parm = DECL_ARGUMENTS (clone);
	   parm;
	   ++parmno,
	     parm = TREE_CHAIN (parm))
	{
	  /* Map the in-charge parameter to an appropriate constant.  */
	  if (DECL_HAS_IN_CHARGE_PARM_P (fn) && parmno == 1)
	    {
	      tree in_charge;
	      in_charge = in_charge_arg_for_name (DECL_NAME (clone));
	      splay_tree_insert (decl_map,
				 (splay_tree_key) parm,
				 (splay_tree_value) in_charge);
	    }
	  else if (DECL_ARTIFICIAL (parm)
		   && DECL_NAME (parm) == vtt_parm_identifier)
	    {
	      /* For a subobject constructor or destructor, the next
		 argument is the VTT parameter.  Remap the VTT_PARM
		 from the CLONE to this parameter.  */
	      if (DECL_HAS_VTT_PARM_P (clone))
		{
		  DECL_ABSTRACT_ORIGIN (clone_parm) = parm;
		  splay_tree_insert (decl_map,
				     (splay_tree_key) parm,
				     (splay_tree_value) clone_parm);
		  clone_parm = TREE_CHAIN (clone_parm);
		}
	      /* Otherwise, map the VTT parameter to `NULL'.  */
	      else
		{
		  splay_tree_insert (decl_map,
				     (splay_tree_key) parm,
				     (splay_tree_value) null_pointer_node);
		}
	    }
	  /* Map other parameters to their equivalents in the cloned
	     function.  */
	  else
	    {
	      splay_tree_insert (decl_map,
				 (splay_tree_key) parm,
				 (splay_tree_value) clone_parm);
	      clone_parm = TREE_CHAIN (clone_parm);
	    }
	}

      /* Clone the body.  */
      clone_body (clone, fn, decl_map);

      /* Clean up.  */
      splay_tree_delete (decl_map);

      /* The clone can throw iff the original function can throw.  */
      cp_function_chain->can_throw = !TREE_NOTHROW (fn);

      /* Now, expand this function into RTL, if appropriate.  */
      finish_function (0);
      BLOCK_ABSTRACT_ORIGIN (DECL_INITIAL (clone)) = DECL_INITIAL (fn);
      expand_or_defer_fn (clone);
      pop_from_top_level ();
    }

  /* We don't need to process the original function any further.  */
  return 1;
}
