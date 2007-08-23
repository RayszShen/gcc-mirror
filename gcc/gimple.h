/* Gimple IR definitions.

   Copyright 2007 Free Software Foundation, Inc.
   Contributed by Aldy Hernandez <aldyh@redhat.com>

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
Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.  */

#ifndef GCC_GIMPLE_IR_H
#define GCC_GIMPLE_IR_H

#include "pointer-set.h"

DEF_VEC_P(gimple);
DEF_VEC_ALLOC_P(gimple,heap);
DEF_VEC_ALLOC_P(gimple,gc);

DEF_VEC_P(gimple_seq);
DEF_VEC_ALLOC_P(gimple_seq,gc);
DEF_VEC_ALLOC_P(gimple_seq,heap);

enum gimple_code {
#define DEFGSCODE(SYM, STRING)	SYM,
#include "gimple.def"
#undef DEFGSCODE
    LAST_AND_UNUSED_GIMPLE_CODE
};


/* A sequence of gimple statements.  */
struct gimple_sequence GTY(())
{
  gimple first;
  gimple last;
};

static inline gimple
gimple_seq_first (gimple_seq s)
{
  return s->first;
}

static inline gimple
gimple_seq_last (gimple_seq s)
{
  return s->last;
}

static inline void
gimple_seq_set_last (gimple_seq s, gimple last)
{
  s->last = last;
}

static inline void
gimple_seq_set_first (gimple_seq s, gimple first)
{
  s->first = first;
}

static inline void
gimple_seq_init (gimple_seq s)
{
  s->first = NULL;
  s->last = NULL;
}

/* Copy the sequence SRC into the sequence DEST.  */

static inline void
gimple_seq_copy (gimple_seq dest, gimple_seq src)
{
  gimple_seq_set_first (dest, gimple_seq_first (src));
  gimple_seq_set_last (dest, gimple_seq_last (src));
}

static inline bool
gimple_seq_empty_p (gimple_seq s)
{
  return s->first == NULL;
}

/* Data structure definitions for GIMPLE tuples.  */

struct gimple_statement_base GTY(())
{
  ENUM_BITFIELD(gimple_code) code : 16;
  unsigned int flags : 16;
  gimple next;
  gimple prev;
  struct basic_block_def *bb;
  location_t locus;
  tree block;
};

struct gimple_statement_with_ops GTY(())
{
  struct gimple_statement_base base;
  unsigned modified : 1;

  /* FIXME tuples.  OP should be amalgamated with DEF_OPS and USE_OPS.
     This duplication is unnecessary.  */
  struct def_optype_d GTY((skip)) *def_ops;
  struct use_optype_d GTY((skip)) *use_ops;

  /* FIXME tuples.  For many tuples, the number of operands can
     be deduced from the code.  */
  size_t num_ops;
  tree * GTY((length ("%h.num_ops"))) op;
};

struct gimple_statement_with_memory_ops GTY(())
{
  struct gimple_statement_with_ops with_ops;
  unsigned has_volatile_ops : 1;
  struct voptype_d GTY((skip)) *vdef_ops;
  struct voptype_d GTY((skip)) *vuse_ops;
  bitmap stores;
  bitmap loads;
};

struct gimple_statement_omp GTY(())
{
  struct gimple_statement_base base;
  struct gimple_sequence body;
};

/* GIMPLE_BIND */
struct gimple_statement_bind GTY(())
{
  struct gimple_statement_base base;
  tree vars;

  /* This is different than the ``block'' in gimple_statement_base, which
     is analogous to TREE_BLOCK.  This block is the equivalent of
     BIND_EXPR_BLOCK in tree land.  See gimple-low.c.  */
  tree block;

  struct gimple_sequence body;
};

/* GIMPLE_CATCH */
struct gimple_statement_catch GTY(())
{
  struct gimple_statement_base base;
  tree types;
  gimple_seq handler;
};

/* GIMPLE_EH_FILTER */
struct gimple_statement_eh_filter GTY(())
{
  struct gimple_statement_base base;
  /* Filter types.  */
  tree types;
  /* Failure actions.  */
  gimple_seq failure;
};

/* GIMPLE_PHI */
struct gimple_statement_phi GTY(())
{
  struct gimple_statement_base base;
  unsigned capacity;
  unsigned nargs;
  tree result;
  struct phi_arg_d GTY ((length ("%h.nargs"))) args[1];
};

/* GIMPLE_RESX */
struct gimple_statement_resx GTY(())
{
  struct gimple_statement_base base;

  /* Exception region number.  */
  int region;
};

/* GIMPLE_TRY */
struct gimple_statement_try GTY(())
{
  struct gimple_statement_base base;

