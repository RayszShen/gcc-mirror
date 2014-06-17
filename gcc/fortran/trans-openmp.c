/* OpenMP directive translation -- generate GCC trees from gfc_code.
   Copyright (C) 2005-2014 Free Software Foundation, Inc.
   Contributed by Jakub Jelinek <jakub@redhat.com>

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
#include "tree.h"
#include "gimple-expr.h"
#include "gimplify.h"	/* For create_tmp_var_raw.  */
#include "stringpool.h"
#include "diagnostic-core.h"	/* For internal_error.  */
#include "gfortran.h"
#include "trans.h"
#include "trans-stmt.h"
#include "trans-types.h"
#include "trans-array.h"
#include "trans-const.h"
#include "arith.h"
#include "omp-low.h"

int ompws_flags;

/* True if OpenMP should privatize what this DECL points to rather
   than the DECL itself.  */

bool
gfc_omp_privatize_by_reference (const_tree decl)
{
  tree type = TREE_TYPE (decl);

  if (TREE_CODE (type) == REFERENCE_TYPE
      && (!DECL_ARTIFICIAL (decl) || TREE_CODE (decl) == PARM_DECL))
    return true;

  if (TREE_CODE (type) == POINTER_TYPE)
    {
      /* Array POINTER/ALLOCATABLE have aggregate types, all user variables
	 that have POINTER_TYPE type and don't have GFC_POINTER_TYPE_P
	 set are supposed to be privatized by reference.  */
      if (GFC_DECL_GET_SCALAR_POINTER (decl)
	  || GFC_DECL_GET_SCALAR_ALLOCATABLE (decl)
	  || GFC_DECL_CRAY_POINTEE (decl))
	return false;

      if (!DECL_ARTIFICIAL (decl)
	  && TREE_CODE (TREE_TYPE (type)) != FUNCTION_TYPE)
	return true;

      /* Some arrays are expanded as DECL_ARTIFICIAL pointers
	 by the frontend.  */
      if (DECL_LANG_SPECIFIC (decl)
	  && GFC_DECL_SAVED_DESCRIPTOR (decl))
	return true;
    }

  return false;
}

/* True if OpenMP sharing attribute of DECL is predetermined.  */

enum omp_clause_default_kind
gfc_omp_predetermined_sharing (tree decl)
{
  /* Associate names preserve the association established during ASSOCIATE.
     As they are implemented either as pointers to the selector or array
     descriptor and shouldn't really change in the ASSOCIATE region,
     this decl can be either shared or firstprivate.  If it is a pointer,
     use firstprivate, as it is cheaper that way, otherwise make it shared.  */
  if (GFC_DECL_ASSOCIATE_VAR_P (decl))
    {
      if (TREE_CODE (TREE_TYPE (decl)) == POINTER_TYPE)
	return OMP_CLAUSE_DEFAULT_FIRSTPRIVATE;
      else
	return OMP_CLAUSE_DEFAULT_SHARED;
    }

  if (DECL_ARTIFICIAL (decl)
      && ! GFC_DECL_RESULT (decl)
      && ! (DECL_LANG_SPECIFIC (decl)
	    && GFC_DECL_SAVED_DESCRIPTOR (decl)))
    return OMP_CLAUSE_DEFAULT_SHARED;

  /* Cray pointees shouldn't be listed in any clauses and should be
     gimplified to dereference of the corresponding Cray pointer.
     Make them all private, so that they are emitted in the debug
     information.  */
  if (GFC_DECL_CRAY_POINTEE (decl))
    return OMP_CLAUSE_DEFAULT_PRIVATE;

  /* Assumed-size arrays are predetermined shared.  */
  if (TREE_CODE (decl) == PARM_DECL
      && GFC_ARRAY_TYPE_P (TREE_TYPE (decl))
      && GFC_TYPE_ARRAY_AKIND (TREE_TYPE (decl)) == GFC_ARRAY_UNKNOWN
      && GFC_TYPE_ARRAY_UBOUND (TREE_TYPE (decl),
				GFC_TYPE_ARRAY_RANK (TREE_TYPE (decl)) - 1)
	 == NULL)
    return OMP_CLAUSE_DEFAULT_SHARED;

  /* Dummy procedures aren't considered variables by OpenMP, thus are
     disallowed in OpenMP clauses.  They are represented as PARM_DECLs
     in the middle-end, so return OMP_CLAUSE_DEFAULT_FIRSTPRIVATE here
     to avoid complaining about their uses with default(none).  */
  if (TREE_CODE (decl) == PARM_DECL
      && TREE_CODE (TREE_TYPE (decl)) == POINTER_TYPE
      && TREE_CODE (TREE_TYPE (TREE_TYPE (decl))) == FUNCTION_TYPE)
    return OMP_CLAUSE_DEFAULT_FIRSTPRIVATE;

  /* COMMON and EQUIVALENCE decls are shared.  They
     are only referenced through DECL_VALUE_EXPR of the variables
     contained in them.  If those are privatized, they will not be
     gimplified to the COMMON or EQUIVALENCE decls.  */
  if (GFC_DECL_COMMON_OR_EQUIV (decl) && ! DECL_HAS_VALUE_EXPR_P (decl))
    return OMP_CLAUSE_DEFAULT_SHARED;

  if (GFC_DECL_RESULT (decl) && ! DECL_HAS_VALUE_EXPR_P (decl))
    return OMP_CLAUSE_DEFAULT_SHARED;

  return OMP_CLAUSE_DEFAULT_UNSPECIFIED;
}

/* Return decl that should be used when reporting DEFAULT(NONE)
   diagnostics.  */

tree
gfc_omp_report_decl (tree decl)
{
  if (DECL_ARTIFICIAL (decl)
      && DECL_LANG_SPECIFIC (decl)
      && GFC_DECL_SAVED_DESCRIPTOR (decl))
    return GFC_DECL_SAVED_DESCRIPTOR (decl);

  return decl;
}

/* Return true if TYPE has any allocatable components.  */

static bool
gfc_has_alloc_comps (tree type, tree decl)
{
  tree field, ftype;

  if (POINTER_TYPE_P (type))
    {
      if (GFC_DECL_GET_SCALAR_ALLOCATABLE (decl))
	type = TREE_TYPE (type);
      else if (GFC_DECL_GET_SCALAR_POINTER (decl))
	return false;
    }

  while (GFC_DESCRIPTOR_TYPE_P (type) || GFC_ARRAY_TYPE_P (type))
    type = gfc_get_element_type (type);

  if (TREE_CODE (type) != RECORD_TYPE)
    return false;

  for (field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
    {
      ftype = TREE_TYPE (field);
      if (GFC_DECL_GET_SCALAR_ALLOCATABLE (field))
	return true;
      if (GFC_DESCRIPTOR_TYPE_P (ftype)
	  && GFC_TYPE_ARRAY_AKIND (ftype) == GFC_ARRAY_ALLOCATABLE)
	return true;
      if (gfc_has_alloc_comps (ftype, field))
	return true;
    }
  return false;
}

/* Return true if DECL in private clause needs
   OMP_CLAUSE_PRIVATE_OUTER_REF on the private clause.  */
bool
gfc_omp_private_outer_ref (tree decl)
{
  tree type = TREE_TYPE (decl);

  if (GFC_DESCRIPTOR_TYPE_P (type)
      && GFC_TYPE_ARRAY_AKIND (type) == GFC_ARRAY_ALLOCATABLE)
    return true;

  if (GFC_DECL_GET_SCALAR_ALLOCATABLE (decl))
    return true;

  if (gfc_omp_privatize_by_reference (decl))
    type = TREE_TYPE (type);

  if (gfc_has_alloc_comps (type, decl))
    return true;

  return false;
}

/* Callback for gfc_omp_unshare_expr.  */

static tree
gfc_omp_unshare_expr_r (tree *tp, int *walk_subtrees, void *)
{
  tree t = *tp;
  enum tree_code code = TREE_CODE (t);

  /* Stop at types, decls, constants like copy_tree_r.  */
  if (TREE_CODE_CLASS (code) == tcc_type
      || TREE_CODE_CLASS (code) == tcc_declaration
      || TREE_CODE_CLASS (code) == tcc_constant
      || code == BLOCK)
    *walk_subtrees = 0;
  else if (handled_component_p (t)
	   || TREE_CODE (t) == MEM_REF)
    {
      *tp = unshare_expr (t);
      *walk_subtrees = 0;
    }

  return NULL_TREE;
}

/* Unshare in expr anything that the FE which normally doesn't
   care much about tree sharing (because during gimplification
   everything is unshared) could cause problems with tree sharing
   at omp-low.c time.  */

static tree
gfc_omp_unshare_expr (tree expr)
{
  walk_tree (&expr, gfc_omp_unshare_expr_r, NULL, NULL);
  return expr;
}

enum walk_alloc_comps
{
  WALK_ALLOC_COMPS_DTOR,
  WALK_ALLOC_COMPS_DEFAULT_CTOR,
  WALK_ALLOC_COMPS_COPY_CTOR
};

/* Handle allocatable components in OpenMP clauses.  */

static tree
gfc_walk_alloc_comps (tree decl, tree dest, tree var,
		      enum walk_alloc_comps kind)
{
  stmtblock_t block, tmpblock;
  tree type = TREE_TYPE (decl), then_b, tem, field;
  gfc_init_block (&block);

  if (GFC_ARRAY_TYPE_P (type) || GFC_DESCRIPTOR_TYPE_P (type))
    {
      if (GFC_DESCRIPTOR_TYPE_P (type))
	{
	  gfc_init_block (&tmpblock);
	  tem = gfc_full_array_size (&tmpblock, decl,
				     GFC_TYPE_ARRAY_RANK (type));
	  then_b = gfc_finish_block (&tmpblock);
	  gfc_add_expr_to_block (&block, gfc_omp_unshare_expr (then_b));
	  tem = gfc_omp_unshare_expr (tem);
	  tem = fold_build2_loc (input_location, MINUS_EXPR,
				 gfc_array_index_type, tem,
				 gfc_index_one_node);
	}
      else
	{
	  if (!TYPE_DOMAIN (type)
	      || TYPE_MAX_VALUE (TYPE_DOMAIN (type)) == NULL_TREE
	      || TYPE_MIN_VALUE (TYPE_DOMAIN (type)) == error_mark_node
	      || TYPE_MAX_VALUE (TYPE_DOMAIN (type)) == error_mark_node)
	    {
	      tem = fold_build2 (EXACT_DIV_EXPR, sizetype,
				 TYPE_SIZE_UNIT (type),
				 TYPE_SIZE_UNIT (TREE_TYPE (type)));
	      tem = size_binop (MINUS_EXPR, tem, size_one_node);
	    }
	  else
	    tem = array_type_nelts (type);
	  tem = fold_convert (gfc_array_index_type, tem);
	}

      tree nelems = gfc_evaluate_now (tem, &block);
      tree index = gfc_create_var (gfc_array_index_type, "S");

      gfc_init_block (&tmpblock);
      tem = gfc_conv_array_data (decl);
      tree declvar = build_fold_indirect_ref_loc (input_location, tem);
      tree declvref = gfc_build_array_ref (declvar, index, NULL);
      tree destvar, destvref = NULL_TREE;
      if (dest)
	{
	  tem = gfc_conv_array_data (dest);
	  destvar = build_fold_indirect_ref_loc (input_location, tem);
	  destvref = gfc_build_array_ref (destvar, index, NULL);
	}
      gfc_add_expr_to_block (&tmpblock,
			     gfc_walk_alloc_comps (declvref, destvref,
						   var, kind));

      gfc_loopinfo loop;
      gfc_init_loopinfo (&loop);
      loop.dimen = 1;
      loop.from[0] = gfc_index_zero_node;
      loop.loopvar[0] = index;
      loop.to[0] = nelems;
      gfc_trans_scalarizing_loops (&loop, &tmpblock);
      gfc_add_block_to_block (&block, &loop.pre);
      return gfc_finish_block (&block);
    }
  else if (GFC_DECL_GET_SCALAR_ALLOCATABLE (var))
    {
      decl = build_fold_indirect_ref_loc (input_location, decl);
      if (dest)
	dest = build_fold_indirect_ref_loc (input_location, dest);
      type = TREE_TYPE (decl);
    }

  gcc_assert (TREE_CODE (type) == RECORD_TYPE);
  for (field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
    {
      tree ftype = TREE_TYPE (field);
      tree declf, destf = NULL_TREE;
      bool has_alloc_comps = gfc_has_alloc_comps (ftype, field);
      if ((!GFC_DESCRIPTOR_TYPE_P (ftype)
	   || GFC_TYPE_ARRAY_AKIND (ftype) != GFC_ARRAY_ALLOCATABLE)
	  && !GFC_DECL_GET_SCALAR_ALLOCATABLE (field)
	  && !has_alloc_comps)
	continue;
      declf = fold_build3_loc (input_location, COMPONENT_REF, ftype,
			       decl, field, NULL_TREE);
      if (dest)
	destf = fold_build3_loc (input_location, COMPONENT_REF, ftype,
				 dest, field, NULL_TREE);

      tem = NULL_TREE;
      switch (kind)
	{
	case WALK_ALLOC_COMPS_DTOR:
	  break;
	case WALK_ALLOC_COMPS_DEFAULT_CTOR:
	  if (GFC_DESCRIPTOR_TYPE_P (ftype)
	      && GFC_TYPE_ARRAY_AKIND (ftype) == GFC_ARRAY_ALLOCATABLE)
	    {
	      gfc_add_modify (&block, unshare_expr (destf),
			      unshare_expr (declf));
	      tem = gfc_duplicate_allocatable_nocopy
					(destf, declf, ftype,
					 GFC_TYPE_ARRAY_RANK (ftype));
	    }
	  else if (GFC_DECL_GET_SCALAR_ALLOCATABLE (field))
	    tem = gfc_duplicate_allocatable_nocopy (destf, declf, ftype, 0);
	  break;
	case WALK_ALLOC_COMPS_COPY_CTOR:
	  if (GFC_DESCRIPTOR_TYPE_P (ftype)
	      && GFC_TYPE_ARRAY_AKIND (ftype) == GFC_ARRAY_ALLOCATABLE)
	    tem = gfc_duplicate_allocatable (destf, declf, ftype,
					     GFC_TYPE_ARRAY_RANK (ftype));
	  else if (GFC_DECL_GET_SCALAR_ALLOCATABLE (field))
	    tem = gfc_duplicate_allocatable (destf, declf, ftype, 0);
	  break;
	}
      if (tem)
	gfc_add_expr_to_block (&block, gfc_omp_unshare_expr (tem));
      if (has_alloc_comps)
	{
	  gfc_init_block (&tmpblock);
	  gfc_add_expr_to_block (&tmpblock,
				 gfc_walk_alloc_comps (declf, destf,
						       field, kind));
	  then_b = gfc_finish_block (&tmpblock);
	  if (GFC_DESCRIPTOR_TYPE_P (ftype)
	      && GFC_TYPE_ARRAY_AKIND (ftype) == GFC_ARRAY_ALLOCATABLE)
	    tem = gfc_conv_descriptor_data_get (unshare_expr (declf));
	  else if (GFC_DECL_GET_SCALAR_ALLOCATABLE (field))
	    tem = unshare_expr (declf);
	  else
	    tem = NULL_TREE;
	  if (tem)
	    {
	      tem = fold_convert (pvoid_type_node, tem);
	      tem = fold_build2_loc (input_location, NE_EXPR,
				     boolean_type_node, tem,
				     null_pointer_node);
	      then_b = build3_loc (input_location, COND_EXPR, void_type_node,
				   tem, then_b,
				   build_empty_stmt (input_location));
	    }
	  gfc_add_expr_to_block (&block, then_b);
	}
      if (kind == WALK_ALLOC_COMPS_DTOR)
	{
	  if (GFC_DESCRIPTOR_TYPE_P (ftype)
	      && GFC_TYPE_ARRAY_AKIND (ftype) == GFC_ARRAY_ALLOCATABLE)
	    {
	      tem = gfc_trans_dealloc_allocated (unshare_expr (declf),
						 false, NULL);
	      gfc_add_expr_to_block (&block, gfc_omp_unshare_expr (tem));
	    }
	  else if (GFC_DECL_GET_SCALAR_ALLOCATABLE (field))
	    {
	      tem = gfc_call_free (unshare_expr (declf));
	      gfc_add_expr_to_block (&block, gfc_omp_unshare_expr (tem));
	    }
	}
    }

  return gfc_finish_block (&block);
}

/* Return code to initialize DECL with its default constructor, or
   NULL if there's nothing to do.  */

tree
gfc_omp_clause_default_ctor (tree clause, tree decl, tree outer)
{
  tree type = TREE_TYPE (decl), size, ptr, cond, then_b, else_b;
  stmtblock_t block, cond_block;

  gcc_assert (OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_PRIVATE
	      || OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_LASTPRIVATE
	      || OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_LINEAR
	      || OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_REDUCTION);

  if ((! GFC_DESCRIPTOR_TYPE_P (type)
       || GFC_TYPE_ARRAY_AKIND (type) != GFC_ARRAY_ALLOCATABLE)
      && !GFC_DECL_GET_SCALAR_ALLOCATABLE (OMP_CLAUSE_DECL (clause)))
    {
      if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
	{
	  gcc_assert (outer);
	  gfc_start_block (&block);
	  tree tem = gfc_walk_alloc_comps (outer, decl,
					   OMP_CLAUSE_DECL (clause),
					   WALK_ALLOC_COMPS_DEFAULT_CTOR);
	  gfc_add_expr_to_block (&block, tem);
	  return gfc_finish_block (&block);
	}
      return NULL_TREE;
    }

  gcc_assert (outer != NULL_TREE);

  /* Allocatable arrays and scalars in PRIVATE clauses need to be set to
     "not currently allocated" allocation status if outer
     array is "not currently allocated", otherwise should be allocated.  */
  gfc_start_block (&block);

  gfc_init_block (&cond_block);

  if (GFC_DESCRIPTOR_TYPE_P (type))
    {
      gfc_add_modify (&cond_block, decl, outer);
      tree rank = gfc_rank_cst[GFC_TYPE_ARRAY_RANK (type) - 1];
      size = gfc_conv_descriptor_ubound_get (decl, rank);
      size = fold_build2_loc (input_location, MINUS_EXPR, gfc_array_index_type,
			      size,
			      gfc_conv_descriptor_lbound_get (decl, rank));
      size = fold_build2_loc (input_location, PLUS_EXPR, gfc_array_index_type,
			      size, gfc_index_one_node);
      if (GFC_TYPE_ARRAY_RANK (type) > 1)
	size = fold_build2_loc (input_location, MULT_EXPR,
				gfc_array_index_type, size,
				gfc_conv_descriptor_stride_get (decl, rank));
      tree esize = fold_convert (gfc_array_index_type,
				 TYPE_SIZE_UNIT (gfc_get_element_type (type)));
      size = fold_build2_loc (input_location, MULT_EXPR, gfc_array_index_type,
			      size, esize);
      size = unshare_expr (size);
      size = gfc_evaluate_now (fold_convert (size_type_node, size),
			       &cond_block);
    }
  else
    size = fold_convert (size_type_node, TYPE_SIZE_UNIT (TREE_TYPE (type)));
  ptr = gfc_create_var (pvoid_type_node, NULL);
  gfc_allocate_using_malloc (&cond_block, ptr, size, NULL_TREE);
  if (GFC_DESCRIPTOR_TYPE_P (type))
    gfc_conv_descriptor_data_set (&cond_block, unshare_expr (decl), ptr);
  else
    gfc_add_modify (&cond_block, unshare_expr (decl),
		    fold_convert (TREE_TYPE (decl), ptr));
  if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
    {
      tree tem = gfc_walk_alloc_comps (outer, decl,
				       OMP_CLAUSE_DECL (clause),
				       WALK_ALLOC_COMPS_DEFAULT_CTOR);
      gfc_add_expr_to_block (&cond_block, tem);
    }
  then_b = gfc_finish_block (&cond_block);

  /* Reduction clause requires allocated ALLOCATABLE.  */
  if (OMP_CLAUSE_CODE (clause) != OMP_CLAUSE_REDUCTION)
    {
      gfc_init_block (&cond_block);
      if (GFC_DESCRIPTOR_TYPE_P (type))
	gfc_conv_descriptor_data_set (&cond_block, unshare_expr (decl),
				      null_pointer_node);
      else
	gfc_add_modify (&cond_block, unshare_expr (decl),
			build_zero_cst (TREE_TYPE (decl)));
      else_b = gfc_finish_block (&cond_block);

      tree tem = fold_convert (pvoid_type_node,
			       GFC_DESCRIPTOR_TYPE_P (type)
			       ? gfc_conv_descriptor_data_get (outer) : outer);
      tem = unshare_expr (tem);
      cond = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
			      tem, null_pointer_node);
      gfc_add_expr_to_block (&block,
			     build3_loc (input_location, COND_EXPR,
					 void_type_node, cond, then_b,
					 else_b));
    }
  else
    gfc_add_expr_to_block (&block, then_b);

  return gfc_finish_block (&block);
}

/* Build and return code for a copy constructor from SRC to DEST.  */

tree
gfc_omp_clause_copy_ctor (tree clause, tree dest, tree src)
{
  tree type = TREE_TYPE (dest), ptr, size, call;
  tree cond, then_b, else_b;
  stmtblock_t block, cond_block;

  gcc_assert (OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_FIRSTPRIVATE
	      || OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_LINEAR);

  if ((! GFC_DESCRIPTOR_TYPE_P (type)
       || GFC_TYPE_ARRAY_AKIND (type) != GFC_ARRAY_ALLOCATABLE)
      && !GFC_DECL_GET_SCALAR_ALLOCATABLE (OMP_CLAUSE_DECL (clause)))
    {
      if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
	{
	  gfc_start_block (&block);
	  gfc_add_modify (&block, dest, src);
	  tree tem = gfc_walk_alloc_comps (src, dest, OMP_CLAUSE_DECL (clause),
					   WALK_ALLOC_COMPS_COPY_CTOR);
	  gfc_add_expr_to_block (&block, tem);
	  return gfc_finish_block (&block);
	}
      else
	return build2_v (MODIFY_EXPR, dest, src);
    }

  /* Allocatable arrays in FIRSTPRIVATE clauses need to be allocated
     and copied from SRC.  */
  gfc_start_block (&block);

  gfc_init_block (&cond_block);

  gfc_add_modify (&cond_block, dest, src);
  if (GFC_DESCRIPTOR_TYPE_P (type))
    {
      tree rank = gfc_rank_cst[GFC_TYPE_ARRAY_RANK (type) - 1];
      size = gfc_conv_descriptor_ubound_get (dest, rank);
      size = fold_build2_loc (input_location, MINUS_EXPR, gfc_array_index_type,
			      size,
			      gfc_conv_descriptor_lbound_get (dest, rank));
      size = fold_build2_loc (input_location, PLUS_EXPR, gfc_array_index_type,
			      size, gfc_index_one_node);
      if (GFC_TYPE_ARRAY_RANK (type) > 1)
	size = fold_build2_loc (input_location, MULT_EXPR,
				gfc_array_index_type, size,
				gfc_conv_descriptor_stride_get (dest, rank));
      tree esize = fold_convert (gfc_array_index_type,
				 TYPE_SIZE_UNIT (gfc_get_element_type (type)));
      size = fold_build2_loc (input_location, MULT_EXPR, gfc_array_index_type,
			      size, esize);
      size = unshare_expr (size);
      size = gfc_evaluate_now (fold_convert (size_type_node, size),
			       &cond_block);
    }
  else
    size = fold_convert (size_type_node, TYPE_SIZE_UNIT (TREE_TYPE (type)));
  ptr = gfc_create_var (pvoid_type_node, NULL);
  gfc_allocate_using_malloc (&cond_block, ptr, size, NULL_TREE);
  if (GFC_DESCRIPTOR_TYPE_P (type))
    gfc_conv_descriptor_data_set (&cond_block, unshare_expr (dest), ptr);
  else
    gfc_add_modify (&cond_block, unshare_expr (dest),
		    fold_convert (TREE_TYPE (dest), ptr));

  tree srcptr = GFC_DESCRIPTOR_TYPE_P (type)
		? gfc_conv_descriptor_data_get (src) : src;
  srcptr = unshare_expr (srcptr);
  srcptr = fold_convert (pvoid_type_node, srcptr);
  call = build_call_expr_loc (input_location,
			      builtin_decl_explicit (BUILT_IN_MEMCPY), 3, ptr,
			      srcptr, size);
  gfc_add_expr_to_block (&cond_block, fold_convert (void_type_node, call));
  if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
    {
      tree tem = gfc_walk_alloc_comps (src, dest,
				       OMP_CLAUSE_DECL (clause),
				       WALK_ALLOC_COMPS_COPY_CTOR);
      gfc_add_expr_to_block (&cond_block, tem);
    }
  then_b = gfc_finish_block (&cond_block);

  gfc_init_block (&cond_block);
  if (GFC_DESCRIPTOR_TYPE_P (type))
    gfc_conv_descriptor_data_set (&cond_block, unshare_expr (dest),
				  null_pointer_node);
  else
    gfc_add_modify (&cond_block, unshare_expr (dest),
		    build_zero_cst (TREE_TYPE (dest)));
  else_b = gfc_finish_block (&cond_block);

  cond = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
			  unshare_expr (srcptr), null_pointer_node);
  gfc_add_expr_to_block (&block,
			 build3_loc (input_location, COND_EXPR,
				     void_type_node, cond, then_b, else_b));

  return gfc_finish_block (&block);
}

