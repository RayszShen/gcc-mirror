/* Graphite polyhedral representation.
   Copyright (C) 2009 Free Software Foundation, Inc.
   Contributed by Sebastian Pop <sebastian.pop@amd.com> and
   Tobias Grosser <grosser@fim.uni-passau.de>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_GRAPHITE_POLY_H
#define GCC_GRAPHITE_POLY_H

typedef struct poly_dr *poly_dr_p;
DEF_VEC_P(poly_dr_p);
DEF_VEC_ALLOC_P (poly_dr_p, heap);

typedef struct poly_bb *poly_bb_p;
DEF_VEC_P(poly_bb_p);
DEF_VEC_ALLOC_P (poly_bb_p, heap);

typedef struct scop *scop_p;
DEF_VEC_P(scop_p);
DEF_VEC_ALLOC_P (scop_p, heap);

typedef ppl_dimension_type graphite_dim_t;

static inline graphite_dim_t pbb_nb_loops (const struct poly_bb*);
static inline graphite_dim_t pbb_nb_scattering (const struct poly_bb*);
static inline graphite_dim_t pbb_nb_params (poly_bb_p);
static inline graphite_dim_t scop_nb_params (scop_p);

/* A data reference can write or read some memory or we
   just know it may write some memory.  */
enum POLY_DR_TYPE
{
  PDR_READ,
  /* PDR_MAY_READs are represented using PDR_READS. This does not limit the
     expressiveness.  */
  PDR_WRITE,
  PDR_MAY_WRITE
};

struct poly_dr
{
  poly_bb_p black_box;

  enum POLY_DR_TYPE type;

  /* The access polyhedron contains the polyhedral space this data
     reference will access.

     The polyhedron contains these dimensions: 

      - The alias set (a):
      Every memory access is classified in at least one alias set.
    
      - The subscripts (s_0, ..., s_n):
      The memory is accessed using zero or more subscript dimensions.

      - The iteration domain (variables and parameters) 

     Do not hardcode the dimensions. Use the accessors pdr_accessp_*_dim.
     
     Example:

     | int A[1335][123];
     | int *p = malloc ();
     |
     | b = ...
     | for i
     |   {
     |     if (unknown_function ())
     |       p = A;
     |       ... = p[?][?];
     | 	   for j
     |       A[i][j+b] = m; 
     |   }

     The data access A[i][j+b] in alias set "5" is described like this:

     | i   j   k   a   s0  s1  1
     | 0   0   0   1   0   0  -5     =  0
     |-1   0   0   0   1   0   0     =  0
     | 0  -1  -1   0   0   1   0     =  0
     | 0   0   0   0   1   0   0     >= 0  # The last four lines describe the
     | 0   0   0   0   0   1   0     >= 0  # array size.
     | 0   0   0   0   1   0  -1335  <= 0
     | 0   0   0   0   0   1  -123   <= 0

     The pointer "*p" in alias set "5" and "7" is described like this:

     | i   k   a   s0  1
     | 0   0   1   0  -5   =  0
     | 0   0   1   0  -7   =  0
     | 0   0   0   1   0   >= 0

     "*p" accesses all of the object allocated with 'malloc'.

     The scalar data access "m" is represented as an array with zero subscript
     dimensions.

     | i   j   k   a   1
     | 0   0   0  -1   15  = 0 */
  ppl_Polyhedron_t accesses;
};

#define PDR_BB(PDR) (PDR->black_box)
#define PDR_TYPE(PDR) (PDR->type)
#define PDR_BASE(PDR) (PDR->base)
#define PDR_ACCESSES(PDR) (PDR->accesses)

/* The number of subscript dims in PDR.  */

static inline graphite_dim_t
pdr_accessp_nb_subscripts (poly_dr_p pdr)
{
  poly_bb_p pbb = PDR_BB (pdr);
  ppl_dimension_type dim;

  ppl_Polyhedron_space_dimension (PDR_ACCESSES (pdr), &dim);
  return dim - pbb_nb_loops (pbb) - pbb_nb_params (pbb) - 1;
}

/* The dimension in PDR containing iterator ITER.  */