  /* Expression to evaluate.  */
  struct gimple_sequence eval;

  /* Cleanup expression.  */
  struct gimple_sequence cleanup;
};

/* Flags stored in GIMPLE_TRY's subcode flags.  */
#define GIMPLE_TRY_CATCH 1 << 0
#define GIMPLE_TRY_FINALLY 1 << 1


/* GIMPLE_ASM */
struct gimple_statement_asm GTY(())
{
  struct gimple_statement_with_memory_ops with_mem_ops;
  const char *string;
  unsigned ni;		/* Number of inputs.  */
  unsigned no;		/* Number of outputs.  */
  unsigned nc;		/* Number of clobbers.  */
};


/* GIMPLE_OMP_CRITICAL */
struct gimple_statement_omp_critical GTY(())
{
  struct gimple_statement_omp omp;
  /* Critical section name.  */
  tree name;
};

/* GIMPLE_OMP_FOR */
struct gimple_statement_omp_for GTY(())
{
  struct gimple_statement_omp omp;
  tree clauses;
  /* Index variable.  */
  tree index;
  /* Initial value.  */
  tree initial;
  /* Final value.  */
  tree final;
  /* Increment.  */
  tree incr;
  /* Pre-body evaluated before the loop body begins.  */
  struct gimple_sequence pre_body;
};


/* Predicate for conds. */
enum gimple_cond {
  /* These must be in sync with op_gimple_cond().  */
  GIMPLE_COND_LT, GIMPLE_COND_GT, GIMPLE_COND_LE, GIMPLE_COND_GE,
  GIMPLE_COND_EQ, GIMPLE_COND_NE
};

/* GIMPLE_OMP_PARALLEL */
struct gimple_statement_omp_parallel GTY(())
{
  struct gimple_statement_omp omp;
  tree clauses;
  tree child_fn;
  /* Shared data argument.  */
  tree data_arg;
};

/* GIMPLE_OMP_SECTION */
/* Uses struct gimple_statement_omp.  */

/* GIMPLE_OMP_SECTIONS */
struct gimple_statement_omp_sections GTY(())
{
  struct gimple_statement_omp omp;
  tree clauses;
};

/* GIMPLE_OMP_SINGLE */
struct gimple_statement_omp_single GTY(())
{
  struct gimple_statement_omp omp;
  tree clauses;
};

/* GIMPLE_OMP_RETURN */
/* Flags stored in GIMPLE_OMP_RETURN's flags.  */
#define OMP_RETURN_NOWAIT_FLAG 1 << 0

/* GIMPLE_OMP_SECTION */
/* Flags stored in GIMPLE_OMP_SECTION's flags.  */
#define OMP_SECTION_LAST_FLAG 1 << 0

/* GIMPLE_OMP_PARALLEL */
/* Flags stored in GIMPLE_OMP_PARALLEL's flags.  */
#define OMP_PARALLEL_COMBINED_FLAG 1 << 0

enum gimple_statement_structure_enum {
#define DEFGSSTRUCT(SYM, STRING)	SYM,
#include "gsstruct.def"
#undef DEFGSSTRUCT
    LAST_GSS_ENUM
};


/* Define the overall contents of a gimple tuple.  It may be any of the
   structures declared above for various types of tuples.  */

union gimple_statement_d GTY ((desc ("gimple_statement_structure (&%h)")))
{
  struct gimple_statement_base GTY ((tag ("GSS_BASE"))) base;
  struct gimple_statement_with_ops GTY ((tag ("GSS_WITH_OPS"))) with_ops;
  struct gimple_statement_with_memory_ops GTY ((tag ("GSS_WITH_MEM_OPS"))) with_mem_ops;
  struct gimple_statement_omp GTY ((tag ("GSS_OMP"))) omp;
  struct gimple_statement_bind GTY ((tag ("GSS_BIND"))) gimple_bind;
  struct gimple_statement_catch GTY ((tag ("GSS_CATCH"))) gimple_catch;
  struct gimple_statement_eh_filter GTY ((tag ("GSS_EH_FILTER"))) gimple_eh_filter;
  struct gimple_statement_phi GTY ((tag ("GSS_PHI"))) gimple_phi;
  struct gimple_statement_resx GTY ((tag ("GSS_RESX"))) gimple_resx;
  struct gimple_statement_try GTY ((tag ("GSS_TRY"))) gimple_try;
  struct gimple_statement_asm GTY ((tag ("GSS_ASM"))) gimple_asm;
  struct gimple_statement_omp_critical GTY ((tag ("GSS_OMP_CRITICAL"))) gimple_omp_critical;
  struct gimple_statement_omp_for GTY ((tag ("GSS_OMP_FOR"))) gimple_omp_for;
  struct gimple_statement_omp_parallel GTY ((tag ("GSS_OMP_PARALLEL"))) gimple_omp_parallel;
  struct gimple_statement_omp_sections GTY ((tag ("GSS_OMP_SECTIONS"))) gimple_omp_sections;
  struct gimple_statement_omp_single GTY ((tag ("GSS_OMP_SINGLE"))) gimple_omp_single;
};