/* Similarly, except use an intrinsic or pointer assignment operator
   instead.  */

tree
gfc_omp_clause_assign_op (tree clause, tree dest, tree src)
{
  tree type = TREE_TYPE (dest), ptr, size, call, nonalloc;
  tree cond, then_b, else_b;
  stmtblock_t block, cond_block, cond_block2, inner_block;

  if ((! GFC_DESCRIPTOR_TYPE_P (type)
       || GFC_TYPE_ARRAY_AKIND (type) != GFC_ARRAY_ALLOCATABLE)
      && !GFC_DECL_GET_SCALAR_ALLOCATABLE (OMP_CLAUSE_DECL (clause)))
    {
      if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
	{
	  gfc_start_block (&block);
	  /* First dealloc any allocatable components in DEST.  */
	  tree tem = gfc_walk_alloc_comps (dest, NULL_TREE,
					   OMP_CLAUSE_DECL (clause),
					   WALK_ALLOC_COMPS_DTOR);
	  gfc_add_expr_to_block (&block, tem);
	  /* Then copy over toplevel data.  */
	  gfc_add_modify (&block, dest, src);
	  /* Finally allocate any allocatable components and copy.  */
	  tem = gfc_walk_alloc_comps (src, dest, OMP_CLAUSE_DECL (clause),
					   WALK_ALLOC_COMPS_COPY_CTOR);
	  gfc_add_expr_to_block (&block, tem);
	  return gfc_finish_block (&block);
	}
      else
	return build2_v (MODIFY_EXPR, dest, src);
    }

  gfc_start_block (&block);

  if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
    {
      then_b = gfc_walk_alloc_comps (dest, NULL_TREE, OMP_CLAUSE_DECL (clause),
				     WALK_ALLOC_COMPS_DTOR);
      tree tem = fold_convert (pvoid_type_node,
			       GFC_DESCRIPTOR_TYPE_P (type)
			       ? gfc_conv_descriptor_data_get (dest) : dest);
      tem = unshare_expr (tem);
      cond = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
			      tem, null_pointer_node);
      tem = build3_loc (input_location, COND_EXPR, void_type_node, cond,
			then_b, build_empty_stmt (input_location));
      gfc_add_expr_to_block (&block, tem);
    }

  gfc_init_block (&cond_block);

  if (GFC_DESCRIPTOR_TYPE_P (type))
    {
      tree rank = gfc_rank_cst[GFC_TYPE_ARRAY_RANK (type) - 1];
      size = gfc_conv_descriptor_ubound_get (src, rank);
      size = fold_build2_loc (input_location, MINUS_EXPR, gfc_array_index_type,
			      size,
			      gfc_conv_descriptor_lbound_get (src, rank));
      size = fold_build2_loc (input_location, PLUS_EXPR, gfc_array_index_type,
			      size, gfc_index_one_node);
      if (GFC_TYPE_ARRAY_RANK (type) > 1)
	size = fold_build2_loc (input_location, MULT_EXPR,
				gfc_array_index_type, size,
				gfc_conv_descriptor_stride_get (src, rank));
      tree esize = fold_convert (gfc_array_index_type,
				 TYPE_SIZE_UNIT (gfc_get_element_type (type)));
      size = fold_build2_loc (input_location, MULT_EXPR, gfc_array_index_type,
			      size, esize);
      size = unshare_expr (size);
      size = gfc_evaluate_now (fold_convert (size_type_node, size),
			       &cond_block);
    }
  else
    size = fold_convert (size_type_node, TYPE_SIZE_UNIT (TREE_TYPE (type)));
  ptr = gfc_create_var (pvoid_type_node, NULL);

  tree destptr = GFC_DESCRIPTOR_TYPE_P (type)
		 ? gfc_conv_descriptor_data_get (dest) : dest;
  destptr = unshare_expr (destptr);
  destptr = fold_convert (pvoid_type_node, destptr);
  gfc_add_modify (&cond_block, ptr, destptr);

  nonalloc = fold_build2_loc (input_location, EQ_EXPR, boolean_type_node,
			      destptr, null_pointer_node);
  cond = nonalloc;
  if (GFC_DESCRIPTOR_TYPE_P (type))
    {
      int i;
      for (i = 0; i < GFC_TYPE_ARRAY_RANK (type); i++)
	{
	  tree rank = gfc_rank_cst[i];
	  tree tem = gfc_conv_descriptor_ubound_get (src, rank);
	  tem = fold_build2_loc (input_location, MINUS_EXPR,
				 gfc_array_index_type, tem,
				 gfc_conv_descriptor_lbound_get (src, rank));
	  tem = fold_build2_loc (input_location, PLUS_EXPR,
				 gfc_array_index_type, tem,
				 gfc_conv_descriptor_lbound_get (dest, rank));
	  tem = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
				 tem, gfc_conv_descriptor_ubound_get (dest,
								      rank));
	  cond = fold_build2_loc (input_location, TRUTH_ORIF_EXPR,
				  boolean_type_node, cond, tem);
	}
    }

  gfc_init_block (&cond_block2);

  if (GFC_DESCRIPTOR_TYPE_P (type))
    {
      gfc_init_block (&inner_block);
      gfc_allocate_using_malloc (&inner_block, ptr, size, NULL_TREE);
      then_b = gfc_finish_block (&inner_block);

      gfc_init_block (&inner_block);
      gfc_add_modify (&inner_block, ptr,
		      gfc_call_realloc (&inner_block, ptr, size));
      else_b = gfc_finish_block (&inner_block);

      gfc_add_expr_to_block (&cond_block2,
			     build3_loc (input_location, COND_EXPR,
					 void_type_node,
					 unshare_expr (nonalloc),
					 then_b, else_b));
      gfc_add_modify (&cond_block2, dest, src);
      gfc_conv_descriptor_data_set (&cond_block2, unshare_expr (dest), ptr);
    }
  else
    {
      gfc_allocate_using_malloc (&cond_block2, ptr, size, NULL_TREE);
      gfc_add_modify (&cond_block2, unshare_expr (dest),
		      fold_convert (type, ptr));
    }
  then_b = gfc_finish_block (&cond_block2);
  else_b = build_empty_stmt (input_location);

  gfc_add_expr_to_block (&cond_block,
			 build3_loc (input_location, COND_EXPR,
				     void_type_node, unshare_expr (cond),
				     then_b, else_b));

  tree srcptr = GFC_DESCRIPTOR_TYPE_P (type)
		? gfc_conv_descriptor_data_get (src) : src;
  srcptr = unshare_expr (srcptr);
  srcptr = fold_convert (pvoid_type_node, srcptr);
  call = build_call_expr_loc (input_location,
			      builtin_decl_explicit (BUILT_IN_MEMCPY), 3, ptr,
			      srcptr, size);
  gfc_add_expr_to_block (&cond_block, fold_convert (void_type_node, call));
  if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
    {
      tree tem = gfc_walk_alloc_comps (src, dest,
				       OMP_CLAUSE_DECL (clause),
				       WALK_ALLOC_COMPS_COPY_CTOR);
      gfc_add_expr_to_block (&cond_block, tem);
    }
  then_b = gfc_finish_block (&cond_block);

  if (OMP_CLAUSE_CODE (clause) == OMP_CLAUSE_COPYIN)
    {
      gfc_init_block (&cond_block);
      if (GFC_DESCRIPTOR_TYPE_P (type))
	gfc_add_expr_to_block (&cond_block,
			       gfc_trans_dealloc_allocated (unshare_expr (dest),
							    false, NULL));
      else
	{
	  destptr = gfc_evaluate_now (destptr, &cond_block);
	  gfc_add_expr_to_block (&cond_block, gfc_call_free (destptr));
	  gfc_add_modify (&cond_block, unshare_expr (dest),
			  build_zero_cst (TREE_TYPE (dest)));
	}
      else_b = gfc_finish_block (&cond_block);

      cond = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
			      unshare_expr (srcptr), null_pointer_node);
      gfc_add_expr_to_block (&block,
			     build3_loc (input_location, COND_EXPR,
					 void_type_node, cond,
					 then_b, else_b));
    }
  else
    gfc_add_expr_to_block (&block, then_b);

  return gfc_finish_block (&block);
}

