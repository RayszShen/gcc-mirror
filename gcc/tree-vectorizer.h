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

#ifndef GCC_TREE_VECTORIZER_H
#define GCC_TREE_VECTORIZER_H

#ifdef USE_MAPPED_LOCATION
  typedef source_location LOC;
  #define UNKNOWN_LOC UNKNOWN_LOCATION
  #define EXPR_LOC(e) EXPR_LOCATION(e)
  #define LOC_FILE(l) LOCATION_FILE (l)
  #define LOC_LINE(l) LOCATION_LINE (l)
#else
  typedef source_locus LOC;
  #define UNKNOWN_LOC NULL
  #define EXPR_LOC(e) EXPR_LOCUS(e)
  #define LOC_FILE(l) (l)->file
  #define LOC_LINE(l) (l)->line
#endif

/* Used for naming of new temporaries.  */
enum vect_var_kind {
  vect_simple_var,
  vect_pointer_var,
  vect_scalar_var
};

/* Defines type of operation: unary or binary.  */
enum operation_type {
  unary_op = 1,
  binary_op
};

/* Define type of available alignment support.  */
enum dr_alignment_support {
  dr_unaligned_unsupported,
  dr_unaligned_supported,
  dr_unaligned_software_pipeline,
  dr_aligned
};

/* Define type of def-use cross-iteraiton cycle.  */
enum vect_def_type {
  vect_constant_def,
  vect_invariant_def,
  vect_loop_def,
  vect_induction_def,
  vect_reduction_def,
  vect_unknown_def_type
};

/* Define verbosity levels.  */
enum verbosity_levels {
  REPORT_NONE,
  REPORT_VECTORIZED_LOOPS,
  REPORT_UNVECTORIZED_LOOPS,
  REPORT_ALIGNMENT,
  REPORT_BAD_FORM_LOOPS,
  REPORT_OUTER_LOOPS,
  REPORT_DETAILS,
  /* New verbosity levels should be added before this one.  */
  MAX_VERBOSITY_LEVEL
};

/*-----------------------------------------------------------------*/
/* Info on vectorized loops.                                       */
/*-----------------------------------------------------------------*/
typedef struct _loop_vec_info {

  /* The loop to which this info struct refers to.  */
  struct loop *loop;

  /* The loop basic blocks.  */
  basic_block *bbs;

  /* The loop exit_condition.  */
  tree exit_cond;

  /* Number of iterations.  */
  tree num_iters;

  /* Is the loop vectorizable? */
  bool vectorizable;

  /* Unrolling factor  */
  int vectorization_factor;

  /* Unknown DRs according to which loop was peeled.  */
  struct data_reference *unaligned_dr;

  /* peeling_for_alignment indicates whether peeling for alignment will take
     place, and what the peeling factor should be:
     peeling_for_alignment = X means: 
        If X=0: Peeling for alignment will not be applied.
        If X>0: Peel first X iterations. 
        If X=-1: Generate a runtime test to calculate the number of iterations
                 to be peeled, using the dataref recorded in the field
                 unaligned_dr.  */
  int peeling_for_alignment;

  /* The mask used to check the alignment of pointers or arrays.  */
  int ptr_mask;
  
  /* All data references in the loop.  */
  varray_type datarefs;

  /* All data dependences in the loop.  */
  varray_type ddrs;

  /* Statements in the loop that have data references that are candidates for a
     runtime (loop versioning) misalignment check.  */
  varray_type may_misalign_stmts;

  /* The loop location in the source.  */
  LOC loop_line_number;
} *loop_vec_info;

/* Access Functions.  */
#define LOOP_VINFO_LOOP(L)           (L)->loop
#define LOOP_VINFO_BBS(L)            (L)->bbs
#define LOOP_VINFO_EXIT_COND(L)      (L)->exit_cond
#define LOOP_VINFO_NITERS(L)         (L)->num_iters
#define LOOP_VINFO_VECTORIZABLE_P(L) (L)->vectorizable
#define LOOP_VINFO_VECT_FACTOR(L)    (L)->vectorization_factor
#define LOOP_VINFO_PTR_MASK(L)       (L)->ptr_mask
#define LOOP_VINFO_DATAREFS(L)       (L)->datarefs
#define LOOP_VINFO_DDRS(L)           (L)->ddrs
#define LOOP_VINFO_INT_NITERS(L) (TREE_INT_CST_LOW ((L)->num_iters))
#define LOOP_PEELING_FOR_ALIGNMENT(L) (L)->peeling_for_alignment
#define LOOP_VINFO_UNALIGNED_DR(L) (L)->unaligned_dr
#define LOOP_VINFO_MAY_MISALIGN_STMTS(L) (L)->may_misalign_stmts
#define LOOP_VINFO_LOC(L)          (L)->loop_line_number