/* Set PREV to be the previous statement to G.  */

static inline void
set_gimple_prev (gimple g, gimple prev)
{
  g->base.prev = prev;
}


/* Set NEXT to be the next statement to G.  */

static inline void
set_gimple_next (gimple g, gimple next)
{
  g->base.next = next;
}


/* Common accessors for all GIMPLE statements.  */

static inline enum gimple_code
gimple_code (gimple g)
{
  return g->base.code;
}

static inline unsigned
gimple_flags (gimple g)
{
  return g->base.flags;
}

static inline void
set_gimple_flags (gimple g, unsigned int flags)
{
  g->base.flags = flags;
}

static inline void
add_gimple_flag (gimple g, unsigned int flag)
{
  g->base.flags |= flag;
}

static inline gimple
gimple_next (gimple g)
{
  return g->base.next;
}

static inline gimple
gimple_prev (gimple g)
{
  return g->base.prev;
}

static inline struct basic_block_def *
gimple_bb (gimple g)
{
  return g->base.bb;
}

static inline void
set_gimple_bb (gimple g, struct basic_block_def * bb)
{
  g->base.bb = bb;
}

static inline tree
gimple_block (gimple g)
{
  return g->base.block;
}

static inline void
set_gimple_block (gimple g, tree block)
{
  g->base.block = block;
}

static inline location_t
gimple_locus (gimple g)
{
  return g->base.locus;
}

static inline void
set_gimple_locus (gimple g, location_t locus)
{
  g->base.locus = locus;
}

static inline bool
gimple_locus_empty_p (gimple g)
{
  return gimple_locus (g).file == NULL && gimple_locus (g).line == 0;
}


/* Return TRUE if a gimple statement has register or memory operands.  */

static inline bool
gimple_has_ops (gimple g)
{
  return gimple_code (g) >= GIMPLE_ASSIGN && gimple_code (g) <= GIMPLE_RETURN;
}


/* Return TRUE if the given statement has operands and the modified
   field has been set.  */

static inline bool
gimple_modified (gimple g)
{
  if (gimple_has_ops (g))
    return (bool) g->with_ops.modified;
  else
    return false;
}


/* Set the MODIFIED flag to MODIFIEDP, iff the gimple stmt G has a
   MODIFIED field.  */

static inline void
set_gimple_modified (gimple g, bool modifiedp)
{
  if (gimple_has_ops (g))
    g->with_ops.modified = (unsigned) modifiedp;
}


/* Returns TRUE if a statement is a GIMPLE_OMP_RETURN and has the
   OMP_RETURN_NOWAIT_FLAG set.  */

static inline bool
gimple_omp_return_nowait_p (gimple g)
{
  gcc_assert (gimple_code (g) == GIMPLE_OMP_RETURN);
  return gimple_flags (g) & OMP_RETURN_NOWAIT_FLAG;
}


/* Returns TRUE if a statement is a GIMPLE_OMP_SECTION and has the
   OMP_SECTION_LAST_FLAG set.  */

static inline bool
gimple_omp_section_last_p (gimple g)
{
  gcc_assert (gimple_code (g) == GIMPLE_OMP_SECTION);
  return gimple_flags (g) & OMP_SECTION_LAST_FLAG;
}


/* Returns TRUE if a GIMPLE_OMP_PARALLEL has the OMP_PARALLEL_COMBINED_FLAG
   set.  */

static inline bool
gimple_omp_parallel_combined_p (gimple g)
{
  gcc_assert (gimple_code (g) == GIMPLE_OMP_PARALLEL);
  return gimple_flags (g) & OMP_PARALLEL_COMBINED_FLAG;
}


/* Prototypes in gimple.c.  */
extern gimple build_gimple_return (bool, tree);
extern gimple build_gimple_assign (tree, tree);
extern gimple build_gimple_call_vec (tree, VEC(tree, gc) *);
extern gimple build_gimple_call (tree, size_t, ...);
extern gimple build_gimple_cond (enum gimple_cond, tree, tree, tree, tree);
extern void gimple_cond_invert (gimple);
extern gimple build_gimple_label (tree label);
extern gimple build_gimple_goto (tree dest);
extern gimple build_gimple_nop (void);
extern gimple build_gimple_bind (tree, gimple_seq);
extern gimple build_gimple_asm (const char *, unsigned, unsigned, unsigned,
                                ...);