/* Build and return code destructing DECL.  Return NULL if nothing
   to be done.  */

tree
gfc_omp_clause_dtor (tree clause, tree decl)
{
  tree type = TREE_TYPE (decl), tem;

  if ((! GFC_DESCRIPTOR_TYPE_P (type)
       || GFC_TYPE_ARRAY_AKIND (type) != GFC_ARRAY_ALLOCATABLE)
      && !GFC_DECL_GET_SCALAR_ALLOCATABLE (OMP_CLAUSE_DECL (clause)))
    {
      if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
	return gfc_walk_alloc_comps (decl, NULL_TREE,
				     OMP_CLAUSE_DECL (clause),
				     WALK_ALLOC_COMPS_DTOR);
      return NULL_TREE;
    }

  if (GFC_DESCRIPTOR_TYPE_P (type))
    /* Allocatable arrays in FIRSTPRIVATE/LASTPRIVATE etc. clauses need
       to be deallocated if they were allocated.  */
    tem = gfc_trans_dealloc_allocated (decl, false, NULL);
  else
    tem = gfc_call_free (decl);
  tem = gfc_omp_unshare_expr (tem);

  if (gfc_has_alloc_comps (type, OMP_CLAUSE_DECL (clause)))
    {
      stmtblock_t block;
      tree then_b;

      gfc_init_block (&block);
      gfc_add_expr_to_block (&block,
			     gfc_walk_alloc_comps (decl, NULL_TREE,
						   OMP_CLAUSE_DECL (clause),
						   WALK_ALLOC_COMPS_DTOR));
      gfc_add_expr_to_block (&block, tem);
      then_b = gfc_finish_block (&block);

      tem = fold_convert (pvoid_type_node,
			  GFC_DESCRIPTOR_TYPE_P (type)
			  ? gfc_conv_descriptor_data_get (decl) : decl);
      tem = unshare_expr (tem);
      tree cond = fold_build2_loc (input_location, NE_EXPR, boolean_type_node,
				   tem, null_pointer_node);
      tem = build3_loc (input_location, COND_EXPR, void_type_node, cond,
			then_b, build_empty_stmt (input_location));
    }
  return tem;
}


/* Return true if DECL's DECL_VALUE_EXPR (if any) should be
   disregarded in OpenMP construct, because it is going to be
   remapped during OpenMP lowering.  SHARED is true if DECL
   is going to be shared, false if it is going to be privatized.  */

bool
gfc_omp_disregard_value_expr (tree decl, bool shared)
{
  if (GFC_DECL_COMMON_OR_EQUIV (decl)
      && DECL_HAS_VALUE_EXPR_P (decl))
    {
      tree value = DECL_VALUE_EXPR (decl);

      if (TREE_CODE (value) == COMPONENT_REF
	  && TREE_CODE (TREE_OPERAND (value, 0)) == VAR_DECL
	  && GFC_DECL_COMMON_OR_EQUIV (TREE_OPERAND (value, 0)))
	{
	  /* If variable in COMMON or EQUIVALENCE is privatized, return
	     true, as just that variable is supposed to be privatized,
	     not the whole COMMON or whole EQUIVALENCE.
	     For shared variables in COMMON or EQUIVALENCE, let them be
	     gimplified to DECL_VALUE_EXPR, so that for multiple shared vars
	     from the same COMMON or EQUIVALENCE just one sharing of the
	     whole COMMON or EQUIVALENCE is enough.  */
	  return ! shared;
	}
    }

  if (GFC_DECL_RESULT (decl) && DECL_HAS_VALUE_EXPR_P (decl))
    return ! shared;

  return false;
}

/* Return true if DECL that is shared iff SHARED is true should
   be put into OMP_CLAUSE_PRIVATE with OMP_CLAUSE_PRIVATE_DEBUG
   flag set.  */

bool
gfc_omp_private_debug_clause (tree decl, bool shared)
{
  if (GFC_DECL_CRAY_POINTEE (decl))
    return true;

  if (GFC_DECL_COMMON_OR_EQUIV (decl)
      && DECL_HAS_VALUE_EXPR_P (decl))
    {
      tree value = DECL_VALUE_EXPR (decl);

      if (TREE_CODE (value) == COMPONENT_REF
	  && TREE_CODE (TREE_OPERAND (value, 0)) == VAR_DECL
	  && GFC_DECL_COMMON_OR_EQUIV (TREE_OPERAND (value, 0)))
	return shared;
    }

  return false;
}

/* Register language specific type size variables as potentially OpenMP
   firstprivate variables.  */

void
gfc_omp_firstprivatize_type_sizes (struct gimplify_omp_ctx *ctx, tree type)
{
  if (GFC_ARRAY_TYPE_P (type) || GFC_DESCRIPTOR_TYPE_P (type))
    {
      int r;

      gcc_assert (TYPE_LANG_SPECIFIC (type) != NULL);
      for (r = 0; r < GFC_TYPE_ARRAY_RANK (type); r++)
	{
	  omp_firstprivatize_variable (ctx, GFC_TYPE_ARRAY_LBOUND (type, r));
	  omp_firstprivatize_variable (ctx, GFC_TYPE_ARRAY_UBOUND (type, r));
	  omp_firstprivatize_variable (ctx, GFC_TYPE_ARRAY_STRIDE (type, r));
	}
      omp_firstprivatize_variable (ctx, GFC_TYPE_ARRAY_SIZE (type));
      omp_firstprivatize_variable (ctx, GFC_TYPE_ARRAY_OFFSET (type));
    }
}


static inline tree
gfc_trans_add_clause (tree node, tree tail)
{
  OMP_CLAUSE_CHAIN (node) = tail;
  return node;
}

static tree
gfc_trans_omp_variable (gfc_symbol *sym, bool declare_simd)
{
  if (declare_simd)
    {
      int cnt = 0;
      gfc_symbol *proc_sym;
      gfc_formal_arglist *f;

      gcc_assert (sym->attr.dummy);
      proc_sym = sym->ns->proc_name;
      if (proc_sym->attr.entry_master)
	++cnt;
      if (gfc_return_by_reference (proc_sym))
	{
	  ++cnt;
	  if (proc_sym->ts.type == BT_CHARACTER)
	    ++cnt;
	}
      for (f = gfc_sym_get_dummy_args (proc_sym); f; f = f->next)
	if (f->sym == sym)
	  break;
	else if (f->sym)
	  ++cnt;
      gcc_assert (f);
      return build_int_cst (integer_type_node, cnt);
    }

  tree t = gfc_get_symbol_decl (sym);
  tree parent_decl;
  int parent_flag;
  bool return_value;
  bool alternate_entry;
  bool entry_master;

  return_value = sym->attr.function && sym->result == sym;
  alternate_entry = sym->attr.function && sym->attr.entry
		    && sym->result == sym;
  entry_master = sym->attr.result
		 && sym->ns->proc_name->attr.entry_master
		 && !gfc_return_by_reference (sym->ns->proc_name);
  parent_decl = current_function_decl
		? DECL_CONTEXT (current_function_decl) : NULL_TREE;

  if ((t == parent_decl && return_value)
       || (sym->ns && sym->ns->proc_name
	   && sym->ns->proc_name->backend_decl == parent_decl
	   && (alternate_entry || entry_master)))
    parent_flag = 1;
  else
    parent_flag = 0;

  /* Special case for assigning the return value of a function.
     Self recursive functions must have an explicit return value.  */
  if (return_value && (t == current_function_decl || parent_flag))
    t = gfc_get_fake_result_decl (sym, parent_flag);

  /* Similarly for alternate entry points.  */
  else if (alternate_entry
	   && (sym->ns->proc_name->backend_decl == current_function_decl
	       || parent_flag))
    {
      gfc_entry_list *el = NULL;

      for (el = sym->ns->entries; el; el = el->next)
	if (sym == el->sym)
	  {
	    t = gfc_get_fake_result_decl (sym, parent_flag);
	    break;
	  }
    }

  else if (entry_master
	   && (sym->ns->proc_name->backend_decl == current_function_decl
	       || parent_flag))
    t = gfc_get_fake_result_decl (sym, parent_flag);

  return t;
}

static tree
gfc_trans_omp_variable_list (enum omp_clause_code code,
			     gfc_omp_namelist *namelist, tree list,
			     bool declare_simd)
{
  for (; namelist != NULL; namelist = namelist->next)
    if (namelist->sym->attr.referenced || declare_simd)
      {
	tree t = gfc_trans_omp_variable (namelist->sym, declare_simd);
	if (t != error_mark_node)
	  {
	    tree node = build_omp_clause (input_location, code);
	    OMP_CLAUSE_DECL (node) = t;
	    list = gfc_trans_add_clause (node, list);
	  }
      }
  return list;
}

struct omp_udr_find_orig_data
{
  gfc_omp_udr *omp_udr;
  bool omp_orig_seen;
};

static int
omp_udr_find_orig (gfc_expr **e, int *walk_subtrees ATTRIBUTE_UNUSED,
		   void *data)
{
  struct omp_udr_find_orig_data *cd = (struct omp_udr_find_orig_data *) data;
  if ((*e)->expr_type == EXPR_VARIABLE
      && (*e)->symtree->n.sym == cd->omp_udr->omp_orig)
    cd->omp_orig_seen = true;

  return 0;
}

static tree
gfc_trans_omp_udr_expr (gfc_omp_namelist *n, bool is_initializer,
			gfc_expr *syme, gfc_expr *outere)
{
  gfc_se symse, outerse;
  gfc_ss *symss, *outerss;
  gfc_loopinfo loop;
  stmtblock_t block, body;
  tree tem;
  int i;
  gfc_namespace *ns = (is_initializer
		       ? n->udr->initializer_ns : n->udr->combiner_ns);

  syme = gfc_copy_expr (syme);
  outere = gfc_copy_expr (outere);
  gfc_init_se (&symse, NULL);
  gfc_init_se (&outerse, NULL);
  gfc_start_block (&block);
  gfc_init_loopinfo (&loop);
  symss = gfc_walk_expr (syme);
  outerss = gfc_walk_expr (outere);
  gfc_add_ss_to_loop (&loop, symss);
  gfc_add_ss_to_loop (&loop, outerss);
  gfc_conv_ss_startstride (&loop);
  /* Enable loop reversal.  */
  for (i = 0; i < GFC_MAX_DIMENSIONS; i++)
    loop.reverse[i] = GFC_ENABLE_REVERSE;
  gfc_conv_loop_setup (&loop, &ns->code->loc);
  gfc_copy_loopinfo_to_se (&symse, &loop);
  gfc_copy_loopinfo_to_se (&outerse, &loop);
  symse.ss = symss;
  outerse.ss = outerss;
  gfc_mark_ss_chain_used (symss, 1);
  gfc_mark_ss_chain_used (outerss, 1);
  gfc_start_scalarized_body (&loop, &body);
  gfc_conv_expr (&symse, syme);
  gfc_conv_expr (&outerse, outere);

  if (is_initializer)
    {
      n->udr->omp_priv->backend_decl = symse.expr;
      n->udr->omp_orig->backend_decl = outerse.expr;
    }
  else
    {
      n->udr->omp_out->backend_decl = outerse.expr;
      n->udr->omp_in->backend_decl = symse.expr;
    }

  if (ns->code->op == EXEC_ASSIGN)
    tem = gfc_trans_assignment (ns->code->expr1, ns->code->expr2,
				false, false);
  else
    tem = gfc_trans_call (ns->code, false, NULL_TREE, NULL_TREE, false);
  gfc_add_expr_to_block (&body, tem);

  gcc_assert (symse.ss == gfc_ss_terminator
	      && outerse.ss == gfc_ss_terminator);
  /* Generate the copying loops.  */
  gfc_trans_scalarizing_loops (&loop, &body);

  /* Wrap the whole thing up.  */
  gfc_add_block_to_block (&block, &loop.pre);
  gfc_add_block_to_block (&block, &loop.post);

  gfc_cleanup_loop (&loop);
  gfc_free_expr (syme);
  gfc_free_expr (outere);

  return gfc_finish_block (&block);
}