static inline ppl_dimension_type
pdr_accessp_nb_iterators (poly_dr_p pdr ATTRIBUTE_UNUSED)
{
  poly_bb_p pbb = PDR_BB (pdr);
  return pbb_nb_loops (pbb);
}

/* The dimension in PDR containing parameter PARAM.  */

static inline ppl_dimension_type
pdr_accessp_nb_params (poly_dr_p pdr)
{
  poly_bb_p pbb = PDR_BB (pdr);
  return pbb_nb_params (pbb);
}

/* The dimension of the alias set in PDR.  */

static inline ppl_dimension_type
pdr_accessp_alias_set_dim (poly_dr_p pdr)
{
  poly_bb_p pbb = PDR_BB (pdr);

  return pbb_nb_loops (pbb) + pbb_nb_params (pbb);
} 

/* The dimension in PDR containing subscript S.  */

static inline ppl_dimension_type
pdr_accessp_subscript_dim (poly_dr_p pdr, graphite_dim_t s)
{
  poly_bb_p pbb = PDR_BB (pdr);

  return pbb_nb_loops (pbb) + pbb_nb_params (pbb) + 1 + s;
}

/* The dimension in PDR containing iterator ITER.  */

static inline ppl_dimension_type
pdr_accessp_iterator_dim (poly_dr_p pdr ATTRIBUTE_UNUSED, graphite_dim_t iter)
{
  return iter;
}

/* The dimension in PDR containing parameter PARAM.  */

static inline ppl_dimension_type
pdr_accessp_param_dim (poly_dr_p pdr, graphite_dim_t param)
{
  poly_bb_p pbb = PDR_BB (pdr);

  return pbb_nb_loops (pbb) + param;
}

/* POLY_BB represents a blackbox in the polyhedral model.  */

struct poly_bb 
{
  gimple_bb_p black_box;

  scop_p scop;

  /* The iteration domain of this bb.
     Example:

     for (i = a - 7*b + 8; i <= 3*a + 13*b + 20; i++)
       for (j = 2; j <= 2*i + 5; j++)
         for (k = 0; k <= 5; k++)
           S (i,j,k)

     Loop iterators: i, j, k 
     Parameters: a, b
      
     | i >=  a -  7b +  8
     | i <= 3a + 13b + 20
     | j >= 2
     | j <= 2i + 5
     | k >= 0 
     | k <= 5

     The number of variables in the DOMAIN may change and is not
     related to the number of loops in the original code.  */
  ppl_Polyhedron_t domain;

  /* LOOPS contains for every column in the graphite domain the corresponding
     gimple loop.  If there exists no corresponding gimple loop LOOPS contains
     NULL. 
  
     Example:

     Original code:

     for (i = 0; i <= 20; i++) 
       for (j = 5; j <= 10; j++)
         A

     Original domain:

     |  i >= 0
     |  i <= 20
     |  j >= 0
     |  j <= 10

     This is a two dimensional domain with "Loop i" represented in
     dimension 0, and "Loop j" represented in dimension 1.  Original
     loops vector:

     | 0         1 
     | Loop i    Loop j

     After some changes (Exchange i and j, strip-mine i), the domain
     is:

     |  i >= 0
     |  i <= 20
     |  j >= 0
     |  j <= 10
     |  ii <= i
     |  ii + 1 >= i 
     |  ii <= 2k
     |  ii >= 2k 

     Iterator vector:
     | 0        1        2         3
     | Loop j   NULL     Loop i    NULL
    
     Means the original loop i is now on dimension 2 of the domain and
     loop j in the original loop nest is now on dimension 0.
     Dimensions 1 and 3 represent the newly created loops.  */
  VEC (loop_p, heap) *loops;

  /* The scattering function containing the transformations.  */
  ppl_Polyhedron_t transformed_scattering;

  /* The original scattering function.  */
  ppl_Polyhedron_t original_scattering;
};

#define PBB_SCOP(PBB) (PBB->scop)
#define PBB_DOMAIN(PBB) (PBB->domain)
#define PBB_BLACK_BOX(PBB) (PBB->black_box)
#define PBB_LOOPS(PBB) (PBB->loops)
#define PBB_TRANSFORMED_SCATTERING(PBB) (PBB->transformed_scattering)
#define PBB_ORIGINAL_SCATTERING(PBB) (PBB->original_scattering)