extern gimple build_gimple_asm_vec (const char *, VEC(tree,gc) * , 
                                    VEC(tree,gc) *, VEC(tree,gc) *);
extern gimple build_gimple_catch (tree, gimple_seq);
extern gimple build_gimple_eh_filter (tree, gimple_seq);
extern gimple build_gimple_try (gimple_seq, gimple_seq, unsigned int);
extern gimple build_gimple_phi (unsigned, unsigned, tree, ...);
extern gimple build_gimple_resx (int);
extern gimple build_gimple_switch (unsigned int, tree, tree, ...);
extern gimple build_gimple_switch_vec (tree, tree, VEC(tree,heap) *);
extern gimple build_gimple_omp_parallel (gimple_seq, tree, tree, tree);
extern gimple build_gimple_omp_for (gimple_seq, tree, tree, tree, tree, tree,
                                    gimple_seq, enum gimple_cond);
extern gimple build_gimple_omp_critical (gimple_seq, tree);
extern gimple build_gimple_omp_section (gimple_seq);
extern gimple build_gimple_omp_continue (gimple_seq);
extern gimple build_gimple_omp_master (gimple_seq);
extern gimple build_gimple_omp_return (bool);
extern gimple build_gimple_omp_ordered (gimple_seq);
extern gimple build_gimple_omp_sections (gimple_seq, tree);
extern gimple build_gimple_omp_single (gimple_seq, tree);
extern enum gimple_statement_structure_enum gimple_statement_structure (gimple);
extern void gimple_add (gimple_seq, gimple);
extern enum gimple_statement_structure_enum gss_for_assign (enum tree_code);
extern void sort_case_labels (VEC(tree,heap) *);
extern void walk_tuple_ops (gimple, walk_tree_fn, void *,
    			    struct pointer_set_t *);
extern void walk_seq_ops (gimple_seq, walk_tree_fn, void *,
                          struct pointer_set_t *);
extern void set_gimple_body (tree, gimple_seq);
extern gimple_seq gimple_body (tree);
extern void gimple_seq_append (gimple_seq, gimple_seq);
extern int gimple_call_flags (gimple);

extern const char *const gimple_code_name[];


/* Error out if a gimple tuple is addressed incorrectly.  */
#if defined ENABLE_GIMPLE_CHECKING
extern void gimple_check_failed (const gimple, const char *, int,          \
                                 const char *, unsigned int, unsigned int) \
                                 ATTRIBUTE_NORETURN;                       \
extern void gimple_range_check_failed (const gimple, const char *, int,    \
                                       const char *, unsigned int,         \
				       unsigned int) ATTRIBUTE_NORETURN;

#define GIMPLE_CHECK(GS, CODE)						\
  do {									\
    const gimple __gs = (GS);						\
    if (gimple_code (__gs) != (CODE))					\
      gimple_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,	\
	  		   (CODE), 0);					\
  } while (0)

#define GIMPLE_CHECK2(GS, CODE1, CODE2)					\
  do {									\
    const gimple __gs = (GS);						\
    if (gimple_code (__gs) != (CODE1)					\
	|| gimple_flags (__gs) != (CODE2))				\
      gimple_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,	\
	  		   (CODE1), (CODE2));				\
  } while (0)

#define GIMPLE_RANGE_CHECK(GS, CODE1, CODE2)				\
  do {									\
    const gimple __gs = (GS);						\
    if (gimple_code (__gs) < (CODE1) || gimple_code (__gs) > (CODE2))	\
      gimple_range_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,\
		                 (CODE1), (CODE2));			\
  } while (0)
#else  /* not ENABLE_GIMPLE_CHECKING  */
#define GIMPLE_CHECK(GS, CODE)			(void)0
#define GIMPLE_CHECK2(GS, C1, C2)		(void)0
#define GIMPLE_RANGE_CHECK(GS, CODE1, CODE2)	(void)0
#endif


/* GIMPLE IR accessor functions.  */

/* GIMPLE_WITH_OPS and GIMPLE_WITH_MEM_OPS accessors.  */

static inline size_t
gimple_num_ops (gimple gs)
{
  GIMPLE_RANGE_CHECK (gs, GIMPLE_ASSIGN, GIMPLE_RETURN);
  return gs->with_ops.num_ops;
}

static inline tree
gimple_op (gimple gs, size_t i)
{
  GIMPLE_RANGE_CHECK (gs, GIMPLE_ASSIGN, GIMPLE_RETURN);
  gcc_assert (i < gs->with_ops.num_ops);
  return gs->with_ops.op[i];
}