static void
gfc_trans_omp_array_reduction_or_udr (tree c, gfc_omp_namelist *n, locus where)
{
  gfc_symbol *sym = n->sym;
  gfc_symtree *root1 = NULL, *root2 = NULL, *root3 = NULL, *root4 = NULL;
  gfc_symtree *symtree1, *symtree2, *symtree3, *symtree4 = NULL;
  gfc_symbol init_val_sym, outer_sym, intrinsic_sym;
  gfc_symbol omp_var_copy[4];
  gfc_expr *e1, *e2, *e3, *e4;
  gfc_ref *ref;
  tree decl, backend_decl, stmt, type, outer_decl;
  locus old_loc = gfc_current_locus;
  const char *iname;
  bool t;

  decl = OMP_CLAUSE_DECL (c);
  gfc_current_locus = where;
  type = TREE_TYPE (decl);
  outer_decl = create_tmp_var_raw (type, NULL);
  if (TREE_CODE (decl) == PARM_DECL
      && TREE_CODE (type) == REFERENCE_TYPE
      && GFC_DESCRIPTOR_TYPE_P (TREE_TYPE (type))
      && GFC_TYPE_ARRAY_AKIND (TREE_TYPE (type)) == GFC_ARRAY_ALLOCATABLE)
    {
      decl = build_fold_indirect_ref (decl);
      type = TREE_TYPE (type);
    }

  /* Create a fake symbol for init value.  */
  memset (&init_val_sym, 0, sizeof (init_val_sym));
  init_val_sym.ns = sym->ns;
  init_val_sym.name = sym->name;
  init_val_sym.ts = sym->ts;
  init_val_sym.attr.referenced = 1;
  init_val_sym.declared_at = where;
  init_val_sym.attr.flavor = FL_VARIABLE;
  if (OMP_CLAUSE_REDUCTION_CODE (c) != ERROR_MARK)
    backend_decl = omp_reduction_init (c, gfc_sym_type (&init_val_sym));
  else if (n->udr->initializer_ns)
    backend_decl = NULL;
  else
    switch (sym->ts.type)
      {
      case BT_LOGICAL:
      case BT_INTEGER:
      case BT_REAL:
      case BT_COMPLEX:
	backend_decl = build_zero_cst (gfc_sym_type (&init_val_sym));
	break;
      default:
	backend_decl = NULL_TREE;
	break;
      }
  init_val_sym.backend_decl = backend_decl;

  /* Create a fake symbol for the outer array reference.  */
  outer_sym = *sym;
  if (sym->as)
    outer_sym.as = gfc_copy_array_spec (sym->as);
  outer_sym.attr.dummy = 0;
  outer_sym.attr.result = 0;
  outer_sym.attr.flavor = FL_VARIABLE;
  outer_sym.backend_decl = outer_decl;
  if (decl != OMP_CLAUSE_DECL (c))
    outer_sym.backend_decl = build_fold_indirect_ref (outer_decl);

  /* Create fake symtrees for it.  */
  symtree1 = gfc_new_symtree (&root1, sym->name);
  symtree1->n.sym = sym;
  gcc_assert (symtree1 == root1);

  symtree2 = gfc_new_symtree (&root2, sym->name);
  symtree2->n.sym = &init_val_sym;
  gcc_assert (symtree2 == root2);

  symtree3 = gfc_new_symtree (&root3, sym->name);
  symtree3->n.sym = &outer_sym;
  gcc_assert (symtree3 == root3);

  memset (omp_var_copy, 0, sizeof omp_var_copy);
  if (n->udr)
    {
      omp_var_copy[0] = *n->udr->omp_out;
      omp_var_copy[1] = *n->udr->omp_in;
      if (sym->attr.dimension)
	{
	  n->udr->omp_out->ts = sym->ts;
	  n->udr->omp_in->ts = sym->ts;
	}
      else
	{
	  *n->udr->omp_out = outer_sym;
	  *n->udr->omp_in = *sym;
	}
      if (n->udr->initializer_ns)
	{
	  omp_var_copy[2] = *n->udr->omp_priv;
	  omp_var_copy[3] = *n->udr->omp_orig;
	  if (sym->attr.dimension)
	    {
	      n->udr->omp_priv->ts = sym->ts;
	      n->udr->omp_orig->ts = sym->ts;
	    }
	  else
	    {
	      *n->udr->omp_priv = *sym;
	      *n->udr->omp_orig = outer_sym;
	    }
	}
    }

  /* Create expressions.  */
  e1 = gfc_get_expr ();
  e1->expr_type = EXPR_VARIABLE;
  e1->where = where;
  e1->symtree = symtree1;
  e1->ts = sym->ts;
  if (sym->attr.dimension)
    {
      e1->ref = ref = gfc_get_ref ();
      ref->type = REF_ARRAY;
      ref->u.ar.where = where;
      ref->u.ar.as = sym->as;
      ref->u.ar.type = AR_FULL;
      ref->u.ar.dimen = 0;
    }
  t = gfc_resolve_expr (e1);
  gcc_assert (t);

  e2 = NULL;
  if (backend_decl != NULL_TREE)
    {
      e2 = gfc_get_expr ();
      e2->expr_type = EXPR_VARIABLE;
      e2->where = where;
      e2->symtree = symtree2;
      e2->ts = sym->ts;
      t = gfc_resolve_expr (e2);
      gcc_assert (t);
    }
  else if (n->udr->initializer_ns == NULL)
    {
      gcc_assert (sym->ts.type == BT_DERIVED);
      e2 = gfc_default_initializer (&sym->ts);
      gcc_assert (e2);
      t = gfc_resolve_expr (e2);
      gcc_assert (t);
    }
  else if (n->udr->initializer_ns->code->op == EXEC_ASSIGN)
    {
      if (!sym->attr.dimension)
	{
	  e2 = gfc_copy_expr (n->udr->initializer_ns->code->expr2);
	  t = gfc_resolve_expr (e2);
	  gcc_assert (t);
	}
    }
  if (n->udr && n->udr->initializer_ns)
    {
      struct omp_udr_find_orig_data cd;
      cd.omp_udr = n->udr;
      cd.omp_orig_seen = false;
      gfc_code_walker (&n->udr->initializer_ns->code,
		       gfc_dummy_code_callback, omp_udr_find_orig, &cd);
      if (cd.omp_orig_seen)
	OMP_CLAUSE_REDUCTION_OMP_ORIG_REF (c) = 1;
    }

  e3 = gfc_copy_expr (e1);
  e3->symtree = symtree3;
  t = gfc_resolve_expr (e3);
  gcc_assert (t);

  iname = NULL;
  e4 = NULL;
  switch (OMP_CLAUSE_REDUCTION_CODE (c))
    {
    case PLUS_EXPR:
    case MINUS_EXPR:
      e4 = gfc_add (e3, e1);
      break;
    case MULT_EXPR:
      e4 = gfc_multiply (e3, e1);
      break;
    case TRUTH_ANDIF_EXPR:
      e4 = gfc_and (e3, e1);
      break;
    case TRUTH_ORIF_EXPR:
      e4 = gfc_or (e3, e1);
      break;
    case EQ_EXPR:
      e4 = gfc_eqv (e3, e1);
      break;
    case NE_EXPR:
      e4 = gfc_neqv (e3, e1);
      break;
    case MIN_EXPR:
      iname = "min";
      break;
    case MAX_EXPR:
      iname = "max";
      break;
    case BIT_AND_EXPR:
      iname = "iand";
      break;
    case BIT_IOR_EXPR:
      iname = "ior";
      break;
    case BIT_XOR_EXPR:
      iname = "ieor";
      break;
    case ERROR_MARK:
      if (n->udr->combiner_ns->code->op == EXEC_ASSIGN)
	{
	  if (!sym->attr.dimension)
	    {
	      gfc_free_expr (e3);
	      e3 = gfc_copy_expr (n->udr->combiner_ns->code->expr1);
	      e4 = gfc_copy_expr (n->udr->combiner_ns->code->expr2);
	      t = gfc_resolve_expr (e3);
	      gcc_assert (t);
	      t = gfc_resolve_expr (e4);
	      gcc_assert (t);
	    }
	}
      break;
    default:
      gcc_unreachable ();
    }
  if (iname != NULL)
    {
      memset (&intrinsic_sym, 0, sizeof (intrinsic_sym));
      intrinsic_sym.ns = sym->ns;
      intrinsic_sym.name = iname;
      intrinsic_sym.ts = sym->ts;
      intrinsic_sym.attr.referenced = 1;
      intrinsic_sym.attr.intrinsic = 1;
      intrinsic_sym.attr.function = 1;
      intrinsic_sym.result = &intrinsic_sym;
      intrinsic_sym.declared_at = where;

      symtree4 = gfc_new_symtree (&root4, iname);
      symtree4->n.sym = &intrinsic_sym;
      gcc_assert (symtree4 == root4);

      e4 = gfc_get_expr ();
      e4->expr_type = EXPR_FUNCTION;
      e4->where = where;
      e4->symtree = symtree4;
      e4->value.function.isym = gfc_find_function (iname);
      e4->value.function.actual = gfc_get_actual_arglist ();
      e4->value.function.actual->expr = e3;
      e4->value.function.actual->next = gfc_get_actual_arglist ();
      e4->value.function.actual->next->expr = e1;
    }
  if (OMP_CLAUSE_REDUCTION_CODE (c) != ERROR_MARK)
    {
      /* e1 and e3 have been stored as arguments of e4, avoid sharing.  */
      e1 = gfc_copy_expr (e1);
      e3 = gfc_copy_expr (e3);
      t = gfc_resolve_expr (e4);
      gcc_assert (t);
    }

  /* Create the init statement list.  */
  pushlevel ();
  if (e2)
    stmt = gfc_trans_assignment (e1, e2, false, false);
  else if (sym->attr.dimension)
    stmt = gfc_trans_omp_udr_expr (n, true, e1, e3);
  else
    stmt = gfc_trans_call (n->udr->initializer_ns->code, false,
			   NULL_TREE, NULL_TREE, false);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  OMP_CLAUSE_REDUCTION_INIT (c) = stmt;

  /* Create the merge statement list.  */
  pushlevel ();
  if (e4)
    stmt = gfc_trans_assignment (e3, e4, false, true);
  else if (sym->attr.dimension)
    stmt = gfc_trans_omp_udr_expr (n, false, e1, e3);
  else
    stmt = gfc_trans_call (n->udr->combiner_ns->code, false,
			   NULL_TREE, NULL_TREE, false);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  OMP_CLAUSE_REDUCTION_MERGE (c) = stmt;

  /* And stick the placeholder VAR_DECL into the clause as well.  */
  OMP_CLAUSE_REDUCTION_PLACEHOLDER (c) = outer_decl;

  gfc_current_locus = old_loc;

  gfc_free_expr (e1);
  if (e2)
    gfc_free_expr (e2);
  gfc_free_expr (e3);
  if (e4)
    gfc_free_expr (e4);
  free (symtree1);
  free (symtree2);
  free (symtree3);
  free (symtree4);
  if (outer_sym.as)
    gfc_free_array_spec (outer_sym.as);

  if (n->udr)
    {
      *n->udr->omp_out = omp_var_copy[0];
      *n->udr->omp_in = omp_var_copy[1];
      if (n->udr->initializer_ns)
	{
	  *n->udr->omp_priv = omp_var_copy[2];
	  *n->udr->omp_orig = omp_var_copy[3];
	}
    }
}

static tree
gfc_trans_omp_reduction_list (gfc_omp_namelist *namelist, tree list,
			      locus where)
{
  for (; namelist != NULL; namelist = namelist->next)
    if (namelist->sym->attr.referenced)
      {
	tree t = gfc_trans_omp_variable (namelist->sym, false);
	if (t != error_mark_node)
	  {
	    tree node = build_omp_clause (where.lb->location,
					  OMP_CLAUSE_REDUCTION);
	    OMP_CLAUSE_DECL (node) = t;
	    switch (namelist->rop)
	      {
	      case OMP_REDUCTION_PLUS:
		OMP_CLAUSE_REDUCTION_CODE (node) = PLUS_EXPR;
		break;
	      case OMP_REDUCTION_MINUS:
		OMP_CLAUSE_REDUCTION_CODE (node) = MINUS_EXPR;
		break;
	      case OMP_REDUCTION_TIMES:
		OMP_CLAUSE_REDUCTION_CODE (node) = MULT_EXPR;
		break;
	      case OMP_REDUCTION_AND:
		OMP_CLAUSE_REDUCTION_CODE (node) = TRUTH_ANDIF_EXPR;
		break;
	      case OMP_REDUCTION_OR:
		OMP_CLAUSE_REDUCTION_CODE (node) = TRUTH_ORIF_EXPR;
		break;
	      case OMP_REDUCTION_EQV:
		OMP_CLAUSE_REDUCTION_CODE (node) = EQ_EXPR;
		break;
	      case OMP_REDUCTION_NEQV:
		OMP_CLAUSE_REDUCTION_CODE (node) = NE_EXPR;
		break;
	      case OMP_REDUCTION_MAX:
		OMP_CLAUSE_REDUCTION_CODE (node) = MAX_EXPR;
		break;
	      case OMP_REDUCTION_MIN:
		OMP_CLAUSE_REDUCTION_CODE (node) = MIN_EXPR;
		break;
 	      case OMP_REDUCTION_IAND:
		OMP_CLAUSE_REDUCTION_CODE (node) = BIT_AND_EXPR;
		break;
 	      case OMP_REDUCTION_IOR:
		OMP_CLAUSE_REDUCTION_CODE (node) = BIT_IOR_EXPR;
		break;
 	      case OMP_REDUCTION_IEOR:
		OMP_CLAUSE_REDUCTION_CODE (node) = BIT_XOR_EXPR;
		break;
	      case OMP_REDUCTION_USER:
		OMP_CLAUSE_REDUCTION_CODE (node) = ERROR_MARK;
		break;
	      default:
		gcc_unreachable ();
	      }
	    if (namelist->sym->attr.dimension
		|| namelist->rop == OMP_REDUCTION_USER
		|| namelist->sym->attr.allocatable)
	      gfc_trans_omp_array_reduction_or_udr (node, namelist, where);
	    list = gfc_trans_add_clause (node, list);
	  }
      }
  return list;
}

static tree
gfc_trans_omp_map_clause_list (enum omp_clause_map_kind kind, 
			       gfc_omp_namelist *namelist, tree list)
{
  for (; namelist != NULL; namelist = namelist->next)
    if (namelist->sym->attr.referenced)
      {
	tree t = gfc_trans_omp_variable (namelist->sym, false);
	if (t != error_mark_node)
	  {
	    tree node = build_omp_clause (input_location, OMP_CLAUSE_MAP);
	    OMP_CLAUSE_DECL (node) = t;
	    OMP_CLAUSE_MAP_KIND (node) = kind;
	    list = gfc_trans_add_clause (node, list);
	  }
      }
  return list;
}

static inline tree
gfc_convert_expr_to_tree (stmtblock_t *block, gfc_expr *expr)
{
  gfc_se se;
  tree result;

  gfc_init_se (&se, NULL );
  gfc_conv_expr (&se, expr);
  gfc_add_block_to_block (block, &se.pre);
  result = gfc_evaluate_now (se.expr, block);
  gfc_add_block_to_block (block, &se.post);

  return result;
}