extern void new_poly_bb (scop_p, gimple_bb_p);
extern void free_poly_bb (poly_bb_p);
extern void debug_loop_vec (poly_bb_p);
extern void schedule_to_scattering (poly_bb_p, int);
extern void print_pbb_domain (FILE *, poly_bb_p);
extern void print_pbb (FILE *, poly_bb_p);
extern void print_scop (FILE *, scop_p);
extern void debug_pbb_domain (poly_bb_p);
extern void debug_pbb (poly_bb_p);
extern void debug_scop (scop_p);

/* The number of loops around PBB.  */

static inline graphite_dim_t
pbb_nb_loops (const struct poly_bb *pbb)
{
  scop_p scop = PBB_SCOP (pbb);
  ppl_dimension_type dim;

  ppl_Polyhedron_space_dimension (PBB_DOMAIN (pbb), &dim);
  return dim - scop_nb_params (scop);
}

/* The number of scattering dimensions in PBB.  */

static inline graphite_dim_t 
pbb_nb_scattering (const struct poly_bb *pbb)
{
  scop_p scop = PBB_SCOP (pbb);
  ppl_dimension_type dim;

  ppl_Polyhedron_space_dimension (PBB_TRANSFORMED_SCATTERING (pbb), &dim);
  return dim - pbb_nb_loops (pbb) - scop_nb_params (scop);
}

/* The number of params defined in PBB.  */
static inline graphite_dim_t
pbb_nb_params (poly_bb_p pbb)
{
  scop_p scop = PBB_SCOP (pbb); 

  return scop_nb_params (scop);
}

/* Returns the gimple loop, that corresponds to the loop_iterator_INDEX.  
   If there is no corresponding gimple loop, we return NULL.  */

static inline loop_p
pbb_loop_at_index (poly_bb_p pbb, int index)
{
  return VEC_index (loop_p, PBB_LOOPS (pbb), index);
}

/* Returns the index of LOOP in the loop nest around GB.  */

static inline int
pbb_loop_index (poly_bb_p pbb, loop_p loop)
{
  int i;
  loop_p l;

  for (i = 0; VEC_iterate (loop_p, PBB_LOOPS (pbb), i, l); i++)
    if (loop == l)
      return i;

  gcc_unreachable();
}

/* A SCOP is a Static Control Part of the program, simple enough to be
   represented in polyhedral form.  */
struct scop
{
  /* A SCOP is defined as a SESE region.  */
  sese region;

  /* All the basic blocks in this scop that contain memory references
     and that will be represented as statements in the polyhedral
     representation.  */
  VEC (poly_bb_p, heap) *bbs;

  /* Data dependence graph for this SCoP.  */
  struct graph *dep_graph;
};

#define SCOP_BBS(S) S->bbs
#define SCOP_REGION(S) S->region
#define SCOP_DEP_GRAPH(S) (S->dep_graph)

extern scop_p new_scop (sese);
extern void free_scop (scop_p);
extern void free_scops (VEC (scop_p, heap) *);
extern void print_generated_program (FILE *, scop_p);
extern void debug_generated_program (scop_p);
extern void print_scattering_function (FILE *, poly_bb_p);
extern void print_scattering_functions (FILE *, scop_p);
extern void debug_scattering_function (poly_bb_p);
extern void debug_scattering_functions (scop_p);
extern int scop_max_loop_depth (scop_p);
extern bool apply_poly_transforms (scop_p);
extern int unify_scattering_dimensions (scop_p);

/* Returns the number of parameters for SCOP.  */

static inline graphite_dim_t
scop_nb_params (scop_p scop)
{
  return sese_nb_params (SCOP_REGION (scop));
}

/* Calculate the number of loops around GB in the current SCOP.  */

static inline graphite_dim_t
nb_loops_around_pbb (poly_bb_p pbb)
{
  return nb_loops_around_loop_in_sese (gbb_loop (PBB_BLACK_BOX (pbb)),
				       SCOP_REGION (PBB_SCOP (pbb)));
}

#endif