static inline void
gimple_set_op (gimple gs, size_t i, tree op)
{
  GIMPLE_RANGE_CHECK (gs, GIMPLE_ASSIGN, GIMPLE_RETURN);
  gcc_assert (i < gs->with_ops.num_ops);
  gs->with_ops.op[i] = op;
}


/* GIMPLE_ASSIGN accessors.  */

static inline tree
gimple_assign_operand (gimple gs, size_t opno)
{
  GIMPLE_CHECK (gs, GIMPLE_ASSIGN);
  gcc_assert (gs->with_ops.num_ops > opno);
  return gs->with_ops.op[opno];
}

static inline void
gimple_assign_set_operand (gimple gs, size_t opno, tree op)
{
  GIMPLE_CHECK (gs, GIMPLE_ASSIGN);
  gcc_assert (gs->with_ops.num_ops > opno);
  gs->with_ops.op[opno] = op;
}

static inline tree
gimple_assign_lhs (gimple gs)
{
  return gimple_assign_operand (gs, 0);
}

static inline void
gimple_assign_set_lhs (gimple gs, tree lhs)
{
  gimple_assign_set_operand (gs, 0, lhs);
}

static inline tree
gimple_assign_rhs1 (gimple gs)
{
  return gimple_assign_operand (gs, 1);
}

static inline void
gimple_assign_set_rhs1 (gimple gs, tree rhs)
{
  gimple_assign_set_operand (gs, 1, rhs);
}

static inline tree
gimple_assign_rhs2 (gimple gs)
{
  return gimple_assign_operand (gs, 2);
}

static inline void
gimple_assign_set_rhs2 (gimple gs, tree rhs)
{
  gimple_assign_set_operand (gs, 2, rhs);
}

/* GIMPLE_CALL accessors. */

static inline tree
gimple_call_lhs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  return gs->with_ops.op[0];
}

static inline void
gimple_call_set_lhs (gimple gs, tree lhs)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gs->with_ops.op[0] = lhs;
}

static inline tree
gimple_call_fn (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > 1);
  return gs->with_ops.op[1];
}

/* If a given GIMPLE_CALL's callee is a FUNCTION_DECL, return it.
   Otherwise return NULL.  This function is analogous to
   get_callee_fndecl in tree land.  */

static inline tree
gimple_call_fndecl (gimple gs)
{
  tree decl;

  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > 1);
  decl = gs->with_ops.op[1];
  if (TREE_CODE (decl) == FUNCTION_DECL)
    return decl;
  else
    return NULL;
}

static inline tree
gimple_call_return_type (gimple gs)
{
  tree fn = gimple_call_fn (gs);
  tree type = TREE_TYPE (fn);

  /* See through pointer to functions.  */
  if (TREE_CODE (type) == POINTER_TYPE)
    type = TREE_TYPE (type);

  gcc_assert (TREE_CODE (type) == FUNCTION_TYPE);

  /* The type returned by a FUNCTION_DECL is the type of its
     function type.  */
  return TREE_TYPE (type);
}

static inline tree
gimple_call_chain (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > 2);
  return gs->with_ops.op[2];
}

static inline void
gimple_call_set_chain (gimple gs, tree chain)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > 2);
  gs->with_ops.op[2] = chain;
}

static inline unsigned long
gimple_call_nargs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops >= 3);
  return gs->with_ops.num_ops - 3;
}

static inline tree
gimple_call_arg (gimple gs, size_t index)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > index + 3);
  return gs->with_ops.op[index + 3];
}

static inline void
gimple_call_set_arg (gimple gs, size_t index, tree arg)
{
  GIMPLE_CHECK (gs, GIMPLE_CALL);
  gcc_assert (gs->with_ops.num_ops > index + 3);
  gs->with_ops.op[index + 3] = arg;
}


/* GIMPLE_COND accessors. */

static inline tree
gimple_cond_lhs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  return gs->with_ops.op[0];
}

static inline void
gimple_cond_set_lhs (gimple gs, tree lhs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  gs->with_ops.op[0] = lhs;
}

static inline tree
gimple_cond_rhs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  return gs->with_ops.op[1];
}

static inline void
gimple_cond_set_rhs (gimple gs, tree rhs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  gs->with_ops.op[1] = rhs;
}

static inline tree
gimple_cond_true_label (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  return gs->with_ops.op[2];
}

static inline void
gimple_cond_set_true_label (gimple gs, tree label)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  gs->with_ops.op[2] = label;
}

static inline void
gimple_cond_set_false_label (gimple gs, tree label)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  gs->with_ops.op[3] = label;
}

static inline tree
gimple_cond_false_label (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_COND);
  gcc_assert (gs->with_ops.num_ops == 4);
  return gs->with_ops.op[3];
}