static tree
gfc_trans_omp_clauses (stmtblock_t *block, gfc_omp_clauses *clauses,
		       locus where, bool declare_simd = false)
{
  tree omp_clauses = NULL_TREE, chunk_size, c;
  int list;
  enum omp_clause_code clause_code;
  gfc_se se;

  if (clauses == NULL)
    return NULL_TREE;

  for (list = 0; list < OMP_LIST_NUM; list++)
    {
      gfc_omp_namelist *n = clauses->lists[list];

      if (n == NULL)
	continue;
      if (list >= OMP_LIST_DATA_CLAUSE_FIRST
	  && list <= OMP_LIST_DATA_CLAUSE_LAST)
	{
	  enum omp_clause_map_kind kind;
	  switch (list) 
	    {
	    case OMP_LIST_COPY:
	      kind = OMP_CLAUSE_MAP_FORCE_TOFROM;
	      break;
	    case OMP_LIST_OACC_COPYIN:
	      kind = OMP_CLAUSE_MAP_FORCE_TO;
	      break;
	    case OMP_LIST_COPYOUT:
	      kind = OMP_CLAUSE_MAP_FORCE_FROM;
	      break;
	    case OMP_LIST_CREATE:
	      kind = OMP_CLAUSE_MAP_FORCE_ALLOC;
	      break;
	    case OMP_LIST_DELETE:
	      kind = OMP_CLAUSE_MAP_FORCE_DEALLOC;
	      break;
	    case OMP_LIST_PRESENT:
	      kind = OMP_CLAUSE_MAP_FORCE_PRESENT;
	      break;
	    case OMP_LIST_PRESENT_OR_COPY:
	      kind = OMP_CLAUSE_MAP_TOFROM;
	      break;
	    case OMP_LIST_PRESENT_OR_COPYIN:
	      kind = OMP_CLAUSE_MAP_TO;
	      break;
	    case OMP_LIST_PRESENT_OR_COPYOUT:
	      kind = OMP_CLAUSE_MAP_FROM;
	      break;
	    case OMP_LIST_PRESENT_OR_CREATE:
	      kind = OMP_CLAUSE_MAP_ALLOC;
	      break;
	    case OMP_LIST_DEVICEPTR:
	      kind = OMP_CLAUSE_MAP_FORCE_DEVICEPTR;
	      break;
	    default:
	      gcc_unreachable ();
	    }
	  omp_clauses = gfc_trans_omp_map_clause_list (kind, n, omp_clauses);
	  continue;
	}
      switch (list)
	{
	case OMP_LIST_REDUCTION:
	  omp_clauses = gfc_trans_omp_reduction_list (n, omp_clauses, where);
	  break;
	case OMP_LIST_PRIVATE:
	  clause_code = OMP_CLAUSE_PRIVATE;
	  goto add_clause;
	case OMP_LIST_SHARED:
	  clause_code = OMP_CLAUSE_SHARED;
	  goto add_clause;
	case OMP_LIST_FIRSTPRIVATE:
	  clause_code = OMP_CLAUSE_FIRSTPRIVATE;
	  goto add_clause;
	case OMP_LIST_LASTPRIVATE:
	  clause_code = OMP_CLAUSE_LASTPRIVATE;
	  goto add_clause;
	case OMP_LIST_COPYIN:
	  clause_code = OMP_CLAUSE_COPYIN;
	  goto add_clause;
	case OMP_LIST_COPYPRIVATE:
	  clause_code = OMP_CLAUSE_COPYPRIVATE;
	  goto add_clause;
	case OMP_LIST_UNIFORM:
	  clause_code = OMP_CLAUSE_UNIFORM;
	  goto add_clause;
	case OMP_LIST_USE_DEVICE:
	  clause_code = OMP_CLAUSE_USE_DEVICE;
	  goto add_clause;
	case OMP_LIST_DEVICE_RESIDENT:
	  clause_code = OMP_CLAUSE_DEVICE_RESIDENT;
	  goto add_clause;
	case OMP_LIST_HOST:
	  clause_code = OMP_CLAUSE_HOST;
	  goto add_clause;
	case OMP_LIST_DEVICE:
	  clause_code = OMP_CLAUSE_OACC_DEVICE;
	  goto add_clause;
	case OMP_LIST_CACHE:
	  clause_code = OMP_NO_CLAUSE_CACHE;
	  goto add_clause;

	add_clause:
	  omp_clauses
	    = gfc_trans_omp_variable_list (clause_code, n, omp_clauses,
					   declare_simd);
	  break;
	case OMP_LIST_ALIGNED:
	  for (; n != NULL; n = n->next)
	    if (n->sym->attr.referenced || declare_simd)
	      {
		tree t = gfc_trans_omp_variable (n->sym, declare_simd);
		if (t != error_mark_node)
		  {
		    tree node = build_omp_clause (input_location,
						  OMP_CLAUSE_ALIGNED);
		    OMP_CLAUSE_DECL (node) = t;
		    if (n->expr)
		      {
			tree alignment_var;

			if (block == NULL)
			  alignment_var = gfc_conv_constant_to_tree (n->expr);
			else
			  {
			    gfc_init_se (&se, NULL);
			    gfc_conv_expr (&se, n->expr);
			    gfc_add_block_to_block (block, &se.pre);
			    alignment_var = gfc_evaluate_now (se.expr, block);
			    gfc_add_block_to_block (block, &se.post);
			  }
			OMP_CLAUSE_ALIGNED_ALIGNMENT (node) = alignment_var;
		      }
		    omp_clauses = gfc_trans_add_clause (node, omp_clauses);
		  }
	      }
	  break;
	case OMP_LIST_LINEAR:
	  {
	    gfc_expr *last_step_expr = NULL;
	    tree last_step = NULL_TREE;

	    for (; n != NULL; n = n->next)
	      {
		if (n->expr)
		  {
		    last_step_expr = n->expr;
		    last_step = NULL_TREE;
		  }
		if (n->sym->attr.referenced || declare_simd)
		  {
		    tree t = gfc_trans_omp_variable (n->sym, declare_simd);
		    if (t != error_mark_node)
		      {
			tree node = build_omp_clause (input_location,
						      OMP_CLAUSE_LINEAR);
			OMP_CLAUSE_DECL (node) = t;
			if (last_step_expr && last_step == NULL_TREE)
			  {
			    if (block == NULL)
			      last_step
				= gfc_conv_constant_to_tree (last_step_expr);
			    else
			      {
				gfc_init_se (&se, NULL);
				gfc_conv_expr (&se, last_step_expr);
				gfc_add_block_to_block (block, &se.pre);
				last_step = gfc_evaluate_now (se.expr, block);
				gfc_add_block_to_block (block, &se.post);
			      }
			  }
			OMP_CLAUSE_LINEAR_STEP (node) = last_step;
			omp_clauses = gfc_trans_add_clause (node, omp_clauses);
		      }
		  }
	      }
	  }
	  break;
	case OMP_LIST_DEPEND_IN:
	case OMP_LIST_DEPEND_OUT:
	  for (; n != NULL; n = n->next)
	    {
	      if (!n->sym->attr.referenced)
		continue;

	      tree node = build_omp_clause (input_location, OMP_CLAUSE_DEPEND);
	      if (n->expr == NULL || n->expr->ref->u.ar.type == AR_FULL)
		{
		  OMP_CLAUSE_DECL (node) = gfc_get_symbol_decl (n->sym);
		  if (DECL_P (OMP_CLAUSE_DECL (node)))
		    TREE_ADDRESSABLE (OMP_CLAUSE_DECL (node)) = 1;
		}
	      else
		{
		  tree ptr;
		  gfc_init_se (&se, NULL);
		  if (n->expr->ref->u.ar.type == AR_ELEMENT)
		    {
		      gfc_conv_expr_reference (&se, n->expr);
		      ptr = se.expr;
		    }
		  else
		    {
		      gfc_conv_expr_descriptor (&se, n->expr);
		      ptr = gfc_conv_array_data (se.expr);
		    }
		  gfc_add_block_to_block (block, &se.pre);
		  gfc_add_block_to_block (block, &se.post);
		  OMP_CLAUSE_DECL (node)
		    = fold_build1_loc (input_location, INDIRECT_REF,
				       TREE_TYPE (TREE_TYPE (ptr)), ptr);
		}
	      OMP_CLAUSE_DEPEND_KIND (node)
		= ((list == OMP_LIST_DEPEND_IN)
		   ? OMP_CLAUSE_DEPEND_IN : OMP_CLAUSE_DEPEND_OUT);
	      omp_clauses = gfc_trans_add_clause (node, omp_clauses);
	    }
	  break;
	default:
	  break;
	}
    }

  if (clauses->if_expr)
    {
      tree if_var;

      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, clauses->if_expr);
      gfc_add_block_to_block (block, &se.pre);
      if_var = gfc_evaluate_now (se.expr, block);
      gfc_add_block_to_block (block, &se.post);

      c = build_omp_clause (where.lb->location, OMP_CLAUSE_IF);
      OMP_CLAUSE_IF_EXPR (c) = if_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->final_expr)
    {
      tree final_var;

      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, clauses->final_expr);
      gfc_add_block_to_block (block, &se.pre);
      final_var = gfc_evaluate_now (se.expr, block);
      gfc_add_block_to_block (block, &se.post);

      c = build_omp_clause (where.lb->location, OMP_CLAUSE_FINAL);
      OMP_CLAUSE_FINAL_EXPR (c) = final_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->num_threads)
    {
      tree num_threads;

      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, clauses->num_threads);
      gfc_add_block_to_block (block, &se.pre);
      num_threads = gfc_evaluate_now (se.expr, block);
      gfc_add_block_to_block (block, &se.post);

      c = build_omp_clause (where.lb->location, OMP_CLAUSE_NUM_THREADS);
      OMP_CLAUSE_NUM_THREADS_EXPR (c) = num_threads;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  chunk_size = NULL_TREE;
  if (clauses->chunk_size)
    {
      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, clauses->chunk_size);
      gfc_add_block_to_block (block, &se.pre);
      chunk_size = gfc_evaluate_now (se.expr, block);
      gfc_add_block_to_block (block, &se.post);
    }

  if (clauses->sched_kind != OMP_SCHED_NONE)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_SCHEDULE);
      OMP_CLAUSE_SCHEDULE_CHUNK_EXPR (c) = chunk_size;
      switch (clauses->sched_kind)
	{
	case OMP_SCHED_STATIC:
	  OMP_CLAUSE_SCHEDULE_KIND (c) = OMP_CLAUSE_SCHEDULE_STATIC;
	  break;
	case OMP_SCHED_DYNAMIC:
	  OMP_CLAUSE_SCHEDULE_KIND (c) = OMP_CLAUSE_SCHEDULE_DYNAMIC;
	  break;
	case OMP_SCHED_GUIDED:
	  OMP_CLAUSE_SCHEDULE_KIND (c) = OMP_CLAUSE_SCHEDULE_GUIDED;
	  break;
	case OMP_SCHED_RUNTIME:
	  OMP_CLAUSE_SCHEDULE_KIND (c) = OMP_CLAUSE_SCHEDULE_RUNTIME;
	  break;
	case OMP_SCHED_AUTO:
	  OMP_CLAUSE_SCHEDULE_KIND (c) = OMP_CLAUSE_SCHEDULE_AUTO;
	  break;
	default:
	  gcc_unreachable ();
	}
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->default_sharing != OMP_DEFAULT_UNKNOWN)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_DEFAULT);
      switch (clauses->default_sharing)
	{
	case OMP_DEFAULT_NONE:
	  OMP_CLAUSE_DEFAULT_KIND (c) = OMP_CLAUSE_DEFAULT_NONE;
	  break;
	case OMP_DEFAULT_SHARED:
	  OMP_CLAUSE_DEFAULT_KIND (c) = OMP_CLAUSE_DEFAULT_SHARED;
	  break;
	case OMP_DEFAULT_PRIVATE:
	  OMP_CLAUSE_DEFAULT_KIND (c) = OMP_CLAUSE_DEFAULT_PRIVATE;
	  break;
	case OMP_DEFAULT_FIRSTPRIVATE:
	  OMP_CLAUSE_DEFAULT_KIND (c) = OMP_CLAUSE_DEFAULT_FIRSTPRIVATE;
	  break;
	default:
	  gcc_unreachable ();
	}
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->nowait)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_NOWAIT);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->ordered)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_ORDERED);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->untied)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_UNTIED);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->mergeable)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_MERGEABLE);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->collapse)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_COLLAPSE);
      OMP_CLAUSE_COLLAPSE_EXPR (c)
	= build_int_cst (integer_type_node, clauses->collapse);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->inbranch)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_INBRANCH);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->notinbranch)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_NOTINBRANCH);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  switch (clauses->cancel)
    {
    case OMP_CANCEL_UNKNOWN:
      break;
    case OMP_CANCEL_PARALLEL:
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_PARALLEL);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
      break;
    case OMP_CANCEL_SECTIONS:
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_SECTIONS);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
      break;
    case OMP_CANCEL_DO:
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_FOR);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
      break;
    case OMP_CANCEL_TASKGROUP:
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_TASKGROUP);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
      break;
    }

  if (clauses->proc_bind != OMP_PROC_BIND_UNKNOWN)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_PROC_BIND);
      switch (clauses->proc_bind)
	{
	case OMP_PROC_BIND_MASTER:
	  OMP_CLAUSE_PROC_BIND_KIND (c) = OMP_CLAUSE_PROC_BIND_MASTER;
	  break;
	case OMP_PROC_BIND_SPREAD:
	  OMP_CLAUSE_PROC_BIND_KIND (c) = OMP_CLAUSE_PROC_BIND_SPREAD;
	  break;
	case OMP_PROC_BIND_CLOSE:
	  OMP_CLAUSE_PROC_BIND_KIND (c) = OMP_CLAUSE_PROC_BIND_CLOSE;
	  break;
	default:
	  gcc_unreachable ();
	}
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->safelen_expr)
    {
      tree safelen_var;

      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, clauses->safelen_expr);
      gfc_add_block_to_block (block, &se.pre);
      safelen_var = gfc_evaluate_now (se.expr, block);
      gfc_add_block_to_block (block, &se.post);

      c = build_omp_clause (where.lb->location, OMP_CLAUSE_SAFELEN);
      OMP_CLAUSE_SAFELEN_EXPR (c) = safelen_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->simdlen_expr)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_SIMDLEN);
      OMP_CLAUSE_SIMDLEN_EXPR (c)
	= gfc_conv_constant_to_tree (clauses->simdlen_expr);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  if (clauses->async)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_ASYNC);
      if (clauses->async_expr)
	OMP_CLAUSE_ASYNC_EXPR (c) =
	    gfc_convert_expr_to_tree (block, clauses->async_expr);
      else
	OMP_CLAUSE_ASYNC_EXPR (c) = NULL;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->seq)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_ORDERED);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->independent)
    {
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_INDEPENDENT);
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->num_gangs_expr)
    {
      tree num_gangs_var = 
	  gfc_convert_expr_to_tree (block, clauses->num_gangs_expr);
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_NUM_GANGS);
      OMP_CLAUSE_NUM_GANGS_EXPR (c) = num_gangs_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->num_workers_expr)
    {
      tree num_workers_var = 
	  gfc_convert_expr_to_tree (block, clauses->num_workers_expr);
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_NUM_WORKERS);
      OMP_CLAUSE_NUM_WORKERS_EXPR (c)= num_workers_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->vector_length_expr)
    {
      tree vector_length_var = 
	  gfc_convert_expr_to_tree (block, clauses->vector_length_expr);
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_VECTOR_LENGTH);
      OMP_CLAUSE_VECTOR_LENGTH_EXPR (c)= vector_length_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }
  if (clauses->vector)
    {
      if (clauses->vector_expr)
	{
	  tree vector_var = 
	      gfc_convert_expr_to_tree (block, clauses->vector_expr);
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_VECTOR);
	  OMP_CLAUSE_VECTOR_EXPR (c)= vector_var;
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
      else
	{
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_VECTOR);
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
    }
  if (clauses->worker)
    {
      if (clauses->worker_expr)
	{
	  tree worker_var = 
	      gfc_convert_expr_to_tree (block, clauses->worker_expr);
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_WORKER);
	  OMP_CLAUSE_WORKER_EXPR (c)= worker_var;
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
      else
	{
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_WORKER);
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
    }
  if (clauses->gang)
    {
      if (clauses->gang_expr)
	{
	  tree gang_var = 
	      gfc_convert_expr_to_tree (block, clauses->gang_expr);
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_GANG);
	  OMP_CLAUSE_GANG_EXPR (c)= gang_var;
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
      else
	{
	  c = build_omp_clause (where.lb->location, OMP_CLAUSE_GANG);
	  omp_clauses = gfc_trans_add_clause (c, omp_clauses);
	}
    }
  if (clauses->non_clause_wait_expr)
    {
      tree wait_var = 
	  gfc_convert_expr_to_tree (block, clauses->non_clause_wait_expr);
      c = build_omp_clause (where.lb->location, OMP_CLAUSE_WAIT);
      OMP_CLAUSE_WAIT_EXPR (c)= wait_var;
      omp_clauses = gfc_trans_add_clause (c, omp_clauses);
    }

  return omp_clauses;
}

/* Like gfc_trans_code, but force creation of a BIND_EXPR around it.  */

static tree
gfc_trans_omp_code (gfc_code *code, bool force_empty)
{
  tree stmt;

  pushlevel ();
  stmt = gfc_trans_code (code);
  if (TREE_CODE (stmt) != BIND_EXPR)
    {
      if (!IS_EMPTY_STMT (stmt) || force_empty)
	{
	  tree block = poplevel (1, 0);
	  stmt = build3_v (BIND_EXPR, NULL, stmt, block);
	}
      else
	poplevel (0, 0);
    }
  else
    poplevel (0, 0);
  return stmt;
}

/* Trans OpenACC directives. */
/* parallel, kernels, data and host_data. */
static tree
gfc_trans_oacc_construct (gfc_code *code)
{
  stmtblock_t block;
  tree stmt, oacc_clauses;
  enum tree_code construct_code;

  switch (code->op)
    {
      case EXEC_OACC_PARALLEL:
	construct_code = OACC_PARALLEL;
	break;
      case EXEC_OACC_KERNELS:
	construct_code = OACC_KERNELS;
	break;
      case EXEC_OACC_DATA:
	construct_code = OACC_DATA;
	break;
      case EXEC_OACC_HOST_DATA:
	construct_code = OACC_HOST_DATA;
	break;
      default:
	gcc_unreachable ();
    }

  gfc_start_block (&block);
  oacc_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
					code->loc);
  stmt = gfc_trans_omp_code (code->block->next, true);
  stmt = build2_loc (input_location, construct_code, void_type_node, stmt,
		     oacc_clauses);
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

/* update, enter_data, exit_data, wait, cache. */
static tree 
gfc_trans_oacc_executable_directive (gfc_code *code)
{
  stmtblock_t block;
  tree stmt, oacc_clauses;
  enum tree_code construct_code;

  switch (code->op)
    {
      case EXEC_OACC_UPDATE:
	construct_code = OACC_UPDATE;
	break;
      case EXEC_OACC_ENTER_DATA:
	construct_code = OACC_ENTER_DATA;
	break;
      case EXEC_OACC_EXIT_DATA:
	construct_code = OACC_EXIT_DATA;
	break;
      case EXEC_OACC_WAIT:
	construct_code = OACC_WAIT;
	break;
      case EXEC_OACC_CACHE:
	construct_code = OACC_CACHE;
	break;
      default:
	gcc_unreachable ();
    }

  gfc_start_block (&block);
  oacc_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
					code->loc);
  stmt = build1_loc (input_location, construct_code, void_type_node, 
		     oacc_clauses);
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree gfc_trans_omp_sections (gfc_code *, gfc_omp_clauses *);
static tree gfc_trans_omp_workshare (gfc_code *, gfc_omp_clauses *);