#define LOOP_LOC(L)    LOOP_VINFO_LOC(L)


#define LOOP_VINFO_NITERS_KNOWN_P(L)                     \
(host_integerp ((L)->num_iters,0)                        \
&& TREE_INT_CST_LOW ((L)->num_iters) > 0)

/*-----------------------------------------------------------------*/
/* Info on vectorized defs.                                        */
/*-----------------------------------------------------------------*/
enum stmt_vec_info_type {
  undef_vec_info_type = 0,
  load_vec_info_type,
  store_vec_info_type,
  op_vec_info_type,
  assignment_vec_info_type,
  select_vec_info_type,
  reduc_vec_info_type
};

typedef struct _stmt_vec_info {

  enum stmt_vec_info_type type;

  /* The stmt to which this info struct refers to.  */
  tree stmt;

  /* The loop_vec_info with respect to which STMT is vectorized.  */
  loop_vec_info loop_vinfo;

  /* Not all stmts in the loop need to be vectorized. e.g, the incrementation
     of the loop induction variable and computation of array indexes. relevant
     indicates whether the stmt needs to be vectorized.  */
  bool relevant;

  /* Indicates whether this stmts is part of a computation whose result is
     used outside the loop.  */
  bool live;

  /* The vector type to be used.  */
  tree vectype;

  /* The vectorized version of the stmt.  */
  tree vectorized_stmt;


  /** The following is relevant only for stmts that contain a non-scalar
     data-ref (array/pointer/struct access). A GIMPLE stmt is expected to have 
     at most one such data-ref.  **/

  /* Information about the data-ref (access function, etc).  */
  struct data_reference *data_ref_info;

  /* Stmt is part of some pattern (computation idiom)  */
  bool in_pattern_p;

  /* If this stmt is part of a pattern (in_pattern_p is true):
        related_stmt is the stmt whose vectorized_stmt should be used to get
	the relevant vector-def that will replace the scalar-def of this stmt.
     Otherwise (this stmt is not part of a pattern): 
        currently used only if this stmt is a new stmt that replaces a 
        computation pattern. related_stmt in this case is the last stmt in the
        original pattern.  */
  tree related_stmt;

  /* List of datarefs that are known to have the same alignment as the dataref
     of this stmt.  */
  varray_type same_align_refs; 

  /* Classify the def of this stmt.  */
  enum vect_def_type def_type;

  /* The loop-closed-form loop-exit phi for after-the-loop-uses of 
     this stmt's def, if exist.  */
  tree after_loop_use;
} *stmt_vec_info;

/* Access Functions.  */
#define STMT_VINFO_TYPE(S)                (S)->type
#define STMT_VINFO_STMT(S)                (S)->stmt
#define STMT_VINFO_LOOP_VINFO(S)          (S)->loop_vinfo
#define STMT_VINFO_RELEVANT_P(S)          (S)->relevant
#define STMT_VINFO_LIVE_P(S)          	  (S)->live
#define STMT_VINFO_VECTYPE(S)             (S)->vectype
#define STMT_VINFO_VEC_STMT(S)            (S)->vectorized_stmt
#define STMT_VINFO_DATA_REF(S)            (S)->data_ref_info
#define STMT_VINFO_IN_PATTERN_P(S)        (S)->in_pattern_p
#define STMT_VINFO_RELATED_STMT(S)        (S)->related_stmt
#define STMT_VINFO_SAME_ALIGN_REFS(S)	  (S)->same_align_refs
#define STMT_VINFO_DEF_TYPE(S)            (S)->def_type
#define STMT_VINFO_EXTERNAL_USE(S)	  (S)->after_loop_use

static inline void set_stmt_info (tree_ann_t ann, stmt_vec_info stmt_info);
static inline stmt_vec_info vinfo_for_stmt (tree stmt);

static inline void
set_stmt_info (tree_ann_t ann, stmt_vec_info stmt_info)
{
  if (ann)
    ann->common.aux = (char *) stmt_info;
}

static inline stmt_vec_info
vinfo_for_stmt (tree stmt)
{
  tree_ann_t ann = tree_ann (stmt);
  return ann ? (stmt_vec_info) ann->common.aux : NULL;
}


/*-----------------------------------------------------------------*/
/* Info on data references alignment.                              */
/*-----------------------------------------------------------------*/

/* FORNOW: the number of alignment checks may change in the future to
   a target specific compilation flag.  */