/* GIMPLE_LABEL accessors. */

static inline tree
gimple_label_label (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_LABEL);
  gcc_assert (gs->with_ops.num_ops == 1);
  return gs->with_ops.op[0];
}

static inline void
gimple_label_set_label (gimple gs, tree label)
{
  GIMPLE_CHECK (gs, GIMPLE_LABEL);
  gcc_assert (gs->with_ops.num_ops == 1);
  gs->with_ops.op[0] = label;
}


/* GIMPLE_GOTO accessors. */

static inline tree
gimple_goto_dest (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_GOTO);
  gcc_assert (gs->with_ops.num_ops == 1);
  return gs->with_ops.op[0];
}

static inline void 
gimple_goto_set_dest (gimple gs, tree dest)
{
  GIMPLE_CHECK (gs, GIMPLE_GOTO);
  gcc_assert (gs->with_ops.num_ops == 1);
  gs->with_ops.op[0] = dest;
}


/* GIMPLE_BIND accessors. */

static inline tree
gimple_bind_vars (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  return gs->gimple_bind.vars;
}

static inline void
gimple_bind_set_vars (gimple gs, tree vars)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  gs->gimple_bind.vars = vars;
}

static inline gimple_seq
gimple_bind_body (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  return &(gs->gimple_bind.body);
}

static inline void
gimple_bind_set_body (gimple gs, gimple_seq seq)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  gimple_seq_copy (&(gs->gimple_bind.body), seq);
}

static inline tree
gimple_bind_block (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  return gs->gimple_bind.block;
}

static inline void
gimple_bind_set_block (gimple gs, tree block)
{
  GIMPLE_CHECK (gs, GIMPLE_BIND);
  gs->gimple_bind.block = block;
}


/* GIMPLE_ASM accessors. */

static inline unsigned int
gimple_asm_ninputs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  return gs->gimple_asm.ni;
}

static inline unsigned int
gimple_asm_noutputs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  return gs->gimple_asm.no;
}

static inline unsigned int
gimple_asm_nclobbered (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  return gs->gimple_asm.nc;
}

static inline tree
gimple_asm_input_op (gimple gs, unsigned int index)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.ni);
  return gs->with_ops.op[index];
}

static inline void
gimple_asm_set_input_op (gimple gs, unsigned int index, tree in_op)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.ni);
  gs->with_ops.op[index] = in_op;
}

static inline tree
gimple_asm_output_op (gimple gs, unsigned int index)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.no);
  return gs->with_ops.op[index + gs->gimple_asm.ni];
}

static inline void
gimple_asm_set_output_op (gimple gs, unsigned int index, tree out_op)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.no);
  gs->with_ops.op[index + gs->gimple_asm.ni] = out_op;
}

static inline tree
gimple_asm_clobber_op (gimple gs, unsigned int index)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.nc);
  return gs->with_ops.op[index + gs->gimple_asm.ni + gs->gimple_asm.no];
}

static inline void
gimple_asm_set_clobber_op (gimple gs, unsigned int index, tree clobber_op)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  gcc_assert (index <= gs->gimple_asm.nc);
  gs->with_ops.op[index + gs->gimple_asm.ni + gs->gimple_asm.no] = clobber_op;
}

static inline const char *
gimple_asm_string (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_ASM);
  return gs->gimple_asm.string;
}


/* GIMPLE_CATCH accessors. */

static inline tree
gimple_catch_types (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CATCH);
  return gs->gimple_catch.types;
}

static inline gimple_seq
gimple_catch_handler (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_CATCH);
  return gs->gimple_catch.handler;
}

static inline void
gimple_catch_set_types (gimple gs, tree t)
{
  GIMPLE_CHECK (gs, GIMPLE_CATCH);
  gs->gimple_catch.types = t;
}

static inline void
gimple_catch_set_handler (gimple gs, gimple_seq handler)
{
  GIMPLE_CHECK (gs, GIMPLE_CATCH);
  gs->gimple_catch.handler = handler;
}


/* GIMPLE_EH_FILTER accessors. */

static inline tree
gimple_eh_filter_types (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_EH_FILTER);
  return gs->gimple_eh_filter.types;
}

static inline gimple_seq
gimple_eh_filter_failure (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_EH_FILTER);
  return gs->gimple_eh_filter.failure;
}

static inline void
gimple_eh_filter_set_types (gimple gs, tree types)
{
  GIMPLE_CHECK (gs, GIMPLE_EH_FILTER);
  gs->gimple_eh_filter.types = types;
}