static tree
gfc_trans_omp_atomic (gfc_code *code)
{
  gfc_code *atomic_code = code;
  gfc_se lse;
  gfc_se rse;
  gfc_se vse;
  gfc_expr *expr2, *e;
  gfc_symbol *var;
  stmtblock_t block;
  tree lhsaddr, type, rhs, x;
  enum tree_code op = ERROR_MARK;
  enum tree_code aop = OMP_ATOMIC;
  bool var_on_left = false;
  bool seq_cst = (atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_SEQ_CST) != 0;

  code = code->block->next;
  gcc_assert (code->op == EXEC_ASSIGN);
  var = code->expr1->symtree->n.sym;

  gfc_init_se (&lse, NULL);
  gfc_init_se (&rse, NULL);
  gfc_init_se (&vse, NULL);
  gfc_start_block (&block);

  expr2 = code->expr2;
  if (expr2->expr_type == EXPR_FUNCTION
      && expr2->value.function.isym->id == GFC_ISYM_CONVERSION)
    expr2 = expr2->value.function.actual->expr;

  switch (atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_MASK)
    {
    case GFC_OMP_ATOMIC_READ:
      gfc_conv_expr (&vse, code->expr1);
      gfc_add_block_to_block (&block, &vse.pre);

      gfc_conv_expr (&lse, expr2);
      gfc_add_block_to_block (&block, &lse.pre);
      type = TREE_TYPE (lse.expr);
      lhsaddr = gfc_build_addr_expr (NULL, lse.expr);

      x = build1 (OMP_ATOMIC_READ, type, lhsaddr);
      OMP_ATOMIC_SEQ_CST (x) = seq_cst;
      x = convert (TREE_TYPE (vse.expr), x);
      gfc_add_modify (&block, vse.expr, x);

      gfc_add_block_to_block (&block, &lse.pre);
      gfc_add_block_to_block (&block, &rse.pre);

      return gfc_finish_block (&block);
    case GFC_OMP_ATOMIC_CAPTURE:
      aop = OMP_ATOMIC_CAPTURE_NEW;
      if (expr2->expr_type == EXPR_VARIABLE)
	{
	  aop = OMP_ATOMIC_CAPTURE_OLD;
	  gfc_conv_expr (&vse, code->expr1);
	  gfc_add_block_to_block (&block, &vse.pre);

	  gfc_conv_expr (&lse, expr2);
	  gfc_add_block_to_block (&block, &lse.pre);
	  gfc_init_se (&lse, NULL);
	  code = code->next;
	  var = code->expr1->symtree->n.sym;
	  expr2 = code->expr2;
	  if (expr2->expr_type == EXPR_FUNCTION
	      && expr2->value.function.isym->id == GFC_ISYM_CONVERSION)
	    expr2 = expr2->value.function.actual->expr;
	}
      break;
    default:
      break;
    }

  gfc_conv_expr (&lse, code->expr1);
  gfc_add_block_to_block (&block, &lse.pre);
  type = TREE_TYPE (lse.expr);
  lhsaddr = gfc_build_addr_expr (NULL, lse.expr);

  if (((atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_MASK)
       == GFC_OMP_ATOMIC_WRITE)
      || (atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_SWAP))
    {
      gfc_conv_expr (&rse, expr2);
      gfc_add_block_to_block (&block, &rse.pre);
    }
  else if (expr2->expr_type == EXPR_OP)
    {
      gfc_expr *e;
      switch (expr2->value.op.op)
	{
	case INTRINSIC_PLUS:
	  op = PLUS_EXPR;
	  break;
	case INTRINSIC_TIMES:
	  op = MULT_EXPR;
	  break;
	case INTRINSIC_MINUS:
	  op = MINUS_EXPR;
	  break;
	case INTRINSIC_DIVIDE:
	  if (expr2->ts.type == BT_INTEGER)
	    op = TRUNC_DIV_EXPR;
	  else
	    op = RDIV_EXPR;
	  break;
	case INTRINSIC_AND:
	  op = TRUTH_ANDIF_EXPR;
	  break;
	case INTRINSIC_OR:
	  op = TRUTH_ORIF_EXPR;
	  break;
	case INTRINSIC_EQV:
	  op = EQ_EXPR;
	  break;
	case INTRINSIC_NEQV:
	  op = NE_EXPR;
	  break;
	default:
	  gcc_unreachable ();
	}
      e = expr2->value.op.op1;
      if (e->expr_type == EXPR_FUNCTION
	  && e->value.function.isym->id == GFC_ISYM_CONVERSION)
	e = e->value.function.actual->expr;
      if (e->expr_type == EXPR_VARIABLE
	  && e->symtree != NULL
	  && e->symtree->n.sym == var)
	{
	  expr2 = expr2->value.op.op2;
	  var_on_left = true;
	}
      else
	{
	  e = expr2->value.op.op2;
	  if (e->expr_type == EXPR_FUNCTION
	      && e->value.function.isym->id == GFC_ISYM_CONVERSION)
	    e = e->value.function.actual->expr;
	  gcc_assert (e->expr_type == EXPR_VARIABLE
		      && e->symtree != NULL
		      && e->symtree->n.sym == var);
	  expr2 = expr2->value.op.op1;
	  var_on_left = false;
	}
      gfc_conv_expr (&rse, expr2);
      gfc_add_block_to_block (&block, &rse.pre);
    }
  else
    {
      gcc_assert (expr2->expr_type == EXPR_FUNCTION);
      switch (expr2->value.function.isym->id)
	{
	case GFC_ISYM_MIN:
	  op = MIN_EXPR;
	  break;
	case GFC_ISYM_MAX:
	  op = MAX_EXPR;
	  break;
	case GFC_ISYM_IAND:
	  op = BIT_AND_EXPR;
	  break;
	case GFC_ISYM_IOR:
	  op = BIT_IOR_EXPR;
	  break;
	case GFC_ISYM_IEOR:
	  op = BIT_XOR_EXPR;
	  break;
	default:
	  gcc_unreachable ();
	}
      e = expr2->value.function.actual->expr;
      gcc_assert (e->expr_type == EXPR_VARIABLE
		  && e->symtree != NULL
		  && e->symtree->n.sym == var);

      gfc_conv_expr (&rse, expr2->value.function.actual->next->expr);
      gfc_add_block_to_block (&block, &rse.pre);
      if (expr2->value.function.actual->next->next != NULL)
	{
	  tree accum = gfc_create_var (TREE_TYPE (rse.expr), NULL);
	  gfc_actual_arglist *arg;

	  gfc_add_modify (&block, accum, rse.expr);
	  for (arg = expr2->value.function.actual->next->next; arg;
	       arg = arg->next)
	    {
	      gfc_init_block (&rse.pre);
	      gfc_conv_expr (&rse, arg->expr);
	      gfc_add_block_to_block (&block, &rse.pre);
	      x = fold_build2_loc (input_location, op, TREE_TYPE (accum),
				   accum, rse.expr);
	      gfc_add_modify (&block, accum, x);
	    }

	  rse.expr = accum;
	}

      expr2 = expr2->value.function.actual->next->expr;
    }

  lhsaddr = save_expr (lhsaddr);
  rhs = gfc_evaluate_now (rse.expr, &block);

  if (((atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_MASK)
       == GFC_OMP_ATOMIC_WRITE)
      || (atomic_code->ext.omp_atomic & GFC_OMP_ATOMIC_SWAP))
    x = rhs;
  else
    {
      x = convert (TREE_TYPE (rhs),
		   build_fold_indirect_ref_loc (input_location, lhsaddr));
      if (var_on_left)
	x = fold_build2_loc (input_location, op, TREE_TYPE (rhs), x, rhs);
      else
	x = fold_build2_loc (input_location, op, TREE_TYPE (rhs), rhs, x);
    }

  if (TREE_CODE (TREE_TYPE (rhs)) == COMPLEX_TYPE
      && TREE_CODE (type) != COMPLEX_TYPE)
    x = fold_build1_loc (input_location, REALPART_EXPR,
			 TREE_TYPE (TREE_TYPE (rhs)), x);

  gfc_add_block_to_block (&block, &lse.pre);
  gfc_add_block_to_block (&block, &rse.pre);

  if (aop == OMP_ATOMIC)
    {
      x = build2_v (OMP_ATOMIC, lhsaddr, convert (type, x));
      OMP_ATOMIC_SEQ_CST (x) = seq_cst;
      gfc_add_expr_to_block (&block, x);
    }
  else
    {
      if (aop == OMP_ATOMIC_CAPTURE_NEW)
	{
	  code = code->next;
	  expr2 = code->expr2;
	  if (expr2->expr_type == EXPR_FUNCTION
	      && expr2->value.function.isym->id == GFC_ISYM_CONVERSION)
	    expr2 = expr2->value.function.actual->expr;

	  gcc_assert (expr2->expr_type == EXPR_VARIABLE);
	  gfc_conv_expr (&vse, code->expr1);
	  gfc_add_block_to_block (&block, &vse.pre);

	  gfc_init_se (&lse, NULL);
	  gfc_conv_expr (&lse, expr2);
	  gfc_add_block_to_block (&block, &lse.pre);
	}
      x = build2 (aop, type, lhsaddr, convert (type, x));
      OMP_ATOMIC_SEQ_CST (x) = seq_cst;
      x = convert (TREE_TYPE (vse.expr), x);
      gfc_add_modify (&block, vse.expr, x);
    }

  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_barrier (void)
{
  tree decl = builtin_decl_explicit (BUILT_IN_GOMP_BARRIER);
  return build_call_expr_loc (input_location, decl, 0);
}

static tree
gfc_trans_omp_cancel (gfc_code *code)
{
  int mask = 0;
  tree ifc = boolean_true_node;
  stmtblock_t block;
  switch (code->ext.omp_clauses->cancel)
    {
    case OMP_CANCEL_PARALLEL: mask = 1; break;
    case OMP_CANCEL_DO: mask = 2; break;
    case OMP_CANCEL_SECTIONS: mask = 4; break;
    case OMP_CANCEL_TASKGROUP: mask = 8; break;
    default: gcc_unreachable ();
    }
  gfc_start_block (&block);
  if (code->ext.omp_clauses->if_expr)
    {
      gfc_se se;
      tree if_var;

      gfc_init_se (&se, NULL);
      gfc_conv_expr (&se, code->ext.omp_clauses->if_expr);
      gfc_add_block_to_block (&block, &se.pre);
      if_var = gfc_evaluate_now (se.expr, &block);
      gfc_add_block_to_block (&block, &se.post);
      tree type = TREE_TYPE (if_var);
      ifc = fold_build2_loc (input_location, NE_EXPR,
			     boolean_type_node, if_var,
			     build_zero_cst (type));
    }
  tree decl = builtin_decl_explicit (BUILT_IN_GOMP_CANCEL);
  tree c_bool_type = TREE_TYPE (TREE_TYPE (decl));
  ifc = fold_convert (c_bool_type, ifc);
  gfc_add_expr_to_block (&block,
			 build_call_expr_loc (input_location, decl, 2,
					      build_int_cst (integer_type_node,
							     mask), ifc));
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_cancellation_point (gfc_code *code)
{
  int mask = 0;
  switch (code->ext.omp_clauses->cancel)
    {
    case OMP_CANCEL_PARALLEL: mask = 1; break;
    case OMP_CANCEL_DO: mask = 2; break;
    case OMP_CANCEL_SECTIONS: mask = 4; break;
    case OMP_CANCEL_TASKGROUP: mask = 8; break;
    default: gcc_unreachable ();
    }
  tree decl = builtin_decl_explicit (BUILT_IN_GOMP_CANCELLATION_POINT);
  return build_call_expr_loc (input_location, decl, 1,
			      build_int_cst (integer_type_node, mask));
}

static tree
gfc_trans_omp_critical (gfc_code *code)
{
  tree name = NULL_TREE, stmt;
  if (code->ext.omp_name != NULL)
    name = get_identifier (code->ext.omp_name);
  stmt = gfc_trans_code (code->block->next);
  return build2_loc (input_location, OMP_CRITICAL, void_type_node, stmt, name);
}

typedef struct dovar_init_d {
  tree var;
  tree init;
} dovar_init;

static tree
gfc_trans_omp_do (gfc_code *code, gfc_exec_op op, stmtblock_t *pblock,
		  gfc_omp_clauses *do_clauses, tree par_clauses)
{
  gfc_se se;
  tree dovar, stmt, from, to, step, type, init, cond, incr;
  tree count = NULL_TREE, cycle_label, tmp, omp_clauses;
  tree_code stmt_code;
  stmtblock_t block;
  stmtblock_t body;
  gfc_omp_clauses *clauses = code->ext.omp_clauses;
  int i, collapse = clauses->collapse;
  vec<dovar_init> inits = vNULL;
  dovar_init *di;
  unsigned ix;

  if (collapse <= 0)
    collapse = 1;

  code = code->block->next;
  gcc_assert (code->op == EXEC_DO);

  init = make_tree_vec (collapse);
  cond = make_tree_vec (collapse);
  incr = make_tree_vec (collapse);

  if (pblock == NULL)
    {
      gfc_start_block (&block);
      pblock = &block;
    }

  omp_clauses = gfc_trans_omp_clauses (pblock, do_clauses, code->loc);

  for (i = 0; i < collapse; i++)
    {
      int simple = 0;
      int dovar_found = 0;
      tree dovar_decl;

      if (clauses)
	{
	  gfc_omp_namelist *n;
	  for (n = clauses->lists[(op == EXEC_OMP_SIMD && collapse == 1)
				  ? OMP_LIST_LINEAR : OMP_LIST_LASTPRIVATE];
	       n != NULL; n = n->next)
	    if (code->ext.iterator->var->symtree->n.sym == n->sym)
	      break;
	  if (n != NULL)
	    dovar_found = 1;
	  else if (n == NULL && op != EXEC_OMP_SIMD)
	    for (n = clauses->lists[OMP_LIST_PRIVATE]; n != NULL; n = n->next)
	      if (code->ext.iterator->var->symtree->n.sym == n->sym)
		break;
	  if (n != NULL)
	    dovar_found++;
	}

      /* Evaluate all the expressions in the iterator.  */
      gfc_init_se (&se, NULL);
      gfc_conv_expr_lhs (&se, code->ext.iterator->var);
      gfc_add_block_to_block (pblock, &se.pre);
      dovar = se.expr;
      type = TREE_TYPE (dovar);
      gcc_assert (TREE_CODE (type) == INTEGER_TYPE);

      gfc_init_se (&se, NULL);
      gfc_conv_expr_val (&se, code->ext.iterator->start);
      gfc_add_block_to_block (pblock, &se.pre);
      from = gfc_evaluate_now (se.expr, pblock);

      gfc_init_se (&se, NULL);
      gfc_conv_expr_val (&se, code->ext.iterator->end);
      gfc_add_block_to_block (pblock, &se.pre);
      to = gfc_evaluate_now (se.expr, pblock);

      gfc_init_se (&se, NULL);
      gfc_conv_expr_val (&se, code->ext.iterator->step);
      gfc_add_block_to_block (pblock, &se.pre);
      step = gfc_evaluate_now (se.expr, pblock);
      dovar_decl = dovar;

      /* Special case simple loops.  */
      if (TREE_CODE (dovar) == VAR_DECL)
	{
	  if (integer_onep (step))
	    simple = 1;
	  else if (tree_int_cst_equal (step, integer_minus_one_node))
	    simple = -1;
	}
      else
	dovar_decl
	  = gfc_trans_omp_variable (code->ext.iterator->var->symtree->n.sym,
				    false);

      /* Loop body.  */
      if (simple)
	{
	  TREE_VEC_ELT (init, i) = build2_v (MODIFY_EXPR, dovar, from);
	  /* The condition should not be folded.  */
	  TREE_VEC_ELT (cond, i) = build2_loc (input_location, simple > 0
					       ? LE_EXPR : GE_EXPR,
					       boolean_type_node, dovar, to);
	  TREE_VEC_ELT (incr, i) = fold_build2_loc (input_location, PLUS_EXPR,
						    type, dovar, step);
	  TREE_VEC_ELT (incr, i) = fold_build2_loc (input_location,
						    MODIFY_EXPR,
						    type, dovar,
						    TREE_VEC_ELT (incr, i));
	}
      else
	{
	  /* STEP is not 1 or -1.  Use:
	     for (count = 0; count < (to + step - from) / step; count++)
	       {
		 dovar = from + count * step;
		 body;
	       cycle_label:;
	       }  */
	  tmp = fold_build2_loc (input_location, MINUS_EXPR, type, step, from);
	  tmp = fold_build2_loc (input_location, PLUS_EXPR, type, to, tmp);
	  tmp = fold_build2_loc (input_location, TRUNC_DIV_EXPR, type, tmp,
				 step);
	  tmp = gfc_evaluate_now (tmp, pblock);
	  count = gfc_create_var (type, "count");
	  TREE_VEC_ELT (init, i) = build2_v (MODIFY_EXPR, count,
					     build_int_cst (type, 0));
	  /* The condition should not be folded.  */
	  TREE_VEC_ELT (cond, i) = build2_loc (input_location, LT_EXPR,
					       boolean_type_node,
					       count, tmp);
	  TREE_VEC_ELT (incr, i) = fold_build2_loc (input_location, PLUS_EXPR,
						    type, count,
						    build_int_cst (type, 1));
	  TREE_VEC_ELT (incr, i) = fold_build2_loc (input_location,
						    MODIFY_EXPR, type, count,
						    TREE_VEC_ELT (incr, i));

	  /* Initialize DOVAR.  */
	  tmp = fold_build2_loc (input_location, MULT_EXPR, type, count, step);
	  tmp = fold_build2_loc (input_location, PLUS_EXPR, type, from, tmp);
	  dovar_init e = {dovar, tmp};
	  inits.safe_push (e);
	}

      if (!dovar_found)
	{
	  if (op == EXEC_OMP_SIMD)
	    {
	      if (collapse == 1)
		{
		  tmp = build_omp_clause (input_location, OMP_CLAUSE_LINEAR);
		  OMP_CLAUSE_LINEAR_STEP (tmp) = step;
		}
	      else
		tmp = build_omp_clause (input_location, OMP_CLAUSE_LASTPRIVATE);
	      if (!simple)
		dovar_found = 2;
	    }
	  else
	    tmp = build_omp_clause (input_location, OMP_CLAUSE_PRIVATE);
	  OMP_CLAUSE_DECL (tmp) = dovar_decl;
	  omp_clauses = gfc_trans_add_clause (tmp, omp_clauses);
	}
      if (dovar_found == 2)
	{
	  tree c = NULL;

	  tmp = NULL;
	  if (!simple)
	    {
	      /* If dovar is lastprivate, but different counter is used,
		 dovar += step needs to be added to
		 OMP_CLAUSE_LASTPRIVATE_STMT, otherwise the copied dovar
		 will have the value on entry of the last loop, rather
		 than value after iterator increment.  */
	      tmp = gfc_evaluate_now (step, pblock);
	      tmp = fold_build2_loc (input_location, PLUS_EXPR, type, dovar,
				     tmp);
	      tmp = fold_build2_loc (input_location, MODIFY_EXPR, type,
				     dovar, tmp);
	      for (c = omp_clauses; c ; c = OMP_CLAUSE_CHAIN (c))
		if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LASTPRIVATE
		    && OMP_CLAUSE_DECL (c) == dovar_decl)
		  {
		    OMP_CLAUSE_LASTPRIVATE_STMT (c) = tmp;
		    break;
		  }
		else if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LINEAR
			 && OMP_CLAUSE_DECL (c) == dovar_decl)
		  {
		    OMP_CLAUSE_LINEAR_STMT (c) = tmp;
		    break;
		  }
	    }
	  if (c == NULL && op == EXEC_OMP_DO && par_clauses != NULL)
	    {
	      for (c = par_clauses; c ; c = OMP_CLAUSE_CHAIN (c))
		if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LASTPRIVATE
		    && OMP_CLAUSE_DECL (c) == dovar_decl)
		  {
		    tree l = build_omp_clause (input_location,
					       OMP_CLAUSE_LASTPRIVATE);
		    OMP_CLAUSE_DECL (l) = dovar_decl;
		    OMP_CLAUSE_CHAIN (l) = omp_clauses;
		    OMP_CLAUSE_LASTPRIVATE_STMT (l) = tmp;
		    omp_clauses = l;
		    OMP_CLAUSE_SET_CODE (c, OMP_CLAUSE_SHARED);
		    break;
		  }
	    }
	  gcc_assert (simple || c != NULL);
	}
      if (!simple)
	{
	  if (op != EXEC_OMP_SIMD)
	    tmp = build_omp_clause (input_location, OMP_CLAUSE_PRIVATE);
	  else if (collapse == 1)
	    {
	      tmp = build_omp_clause (input_location, OMP_CLAUSE_LINEAR);
	      OMP_CLAUSE_LINEAR_STEP (tmp) = step;
	      OMP_CLAUSE_LINEAR_NO_COPYIN (tmp) = 1;
	      OMP_CLAUSE_LINEAR_NO_COPYOUT (tmp) = 1;
	    }
	  else
	    tmp = build_omp_clause (input_location, OMP_CLAUSE_LASTPRIVATE);
	  OMP_CLAUSE_DECL (tmp) = count;
	  omp_clauses = gfc_trans_add_clause (tmp, omp_clauses);
	}

      if (i + 1 < collapse)
	code = code->block->next;
    }

  if (pblock != &block)
    {
      pushlevel ();
      gfc_start_block (&block);
    }

  gfc_start_block (&body);

  FOR_EACH_VEC_ELT (inits, ix, di)
    gfc_add_modify (&body, di->var, di->init);
  inits.release ();

  /* Cycle statement is implemented with a goto.  Exit statement must not be
     present for this loop.  */
  cycle_label = gfc_build_label_decl (NULL_TREE);

  /* Put these labels where they can be found later.  */

  code->cycle_label = cycle_label;
  code->exit_label = NULL_TREE;

  /* Main loop body.  */
  tmp = gfc_trans_omp_code (code->block->next, true);
  gfc_add_expr_to_block (&body, tmp);

  /* Label for cycle statements (if needed).  */
  if (TREE_USED (cycle_label))
    {
      tmp = build1_v (LABEL_EXPR, cycle_label);
      gfc_add_expr_to_block (&body, tmp);
    }

  /* End of loop body.  */
  switch (op)
    {
    case EXEC_OMP_SIMD:
      stmt_code = OMP_SIMD;
      break;
    case EXEC_OACC_LOOP:
      stmt_code = OACC_LOOP;
      break;
    default:
      stmt_code = OMP_FOR;
    }
 
  stmt = make_node (stmt_code);

  TREE_TYPE (stmt) = void_type_node;
  OMP_FOR_BODY (stmt) = gfc_finish_block (&body);
  OMP_FOR_CLAUSES (stmt) = omp_clauses;
  OMP_FOR_INIT (stmt) = init;
  OMP_FOR_COND (stmt) = cond;
  OMP_FOR_INCR (stmt) = incr;
  gfc_add_expr_to_block (&block, stmt);

  return gfc_finish_block (&block);
}