#define MAX_RUNTIME_ALIGNMENT_CHECKS 6

/* Reflects actual alignment of first access in the vectorized loop,
   taking into account peeling/versioning if applied.  */
#define DR_MISALIGNMENT(DR) (DR)->aux  

static inline bool
aligned_access_p (struct data_reference *data_ref_info)
{
  return (DR_MISALIGNMENT (data_ref_info) == 0);
}

static inline bool
known_alignment_for_access_p (struct data_reference *data_ref_info)
{
  return (DR_MISALIGNMENT (data_ref_info) != -1);
}

/* Perform signed modulo, always returning a non-negative value.  */
#define VECT_SMODULO(x,y) ((x) % (y) < 0 ? ((x) % (y) + (y)) : (x) % (y))

/* vect_dump will be set to stderr or dump_file if exist.  */
extern FILE *vect_dump;
extern enum verbosity_levels vect_verbosity_level;
extern unsigned int loops_num;

/*-----------------------------------------------------------------*/
/* Function prototypes.                                            */
/*-----------------------------------------------------------------*/

/*************************************************************************
  Simple Loop Peeling Utilities - in tree-vectorizer.c
 *************************************************************************/
/* Entry point for peeling of simple loops.
   Peel the first/last iterations of a loop.
   It can be used outside of the vectorizer for loops that are simple enough
   (see function documentation).  In the vectorizer it is used to peel the
   last few iterations when the loop bound is unknown or does not evenly
   divide by the vectorization factor, and to peel the first few iterations
   to force the alignment of data references in the loop.  */
extern struct loop *slpeel_tree_peel_loop_to_edge 
  (struct loop *, struct loops *, edge, tree, tree, bool);
extern void slpeel_make_loop_iterate_ntimes (struct loop *, tree);
extern bool slpeel_can_duplicate_loop_p (struct loop *, edge);
#ifdef ENABLE_CHECKING
extern void slpeel_verify_cfg_after_peeling (struct loop *, struct loop *);
#endif


/*************************************************************************
  General Vectorization Utilities
 *************************************************************************/
/** In tree-vectorizer.c **/
extern tree vect_strip_conversion (tree);
extern tree get_vectype_for_scalar_type (tree);
extern bool vect_is_simple_use (tree, loop_vec_info, tree *, tree *, 
				enum vect_def_type *);
extern bool vect_is_simple_iv_evolution (unsigned, tree, tree *, tree *);
extern tree vect_is_simple_reduction (struct loop *, tree);
extern bool vect_can_force_dr_alignment_p (tree, unsigned int);
extern enum dr_alignment_support vect_supportable_dr_alignment
  (struct data_reference *);
extern bool reduction_code_for_scalar_code (enum tree_code, enum tree_code *);

/* Creation and deletion of loop and stmt info structs.  */
extern loop_vec_info new_loop_vec_info (struct loop *loop);
extern void destroy_loop_vec_info (loop_vec_info);
extern stmt_vec_info new_stmt_vec_info (tree stmt, loop_vec_info);
/* Main driver.  */
extern void vectorize_loops (struct loops *);

/** In tree-vect-analyze.c  **/
/* Driver for analysis stage.  */
extern loop_vec_info vect_analyze_loop (struct loop *);
/* Pattern recognition functions.  */
tree vect_recog_unsigned_subsat_pattern (tree, varray_type *);
typedef tree (* _recog_func_ptr) (tree, varray_type *);
/* Additional pattern recognition functions can (and will) be added
   in the future.  */
#define NUM_PATTERNS 1
extern _recog_func_ptr vect_pattern_recog_func[NUM_PATTERNS];

/** In tree-vect-transform.c  **/
extern bool vectorizable_load (tree, block_stmt_iterator *, tree *);
extern bool vectorizable_store (tree, block_stmt_iterator *, tree *);
extern bool vectorizable_operation (tree, block_stmt_iterator *, tree *);
extern bool vectorizable_assignment (tree, block_stmt_iterator *, tree *);
extern bool vectorizable_reduction (tree, block_stmt_iterator *, tree *);
extern bool vectorizable_select (tree, block_stmt_iterator *, tree *);
/* Driver for transformation stage.  */
extern void vect_transform_loop (loop_vec_info, struct loops *);

/*************************************************************************
  Vectorization Debug Information - in tree-vectorizer.c
 *************************************************************************/
extern bool vect_print_dump_info (enum verbosity_levels, LOC);
extern void vect_set_verbosity_level (const char *);
extern LOC find_loop_location (struct loop *);

#endif  /* GCC_TREE_VECTORIZER_H  */