static inline void
gimple_eh_filter_set_failure (gimple gs, gimple_seq failure)
{
  GIMPLE_CHECK (gs, GIMPLE_EH_FILTER);
  gs->gimple_eh_filter.failure = failure;
}


/* GIMPLE_TRY accessors. */

static inline gimple_seq
gimple_try_eval (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_TRY);
  return &gs->gimple_try.eval;
}

static inline gimple_seq
gimple_try_cleanup (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_TRY);
  return &gs->gimple_try.cleanup;
}

static inline void
gimple_try_set_eval (gimple gs, gimple_seq eval)
{
  GIMPLE_CHECK (gs, GIMPLE_TRY);
  gimple_seq_copy (gimple_try_eval (gs), eval);
}

static inline void
gimple_try_set_cleanup (gimple gs, gimple_seq cleanup)
{
  GIMPLE_CHECK (gs, GIMPLE_TRY);
  gimple_seq_copy (gimple_try_cleanup (gs), cleanup);
}


/* GIMPLE_PHI accessors. */

static inline unsigned int
gimple_phi_capacity (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  return gs->gimple_phi.capacity;
}

static inline void
gimple_phi_set_capacity (gimple gs, unsigned int capacity)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  gs->gimple_phi.capacity = capacity;
}

static inline unsigned int
gimple_phi_nargs (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  return gs->gimple_phi.nargs;
}

static inline void
gimple_phi_set_nargs (gimple gs, unsigned int nargs)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  gs->gimple_phi.nargs = nargs;
}

static inline tree
gimple_phi_result (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  return gs->gimple_phi.result;
}

static inline void
gimple_phi_set_result (gimple gs, tree result)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  gs->gimple_phi.result = result;
}

static inline struct phi_arg_d *
gimple_phi_arg (gimple gs, unsigned int index)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  gcc_assert (index <= gs->gimple_phi.nargs);
  return &(gs->gimple_phi.args[index]);
}

static inline void
gimple_phi_set_arg (gimple gs, unsigned int index, struct phi_arg_d * phiarg)
{
  GIMPLE_CHECK (gs, GIMPLE_PHI);
  gcc_assert (index <= gs->gimple_phi.nargs);
  memcpy (gs->gimple_phi.args + index, phiarg, sizeof (struct phi_arg_d));
}


/* GIMPLE_RESX accessors. */

static inline int
gimple_resx_region (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_RESX);
  return gs->gimple_resx.region;
}

static inline void
gimple_resx_set_region (gimple gs, int region)
{
  GIMPLE_CHECK (gs, GIMPLE_RESX);
  gs->gimple_resx.region = region;
}


/* GIMPLE_SWITCH accessors. */

static inline size_t
gimple_switch_num_labels (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_SWITCH);
  gcc_assert (gs->with_ops.num_ops > 1);
  return gs->with_ops.num_ops - 1;
}

static inline void
gimple_switch_set_num_labels (gimple g, size_t nlabels)
{
  GIMPLE_CHECK (g, GIMPLE_SWITCH);
  g->with_ops.num_ops = nlabels + 1;
}

static inline tree
gimple_switch_index (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_SWITCH);
  return gs->with_ops.op[0];
}

static inline void
gimple_switch_set_index (gimple gs, tree index)
{
  GIMPLE_CHECK (gs, GIMPLE_SWITCH);
  gs->with_ops.op[0] = index;
}

/* Return the label numbered INDEX.  The default label is 0, followed by any
   labels in a switch statement.  */

static inline tree
gimple_switch_label (gimple gs, size_t index)
{
  GIMPLE_CHECK (gs, GIMPLE_SWITCH);
  gcc_assert (gs->with_ops.num_ops > index + 1);
  return gs->with_ops.op[index + 1];
}

/* Set the label number INDEX to LABEL.  0 is always the default label.  */

static inline void
gimple_switch_set_label (gimple gs, size_t index, tree label)
{
  GIMPLE_CHECK (gs, GIMPLE_SWITCH);
  gcc_assert (gs->with_ops.num_ops > index + 1);
  gs->with_ops.op[index + 1] = label;
}

/* Return the default label for a switch statement.  */

static inline tree
gimple_switch_default_label (gimple gs)
{
  return gimple_switch_label (gs, 0);
}

/* Set the default label for a switch statement.  */

static inline void
gimple_switch_set_default_label (gimple gs, tree label)
{
  gimple_switch_set_label (gs, 0, label);
}


/* GIMPLE_OMP_* accessors. */

static inline gimple_seq 
gimple_omp_body (gimple gs)
{
  return &(gs->omp.body);
}

static inline void
gimple_omp_set_body (gimple gs, gimple_seq body)
{
  gimple_seq_copy (&(gs->omp.body), body);
}