/* parallel loop and kernels loop. */
static tree
gfc_trans_oacc_combined_directive (gfc_code *code)
{
  stmtblock_t block, *pblock = NULL;
  gfc_omp_clauses construct_clauses, loop_clauses;
  tree stmt, oacc_clauses = NULL_TREE;
  enum tree_code construct_code;

  switch (code->op)
    {
      case EXEC_OACC_PARALLEL_LOOP:
	construct_code = OACC_PARALLEL;
	break;
      case EXEC_OACC_KERNELS_LOOP:
	construct_code = OACC_KERNELS;
	break;
      default:
	gcc_unreachable ();
    }

  gfc_start_block (&block);

  memset (&loop_clauses, 0, sizeof (loop_clauses));
  if (code->ext.omp_clauses != NULL)
    {
      memcpy (&construct_clauses, code->ext.omp_clauses,
	      sizeof (construct_clauses));
      loop_clauses.collapse = construct_clauses.collapse;
      loop_clauses.gang = construct_clauses.gang;
      loop_clauses.vector = construct_clauses.vector;
      loop_clauses.worker = construct_clauses.worker;
      loop_clauses.seq = construct_clauses.seq;
      loop_clauses.independent = construct_clauses.independent;
      construct_clauses.collapse = 0;
      construct_clauses.gang = false;
      construct_clauses.vector = false;
      construct_clauses.worker = false;
      construct_clauses.seq = false;
      construct_clauses.independent = false;
      oacc_clauses = gfc_trans_omp_clauses (&block, &construct_clauses,
					    code->loc);
    }
  if (!loop_clauses.seq)
    pblock = &block;
  else
    pushlevel ();
  stmt = gfc_trans_omp_do (code, code->op, pblock, &loop_clauses, NULL);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  stmt = build2_loc (input_location, construct_code, void_type_node, stmt,
		     oacc_clauses);
  if (code->op == EXEC_OACC_KERNELS_LOOP)
    OACC_KERNELS_COMBINED (stmt) = 1;
  else
    OACC_PARALLEL_COMBINED (stmt) = 1;
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_flush (void)
{
  tree decl = builtin_decl_explicit (BUILT_IN_SYNC_SYNCHRONIZE);
  return build_call_expr_loc (input_location, decl, 0);
}

static tree
gfc_trans_omp_master (gfc_code *code)
{
  tree stmt = gfc_trans_code (code->block->next);
  if (IS_EMPTY_STMT (stmt))
    return stmt;
  return build1_v (OMP_MASTER, stmt);
}

static tree
gfc_trans_omp_ordered (gfc_code *code)
{
  return build1_v (OMP_ORDERED, gfc_trans_code (code->block->next));
}

static tree
gfc_trans_omp_parallel (gfc_code *code)
{
  stmtblock_t block;
  tree stmt, omp_clauses;

  gfc_start_block (&block);
  omp_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
				       code->loc);
  stmt = gfc_trans_omp_code (code->block->next, true);
  stmt = build2_loc (input_location, OMP_PARALLEL, void_type_node, stmt,
		     omp_clauses);
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

enum
{
  GFC_OMP_SPLIT_SIMD,
  GFC_OMP_SPLIT_DO,
  GFC_OMP_SPLIT_PARALLEL,
  GFC_OMP_SPLIT_NUM
};

enum
{
  GFC_OMP_MASK_SIMD = (1 << GFC_OMP_SPLIT_SIMD),
  GFC_OMP_MASK_DO = (1 << GFC_OMP_SPLIT_DO),
  GFC_OMP_MASK_PARALLEL = (1 << GFC_OMP_SPLIT_PARALLEL)
};

static void
gfc_split_omp_clauses (gfc_code *code,
		       gfc_omp_clauses clausesa[GFC_OMP_SPLIT_NUM])
{
  int mask = 0, innermost = 0;
  memset (clausesa, 0, GFC_OMP_SPLIT_NUM * sizeof (gfc_omp_clauses));
  switch (code->op)
    {
    case EXEC_OMP_DO_SIMD:
      mask = GFC_OMP_MASK_DO | GFC_OMP_MASK_SIMD;
      innermost = GFC_OMP_SPLIT_SIMD;
      break;
    case EXEC_OMP_PARALLEL_DO:
      mask = GFC_OMP_MASK_PARALLEL | GFC_OMP_MASK_DO;
      innermost = GFC_OMP_SPLIT_DO;
      break;
    case EXEC_OMP_PARALLEL_DO_SIMD:
      mask = GFC_OMP_MASK_PARALLEL | GFC_OMP_MASK_DO | GFC_OMP_MASK_SIMD;
      innermost = GFC_OMP_SPLIT_SIMD;
      break;
    default:
      gcc_unreachable ();
    }
  if (code->ext.omp_clauses != NULL)
    {
      if (mask & GFC_OMP_MASK_PARALLEL)
	{
	  /* First the clauses that are unique to some constructs.  */
	  clausesa[GFC_OMP_SPLIT_PARALLEL].lists[OMP_LIST_COPYIN]
	    = code->ext.omp_clauses->lists[OMP_LIST_COPYIN];
	  clausesa[GFC_OMP_SPLIT_PARALLEL].num_threads
	    = code->ext.omp_clauses->num_threads;
	  clausesa[GFC_OMP_SPLIT_PARALLEL].proc_bind
	    = code->ext.omp_clauses->proc_bind;
	  /* Shared and default clauses are allowed on parallel and teams.  */
	  clausesa[GFC_OMP_SPLIT_PARALLEL].lists[OMP_LIST_SHARED]
	    = code->ext.omp_clauses->lists[OMP_LIST_SHARED];
	  clausesa[GFC_OMP_SPLIT_PARALLEL].default_sharing
	    = code->ext.omp_clauses->default_sharing;
	  /* FIXME: This is currently being discussed.  */
	  clausesa[GFC_OMP_SPLIT_PARALLEL].if_expr
	    = code->ext.omp_clauses->if_expr;
	}
      if (mask & GFC_OMP_MASK_DO)
	{
	  /* First the clauses that are unique to some constructs.  */
	  clausesa[GFC_OMP_SPLIT_DO].ordered
	    = code->ext.omp_clauses->ordered;
	  clausesa[GFC_OMP_SPLIT_DO].sched_kind
	    = code->ext.omp_clauses->sched_kind;
	  clausesa[GFC_OMP_SPLIT_DO].chunk_size
	    = code->ext.omp_clauses->chunk_size;
	  clausesa[GFC_OMP_SPLIT_DO].nowait
	    = code->ext.omp_clauses->nowait;
	  /* Duplicate collapse.  */
	  clausesa[GFC_OMP_SPLIT_DO].collapse
	    = code->ext.omp_clauses->collapse;
	}
      if (mask & GFC_OMP_MASK_SIMD)
	{
	  clausesa[GFC_OMP_SPLIT_SIMD].safelen_expr
	    = code->ext.omp_clauses->safelen_expr;
	  clausesa[GFC_OMP_SPLIT_SIMD].lists[OMP_LIST_LINEAR]
	    = code->ext.omp_clauses->lists[OMP_LIST_LINEAR];
	  clausesa[GFC_OMP_SPLIT_SIMD].lists[OMP_LIST_ALIGNED]
	    = code->ext.omp_clauses->lists[OMP_LIST_ALIGNED];
	  /* Duplicate collapse.  */
	  clausesa[GFC_OMP_SPLIT_SIMD].collapse
	    = code->ext.omp_clauses->collapse;
	}
      /* Private clause is supported on all constructs but target,
	 it is enough to put it on the innermost one.  For
	 !$ omp do put it on parallel though,
	 as that's what we did for OpenMP 3.1.  */
      clausesa[innermost == GFC_OMP_SPLIT_DO
	       ? (int) GFC_OMP_SPLIT_PARALLEL
	       : innermost].lists[OMP_LIST_PRIVATE]
	= code->ext.omp_clauses->lists[OMP_LIST_PRIVATE];
      /* Firstprivate clause is supported on all constructs but
	 target and simd.  Put it on the outermost of those and
	 duplicate on parallel.  */
      if (mask & GFC_OMP_MASK_PARALLEL)
	clausesa[GFC_OMP_SPLIT_PARALLEL].lists[OMP_LIST_FIRSTPRIVATE]
	  = code->ext.omp_clauses->lists[OMP_LIST_FIRSTPRIVATE];
      else if (mask & GFC_OMP_MASK_DO)
	clausesa[GFC_OMP_SPLIT_DO].lists[OMP_LIST_FIRSTPRIVATE]
	  = code->ext.omp_clauses->lists[OMP_LIST_FIRSTPRIVATE];
      /* Lastprivate is allowed on do and simd.  In
	 parallel do{, simd} we actually want to put it on
	 parallel rather than do.  */
      if (mask & GFC_OMP_MASK_PARALLEL)
	clausesa[GFC_OMP_SPLIT_PARALLEL].lists[OMP_LIST_LASTPRIVATE]
	  = code->ext.omp_clauses->lists[OMP_LIST_LASTPRIVATE];
      else if (mask & GFC_OMP_MASK_DO)
	clausesa[GFC_OMP_SPLIT_DO].lists[OMP_LIST_LASTPRIVATE]
	  = code->ext.omp_clauses->lists[OMP_LIST_LASTPRIVATE];
      if (mask & GFC_OMP_MASK_SIMD)
	clausesa[GFC_OMP_SPLIT_SIMD].lists[OMP_LIST_LASTPRIVATE]
	  = code->ext.omp_clauses->lists[OMP_LIST_LASTPRIVATE];
      /* Reduction is allowed on simd, do, parallel and teams.
	 Duplicate it on all of them, but omit on do if
	 parallel is present.  */
      if (mask & GFC_OMP_MASK_PARALLEL)
	clausesa[GFC_OMP_SPLIT_PARALLEL].lists[OMP_LIST_REDUCTION]
	  = code->ext.omp_clauses->lists[OMP_LIST_REDUCTION];
      else if (mask & GFC_OMP_MASK_DO)
	clausesa[GFC_OMP_SPLIT_DO].lists[OMP_LIST_REDUCTION]
	  = code->ext.omp_clauses->lists[OMP_LIST_REDUCTION];
      if (mask & GFC_OMP_MASK_SIMD)
	clausesa[GFC_OMP_SPLIT_SIMD].lists[OMP_LIST_REDUCTION]
	  = code->ext.omp_clauses->lists[OMP_LIST_REDUCTION];
    }
  if ((mask & (GFC_OMP_MASK_PARALLEL | GFC_OMP_MASK_DO))
      == (GFC_OMP_MASK_PARALLEL | GFC_OMP_MASK_DO))
    clausesa[GFC_OMP_SPLIT_DO].nowait = true;
}

static tree
gfc_trans_omp_do_simd (gfc_code *code, gfc_omp_clauses *clausesa,
		       tree omp_clauses)
{
  stmtblock_t block, *pblock = NULL;
  gfc_omp_clauses clausesa_buf[GFC_OMP_SPLIT_NUM];
  tree stmt, body, omp_do_clauses = NULL_TREE;

  gfc_start_block (&block);

  if (clausesa == NULL)
    {
      clausesa = clausesa_buf;
      gfc_split_omp_clauses (code, clausesa);
    }
  if (gfc_option.gfc_flag_openmp)
    omp_do_clauses
      = gfc_trans_omp_clauses (&block, &clausesa[GFC_OMP_SPLIT_DO], code->loc);
  pblock = &block;
  body = gfc_trans_omp_do (code, EXEC_OMP_SIMD, pblock,
			   &clausesa[GFC_OMP_SPLIT_SIMD], omp_clauses);
  if (TREE_CODE (body) != BIND_EXPR)
    body = build3_v (BIND_EXPR, NULL, body, poplevel (1, 0));
  else
    poplevel (0, 0);
  if (gfc_option.gfc_flag_openmp)
    {
      stmt = make_node (OMP_FOR);
      TREE_TYPE (stmt) = void_type_node;
      OMP_FOR_BODY (stmt) = body;
      OMP_FOR_CLAUSES (stmt) = omp_do_clauses;
    }
  else
    stmt = body;
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_parallel_do (gfc_code *code)
{
  stmtblock_t block, *pblock = NULL;
  gfc_omp_clauses clausesa[GFC_OMP_SPLIT_NUM];
  tree stmt, omp_clauses = NULL_TREE;

  gfc_start_block (&block);

  gfc_split_omp_clauses (code, clausesa);
  omp_clauses
    = gfc_trans_omp_clauses (&block, &clausesa[GFC_OMP_SPLIT_PARALLEL],
			     code->loc);
  if (!clausesa[GFC_OMP_SPLIT_DO].ordered
      && clausesa[GFC_OMP_SPLIT_DO].sched_kind != OMP_SCHED_STATIC)
    pblock = &block;
  else
    pushlevel ();
  stmt = gfc_trans_omp_do (code, EXEC_OMP_DO, pblock,
			   &clausesa[GFC_OMP_SPLIT_DO], omp_clauses);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  stmt = build2_loc (input_location, OMP_PARALLEL, void_type_node, stmt,
		     omp_clauses);
  OMP_PARALLEL_COMBINED (stmt) = 1;
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_parallel_do_simd (gfc_code *code)
{
  stmtblock_t block;
  gfc_omp_clauses clausesa[GFC_OMP_SPLIT_NUM];
  tree stmt, omp_clauses = NULL_TREE;

  gfc_start_block (&block);

  gfc_split_omp_clauses (code, clausesa);
  if (gfc_option.gfc_flag_openmp)
    omp_clauses
      = gfc_trans_omp_clauses (&block, &clausesa[GFC_OMP_SPLIT_PARALLEL],
			       code->loc);
  pushlevel ();
  stmt = gfc_trans_omp_do_simd (code, clausesa, omp_clauses);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  if (gfc_option.gfc_flag_openmp)
    {
      stmt = build2_loc (input_location, OMP_PARALLEL, void_type_node, stmt,
			 omp_clauses);
      OMP_PARALLEL_COMBINED (stmt) = 1;
    }
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_parallel_sections (gfc_code *code)
{
  stmtblock_t block;
  gfc_omp_clauses section_clauses;
  tree stmt, omp_clauses;

  memset (&section_clauses, 0, sizeof (section_clauses));
  section_clauses.nowait = true;

  gfc_start_block (&block);
  omp_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
				       code->loc);
  pushlevel ();
  stmt = gfc_trans_omp_sections (code, &section_clauses);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  stmt = build2_loc (input_location, OMP_PARALLEL, void_type_node, stmt,
		     omp_clauses);
  OMP_PARALLEL_COMBINED (stmt) = 1;
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_parallel_workshare (gfc_code *code)
{
  stmtblock_t block;
  gfc_omp_clauses workshare_clauses;
  tree stmt, omp_clauses;

  memset (&workshare_clauses, 0, sizeof (workshare_clauses));
  workshare_clauses.nowait = true;

  gfc_start_block (&block);
  omp_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
				       code->loc);
  pushlevel ();
  stmt = gfc_trans_omp_workshare (code, &workshare_clauses);
  if (TREE_CODE (stmt) != BIND_EXPR)
    stmt = build3_v (BIND_EXPR, NULL, stmt, poplevel (1, 0));
  else
    poplevel (0, 0);
  stmt = build2_loc (input_location, OMP_PARALLEL, void_type_node, stmt,
		     omp_clauses);
  OMP_PARALLEL_COMBINED (stmt) = 1;
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_sections (gfc_code *code, gfc_omp_clauses *clauses)
{
  stmtblock_t block, body;
  tree omp_clauses, stmt;
  bool has_lastprivate = clauses->lists[OMP_LIST_LASTPRIVATE] != NULL;

  gfc_start_block (&block);

  omp_clauses = gfc_trans_omp_clauses (&block, clauses, code->loc);

  gfc_init_block (&body);
  for (code = code->block; code; code = code->block)
    {
      /* Last section is special because of lastprivate, so even if it
	 is empty, chain it in.  */
      stmt = gfc_trans_omp_code (code->next,
				 has_lastprivate && code->block == NULL);
      if (! IS_EMPTY_STMT (stmt))
	{
	  stmt = build1_v (OMP_SECTION, stmt);
	  gfc_add_expr_to_block (&body, stmt);
	}
    }
  stmt = gfc_finish_block (&body);

  stmt = build2_loc (input_location, OMP_SECTIONS, void_type_node, stmt,
		     omp_clauses);
  gfc_add_expr_to_block (&block, stmt);

  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_single (gfc_code *code, gfc_omp_clauses *clauses)
{
  tree omp_clauses = gfc_trans_omp_clauses (NULL, clauses, code->loc);
  tree stmt = gfc_trans_omp_code (code->block->next, true);
  stmt = build2_loc (input_location, OMP_SINGLE, void_type_node, stmt,
		     omp_clauses);
  return stmt;
}

static tree
gfc_trans_omp_task (gfc_code *code)
{
  stmtblock_t block;
  tree stmt, omp_clauses;

  gfc_start_block (&block);
  omp_clauses = gfc_trans_omp_clauses (&block, code->ext.omp_clauses,
				       code->loc);
  stmt = gfc_trans_omp_code (code->block->next, true);
  stmt = build2_loc (input_location, OMP_TASK, void_type_node, stmt,
		     omp_clauses);
  gfc_add_expr_to_block (&block, stmt);
  return gfc_finish_block (&block);
}

static tree
gfc_trans_omp_taskgroup (gfc_code *code)
{
  tree stmt = gfc_trans_code (code->block->next);
  return build1_loc (input_location, OMP_TASKGROUP, void_type_node, stmt);
}

static tree
gfc_trans_omp_taskwait (void)
{
  tree decl = builtin_decl_explicit (BUILT_IN_GOMP_TASKWAIT);
  return build_call_expr_loc (input_location, decl, 0);
}

static tree
gfc_trans_omp_taskyield (void)
{
  tree decl = builtin_decl_explicit (BUILT_IN_GOMP_TASKYIELD);
  return build_call_expr_loc (input_location, decl, 0);
}

static tree
gfc_trans_omp_workshare (gfc_code *code, gfc_omp_clauses *clauses)
{
  tree res, tmp, stmt;
  stmtblock_t block, *pblock = NULL;
  stmtblock_t singleblock;
  int saved_ompws_flags;
  bool singleblock_in_progress = false;
  /* True if previous gfc_code in workshare construct is not workshared.  */
  bool prev_singleunit;

  code = code->block->next;

  pushlevel ();

  gfc_start_block (&block);
  pblock = &block;

  ompws_flags = OMPWS_WORKSHARE_FLAG;
  prev_singleunit = false;

  /* Translate statements one by one to trees until we reach
     the end of the workshare construct.  Adjacent gfc_codes that
     are a single unit of work are clustered and encapsulated in a
     single OMP_SINGLE construct.  */
  for (; code; code = code->next)
    {
      if (code->here != 0)
	{
	  res = gfc_trans_label_here (code);
	  gfc_add_expr_to_block (pblock, res);
	}

      /* No dependence analysis, use for clauses with wait.
	 If this is the last gfc_code, use default omp_clauses.  */
      if (code->next == NULL && clauses->nowait)
	ompws_flags |= OMPWS_NOWAIT;

      /* By default, every gfc_code is a single unit of work.  */
      ompws_flags |= OMPWS_CURR_SINGLEUNIT;
      ompws_flags &= ~OMPWS_SCALARIZER_WS;

      switch (code->op)
	{
	case EXEC_NOP:
	  res = NULL_TREE;
	  break;

	case EXEC_ASSIGN:
	  res = gfc_trans_assign (code);
	  break;

	case EXEC_POINTER_ASSIGN:
	  res = gfc_trans_pointer_assign (code);
	  break;

	case EXEC_INIT_ASSIGN:
	  res = gfc_trans_init_assign (code);
	  break;

	case EXEC_FORALL:
	  res = gfc_trans_forall (code);
	  break;

	case EXEC_WHERE:
	  res = gfc_trans_where (code);
	  break;

	case EXEC_OMP_ATOMIC:
	  res = gfc_trans_omp_directive (code);
	  break;

	case EXEC_OMP_PARALLEL:
	case EXEC_OMP_PARALLEL_DO:
	case EXEC_OMP_PARALLEL_SECTIONS:
	case EXEC_OMP_PARALLEL_WORKSHARE:
	case EXEC_OMP_CRITICAL:
	  saved_ompws_flags = ompws_flags;
	  ompws_flags = 0;
	  res = gfc_trans_omp_directive (code);
	  ompws_flags = saved_ompws_flags;
	  break;
	
	default:
	  internal_error ("gfc_trans_omp_workshare(): Bad statement code");
	}

      gfc_set_backend_locus (&code->loc);

      if (res != NULL_TREE && ! IS_EMPTY_STMT (res))
	{
	  if (prev_singleunit)
	    {
	      if (ompws_flags & OMPWS_CURR_SINGLEUNIT)
		/* Add current gfc_code to single block.  */
		gfc_add_expr_to_block (&singleblock, res);
	      else
		{
		  /* Finish single block and add it to pblock.  */
		  tmp = gfc_finish_block (&singleblock);
		  tmp = build2_loc (input_location, OMP_SINGLE,
				    void_type_node, tmp, NULL_TREE);
		  gfc_add_expr_to_block (pblock, tmp);
		  /* Add current gfc_code to pblock.  */
		  gfc_add_expr_to_block (pblock, res);
		  singleblock_in_progress = false;
		}
	    }
	  else
	    {
	      if (ompws_flags & OMPWS_CURR_SINGLEUNIT)
		{
		  /* Start single block.  */
		  gfc_init_block (&singleblock);
		  gfc_add_expr_to_block (&singleblock, res);
		  singleblock_in_progress = true;
		}
	      else
		/* Add the new statement to the block.  */
		gfc_add_expr_to_block (pblock, res);
	    }
	  prev_singleunit = (ompws_flags & OMPWS_CURR_SINGLEUNIT) != 0;
	}
    }

  /* Finish remaining SINGLE block, if we were in the middle of one.  */
  if (singleblock_in_progress)
    {
      /* Finish single block and add it to pblock.  */
      tmp = gfc_finish_block (&singleblock);
      tmp = build2_loc (input_location, OMP_SINGLE, void_type_node, tmp,
			clauses->nowait
			? build_omp_clause (input_location, OMP_CLAUSE_NOWAIT)
			: NULL_TREE);
      gfc_add_expr_to_block (pblock, tmp);
    }

  stmt = gfc_finish_block (pblock);
  if (TREE_CODE (stmt) != BIND_EXPR)
    {
      if (!IS_EMPTY_STMT (stmt))
	{
	  tree bindblock = poplevel (1, 0);
	  stmt = build3_v (BIND_EXPR, NULL, stmt, bindblock);
	}
      else
	poplevel (0, 0);
    }
  else
    poplevel (0, 0);

  if (IS_EMPTY_STMT (stmt) && !clauses->nowait)
    stmt = gfc_trans_omp_barrier ();

  ompws_flags = 0;
  return stmt;
}

tree
gfc_trans_oacc_declare (stmtblock_t *block, gfc_namespace *ns)
{
  tree oacc_clauses;
  oacc_clauses = gfc_trans_omp_clauses (block, ns->oacc_declare_clauses,
					ns->oacc_declare_clauses->ext.loc);
  return build1_loc (ns->oacc_declare_clauses->ext.loc.lb->location, 
		     OACC_DECLARE, void_type_node, oacc_clauses);
}

tree
gfc_trans_oacc_directive (gfc_code *code)
{
  switch (code->op)
    {
    case EXEC_OACC_PARALLEL_LOOP:
    case EXEC_OACC_KERNELS_LOOP:
      return gfc_trans_oacc_combined_directive (code);
    case EXEC_OACC_PARALLEL:
    case EXEC_OACC_KERNELS:
    case EXEC_OACC_DATA:
    case EXEC_OACC_HOST_DATA:
      return gfc_trans_oacc_construct (code);
    case EXEC_OACC_LOOP:
      return gfc_trans_omp_do (code, code->op, NULL, code->ext.omp_clauses,
			       NULL);
    case EXEC_OACC_UPDATE:
    case EXEC_OACC_WAIT:
    case EXEC_OACC_CACHE:
    case EXEC_OACC_ENTER_DATA:
    case EXEC_OACC_EXIT_DATA:
      return gfc_trans_oacc_executable_directive (code);
    default:
      gcc_unreachable ();
    }
}

tree
gfc_trans_omp_directive (gfc_code *code)
{
  switch (code->op)
    {
    case EXEC_OMP_ATOMIC:
      return gfc_trans_omp_atomic (code);
    case EXEC_OMP_BARRIER:
      return gfc_trans_omp_barrier ();
    case EXEC_OMP_CANCEL:
      return gfc_trans_omp_cancel (code);
    case EXEC_OMP_CANCELLATION_POINT:
      return gfc_trans_omp_cancellation_point (code);
    case EXEC_OMP_CRITICAL:
      return gfc_trans_omp_critical (code);
    case EXEC_OMP_DO:
    case EXEC_OMP_SIMD:
      return gfc_trans_omp_do (code, code->op, NULL, code->ext.omp_clauses,
			       NULL);
    case EXEC_OMP_DO_SIMD:
      return gfc_trans_omp_do_simd (code, NULL, NULL_TREE);
    case EXEC_OMP_FLUSH:
      return gfc_trans_omp_flush ();
    case EXEC_OMP_MASTER:
      return gfc_trans_omp_master (code);
    case EXEC_OMP_ORDERED:
      return gfc_trans_omp_ordered (code);
    case EXEC_OMP_PARALLEL:
      return gfc_trans_omp_parallel (code);
    case EXEC_OMP_PARALLEL_DO:
      return gfc_trans_omp_parallel_do (code);
    case EXEC_OMP_PARALLEL_DO_SIMD:
      return gfc_trans_omp_parallel_do_simd (code);
    case EXEC_OMP_PARALLEL_SECTIONS:
      return gfc_trans_omp_parallel_sections (code);
    case EXEC_OMP_PARALLEL_WORKSHARE:
      return gfc_trans_omp_parallel_workshare (code);
    case EXEC_OMP_SECTIONS:
      return gfc_trans_omp_sections (code, code->ext.omp_clauses);
    case EXEC_OMP_SINGLE:
      return gfc_trans_omp_single (code, code->ext.omp_clauses);
    case EXEC_OMP_TASK:
      return gfc_trans_omp_task (code);
    case EXEC_OMP_TASKGROUP:
      return gfc_trans_omp_taskgroup (code);
    case EXEC_OMP_TASKWAIT:
      return gfc_trans_omp_taskwait ();
    case EXEC_OMP_TASKYIELD:
      return gfc_trans_omp_taskyield ();
    case EXEC_OMP_WORKSHARE:
      return gfc_trans_omp_workshare (code, code->ext.omp_clauses);
    default:
      gcc_unreachable ();
    }
}

void
gfc_trans_omp_declare_simd (gfc_namespace *ns)
{
  if (ns->entries)
    return;

  gfc_omp_declare_simd *ods;
  for (ods = ns->omp_declare_simd; ods; ods = ods->next)
    {
      tree c = gfc_trans_omp_clauses (NULL, ods->clauses, ods->where, true);
      tree fndecl = ns->proc_name->backend_decl;
      if (c != NULL_TREE)
	c = tree_cons (NULL_TREE, c, NULL_TREE);
      c = build_tree_list (get_identifier ("omp declare simd"), c);
      TREE_CHAIN (c) = DECL_ATTRIBUTES (fndecl);
      DECL_ATTRIBUTES (fndecl) = c;
    }
}
