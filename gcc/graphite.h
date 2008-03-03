/* Gimple Represented as Polyhedra.
   Copyright (C) 2006, 2007 Free Software Foundation, Inc.
   Contributed by Sebastian Pop <sebastian.pop@inria.fr>.

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

#include "tree-data-ref.h"

typedef struct graphite_bb *graphite_bb_p;
DEF_VEC_P(graphite_bb_p);
DEF_VEC_ALLOC_P (graphite_bb_p, heap);

DEF_VEC_P(scop_p);
DEF_VEC_ALLOC_P (scop_p, heap);

struct graphite_bb
{
  basic_block bb;
  scop_p scop;

  lambda_vector static_schedule;
  lambda_vector compressed_alpha_matrix;
  VEC (data_reference_p, heap) *data_refs;
};

#define GBB_BB(GBB) GBB->bb
#define GBB_SCOP(GBB) GBB->scop
#define GBB_STATIC_SCHEDULE(GBB) GBB->static_schedule
#define GBB_DATA_REFS(GBB) GBB->data_refs
#define GBB_ALPHA(GBB) GBB->compressed_alpha_matrix

/* Return the loop that contains the basic block GBB.  */

static inline struct loop *
gbb_loop (struct graphite_bb *gbb)
{
  return GBB_BB (gbb)->loop_father;
}

/* A SCoP is a Static Control Part of the program, simple enough to be
   represented in polyhedral form.  */
struct scop
{
  /* The entry bb dominates all the bbs of the scop.  The exit bb
     post-dominates all the bbs of the scop.  These entry and exit bbs
     potentially contain non affine data accesses, side effect
     statements or difficult constructs, and thus they are not
     considered part of the scop, but just boundaries.  */
  basic_block entry, exit;

  /* All the basic blocks in the scope.  They have extra information
     attached to them, in the graphite_bb structure.  */
  VEC (graphite_bb_p, heap) *bbs;

  /* Set for a basic block index when it belongs to this scope.  */
  bitmap bbs_b;

  lambda_vector static_schedule;

  /* Parameters used within the SCOP.  */
  VEC (tree, heap) *params;

  /* Loops contained in the scop.  */
  bitmap loops;
  VEC (loop_p, heap) *loop_nest;

  htab_t loop2cloog_loop;

  /* Cloog representation of this scop.  */
  CloogProgram *program;
};

#define SCOP_BBS(S) S->bbs
#define SCOP_BBS_B(S) S->bbs_b
#define SCOP_ENTRY(S) S->entry
#define SCOP_EXIT(S) S->exit
#define SCOP_STATIC_SCHEDULE(S) S->static_schedule
#define SCOP_LOOPS(S) S->loops
#define SCOP_LOOP_NEST(S) S->loop_nest
#define SCOP_PARAMS(S) S->params
#define SCOP_PROG(S) S->program
#define SCOP_LOOP2CLOOG_LOOP(S) S->loop2cloog_loop

extern void debug_scop (scop_p, int);
extern void debug_scops (int);
extern void print_graphite_bb (FILE *, graphite_bb_p, int, int);
extern void dot_scop (scop_p);
extern void dot_all_scops (void);

/* Return the number of parameters used in SCOP.  */

static inline int
scop_nb_params (scop_p scop)
{
  return VEC_length (tree, SCOP_PARAMS (scop));
}

/* Return the number of loops contained in SCOP.  */

static inline int
scop_nb_loops (scop_p scop)
{
  return VEC_length (loop_p, SCOP_LOOP_NEST (scop));
}

/* Return the dimension of the domains for SCOP.  */

static inline int
scop_dim_domain (scop_p scop)
{
  return scop_nb_loops (scop) + scop_nb_params (scop) + 1;
}

/* Return the dimension of the domains for GB.  */

static inline int
gbb_dim_domain (graphite_bb_p gb)
{
  return scop_dim_domain (GBB_SCOP (gb));
}

/* Returns the index of LOOP in the domain matrix for the SCOP.  */

static inline int
scop_loop_index (scop_p scop, struct loop *loop)
{
  unsigned i;
  struct loop *l;

  gcc_assert (bitmap_bit_p (SCOP_LOOPS (scop), loop->num));

  for (i = 0; VEC_iterate (loop_p, SCOP_LOOP_NEST (scop), i, l); i++)
    if (l == loop)
      return i;

  gcc_unreachable ();
  return -1;
}

/* Associate a POLYHEDRON dependence description to two data
   references A and B.  */
struct data_dependence_polyhedron
{
  struct data_reference *a;
  struct data_reference *b;
  bool reversed_p;
  bool loop_carried; /*TODO:konrad get rid of this, make level signed */
  signed level;
  CloogDomain *polyhedron;  
};

#define RDGE_DDP(E)   ((struct data_dependence_polyhedron*) ((E)->data))

typedef struct data_dependence_polyhedron *ddp_p;

DEF_VEC_P(ddp_p);
DEF_VEC_ALLOC_P(ddp_p,heap);

struct loop_to_cloog_loop_str
{
  unsigned int loop_num;
  unsigned int loop_position; /* the column that represents this loop */
  CloogLoop *cloog_loop;
};