/* GIMPLE_OMP_CRITICAL accessors. */

static inline tree
gimple_omp_critical_name (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_CRITICAL);
  return gs->gimple_omp_critical.name;
}

static inline void
gimple_omp_critical_set_name (gimple gs, tree name)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_CRITICAL);
  gs->gimple_omp_critical.name = name;
}

/* GIMPLE_OMP_FOR accessors. */

static inline tree
gimple_omp_for_clauses (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return gs->gimple_omp_for.clauses;
}

static inline void
gimple_omp_for_set_clauses (gimple gs, tree clauses)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gs->gimple_omp_for.clauses = clauses;
}

static inline tree
gimple_omp_for_index (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return gs->gimple_omp_for.index;
}

static inline void
gimple_omp_for_set_index (gimple gs, tree index)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gs->gimple_omp_for.index = index;
}

static inline tree
gimple_omp_for_initial (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return gs->gimple_omp_for.initial;
}

static inline void
gimple_omp_for_set_initial (gimple gs, tree initial)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gs->gimple_omp_for.initial = initial;
}

static inline tree
gimple_omp_for_final (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return gs->gimple_omp_for.final;
}

static inline void
gimple_omp_for_set_final (gimple gs, tree final)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gs->gimple_omp_for.final = final;
}

static inline tree
gimple_omp_for_incr (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return gs->gimple_omp_for.incr;
}

static inline void
gimple_omp_for_set_incr (gimple gs, tree incr)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gs->gimple_omp_for.incr = incr;
}

static inline gimple_seq
gimple_omp_for_pre_body (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return &(gs->gimple_omp_for.pre_body);
}

static inline void
gimple_omp_for_set_pre_body (gimple gs, gimple_seq pre_body)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  gimple_seq_copy (&(gs->gimple_omp_for.pre_body),  pre_body);
}

/* GIMPLE_OMP_PARALLEL accessors. */

static inline tree
gimple_omp_parallel_clauses (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  return gs->gimple_omp_parallel.clauses;
}

static inline void
gimple_omp_parallel_set_clauses (gimple gs, tree clauses)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  gs->gimple_omp_parallel.clauses = clauses;
}

static inline tree
gimple_omp_parallel_child_fn (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  return gs->gimple_omp_parallel.child_fn;
}

static inline void
gimple_omp_parallel_set_child_fn (gimple gs, tree child_fn)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  gs->gimple_omp_parallel.child_fn = child_fn;
}

static inline tree
gimple_omp_parallel_data_arg (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  return gs->gimple_omp_parallel.data_arg;
}

static inline void
gimple_omp_parallel_set_data_arg (gimple gs, tree data_arg)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_PARALLEL);
  gs->gimple_omp_parallel.data_arg = data_arg;
}

/* GIMPLE_OMP_SECTION accessors. */

/* GIMPLE_OMP_SINGLE accessors. */

static inline tree
gimple_omp_single_clauses (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_SINGLE);
  return gs->gimple_omp_single.clauses;
}

static inline void
gimple_omp_single_set_clauses (gimple gs, tree clauses)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_SINGLE);
  gs->gimple_omp_single.clauses = clauses;
}

static inline tree
gimple_omp_sections_clauses (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_SECTIONS);
  return gs->gimple_omp_sections.clauses;
}

static inline void
gimple_omp_sections_set_clauses (gimple gs, tree clauses)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_SECTIONS);
  gs->gimple_omp_sections.clauses = clauses;
}


/* get or set the OMP_FOR_COND stored in the subcode flags */
static inline void
gimple_assign_omp_for_cond (gimple gs, enum gimple_cond cond)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  set_gimple_flags (gs, cond);
}

static inline enum gimple_cond
gimple_omp_for_cond (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_OMP_FOR);
  return (enum gimple_cond) gimple_flags (gs);
}

/* GIMPLE_RETURN accessors.  */

static inline tree
gimple_return_retval (gimple gs)
{
  GIMPLE_CHECK (gs, GIMPLE_RETURN);
  gcc_assert (gs->with_ops.num_ops == 1);
  return gs->with_ops.op[0];
}

static inline void
gimple_return_set_retval (gimple gs, tree retval)
{
  GIMPLE_CHECK (gs, GIMPLE_RETURN);
  gcc_assert (gs->with_ops.num_ops == 1);
  gs->with_ops.op[0] = retval;
}


/* GIMPLE_NOP.  */

/* Returns TRUE if a stmt is a GIMPLE_NOP.  */

static inline bool
gimple_nop_p (gimple g)
{
  return gimple_code (g) == GIMPLE_NOP;
}

#include "gimple-iterator.h"

#endif  /* GCC_GIMPLE_IR_H */
