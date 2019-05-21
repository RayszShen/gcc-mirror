/* ACLE support for AArch64 SVE
   Copyright (C) 2018 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "langhooks.h"
#include "stringpool.h"
#include "rtl.h"
#include "tm_p.h"
#include "memmodel.h"
#include "insn-codes.h"
#include "optabs.h"
#include "expr.h"
#include "diagnostic.h"
#include "recog.h"
#include "basic-block.h"
#include "function.h"
#include "fold-const.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimplify.h"
#include "tree-vector-builder.h"
#include "rtx-vector-builder.h"
#include "vec-perm-indices.h"
#include "stor-layout.h"
#include "explow.h"
#include "emit-rtl.h"
#include "regs.h"
#include "alias.h"
#include "gimple-fold.h"

namespace aarch64_sve {

/* Used to represent the default merge argument index for _m functions.  */
const unsigned int DEFAULT_MERGE_ARGNO = ~0U;

/* The maximum number of vectors in an ACLE tuple type.  */
const unsigned int MAX_TUPLE_SIZE = 4;

/* A dummy unspec code that says that floating-point operations should
   be bitcast to integers.  */
const int BITCAST_TO_INT = -2;

/* Enumerates the SVE predicate and (data) vector types, together called
   "vector types" for brevity.  */
enum vector_type {
#define DEF_SVE_TYPE(ACLE_NAME, NCHARS, ABI_NAME, SCALAR_TYPE) \
  VECTOR_TYPE_ ## ACLE_NAME,
#include "aarch64-sve-builtins.def"
  NUM_VECTOR_TYPES
};

/* Classifies the available measurement units for an address displacement.  */
enum units {
  UNITS_none,
  UNITS_bytes,
  UNITS_elements,
  UNITS_vectors
};

/* Describes the various uses of a governing predicate.  */
enum predication {
  /* No governing predicate present.  */
  PRED_none,

  /* No predication suffix is present, either because the result is neither
     a vector nor a predicate (and so the distinction between "zeroing"
     and "merging" doesn't apply), or because a suffix would be redundant
     (such as for loads and comparisons, which are inherently zeroing
     operations).  */
  PRED_inherent,

  /* Merging predication: copy inactive lanes from the first data argument
     to the vector result.  */
  PRED_m,

  /* "Don't care" predication: set inactive lanes of the vector result
     to arbitrary values.  */
  PRED_x,

  /* "Zero" predication: set inactive lanes of the vector result to zero.  */
  PRED_z,

  NUM_PREDS
};

/* Classifies functions into "shapes".  The idea is to take all the
   signatures for an overloaded function, remove the governing predicate
   (if any), and classify what's left based on:

   - the number of arguments (which is always fixed for a given overload)

   - the process of determining the types in the signature from the type
     suffixes (including the types that are not affected by the suffixes)

   - which arguments must be integer constant expressions, and what range
     those arguments have

   - the process for mapping overloaded names to "full" names.

   In the comments below, _t0 represents the first type suffix and _t1
   represents the second.  Square brackets enclose characters that are
   present only in the full name, not the overloaded name.  Governing
   predicate arguments and predicate suffixes are not shown.  */
enum function_shape {
  /* sv<m0>_t svfoo[_m0base]_[m1]index(sv<m0>_t, sv<m1>_t).  */
  SHAPE_adr_index,

  /* sv<m0>_t svfoo[_m0base]_[m1]offset(sv<m0>_t, sv<m1>_t).  */
  SHAPE_adr_offset,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0>_t).  */
  SHAPE_binary,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0>_t, uint64_t).  */
  SHAPE_binary_extract,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0:uint>_t).  */
  SHAPE_binary_index,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, <t0:uint>_t).  */
  SHAPE_binary_index_n,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, uint64_t).  */
  SHAPE_binary_index64_n,

  /* svbool_t svfoo_<t0>(sv<t0>_t, sv<t0>_t, uint64_t)

     where the final argument is a constant lane number.  */
  SHAPE_binary_lane,

  /* sv<t0>_t svfoo[_n_t0](sv<t0>_t, <t0>_t).  */
  SHAPE_binary_n,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0>_t)
     sv<t0>_t svfoo[_n_t0](sv<t0>_t, <t0>_t).  */
  SHAPE_binary_opt_n,

  /* svbool_t svfoo_<t0>(svbool_t, svbool_t).  */
  SHAPE_binary_pred,

  /* svbool_t svfoo_<t0>(sv<t0>_t, sv<t0>_t, uint64_t)

     where the final argument must be 90 or 270.  */
  SHAPE_binary_rotate,

  /* sv<t0>_t svfoo_t0(<t0>_t, <t0>_t).  */
  SHAPE_binary_scalar,

  /* sv<t0>_t svfoo_wide[_t0](sv<t0>_t, svuint64_t).  */
  SHAPE_binary_wide,

  /* svbool_t svfoo[_t0](sv<t0>_t, sv<t0>_t)
     svbool_t svfoo[_n_t0](sv<t0>_t, int64_t)  (for signed t0)
     svbool_t svfoo[_n_t0](sv<t0>_t, uint64_t)  (for unsigned t0)
     svbool_t svfoo[_n_t0](sv<t0>_t, <t0>_t)  (for floating-point t0).  */
  SHAPE_compare_opt_n,

  /* svbool_t svfoo_t0[_t1](<t1>_t, <t1>_t).  */
  SHAPE_compare_scalar,

  /* svbool_t svfoo_wide[_t0](sv<t0>_t, svint64_t)  (for signed t0)
     svbool_t svfoo_wide[_t0](sv<t0>_t, svuint64_t)  (for unsigned t0).  */
  SHAPE_compare_wide,

  /* sv<t0>x2_t svfoo[_t0](sv<t0>_t, sv<t0>_t).  */
  SHAPE_create2,

  /* sv<t0>x3_t svfoo[_t0](sv<t0>_t, sv<t0>_t, sv<t0>_t).  */
  SHAPE_create3,

  /* sv<t0>x4_t svfoo[_t0](sv<t0>_t, sv<t0>_t, sv<t0>_t, sv<t0>_t).  */
  SHAPE_create4,

  /* sv<t0>_t svdupq_t0(<t0>_t, ..., <t0>_t)

     where there are enough arguments to fill 128 bits of data (or to
     control 128 bits of data in the case of predicates).  */
  SHAPE_dupq,

  /* <t0>_t svfoo[_t0](<t0>_t, sv<t0>_t).  */
  SHAPE_fold_left,

  /* sv<t0>_t svfoo[_t0](sv<t0>xN_t, uint64_t).  */
  SHAPE_get2,
  SHAPE_get3,
  SHAPE_get4,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, uint64_t)
     <t0>_t svfoo[_n_t0](<t0>_t, uint64_t)

     where the t0 in the vector form is a signed or unsigned integer
     whose size is tied to the [bhwd] suffix of "svfoo".  */
  SHAPE_inc_dec,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, enum svpattern, uint64_t)
     <t0>_t svfoo[_n_t0](<t0>_t, enum svpattern, uint64_t)

     where the t0 in the vector form is a signed or unsigned integer
     whose size is tied to the [bhwd] suffix of "svfoo".  */
  SHAPE_inc_dec_pattern,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, svbool_t).  */
  SHAPE_inc_dec_pred,

  /* <t0>_t svfoo[_n_t0]_t1(<t0>_t, svbool_t).  */
  SHAPE_inc_dec_pred_n,

  /* sv<t0>_t svfoo_t0().  */
  SHAPE_inherent,

  /* sv<t0>xN_t svfoo_t0().  */
  SHAPE_inherent2,
  SHAPE_inherent3,
  SHAPE_inherent4,

  /* svbool_t svfoo[_b]().  */
  SHAPE_inherent_b,

  /* uint64_t svfoo().  */
  SHAPE_inherent_count,

  /* sv<t0>_t svfoo[_t0](const <t0>_t *)
     sv<t0>_t svfoo_vnum[_t0](const <t0>_t *, int64_t).  */
  SHAPE_load,

  /* sv<t0>xN_t svfoo[_t0](const <t0>_t *)
     sv<t0>xN_t svfoo_vnum[_t0](const <t0>_t *, int64_t).  */
  SHAPE_load2,
  SHAPE_load3,
  SHAPE_load4,

  /* sv<t0>_t svfoo_t0(const <X>_t *)
     sv<t0>_t svfoo_vnum_t0(const <X>_t *, int64_t)

     where <X> is determined by the function base name.  */
  SHAPE_load_ext,

  /* sv<t0>_t svfoo_[s32]index_t0(const <X>_t *, svint32_t)
     sv<t0>_t svfoo_[s64]index_t0(const <X>_t *, svint64_t)
     sv<t0>_t svfoo_[u32]index_t0(const <X>_t *, svuint32_t)
     sv<t0>_t svfoo_[u64]index_t0(const <X>_t *, svuint64_t)

     sv<t0>_t svfoo[_u32base]_index_t0(svuint32_t, int64_t)
     sv<t0>_t svfoo[_u64base]_index_t0(svuint64_t, int64_t)

     where <X> is determined by the function base name.  */
  SHAPE_load_ext_gather_index,

  /* sv<t0>_t svfoo_[s32]offset_t0(const <X>_t *, svint32_t)
     sv<t0>_t svfoo_[s64]offset_t0(const <X>_t *, svint64_t)
     sv<t0>_t svfoo_[u32]offset_t0(const <X>_t *, svuint32_t)
     sv<t0>_t svfoo_[u64]offset_t0(const <X>_t *, svuint64_t)

     sv<t0>_t svfoo[_u32base]_t0(svuint32_t)
     sv<t0>_t svfoo[_u64base]_t0(svuint64_t)

     sv<t0>_t svfoo[_u32base]_offset_t0(svuint32_t, int64_t)
     sv<t0>_t svfoo[_u64base]_offset_t0(svuint64_t, int64_t)

     where <X> is determined by the function base name.  */
  SHAPE_load_ext_gather_offset,

  /* sv<t0>_t svfoo_[s32]index[_t0](const <t0>_t *, svint32_t)
     sv<t0>_t svfoo_[s64]index[_t0](const <t0>_t *, svint64_t)
     sv<t0>_t svfoo_[u32]index[_t0](const <t0>_t *, svuint32_t)
     sv<t0>_t svfoo_[u64]index[_t0](const <t0>_t *, svuint64_t)

     sv<t0>_t svfoo_[s32]offset[_t0](const <t0>_t *, svint32_t)
     sv<t0>_t svfoo_[s64]offset[_t0](const <t0>_t *, svint64_t)
     sv<t0>_t svfoo_[u32]offset[_t0](const <t0>_t *, svuint32_t)
     sv<t0>_t svfoo_[u64]offset[_t0](const <t0>_t *, svuint64_t).  */
  SHAPE_load_gather_sv,

  /* sv<t0>_t svfoo[_u32base]_t0(svuint32_t)
     sv<t0>_t svfoo[_u64base]_t0(svuint64_t)

     sv<t0>_t svfoo[_u32base]_index_t0(svuint32_t, int64_t)
     sv<t0>_t svfoo[_u64base]_index_t0(svuint64_t, int64_t)

     sv<t0>_t svfoo[_u32base]_offset_t0(svuint32_t, int64_t)
     sv<t0>_t svfoo[_u64base]_offset_t0(svuint64_t, int64_t).  */
  SHAPE_load_gather_vs,

  /* sv<t0>_t svfoo[_t0](const <t0>_t *).  */
  SHAPE_load_scalar,

  /* uint64_t svfoo(enum svpattern).  */
  SHAPE_pattern_count,

  /* svbool_t svfoo_t0(enum svpattern).  */
  SHAPE_pattern_pred,

  /* uint64_t svfoo_t0(svbool_t).  */
  SHAPE_pred_count,

  /* void svfoo(const void *, svprfop)
     void svfoo_vnum(const void *, int64_t, svprfop).  */
  SHAPE_prefetch,

  /* void svfoo_[s32]index(const void *, svint32_t, svprfop)
     void svfoo_[s64]index(const void *, svint64_t, svprfop)
     void svfoo_[u32]index(const void *, svuint32_t, svprfop)
     void svfoo_[u64]index(const void *, svuint64_t, svprfop)

     void svfoo[_u32base](svuint32_t, svprfop)
     void svfoo[_u64base](svuint64_t, svprfop)

     void svfoo[_u32base]_index(svuint32_t, int64_t, svprfop)
     void svfoo[_u64base]_index(svuint64_t, int64_t, svprfop).  */
  SHAPE_prefetch_gather_index,

  /* void svfoo_[s32]offset(const void *, svint32_t, svprfop)
     void svfoo_[s64]offset(const void *, svint64_t, svprfop)
     void svfoo_[u32]offset(const void *, svuint32_t, svprfop)
     void svfoo_[u64]offset(const void *, svuint64_t, svprfop)

     void svfoo[_u32base](svuint32_t, svprfop)
     void svfoo[_u64base](svuint64_t, svprfop)

     void svfoo[_u32base]_offset(svuint32_t, int64_t, svprfop)
     void svfoo[_u64base]_offset(svuint64_t, int64_t, svprfop).  */
  SHAPE_prefetch_gather_offset,

  /* bool svfoo(svbool_t).  */
  SHAPE_ptest,

  /* svbool_t svfoo().  */
  SHAPE_rdffr,

  /* <t0>_t svfoo[_t0](sv<t0>_t).  */
  SHAPE_reduction,

  /* int64_t svfoo[_t0](sv<t0>_t)  (for signed t0)
     uint64_t svfoo[_t0](sv<t0>_t)  (for unsigned t0)
     <t0>_t svfoo[_t0](sv<t0>_t)  (for floating-point t0).  */
  SHAPE_reduction_wide,

  /* sv<t0>xN_t svfoo[_t0](sv<t0>xN_t, uint64_t, sv<t0>_t).  */
  SHAPE_set2,
  SHAPE_set3,
  SHAPE_set4,

  /* void svfoo().  */
  SHAPE_setffr,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0>_t)
     sv<t0>_t svfoo[_t0](sv<t0>_t, uint64_t).  */
  SHAPE_shift_opt_n,

  /* sv<t0>_t svfoo[_n_t0])(sv<t0>_t, uint64_t)

     The final argument must be an integer constant expression in the
     range [1, <t0>_BITS].  */
  SHAPE_shift_right_imm,

  /* void svfoo[_t0](<t0>_t *, sv<t0>_t)
     void svfoo_vnum[_t0](<t0>_t *, int64_t, sv<t0>_t).  */
  SHAPE_store,

  /* void svfoo[_t0](<t0>_t *, sv<t0>xN_t)
     void svfoo_vnum[_t0](<t0>_t *, int64_t, sv<t0>xN_t).  */
  SHAPE_store2,
  SHAPE_store3,
  SHAPE_store4,

  /* void svfoo_[s32]index[_t0](<t0>_t *, svint32_t, sv<t0>_t)
     void svfoo_[s64]index[_t0](<t0>_t *, svint64_t, sv<t0>_t)
     void svfoo_[u32]index[_t0](<t0>_t *, svuint32_t, sv<t0>_t)
     void svfoo_[u64]index[_t0](<t0>_t *, svuint64_t, sv<t0>_t)

     void svfoo[_u32base]_index[_t0](svuint32_t, int64_t, sv<t0>_t)
     void svfoo[_u64base]_index[_t0](svuint64_t, int64_t, sv<t0>_t).  */
  SHAPE_store_scatter_index,

  /* void svfoo_[s32]offset[_t0](<t0>_t *, svint32_t, sv<t0>_t)
     void svfoo_[s64]offset[_t0](<t0>_t *, svint64_t, sv<t0>_t)
     void svfoo_[u32]offset[_t0](<t0>_t *, svuint32_t, sv<t0>_t)
     void svfoo_[u64]offset[_t0](<t0>_t *, svuint64_t, sv<t0>_t)

     void svfoo[_u32base_t0](svuint32_t, sv<t0>_t)
     void svfoo[_u64base_t0](svuint64_t, sv<t0>_t)

     void svfoo[_u32base]_offset[_t0](svuint32_t, int64_t, sv<t0>_t)
     void svfoo[_u64base]_offset[_t0](svuint64_t, int64_t, sv<t0>_t).  */
  SHAPE_store_scatter_offset,

  /* svbool_t svfoo_<t0>(sv<t0>_t, sv<t0>_t, sv<t0>_t, uint64_t)

     where the final argument is a constant lane number.  */
  SHAPE_ternary_lane,

  /* svbool_t svfoo_<t0>(sv<t0>_t, sv<t0>_t, sv<t0>_t, uint64_t, uint64_t)

     where the penultimate argument is a constant lane number for complex
     numbers and where the final argument must be 0, 90, 180 or 270.  */
  SHAPE_ternary_lane_rotate,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0>_t, sv<t0>_t)
     sv<t0>_t svfoo[_n_t0](sv<t0>_t, sv<t0>_t, <t0>_t).  */
  SHAPE_ternary_opt_n,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0.quarter>_t, sv<t0.quarter>_t,
			 uint64_t)

     where the final argument is a constant lane number for a group of
     four quarter-sized elements.  */
  SHAPE_ternary_qq_lane,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t, sv<t0.quarter>_t, sv<t0.quarter>_t)
     sv<t0>_t svfoo[_n_t0](sv<t0>_t, sv<t0.quarter>_t, <t0.quarter>_t).  */
  SHAPE_ternary_qq_opt_n,

  /* svbool_t svfoo_<t0>(sv<t0>_t, sv<t0>_t, sv<t0>_t, uint64_t)

     where the final argument must be 0, 90, 180 or 270.  */
  SHAPE_ternary_rotate,

  /* sv<t0>_t svfoo[_t0](sv<t0>_t).  */
  SHAPE_unary,

  /* sv<t0:uint>_t svfoo[_t0](sv<t0>_t).  */
  SHAPE_unary_count,

  /* sv<t0>_t svfoo[_n]_t0(<t0>_t).  */
  SHAPE_unary_n,

  /* svbool_t svfoo_<t0>(svbool_t).  */
  SHAPE_unary_pred
};

/* Classifies an operation into "modes"; for example, to distinguish
   vector-scalar operations from vector-vector operations, or to
   distinguish between different addressing modes.

   The overloaded and full names of a function are determined by:
   - the base name ("svadd", etc.)
   - the mode
   - the type suffixes
   - the type of predication

   Thus the mode must fill in any information that isn't given by the
   other three.  */
enum function_mode {
#define DEF_SVE_MODE(NAME, BASE, DISPLACEMENT, UNITS) MODE_##NAME,
#include "aarch64-sve-builtins.def"
  MODE_none
};

/* Enumerates the possible type suffixes.  Each suffix is associated with
   a vector type, but provides more information about element sizes in
   the case of predicates.  */
enum type_suffix {
#define DEF_SVE_TYPE_SUFFIX(NAME, ACLE_TYPE, BITS, MODE) \
  TYPE_SUFFIX_ ## NAME,
#include "aarch64-sve-builtins.def"
  NUM_TYPE_SUFFIXES
};

/* Combines two type suffixes.  */
typedef enum type_suffix type_suffix_pair[2];

/* Enumerates the function base names, such as "svadd".  */
enum function {
#define DEF_SVE_BASE_NAME(NAME) FUNC_##NAME,
#define DEF_SVE_LAST_BASE_NAME(NAME) FUNC_##NAME
#include "aarch64-sve-builtins.def"
};

/* Static information about a mode.  */
struct mode_info {
  /* The function suffix associated with the mode.  */
  const char *suffix;

  /* The type of the vector base address, or NUM_VECTOR_TYPES if the
     mode does not take a vector base address.  */
  vector_type base_vector_type;

  /* The type of the vector displacement, or NUM_VECTOR_TYPES if the
     mode does not take a vector displacement.  (Scalar displacements
     are always int64_t.)  */
  vector_type displacement_vector_type;

  /* The units in which the displacement is measured, or UNITS_none if
     the mode doesn't take a displacement.  */
  units displacement_units;
};

/* Static information about each single-predicate or single-vector
   ABI and ACLE type.  */
struct vector_type_info {
  /* The name of the type as declared by arm_sve.h.  */
  const char *acle_name;

  /* The name of the type specified in the AAPCS.  The type is always
     available under this name, even when arm_sve.h isn't included.  */
  const char *abi_name;

  /* The C++ mangling of ABI_NAME.  */
  const char *mangled_name;
};

/* Static information about a type suffix.  */
struct type_suffix_info {
  /* The suffix itself.  */
  const char *suffix;

  /* The associated ACLE vector or predicate type.  */
  vector_type type : 8;

  /* The number of bits and bytes in an element.  For predicates this
     measures the associated data elements.  */
  unsigned int elem_bits : 8;
  unsigned int elem_bytes : 8;

  /* True if the suffix is for an integer type.  */
  unsigned int integer_p : 1;
  /* True if the suffix is for an unsigned type.  */
  unsigned int unsigned_p : 1;
  /* True if the suffix is for a boolean type.  */
  unsigned int bool_p : 1;
  unsigned int spare : 5;

  /* The associated vector or predicate mode.  */
  machine_mode mode : 16;
};

/* Static information about a set of functions.  */
struct function_group {
  /* The base name.  */
  function func;

  /* The shape of the functions, as described above the enum definition.
     It's possible to have entries with the same base name but different
     shapes.  */
  function_shape shape;

  /* A list of the available predication types, and of the available
     type suffixes.  The function supports every permutation of the two.

     The list of predication types is terminated by NUM_PREDS while
     the list of type suffixes is terminated by two NUM_TYPE_SUFFIXES.
     The list of type suffixes is lexicographically ordered.  */
  const type_suffix_pair *types;
  const predication *preds;
};

/* Describes a fully-resolved function (i.e. one that has a unique full
   name).  */
struct GTY(()) function_instance {
  function_instance (function, function_shape, function_mode,
		     const type_suffix_pair &, predication);

  bool operator== (const function_instance &) const;
  bool operator!= (const function_instance &) const;
  hashval_t hash () const;

  units displacement_units () const;

  const char *end_base_name () const;
  machine_mode bhwd_vector_mode () const;
  scalar_mode bhwd_scalar_mode () const;
  unsigned int bhwd_nelts_per_vq () const;
  machine_mode memory_vector_mode () const;

  tree base_vector_type () const;
  tree displacement_vector_type () const;
  tree memory_scalar_type () const;
  tree scalar_type (unsigned int) const;
  tree vector_type (unsigned int) const;
  tree tuple_type (unsigned int, unsigned int) const;
  tree quarter_vector_type (unsigned int i) const;
  tree quarter_scalar_type (unsigned int i) const;
  tree unsigned_scalar_type (unsigned int) const;
  tree unsigned_vector_type (unsigned int) const;
  tree wide_vector_type (unsigned int i) const;
  tree wide_scalar_type (unsigned int i) const;

  unsigned int nelts_per_vq (unsigned int i) const;

  /* The explicit "enum"s are required for gengtype.  */
  enum function func;
  enum function_shape shape;
  enum function_mode mode;
  type_suffix_pair types;
  enum predication pred;
};

/* Describes a function decl.  */
struct GTY(()) registered_function
{
  /* The ACLE function that the decl represents.  If OVERLOADED_P:

     - the mode for functions with an optional _n suffix is MODE_none
     - any type suffixes determined by resolving the overload are
       NUM_TYPE_SUFFIXES.  */
  function_instance instance;

  /* True if the decl represents an overloaded function that needs to be
     resolved by function_resolver.  */
  bool overloaded_p;

  /* The decl itself.  */
  tree decl;
};

/* Hash traits for registered_function.  */
struct registered_function_hasher : nofree_ptr_hash <registered_function>
{
  typedef function_instance compare_type;

  static hashval_t hash (value_type);
  static bool equal (value_type, const compare_type &);
};

/* RAII class for enabling enough SVE features to define the built-in
   types and implement the arm_sve.h pragma.  */
class sve_switcher
{
public:
  sve_switcher ();
  ~sve_switcher ();

private:
  unsigned long m_old_isa_flags;
  bool m_old_have_regs_of_mode[MAX_MACHINE_MODE];
};

/* A class for building the decls associated with:

     #pragma GCC aarch64 "arm_sve.h".  */
class arm_sve_h_builder
{
public:
  arm_sve_h_builder ();
  ~arm_sve_h_builder ();

  void register_type (vector_type);
  void register_svpattern ();
  void register_svprfop ();
  void build (const function_group &);

private:
  typedef void (arm_sve_h_builder::*function_signature)
    (const function_instance &, vec<tree> &);

  void register_tuple_type (unsigned int, vector_type);

  void build_sv_index (function_signature, const function_group &);
  void build_sv_offset (function_signature, const function_group &);
  void build_v_base (function_signature, const function_group &, bool = false);
  void build_vs_index (function_signature, const function_group &,
		       bool = false);
  void build_vs_offset (function_signature, const function_group &,
			bool = false);
  void build_32_64 (function_signature, const function_group &, function_mode,
		    function_mode, bool = false);
  void build_all (function_signature, const function_group &, function_mode,
		  bool = false);
  void build_one (function_signature, const function_group &, function_mode,
		  unsigned int, unsigned int, bool);
  void sig_adr (const function_instance &, vec<tree> &);
  void sig_compare (const function_instance &, vec<tree> &);
  void sig_compare_n (const function_instance &, vec<tree> &);
  void sig_compare_scalar (const function_instance &, vec<tree> &);
  void sig_compare_wide (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_create (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_count (const function_instance &, vec<tree> &);
  void sig_dupq (const function_instance &, vec<tree> &);
  void sig_fold_left (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_get (const function_instance &, vec<tree> &);
  void sig_inc_dec (const function_instance &, vec<tree> &);
  void sig_inc_dec_n (const function_instance &, vec<tree> &);
  void sig_inc_dec_pred (const function_instance &, vec<tree> &);
  void sig_inc_dec_pred_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_inherent (const function_instance &, vec<tree> &);
  void sig_inherent_count (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_load (const function_instance &, vec<tree> &);
  void sig_load_gather_sv (const function_instance &, vec<tree> &);
  void sig_load_gather_vs (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_index (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_index_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_index64_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_nn (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_qq (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_qq_u64_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_qq_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_u64 (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_u64_n (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_nary_u64_nn (const function_instance &, vec<tree> &);
  void sig_pattern_count (const function_instance &, vec<tree> &);
  void sig_pattern_pred (const function_instance &, vec<tree> &);
  void sig_pred_count (const function_instance &, vec<tree> &);
  void sig_prefetch (const function_instance &, vec<tree> &);
  void sig_prefetch_gather_sv (const function_instance &, vec<tree> &);
  void sig_prefetch_gather_vs (const function_instance &, vec<tree> &);
  void sig_ptest (const function_instance &, vec<tree> &);
  void sig_rdffr (const function_instance &, vec<tree> &);
  void sig_reduction (const function_instance &, vec<tree> &);
  void sig_reduction_wide (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_set (const function_instance &, vec<tree> &);
  void sig_setffr (const function_instance &, vec<tree> &);
  template <unsigned int N>
  void sig_store (const function_instance &, vec<tree> &);
  void sig_store_scatter_sv (const function_instance &, vec<tree> &);
  void sig_store_scatter_vs (const function_instance &, vec<tree> &);

  tree build_const_pointer (tree);

  void apply_predication (const function_instance &, vec<tree> &);

  void add_function_instance (const function_instance &, vec<tree> &, bool);
  void add_overloaded_functions (const function_group &, function_mode);
  void add_overloaded_function (const function_instance &);

  registered_function &add_function (const function_instance &,
				     const char *, tree, tree, bool);
  tree get_attributes (const function_instance &);

  unsigned int get_explicit_types (function_shape);
  char *get_name (const function_instance &, bool);
  void append_name (const char *);
  char *finish_name ();

  /* The function type to use for functions that are resolved by
     function_resolver.  */
  tree m_overload_type;

  /* True if we should use create a separate decl for each instance of an
     overloaded function, instead of using function_resolver.  */
  bool m_direct_overloads;

  /* Used for building up function names.  */
  obstack m_string_obstack;

  /* A hash of all built-in function names that we've registered so far.  */
  hash_set<nofree_string_hash> m_overload_names;
};

/* A class for resolving an overloaded function call.  */
class function_resolver
{
public:
  function_resolver (location_t, const registered_function &,
		     vec<tree, va_gc> &);
  tree resolve ();

private:
  tree resolve_pointer ();
  tree resolve_uniform (unsigned int);
  tree resolve_adr ();
  tree resolve_create (unsigned int);
  tree resolve_fold_left ();
  tree resolve_get (unsigned int);
  tree resolve_inc_dec (unsigned int);
  tree resolve_inc_dec_pred ();
  tree resolve_inc_dec_pred_n ();
  tree resolve_load_gather_sv ();
  tree resolve_load_gather_sv_or_vs ();
  tree resolve_multitype (unsigned int, unsigned int);
  tree resolve_prefetch_gather ();
  tree resolve_set (unsigned int);
  tree resolve_store (unsigned int);
  tree resolve_store_scatter ();
  tree resolve_binary_wide ();
  tree resolve_compare_scalar ();
  tree resolve_compare_wide ();
  tree resolve_uniform_imm (unsigned int, unsigned int);

  bool check_first_vector_argument (unsigned int, unsigned int &,
				    unsigned int &, vector_type &);
  bool check_num_arguments (unsigned int);
  bool check_argument_against_decl (tree, unsigned int, tree);
  bool check_argument (unsigned int, vector_type);
  vector_type require_pointer_type (unsigned int, bool = false);
  vector_type require_vector_type (unsigned int);
  vector_type require_integer_vector_type (unsigned int);
  vector_type require_sd_vector_type (unsigned int);
  vector_type require_tuple_type (unsigned int, unsigned int);
  bool require_matching_type (unsigned int, vector_type);
  bool scalar_argument_p (unsigned int);
  bool require_scalar_argument (unsigned int, const char *);
  bool require_integer_immediate (unsigned int);
  vector_type require_integer_scalar_type (unsigned int);
  bool require_matching_scalar_types (unsigned int, unsigned int,
				      vector_type, vector_type);
  vector_type require_vector_base (unsigned int);
  function_mode require_vector_displacement (unsigned int, vector_type, bool);
  function_mode require_gather_address (unsigned int, vector_type, bool);

  tree require_n_form (type_suffix, type_suffix = NUM_TYPE_SUFFIXES);
  tree require_form (function_mode, type_suffix = NUM_TYPE_SUFFIXES,
		     type_suffix = NUM_TYPE_SUFFIXES);
  tree lookup_form (function_mode, type_suffix, type_suffix);

  tree get_vector_type (vector_type);
  tree get_vector_type (type_suffix);
  unsigned int get_element_bits (vector_type);
  const char *get_scalar_type_name (vector_type);
  tree get_argument_type (unsigned int);
  type_suffix get_type_suffix (vector_type);

  /* The location of the call.  */
  location_t m_location;

  /* The overloaded function.  */
  const registered_function &m_rfn;
  const function_instance &m_fi;

  /* The arguments to the overloaded function.  */
  vec<tree, va_gc> &m_arglist;
};

/* A class for checking that the semantic constraints on a function call are
   satisfied, such as arguments being integer constant expressions with
   a particular range.  */
class function_checker
{
public:
  function_checker (location_t, const registered_function &, tree,
		    unsigned int, tree *);
  bool check ();

private:
  bool check_shift_right_imm ();

  bool require_immediate (unsigned int, HOST_WIDE_INT &);
  bool require_immediate_extract_index (unsigned int);
  bool require_immediate_lane_index (unsigned int, unsigned int = 1);
  bool require_immediate_range (unsigned int, HOST_WIDE_INT, HOST_WIDE_INT);
  bool require_immediate_rotate (unsigned int, bool);
  bool require_immediate_svpattern (unsigned int);
  bool require_immediate_svprfop (unsigned int);

  /* The location of the call.  */
  location_t m_location;

  /* The non-overloaded function being called.  */
  const function_instance &m_fi;

  /* The type of M_FI.  */
  tree m_fntype;

  /* The function that the user called (which might be an overloaded form
     of M_FI).  */
  tree m_decl;

  /* The arguments to the function.  */
  unsigned int m_nargs;
  tree *m_args;
};

/* A class for folding a gimple function call.  */
class gimple_folder
{
public:
  gimple_folder (unsigned int, gimple_stmt_iterator *, gcall *);
  gimple *fold ();

private:
  gimple *fold_cnt_bhwd ();
  gimple *fold_cnt_bhwd_pat ();
  gimple *fold_create ();
  gimple *fold_dup ();
  gimple *fold_dupq ();
  gimple *fold_get ();
  gimple *fold_insr ();
  gimple *fold_ld1 ();
  gimple *fold_ld234 ();
  gimple *fold_pfalse ();
  gimple *fold_ptrue ();
  gimple *fold_ptrue_pat ();
  gimple *fold_rev ();
  gimple *fold_sel ();
  gimple *fold_set ();
  gimple *fold_st1 ();
  gimple *fold_st234 ();
  gimple *fold_trn ();
  gimple *fold_undef ();
  gimple *fold_uzp ();
  template<typename T> gimple *fold_while_type (bool);
  gimple *fold_while (bool);
  gimple *fold_zip ();

  gimple *fold_permute (const vec_perm_builder &);
  gimple *fold_to_vl_pred (unsigned int, unsigned int);

  tree convert_pred (gimple_seq &, tree, unsigned int);
  tree fold_contiguous_base (gimple_seq &, tree);
  tree load_store_cookie (tree);

  /* The function being called.  */
  const function_instance &m_fi;

  /* Where to insert extra statements that feed the final replacement.  */
  gimple_stmt_iterator *m_gsi;

  /* The call we're folding.  */
  gcall *m_call;

  /* The result of the call, or null if none.  */
  tree m_lhs;
};

/* A class for expanding a function call into RTL.  */
class function_expander
{
public:
  function_expander (unsigned int, tree, rtx);
  rtx expand ();

private:
  rtx expand_abd ();
  rtx expand_abs ();
  rtx expand_ac (int);
  rtx expand_add (unsigned int);
  rtx expand_adda ();
  rtx expand_adrb ();
  rtx expand_adrhwd ();
  rtx expand_and (unsigned int = DEFAULT_MERGE_ARGNO);
  rtx expand_asrd ();
  rtx expand_brk_unary (int);
  rtx expand_brk_binary (int);
  rtx expand_bic ();
  rtx expand_cadd ();
  rtx expand_cmla ();
  rtx expand_cmla_lane ();
  rtx expand_cmp (rtx_code, rtx_code, int, int);
  rtx expand_cmp_wide (int, int);
  rtx expand_cmpuo ();
  rtx expand_cnt_bhwd ();
  rtx expand_cnt_bhwd_pat ();
  rtx expand_cntp ();
  rtx expand_compact ();
  rtx expand_create ();
  rtx expand_div (bool);
  rtx expand_dot ();
  rtx expand_dot_lane ();
  rtx expand_dup ();
  rtx expand_dup_lane ();
  rtx expand_dupq ();
  rtx expand_dupq_lane ();
  rtx expand_eor ();
  rtx expand_ext ();
  rtx expand_ext_bhw ();
  rtx expand_get ();
  rtx expand_index ();
  rtx expand_insr ();
  rtx expand_ld1 ();
  rtx expand_ld1_ext (rtx_code);
  rtx expand_ld1_ext_gather (rtx_code);
  rtx expand_ld1_gather ();
  rtx expand_ld1rq ();
  rtx expand_ld234 ();
  rtx expand_ldff1 (int = UNSPEC_LDFF1);
  rtx expand_ldff1_ext (rtx_code, int = UNSPEC_LDFF1);
  rtx expand_ldff1_ext_gather (rtx_code);
  rtx expand_ldff1_gather ();
  rtx expand_ldnt1 ();
  rtx expand_mad (unsigned int);
  rtx expand_max ();
  rtx expand_maxnm ();
  rtx expand_min ();
  rtx expand_minnm ();
  rtx expand_mla ();
  rtx expand_mla_mls_lane (int);
  rtx expand_mls ();
  rtx expand_msb (unsigned int);
  rtx expand_mul ();
  rtx expand_mul_lane ();
  rtx expand_mulh ();
  rtx expand_mulx ();
  rtx expand_nand ();
  rtx expand_neg ();
  rtx expand_nmad ();
  rtx expand_nmla ();
  rtx expand_nmls ();
  rtx expand_nmsb ();
  rtx expand_nor ();
  rtx expand_not ();
  rtx expand_orn ();
  rtx expand_orr ();
  rtx expand_permute (int);
  rtx expand_pfalse ();
  rtx expand_prf ();
  rtx expand_prf_gather ();
  rtx expand_ptest (rtx_code);
  rtx expand_ptrue ();
  rtx expand_ptrue_pat ();
  rtx expand_qadd ();
  rtx expand_qinc_qdec (rtx_code, rtx_code);
  rtx expand_qincp_qdecp (rtx_code, rtx_code);
  rtx expand_qsub ();
  rtx expand_rbit ();
  rtx expand_rdffr ();
  rtx expand_reduction (int, int, int);
  rtx expand_rev ();
  rtx expand_sel ();
  rtx expand_set ();
  rtx expand_setffr ();
  rtx expand_shift (rtx_code, int);
  rtx expand_shift_wide (int);
  rtx expand_splice ();
  rtx expand_sqrt ();
  rtx expand_st1 ();
  rtx expand_st1_scatter ();
  rtx expand_st1_scatter_trunc ();
  rtx expand_st1_trunc ();
  rtx expand_st234 ();
  rtx expand_stnt1 ();
  rtx expand_sub (bool);
  rtx expand_tbl ();
  rtx expand_unary_count (rtx_code);
  rtx expand_undef ();
  rtx expand_while (int, int);
  rtx expand_wrffr ();

  rtx expand_pred_op (rtx_code, int, unsigned int = DEFAULT_MERGE_ARGNO);
  rtx expand_pred_op (int, int);
  rtx expand_signed_pred_op (rtx_code, rtx_code, int,
			     unsigned int = DEFAULT_MERGE_ARGNO);
  rtx expand_signed_pred_op (int, int, int);
  rtx expand_via_unpred_direct_optab (optab, machine_mode = VOIDmode);
  rtx expand_via_exact_insn (insn_code);
  rtx expand_via_load_insn (insn_code);
  rtx expand_via_prefetch_insn (insn_code);
  rtx expand_via_store_insn (insn_code);
  rtx expand_via_unpred_insn (insn_code);
  rtx expand_via_pred_direct_optab (optab, unsigned int = DEFAULT_MERGE_ARGNO);
  rtx expand_via_sel_insn (insn_code);
  rtx expand_via_pred_insn (insn_code, unsigned int = DEFAULT_MERGE_ARGNO);
  rtx expand_via_signed_unpred_insn (rtx_code, rtx_code);
  rtx expand_via_pred_x_insn (insn_code);
  rtx expand_pred_shift_right_imm (insn_code);

  void require_immediate_extract_index (unsigned int);
  void require_immediate_lane_index (unsigned int, unsigned int = 1);
  void require_immediate_range (unsigned int, HOST_WIDE_INT, HOST_WIDE_INT);
  int get_immediate_rotate (unsigned int, bool);
  void require_immediate_svpattern (unsigned int);
  void require_immediate_svprfop (unsigned int);

  rtx get_contiguous_base (machine_mode);
  void prepare_gather_address_operands (unsigned int);
  void prepare_prefetch_operands ();
  rtx prepare_dupq_lane_index (unsigned int);
  void rotate_inputs_left (unsigned int, unsigned int);
  bool try_negating_argument (unsigned int, machine_mode);
  bool overlaps_input_p (rtx);

  bool function_returns_void_p ();
  machine_mode get_mode (unsigned int);
  machine_mode get_quarter_mode (unsigned int);
  machine_mode get_pred_mode (unsigned int);
  rtx get_fallback_value (machine_mode, unsigned int,
			  unsigned int, unsigned int &);

  machine_mode get_next_operand_mode (insn_code);
  void add_output_operand (insn_code);
  void add_input_operand (insn_code, rtx);
  void add_integer_operand (HOST_WIDE_INT);
  void add_mem_operand (machine_mode, rtx);
  void add_address_operand (rtx);
  void add_fixed_operand (rtx);
  rtx generate_insn (insn_code);

  /* The function being called.  */
  const registered_function &m_rfn;
  const function_instance &m_fi;

  /* The function call expression.  */
  tree m_exp;

  /* The location of the call.  */
  location_t m_location;

  /* Where the result should go, if convenient.  */
  rtx m_target;

  /* The expanded arguments.  */
  auto_vec<rtx, 16> m_args;

  /* Used to build up the operands to an instruction.  */
  auto_vec<expand_operand, 8> m_ops;
};

/* Information about each single-predicate or single-vector type.  */
static const vector_type_info vector_types[] = {
#define DEF_SVE_TYPE(ACLE_NAME, NCHARS, ABI_NAME, SCALAR_TYPE) \
  { #ACLE_NAME, #ABI_NAME, #NCHARS #ABI_NAME },
#include "aarch64-sve-builtins.def"
};

/* The function name suffix associated with each predication type.  */
static const char *const pred_suffixes[NUM_PREDS + 1] = {
  "",
  "",
  "_m",
  "_x",
  "_z",
  ""
};

/* The function name suffix associated with each mode, for full
   (non-overloaded) names.  */
static const mode_info modes[] = {
#define VECTOR_TYPE_none NUM_VECTOR_TYPES
#define DEF_SVE_MODE(NAME, BASE, DISPLACEMENT, UNITS) \
  { "_" #NAME, VECTOR_TYPE_##BASE, VECTOR_TYPE_##DISPLACEMENT, UNITS_##UNITS },
#include "aarch64-sve-builtins.def"
#undef VECTOR_TYPE_none
  { "", NUM_VECTOR_TYPES, NUM_VECTOR_TYPES, UNITS_none }
};

/* The function name suffix associated with each element type.  */
static const type_suffix_info type_suffixes[NUM_TYPE_SUFFIXES + 1] = {
#define DEF_SVE_TYPE_SUFFIX(NAME, ACLE_TYPE, BITS, MODE) \
  { "_" #NAME, VECTOR_TYPE_ ## ACLE_TYPE, BITS, BITS / BITS_PER_UNIT, \
    #NAME[0] == 's' || #NAME[0] == 'u', \
    #NAME[0] == 'u', #NAME[0] == 'b', 0, MODE },
#include "aarch64-sve-builtins.def"
  { "", NUM_VECTOR_TYPES, 0, 0, false, false, false, 0, VOIDmode }
};

/* _b only.  */
#define TYPES_b(S, D) \
  S (b)

/* _b8 _b16 _b32 _b64.  */
#define TYPES_all_pred(S, D) \
  S (b8), S (b16), S (b32), S (b64)

/* _f16 _f32 _f64.  */
#define TYPES_all_float(S, D) \
  S (f16), S (f32), S (f64)

/* _s8 _s16 _s32 _s64.  */
#define TYPES_all_signed(S, D) \
  S (s8), S (s16), S (s32), S (s64)

/* _u8 _u16 _u32 _u64.  */
#define TYPES_all_unsigned(S, D) \
  S (u8), S (u16), S (u32), S (u64)

/* _s8 _s16 _s32 _s64
   _u8 _u16 _u32 _u64.  */
#define TYPES_all_integer(S, D) \
  TYPES_all_signed (S, D), TYPES_all_unsigned (S, D)

/*     _f16 _f32 _f64
   _u8 _u16 _u32 _u64
   _s8 _s16 _s32 _s64.  */
#define TYPES_all_data(S, D) \
  TYPES_all_float (S, D), TYPES_all_integer (S, D)

/* _s8 _s16 _s32.  */
#define TYPES_bhs_signed(S, D) \
  S (s8), S (s16), S (s32)

/* _u8 _u16 _u32.  */
#define TYPES_bhs_unsigned(S, D) \
  S (u8), S (u16), S (u32)

/* _s8 _s16 _s32
   _u8 _u16 _u32.  */
#define TYPES_bhs_integer(S, D) \
  TYPES_bhs_signed (S, D), TYPES_bhs_unsigned (S, D)

/* _s16 _u16.  */
#define TYPES_h_integer(S, D) \
  S (s16), S (u16)

/* _f16 _f32.  */
#define TYPES_hs_float(S, D) \
  S (f16), S (f32)

/* _s16 _s32 _s64
   _u16 _u32 _u64.  */
#define TYPES_hsd_integer(S, D) \
  S (s16), S (s32), S (s64), S (u16), S (u32), S (u64)

/* _s32 _u32.  */
#define TYPES_s_integer(S, D) \
  S (s32), S (u32)

/* _s32 _s64 _u32 _u64.  */
#define TYPES_sd_integer(S, D) \
  S (s32), S (s64), S (u32), S (u64)

/* _f32 _f64
   _u32 _u64
   _s32 _s64.  */
#define TYPES_sd_data(S, D) \
  S (f32), S (f64), TYPES_sd_integer (S, D)

/* _s64 _u64.  */
#define TYPES_d_integer(S, D) \
  S (s64), S (u64)

/* _f16 _f32 _f64
	_u32 _u64
	_s32 _s64.  */
#define TYPES_all_float_and_sd_integer(S, D) \
  TYPES_all_float (S, D), TYPES_sd_integer (S, D)

/*     _f16 _f32 _f64
   _s8 _s16 _s32 _s64.  */
#define TYPES_all_float_and_signed(S, D) \
  TYPES_all_float (S, D), TYPES_all_signed (S, D)

/* {_s32 _s64 _u32 _u64 } x {_b8 _b16 _b32 _b64} */
#define TYPES_inc_dec_n_bN(D, bn) \
  D (s32, bn), D (s64, bn), D (u32, bn), D (u64, bn)
#define TYPES_inc_dec_n(S, D) \
  TYPES_inc_dec_n_bN (D, b8), \
  TYPES_inc_dec_n_bN (D, b16), \
  TYPES_inc_dec_n_bN (D, b32), \
  TYPES_inc_dec_n_bN (D, b64)

/* {_b8 _b16 _b32 _b64} x {_s32 _s64 _u32 _u64 } */
#define TYPES_while_bN(D, bn) \
  D (bn, s32), D (bn, s64), D (bn, u32), D (bn, u64)
#define TYPES_while(S, D) \
  TYPES_while_bN (D, b8), \
  TYPES_while_bN (D, b16), \
  TYPES_while_bN (D, b32), \
  TYPES_while_bN (D, b64)

/* Describe a pair of type suffixes in which only the first is used.  */
#define DEF_VECTOR_TYPE(X) { TYPE_SUFFIX_ ## X, NUM_TYPE_SUFFIXES }

/* Describe a pair of type suffixes in which both are used.  */
#define DEF_DOUBLE_TYPE(X, Y) { TYPE_SUFFIX_ ## X, TYPE_SUFFIX_ ## Y }

/* Create an array that can be used in aarch64-sve-builtins.def to
   select the type suffixes in TYPES_<NAME>.  */
#define DEF_SVE_TYPES_ARRAY(NAME) \
  static const type_suffix_pair types_##NAME[] = { \
    TYPES_##NAME (DEF_VECTOR_TYPE, DEF_DOUBLE_TYPE), \
    { NUM_TYPE_SUFFIXES, NUM_TYPE_SUFFIXES } \
  }

/* For functions that don't take any type suffixes.  */
static const type_suffix_pair types_none[] = {
  { NUM_TYPE_SUFFIXES, NUM_TYPE_SUFFIXES },
  { NUM_TYPE_SUFFIXES, NUM_TYPE_SUFFIXES }
};

DEF_SVE_TYPES_ARRAY (all_data);
DEF_SVE_TYPES_ARRAY (all_float);
DEF_SVE_TYPES_ARRAY (all_float_and_sd_integer);
DEF_SVE_TYPES_ARRAY (all_float_and_signed);
DEF_SVE_TYPES_ARRAY (all_integer);
DEF_SVE_TYPES_ARRAY (all_pred);
DEF_SVE_TYPES_ARRAY (all_signed);
DEF_SVE_TYPES_ARRAY (all_unsigned);
DEF_SVE_TYPES_ARRAY (b);
DEF_SVE_TYPES_ARRAY (bhs_signed);
DEF_SVE_TYPES_ARRAY (bhs_unsigned);
DEF_SVE_TYPES_ARRAY (bhs_integer);
DEF_SVE_TYPES_ARRAY (h_integer);
DEF_SVE_TYPES_ARRAY (hs_float);
DEF_SVE_TYPES_ARRAY (hsd_integer);
DEF_SVE_TYPES_ARRAY (s_integer);
DEF_SVE_TYPES_ARRAY (sd_data);
DEF_SVE_TYPES_ARRAY (sd_integer);
DEF_SVE_TYPES_ARRAY (d_integer);
DEF_SVE_TYPES_ARRAY (inc_dec_n);
DEF_SVE_TYPES_ARRAY (while);

/* Used by functions in aarch64-sve-builtins.def that have no governing
   predicate.  */
static const predication preds_none[] = { PRED_none, NUM_PREDS };

/* Used by functions in aarch64-sve-builtins.def that have a governing
   predicate but do not have an explicit suffix.  */
static const predication preds_inherent[] = { PRED_inherent, NUM_PREDS };

/* Used by functions in aarch64-sve-builtins.def that allow merging,
   zeroing and "don't care" predication.  */
static const predication preds_mxz[] = { PRED_m, PRED_x, PRED_z, NUM_PREDS };

/* Used by functions that have the mxz predicated forms above, and in addition
   have an unpredicated form.  */
static const predication preds_mxz_or_none[] = {
  PRED_m, PRED_x, PRED_z, PRED_none, NUM_PREDS
};

/* Used by functions in aarch64-sve-builtins.def that allow merging and
   zeroing predication but have no "_x" form.  */
static const predication preds_mz[] = { PRED_m, PRED_z, NUM_PREDS };

/* Used by functions that have an unpredicated form and a _z predicated
   form.  */
static const predication preds_z_or_none[] = { PRED_z, PRED_none, NUM_PREDS };

/* Used by (mostly predicate) functions that only support "_z" predication.  */
static const predication preds_z[] = { PRED_z, NUM_PREDS };

/* Each SVE function base name as a string.  */
static const char *const base_names[] = {
#define DEF_SVE_BASE_NAME(NAME) #NAME,
#define DEF_SVE_LAST_BASE_NAME(NAME) #NAME
#include "aarch64-sve-builtins.def"
};

/* A list of all SVE ACLE functions.  */
static const function_group function_groups[] = {
#define DEF_SVE_FUNCTION(NAME, SHAPE, TYPES, PREDS) \
  { FUNC_##NAME, SHAPE_##SHAPE, types_##TYPES, preds_##PREDS },
#include "aarch64-sve-builtins.def"
};

/* The scalar type associated with each vector type.  */
static GTY(()) tree scalar_types[NUM_VECTOR_TYPES];

/* The single-predicate and single-vector types, with their built-in
   "__SV..._t" name.  */
static GTY(()) tree abi_vector_types[NUM_VECTOR_TYPES];

/* Same, but with the arm_sve.h "sv..._t" name.  Allow an index of
   NUM_VECTOR_TYPES, which always yields a null tree.  */
static GTY(()) tree acle_vector_types[MAX_TUPLE_SIZE][NUM_VECTOR_TYPES + 1];

/* The svpattern enum type.  */
static tree acle_svpattern;

/* The svprfop enum type.  */
static tree acle_svprfop;

/* The list of all registered function decls, indexed by code.  */
static GTY(()) vec<registered_function *, va_gc> *registered_functions;

/* All registered function decls, hashed on the function_instance
   that they implement.  This is used for looking up implementations of
   overloaded functions.  */
static hash_table<registered_function_hasher> *function_table;

/* True if we've already complained about attempts to use functions
   when the required feature is disabled.  */
static bool reported_missing_feature_p;

/* Return the ACLE type svbool_t.  */
static tree
get_svbool_t (void)
{
  return acle_vector_types[0][VECTOR_TYPE_svbool_t];
}

/* Return true if TYPE is a vector of integers.  */
bool
integral_type_p (vector_type type)
{
  return type != VECTOR_TYPE_svbool_t && INTEGRAL_TYPE_P (scalar_types[type]);
}

/* Try to find a mode with the given mode_info fields.  Return the mode
   on success or MODE_none on failure.  */
static function_mode
find_function_mode (vector_type base_vector_type,
		    vector_type displacement_vector_type,
		    units displacement_units)
{
  for (unsigned int mode = 0; mode < ARRAY_SIZE (modes); ++mode)
    if (modes[mode].base_vector_type == base_vector_type
	&& modes[mode].displacement_vector_type == displacement_vector_type
	&& modes[mode].displacement_units == displacement_units)
      return function_mode (mode);
  return MODE_none;
}

/* If TYPE is an ACLE vector type, return the associated vector_type,
   otherwise return NUM_VECTOR_TYPES.  */
static vector_type
find_vector_type (const_tree type)
{
  /* A linear search should be OK here, since the code isn't hot and
     the number of types is only small.  */
  for (unsigned int i = 0; i < ARRAY_SIZE (abi_vector_types); ++i)
    if (TYPE_MAIN_VARIANT (type) == abi_vector_types[i])
      return (vector_type) i;
  return NUM_VECTOR_TYPES;
}

/* If TYPE is a valid SVE element type, return the corresponding vector type,
   otherwise return NUM_VECTOR_TYPES.  */
static vector_type
find_vector_type_from_scalar_type (const_tree type)
{
  /* A linear search should be OK here, since the code isn't hot and
     the number of types is only small.  */
  for (unsigned int i = 0; i < ARRAY_SIZE (abi_vector_types); ++i)
    if (scalar_types[i] != boolean_type_node
	&& TYPE_MAIN_VARIANT (type) == scalar_types[i])
      return (vector_type) i;
  return NUM_VECTOR_TYPES;
}

/* Return the type suffix associated with integer elements that have
   ELEM_BITS bits and the signedness given by UNSIGNED_P.  */
static type_suffix
find_integer_type_suffix (bool unsigned_p, unsigned int elem_bits)
{
  for (unsigned int i = 0; i < NUM_TYPE_SUFFIXES; ++i)
    if (type_suffixes[i].integer_p
	&& type_suffixes[i].unsigned_p == unsigned_p
	&& type_suffixes[i].elem_bits == elem_bits)
      return type_suffix (i);
  gcc_unreachable ();
}

/* Find the type suffix for unsigned integers that have the same number
   of element bits as TYPE.  */
static type_suffix
unsigned_type_suffix (type_suffix type)
{
  return find_integer_type_suffix (true, type_suffixes[type].elem_bits);
}

/* Return the type suffix for elements that are a quarter the size of
   integer type suffix TYPE.  */
static type_suffix
find_quarter_type_suffix (type_suffix type)
{
  return find_integer_type_suffix (type_suffixes[type].unsigned_p,
				   type_suffixes[type].elem_bits / 4);
}

/* Return the wide version of TYPE.  */
static vector_type
wide_type_for (vector_type type)
{
  return (!integral_type_p (type) ? type
	  : TYPE_UNSIGNED (scalar_types[type]) ? VECTOR_TYPE_svuint64_t
	  : VECTOR_TYPE_svint64_t);
}

/* Return the single field in tuple type TYPE.  */
static tree
tuple_type_field (tree type)
{
  for (tree field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
    if (TREE_CODE (field) == FIELD_DECL)
      return field;
  gcc_unreachable ();
}

/* Report an error against LOCATION that the user has tried to use
   function DECL when feature FEATURE is disabled.  */
static void
report_missing_feature (location_t location, tree decl, const char *feature)
{
  /* Avoid reporting a slew of messages for a single oversight.  */
  if (reported_missing_feature_p)
    return;

  error_at (location, "ACLE function %qD requires ISA extension %qs",
	    decl, feature);
  inform (location, "you can enable %qs using the command-line"
	  " option %<-march%>, or by using the %<target%>"
	  " attribute or pragma", feature);
  reported_missing_feature_p = true;
}

/* Report that LOCATION has a call to DECL in which argument ARGNO
   was not an integer constant expression.  */
static void
report_non_ice (location_t location, tree decl, unsigned int argno)
{
  error_at (location, "argument %d of %qE must be an integer constant"
	    " expression", argno + 1, decl);
}

/* Report that LOCATION has a call to DECL in which argument ARGNO has
   the value ACTUAL, whereas the function requires a value in the range
   [MIN, MAX].  */
static void
report_out_of_range (location_t location, tree decl, unsigned int argno,
		     HOST_WIDE_INT actual, HOST_WIDE_INT min,
		     HOST_WIDE_INT max)
{
  error_at (location, "passing %wd to argument %d of %qE, which expects"
	    " a value in the range [%wd, %wd]", actual, argno + 1, decl,
	    min, max);
}

/* Report that LOCATION has a call to DECL in which argument ARGNO has
   the value ACTUAL, whereas the function requires either VALUE0 or
   VALUE1.  */
static void
report_neither_nor (location_t location, tree decl, unsigned int argno,
		    HOST_WIDE_INT actual, HOST_WIDE_INT value0,
		    HOST_WIDE_INT value1)
{
  error_at (location, "passing %wd to argument %d of %qE, which expects"
	    " either %wd or %wd", actual, argno + 1, decl, value0, value1);
}

/* Report that LOCATION has a call to DECL in which argument ARGNO has
   the value ACTUAL, whereas the function requires one of VALUE0..3.  */
static void
report_not_one_of (location_t location, tree decl, unsigned int argno,
		   HOST_WIDE_INT actual, HOST_WIDE_INT value0,
		   HOST_WIDE_INT value1, HOST_WIDE_INT value2,
		   HOST_WIDE_INT value3)
{
  error_at (location, "passing %wd to argument %d of %qE, which expects"
	    " %wd, %wd, %wd or %wd", actual, argno + 1, decl, value0, value1,
	    value2, value3);
}

/* Report that LOCATION has a call to DECL in which argument ARGNO has
   the value ACTUAL, whereas the function requires a valid value of
   enum type ENUMTYPE.  */
static void
report_not_enum (location_t location, tree decl, unsigned int argno,
		 HOST_WIDE_INT actual, tree enumtype)
{
  error_at (location, "passing %wd to argument %d of %qE, which expects"
	    " a valid %qT value", actual, argno + 1, decl, enumtype);
}

inline
function_instance::function_instance (function func_in,
				      function_shape shape_in,
				      function_mode mode_in,
				      const type_suffix_pair &types_in,
				      predication pred_in)
  : func (func_in), shape (shape_in), mode (mode_in), pred (pred_in)
{
  memcpy (types, types_in, sizeof (types));
}

inline bool
function_instance::operator== (const function_instance &other) const
{
  return (func == other.func
	  && shape == other.shape
	  && mode == other.mode
	  && pred == other.pred
	  && types[0] == other.types[0]
	  && types[1] == other.types[1]);
}

inline bool
function_instance::operator!= (const function_instance &other) const
{
  return !operator== (other);
}

hashval_t
function_instance::hash () const
{
  inchash::hash h;
  h.add_int (func);
  h.add_int (shape);
  h.add_int (mode);
  h.add_int (types[0]);
  h.add_int (types[1]);
  h.add_int (pred);
  return h.end ();
}

/* If the function takes a vector or scalar displacement, return the units
   in which the displacement is measured, otherwise return UNITS_none.  */
inline units
function_instance::displacement_units () const
{
  return modes[mode].displacement_units;
}

/* Return a pointer to the first '_' in the function base name, or to the
   null terminator if there is no '_'.  */
const char *
function_instance::end_base_name () const
{
  const char *p = base_names[func];
  while (p[0] && p[0] != '_')
    ++p;
  return p;
}

/* For a function with a [bhwd] suffix, return the SVE vector mode
   associated with that suffix.  */
machine_mode
function_instance::bhwd_vector_mode () const
{
  const char *end = end_base_name ();
  switch (end[-1])
    {
    case 'b': return VNx16QImode;
    case 'h': return VNx8HImode;
    case 'w': return VNx4SImode;
    case 'd': return VNx2DImode;
    default:
      gcc_unreachable ();
    }
}

/* For a function with a [bhwd] suffix, return the scalar mode associated
   with that suffix.  */
scalar_mode
function_instance::bhwd_scalar_mode () const
{
  return GET_MODE_INNER (bhwd_vector_mode ());
}

/* For a function with a [bhwd] suffix, return the number of elements
   of that size that fit within a 128-bit block.  */
unsigned int
function_instance::bhwd_nelts_per_vq () const
{
  return 128 / GET_MODE_BITSIZE (bhwd_scalar_mode ());
}

/* Return the memory vector mode, if meaningful.  */
machine_mode
function_instance::memory_vector_mode () const
{
  machine_mode mode = type_suffixes[types[0]].mode;
  switch (func)
    {
    case FUNC_svld1:
    case FUNC_svld1_gather:
    case FUNC_svldff1:
    case FUNC_svldff1_gather:
    case FUNC_svldnf1:
    case FUNC_svldnt1:
    case FUNC_svst1:
    case FUNC_svst1_scatter:
    case FUNC_svstnt1:
      return mode;

    case FUNC_svld1rq:
      return aarch64_vq_mode (GET_MODE_INNER (mode)).require ();

    case FUNC_svld2:
    case FUNC_svst2:
      return targetm.array_mode (mode, 2).require ();

    case FUNC_svld3:
    case FUNC_svst3:
      return targetm.array_mode (mode, 3).require ();

    case FUNC_svld4:
    case FUNC_svst4:
      return targetm.array_mode (mode, 4).require ();

    case FUNC_svld1sb:
    case FUNC_svld1sb_gather:
    case FUNC_svld1sh:
    case FUNC_svld1sh_gather:
    case FUNC_svld1sw:
    case FUNC_svld1sw_gather:
    case FUNC_svld1ub:
    case FUNC_svld1ub_gather:
    case FUNC_svld1uh:
    case FUNC_svld1uh_gather:
    case FUNC_svld1uw:
    case FUNC_svld1uw_gather:
    case FUNC_svldff1sb:
    case FUNC_svldff1sb_gather:
    case FUNC_svldff1sh:
    case FUNC_svldff1sh_gather:
    case FUNC_svldff1sw:
    case FUNC_svldff1sw_gather:
    case FUNC_svldff1ub:
    case FUNC_svldff1ub_gather:
    case FUNC_svldff1uh:
    case FUNC_svldff1uh_gather:
    case FUNC_svldff1uw:
    case FUNC_svldff1uw_gather:
    case FUNC_svldnf1sb:
    case FUNC_svldnf1sh:
    case FUNC_svldnf1sw:
    case FUNC_svldnf1ub:
    case FUNC_svldnf1uh:
    case FUNC_svldnf1uw:
    case FUNC_svst1b:
    case FUNC_svst1b_scatter:
    case FUNC_svst1h:
    case FUNC_svst1h_scatter:
    case FUNC_svst1w:
    case FUNC_svst1w_scatter:
      return aarch64_sve_data_mode (bhwd_scalar_mode (),
				    GET_MODE_NUNITS (mode)).require ();

    case FUNC_svprfb_gather:
    case FUNC_svprfd_gather:
    case FUNC_svprfh_gather:
    case FUNC_svprfw_gather:
      return aarch64_full_sve_mode (bhwd_scalar_mode ()).require ();

    default:
      gcc_unreachable ();
    }
}

/* Return the type of the function's vector base address argument,
   or null it doesn't have a vector base address.  */
tree
function_instance::base_vector_type () const
{
  return acle_vector_types[0][modes[mode].base_vector_type];
}

/* Return the type of the function's vector index or offset argument,
   or null if doesn't have a vector index or offset argument.  */
tree
function_instance::displacement_vector_type () const
{
  return acle_vector_types[0][modes[mode].displacement_vector_type];
}

/* Return the memory element type, if meaningful.  */
tree
function_instance::memory_scalar_type () const
{
  switch (func)
    {
    case FUNC_svld1:
    case FUNC_svld1_gather:
    case FUNC_svld1rq:
    case FUNC_svld2:
    case FUNC_svld3:
    case FUNC_svld4:
    case FUNC_svldff1:
    case FUNC_svldff1_gather:
    case FUNC_svldnf1:
    case FUNC_svldnt1:
    case FUNC_svst1:
    case FUNC_svst1_scatter:
    case FUNC_svst2:
    case FUNC_svst3:
    case FUNC_svst4:
    case FUNC_svstnt1:
      return scalar_type (0);

    case FUNC_svld1sb:
    case FUNC_svld1sb_gather:
    case FUNC_svldff1sb:
    case FUNC_svldff1sb_gather:
    case FUNC_svldnf1sb:
      return scalar_types[VECTOR_TYPE_svint8_t];

    case FUNC_svld1sh:
    case FUNC_svld1sh_gather:
    case FUNC_svldff1sh:
    case FUNC_svldff1sh_gather:
    case FUNC_svldnf1sh:
      return scalar_types[VECTOR_TYPE_svint16_t];

    case FUNC_svld1sw:
    case FUNC_svld1sw_gather:
    case FUNC_svldff1sw:
    case FUNC_svldff1sw_gather:
    case FUNC_svldnf1sw:
      return scalar_types[VECTOR_TYPE_svint32_t];

    case FUNC_svld1ub:
    case FUNC_svld1ub_gather:
    case FUNC_svldff1ub:
    case FUNC_svldff1ub_gather:
    case FUNC_svldnf1ub:
      return scalar_types[VECTOR_TYPE_svuint8_t];

    case FUNC_svld1uh:
    case FUNC_svld1uh_gather:
    case FUNC_svldff1uh:
    case FUNC_svldff1uh_gather:
    case FUNC_svldnf1uh:
      return scalar_types[VECTOR_TYPE_svuint16_t];

    case FUNC_svld1uw:
    case FUNC_svld1uw_gather:
    case FUNC_svldff1uw:
    case FUNC_svldff1uw_gather:
    case FUNC_svldnf1uw:
      return scalar_types[VECTOR_TYPE_svuint32_t];

    case FUNC_svst1b:
    case FUNC_svst1b_scatter:
      return scalar_types[TYPE_UNSIGNED (scalar_type (0))
			  ? VECTOR_TYPE_svuint8_t
			  : VECTOR_TYPE_svint8_t];

    case FUNC_svst1h:
    case FUNC_svst1h_scatter:
      return scalar_types[TYPE_UNSIGNED (scalar_type (0))
			  ? VECTOR_TYPE_svuint16_t
			  : VECTOR_TYPE_svint16_t];

    case FUNC_svst1w:
    case FUNC_svst1w_scatter:
      return scalar_types[TYPE_UNSIGNED (scalar_type (0))
			  ? VECTOR_TYPE_svuint32_t
			  : VECTOR_TYPE_svint32_t];

    default:
      gcc_unreachable ();
    }
}

/* Return the scalar type associated with type suffix I.  */
inline tree
function_instance::scalar_type (unsigned int i) const
{
  return scalar_types[type_suffixes[types[i]].type];
}

/* Return the vector type associated with type suffix I.  */
inline tree
function_instance::vector_type (unsigned int i) const
{
  return acle_vector_types[0][type_suffixes[types[i]].type];
}

/* Return the tuple type with NUM_VECTORS vectors associated with type
   suffix I.  */
inline tree
function_instance::tuple_type (unsigned int num_vectors, unsigned int i) const
{
  return acle_vector_types[num_vectors - 1][type_suffixes[types[i]].type];
}

/* Return the quarter size vector type associated with type suffix I.  */
tree
function_instance::quarter_vector_type (unsigned int i) const
{
  type_suffix quarter_type = find_quarter_type_suffix (types[i]);
  return acle_vector_types[0][type_suffixes[quarter_type].type];
}

/* Return the quarter size scalar type associated with type suffix I.  */
tree
function_instance::quarter_scalar_type (unsigned int i) const
{
  type_suffix quarter_type = find_quarter_type_suffix (types[i]);
  return scalar_types[type_suffixes[quarter_type].type];
}

/* Return the unsigned scalar type associated with type suffix I.  */
inline tree
function_instance::unsigned_scalar_type (unsigned int i) const
{
  type_suffix unsigned_type = unsigned_type_suffix (types[i]);
  return scalar_types[type_suffixes[unsigned_type].type];
}

/* Return the unsigned vector type associated with type suffix I.  */
inline tree
function_instance::unsigned_vector_type (unsigned int i) const
{
  type_suffix unsigned_type = unsigned_type_suffix (types[i]);
  return acle_vector_types[0][type_suffixes[unsigned_type].type];
}

/* Return the wide vector type associated with type suffix I.  */
tree
function_instance::wide_vector_type (unsigned int i) const
{
  return acle_vector_types[0][wide_type_for (type_suffixes[types[i]].type)];
}

/* Return the wide scalar type associated with type suffix I.  */
tree
function_instance::wide_scalar_type (unsigned int i) const
{
  return scalar_types[wide_type_for (type_suffixes[types[i]].type)];
}

/* Return the number of elements of type suffix I that fit within a
   128-bit block.  */
unsigned int
function_instance::nelts_per_vq (unsigned int i) const
{
  return 128 / type_suffixes[types[i]].elem_bits;
}

inline hashval_t
registered_function_hasher::hash (value_type value)
{
  return value->instance.hash ();
}

inline bool
registered_function_hasher::equal (value_type value, const compare_type &key)
{
  return value->instance == key;
}

sve_switcher::sve_switcher ()
  : m_old_isa_flags (aarch64_isa_flags)
{
  /* Changing the ISA flags and have_regs_of_mode should be enough here.
     We shouldn't need to pay the compile-time cost of a full target
     switch.  */
  aarch64_isa_flags = (AARCH64_FL_FP | AARCH64_FL_SIMD | AARCH64_FL_F16
		       | AARCH64_FL_SVE);

  memcpy (m_old_have_regs_of_mode, have_regs_of_mode,
	  sizeof (have_regs_of_mode));
  for (int i = 0; i < NUM_MACHINE_MODES; ++i)
    if (aarch64_sve_mode_p ((machine_mode) i))
      have_regs_of_mode[i] = true;
}

sve_switcher::~sve_switcher ()
{
  memcpy (have_regs_of_mode, m_old_have_regs_of_mode,
	  sizeof (have_regs_of_mode));
  aarch64_isa_flags = m_old_isa_flags;
}

arm_sve_h_builder::arm_sve_h_builder ()
{
  m_overload_type = build_function_type (void_type_node, void_list_node);
  m_direct_overloads = lang_GNU_CXX ();
  gcc_obstack_init (&m_string_obstack);
}

arm_sve_h_builder::~arm_sve_h_builder ()
{
  obstack_free (&m_string_obstack, NULL);
}

/* Register TYPE under its arm_sve.h name.  */
void
arm_sve_h_builder::register_type (vector_type type)
{
  tree vectype = abi_vector_types[type];
  tree id = get_identifier (vector_types[type].acle_name);
  tree decl = build_decl (input_location, TYPE_DECL, id, vectype);
  decl = lang_hooks.decls.pushdecl (decl);
  if (TREE_CODE (decl) == TYPE_DECL
      && TYPE_MAIN_VARIANT (TREE_TYPE (decl)) == vectype)
    vectype = TREE_TYPE (decl);
  acle_vector_types[0][type] = vectype;

  if (type != VECTOR_TYPE_svbool_t)
    for (unsigned int i = 2; i <= MAX_TUPLE_SIZE; ++i)
      register_tuple_type (i, type);
}

/* Register the svpattern enum.  */
void
arm_sve_h_builder::register_svpattern ()
{
  auto_vec<string_int_pair, 32> values;
#define PUSH(UPPER, LOWER, VALUE) \
    values.quick_push (string_int_pair ("SV_" #UPPER, VALUE));
  AARCH64_FOR_SVPATTERN (PUSH)
#undef PUSH

  acle_svpattern = lang_hooks.types.build_enum (input_location,
						"svpattern", values);
}

/* Register the svprfop enum.  */
void
arm_sve_h_builder::register_svprfop ()
{
  auto_vec<string_int_pair, 16> values;
#define PUSH(UPPER, LOWER, VALUE) \
    values.quick_push (string_int_pair ("SV_" #UPPER, VALUE));
  AARCH64_FOR_SVPRFOP (PUSH)
#undef PUSH

  acle_svprfop = lang_hooks.types.build_enum (input_location,
					      "svprfop", values);
}

/* Define all functions associated with GROUP.  */
void
arm_sve_h_builder::build (const function_group &group)
{
  switch (group.shape)
    {
    case SHAPE_adr_index:
      add_overloaded_functions (group, MODE_index);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u32base_s32index);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u32base_u32index);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u64base_s64index);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u64base_u64index);
      break;

    case SHAPE_adr_offset:
      add_overloaded_functions (group, MODE_offset);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u32base_s32offset);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u32base_u32offset);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u64base_s64offset);
      build_all (&arm_sve_h_builder::sig_adr, group, MODE_u64base_u64offset);
      break;

    case SHAPE_binary:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary<2>, group, MODE_none);
      break;

    case SHAPE_binary_extract:
    case SHAPE_binary_lane:
    case SHAPE_binary_rotate:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_u64_n<3>, group, MODE_none);
      break;

    case SHAPE_binary_index:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_index<2>, group, MODE_none);
      break;

    case SHAPE_binary_index_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_index_n<2>, group, MODE_none);
      break;

    case SHAPE_binary_index64_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_index64_n<2>, group, MODE_none);
      break;

    case SHAPE_binary_n:
      add_overloaded_functions (group, MODE_n);
      build_all (&arm_sve_h_builder::sig_nary_n<2>, group, MODE_n);
      break;

    case SHAPE_binary_opt_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary<2>, group, MODE_none);
      /* _b functions do not have an _n form, but are classified as
	 binary_opt_n so that they can be overloaded with vector
	 functions.  */
      if (group.types[0][0] == TYPE_SUFFIX_b)
	gcc_assert (group.types[0][1] == NUM_TYPE_SUFFIXES);
      else
	build_all (&arm_sve_h_builder::sig_nary_n<2>, group, MODE_n);
      break;

    case SHAPE_binary_pred:
      build_all (&arm_sve_h_builder::sig_nary<2>, group, MODE_none);
      break;

    case SHAPE_binary_scalar:
      build_all (&arm_sve_h_builder::sig_nary_nn<2>, group, MODE_none);
      break;

    case SHAPE_binary_wide:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_u64<2>, group, MODE_none);
      break;

    case SHAPE_compare_opt_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_compare, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_compare_n, group, MODE_n);
      break;

    case SHAPE_compare_scalar:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_compare_scalar, group, MODE_none);
      break;

    case SHAPE_compare_wide:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_compare_wide, group, MODE_none);
      break;

    case SHAPE_create2:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_create<2>, group, MODE_none);
      break;

    case SHAPE_create3:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_create<3>, group, MODE_none);
      break;

    case SHAPE_create4:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_create<4>, group, MODE_none);
      break;

    case SHAPE_dupq:
      build_all (&arm_sve_h_builder::sig_dupq, group, MODE_n, true);
      break;

    case SHAPE_fold_left:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_fold_left, group, MODE_none);
      break;

    case SHAPE_get2:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_get<2>, group, MODE_none);
      break;

    case SHAPE_get3:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_get<3>, group, MODE_none);
      break;

    case SHAPE_get4:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_get<4>, group, MODE_none);
      break;

    case SHAPE_inc_dec:
    case SHAPE_inc_dec_pattern:
      add_overloaded_functions (group, MODE_none);
      /* These functions are unusual in that the type suffixes for
	 the scalar and vector forms are not related.  The vector
	 form always has exactly two potential suffixes while the
	 scalar form always has four.  */
      if (group.types[2][0] == NUM_TYPE_SUFFIXES)
	build_all (&arm_sve_h_builder::sig_inc_dec, group, MODE_none);
      else
	build_all (&arm_sve_h_builder::sig_inc_dec_n, group, MODE_n);
      break;

    case SHAPE_inc_dec_pred:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_inc_dec_pred, group, MODE_none);
      break;

    case SHAPE_inc_dec_pred_n:
      add_overloaded_functions (group, MODE_n);
      build_all (&arm_sve_h_builder::sig_inc_dec_pred_n, group, MODE_n);
      break;

    case SHAPE_inherent:
      /* No overloaded functions here.  */
      build_all (&arm_sve_h_builder::sig_inherent<1>, group, MODE_none);
      break;

    case SHAPE_inherent2:
      build_all (&arm_sve_h_builder::sig_inherent<2>, group, MODE_none);
      break;

    case SHAPE_inherent3:
      build_all (&arm_sve_h_builder::sig_inherent<3>, group, MODE_none);
      break;

    case SHAPE_inherent4:
      build_all (&arm_sve_h_builder::sig_inherent<4>, group, MODE_none);
      break;

    case SHAPE_inherent_b:
      build_all (&arm_sve_h_builder::sig_inherent<1>, group, MODE_none, true);
      break;

    case SHAPE_inherent_count:
      build_all (&arm_sve_h_builder::sig_inherent_count, group, MODE_none);
      break;

    case SHAPE_load:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      /* Fall through.  */
    case SHAPE_load_ext:
      build_all (&arm_sve_h_builder::sig_load<1>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_load<1>, group, MODE_vnum);
      break;

    case SHAPE_load2:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_load<2>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_load<2>, group, MODE_vnum);
      break;

    case SHAPE_load3:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_load<3>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_load<3>, group, MODE_vnum);
      break;

    case SHAPE_load4:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_load<4>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_load<4>, group, MODE_vnum);
      break;

    case SHAPE_load_ext_gather_index:
      add_overloaded_functions (group, MODE_index);
      build_sv_index (&arm_sve_h_builder::sig_load_gather_sv, group);
      build_vs_index (&arm_sve_h_builder::sig_load_gather_vs, group);
      break;

    case SHAPE_load_ext_gather_offset:
      add_overloaded_functions (group, MODE_offset);
      build_sv_offset (&arm_sve_h_builder::sig_load_gather_sv, group);
      build_v_base (&arm_sve_h_builder::sig_load_gather_vs, group, true);
      build_vs_offset (&arm_sve_h_builder::sig_load_gather_vs, group);
      break;

    case SHAPE_load_gather_sv:
      add_overloaded_functions (group, MODE_index);
      add_overloaded_functions (group, MODE_offset);
      build_sv_index (&arm_sve_h_builder::sig_load_gather_sv, group);
      build_sv_offset (&arm_sve_h_builder::sig_load_gather_sv, group);
      break;

    case SHAPE_load_gather_vs:
      build_v_base (&arm_sve_h_builder::sig_load_gather_vs, group, true);
      build_vs_index (&arm_sve_h_builder::sig_load_gather_vs, group, true);
      build_vs_offset (&arm_sve_h_builder::sig_load_gather_vs, group, true);
      break;

    case SHAPE_load_scalar:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_load<1>, group, MODE_none);
      break;

    case SHAPE_pattern_count:
      build_all (&arm_sve_h_builder::sig_pattern_count, group, MODE_none);
      break;

    case SHAPE_pattern_pred:
      build_all (&arm_sve_h_builder::sig_pattern_pred, group, MODE_none);
      break;

    case SHAPE_pred_count:
      build_all (&arm_sve_h_builder::sig_pred_count, group, MODE_none);
      break;

    case SHAPE_prefetch:
      build_all (&arm_sve_h_builder::sig_prefetch, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_prefetch, group, MODE_vnum);
      break;

    case SHAPE_prefetch_gather_index:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_index);
      build_sv_index (&arm_sve_h_builder::sig_prefetch_gather_sv, group);
      build_v_base (&arm_sve_h_builder::sig_prefetch_gather_vs, group);
      build_vs_index (&arm_sve_h_builder::sig_prefetch_gather_vs, group);
      break;

    case SHAPE_prefetch_gather_offset:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_offset);
      build_sv_offset (&arm_sve_h_builder::sig_prefetch_gather_sv, group);
      build_v_base (&arm_sve_h_builder::sig_prefetch_gather_vs, group);
      build_vs_offset (&arm_sve_h_builder::sig_prefetch_gather_vs, group);
      break;

    case SHAPE_ptest:
      build_all (&arm_sve_h_builder::sig_ptest, group, MODE_none);
      break;

    case SHAPE_rdffr:
      build_all (&arm_sve_h_builder::sig_rdffr, group, MODE_none);
      break;

    case SHAPE_reduction:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_reduction, group, MODE_none);
      break;

    case SHAPE_reduction_wide:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_reduction_wide, group, MODE_none);
      break;

    case SHAPE_set2:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_set<2>, group, MODE_none);
      break;

    case SHAPE_set3:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_set<3>, group, MODE_none);
      break;

    case SHAPE_set4:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_set<4>, group, MODE_none);
      break;

    case SHAPE_setffr:
      build_all (&arm_sve_h_builder::sig_setffr, group, MODE_none);
      break;

    case SHAPE_shift_opt_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary<2>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_u64_n<2>, group, MODE_n);
      break;

    case SHAPE_shift_right_imm:
      add_overloaded_functions (group, MODE_n);
      build_all (&arm_sve_h_builder::sig_nary_u64_n<2>, group, MODE_n);
      break;

    case SHAPE_store:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_store<1>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_store<1>, group, MODE_vnum);
      break;

    case SHAPE_store2:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_store<2>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_store<2>, group, MODE_vnum);
      break;

    case SHAPE_store3:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_store<3>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_store<3>, group, MODE_vnum);
      break;

    case SHAPE_store4:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_vnum);
      build_all (&arm_sve_h_builder::sig_store<4>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_store<4>, group, MODE_vnum);
      break;

    case SHAPE_store_scatter_index:
      add_overloaded_functions (group, MODE_index);
      build_sv_index (&arm_sve_h_builder::sig_store_scatter_sv, group);
      build_vs_index (&arm_sve_h_builder::sig_store_scatter_vs, group);
      break;

    case SHAPE_store_scatter_offset:
      add_overloaded_functions (group, MODE_none);
      add_overloaded_functions (group, MODE_offset);
      build_sv_offset (&arm_sve_h_builder::sig_store_scatter_sv, group);
      build_v_base (&arm_sve_h_builder::sig_store_scatter_vs, group);
      build_vs_offset (&arm_sve_h_builder::sig_store_scatter_vs, group);
      break;

    case SHAPE_ternary_lane:
    case SHAPE_ternary_rotate:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_u64_n<4>, group, MODE_none);
      break;

    case SHAPE_ternary_lane_rotate:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_u64_nn<5>, group, MODE_none);
      break;

    case SHAPE_ternary_opt_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary<3>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_n<3>, group, MODE_n);
      break;

    case SHAPE_ternary_qq_lane:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_qq_u64_n<4>, group, MODE_none);
      break;

    case SHAPE_ternary_qq_opt_n:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_qq<3>, group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary_qq_n<3>, group, MODE_n);
      break;

    case SHAPE_unary:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_nary<1>, group, MODE_none);
      break;

    case SHAPE_unary_count:
      add_overloaded_functions (group, MODE_none);
      build_all (&arm_sve_h_builder::sig_count<1>, group, MODE_none);
      break;

    case SHAPE_unary_n:
      build_all (&arm_sve_h_builder::sig_nary_n<1>, group, MODE_n, true);
      break;

    case SHAPE_unary_pred:
      build_all (&arm_sve_h_builder::sig_nary<1>, group, MODE_none);
      break;
    }
}

/* Register the tuple type that contains NVECTORS vectors of type TYPE.  */
void
arm_sve_h_builder::register_tuple_type (unsigned int nvectors,
					vector_type type)
{
  tree tuple_type = lang_hooks.types.make_type (RECORD_TYPE);
  TYPE_SIZELESS_P (tuple_type) = 1;

  /* The contents of the type are opaque, so we can define them in any
     way that maps to the correct ABI type.

     Here we choose to use the same layout as for arm_neon.h, but with
     "__val" instead of "val":

	struct svfooxN_t { svfoo_t __val[N]; };

     (It wouldn't be possible to write that directly in C or C++ for
     sizeless types, but that's not a problem for this function.)

     Using arrays simplifies the handling of svget and svset for variable
     arguments.  */
  tree vector_type = acle_vector_types[0][type];
  tree array_type = build_array_type_nelts (vector_type, nvectors);
  gcc_assert (VECTOR_MODE_P (TYPE_MODE (array_type))
	      && TYPE_MODE_RAW (array_type) == TYPE_MODE (array_type)
	      && TYPE_ALIGN (array_type) == 128);
  tree field = build_decl (input_location, FIELD_DECL,
			   get_identifier ("__val"), array_type);
  DECL_FIELD_CONTEXT (field) = tuple_type;
  TYPE_FIELDS (tuple_type) = field;

  layout_type (tuple_type);
  gcc_assert (VECTOR_MODE_P (TYPE_MODE (tuple_type))
	      && TYPE_MODE_RAW (tuple_type) == TYPE_MODE (tuple_type)
	      && TYPE_ALIGN (tuple_type) == 128);

  /* Work out the structure name.  */
  char buffer[sizeof ("svfloat64x4_t")];
  const char *vector_type_name = vector_types[type].acle_name;
  snprintf (buffer, sizeof (buffer), "%.*sx%d_t",
	    (int) strlen (vector_type_name) - 2, vector_type_name, nvectors);

  tree decl = build_decl (input_location, TYPE_DECL,
			  get_identifier (buffer), tuple_type);
  TYPE_NAME (tuple_type) = decl;
  TYPE_STUB_DECL (tuple_type) = decl;
  lang_hooks.decls.pushdecl (decl);
  /* ??? Undo the effect of set_underlying_type for C.  The C frontend
     doesn't recognize DECL as a built-in because (as intended) the decl has
     a real location instead of BUILTINS_LOCATION.  The frontend therefore
     treats the decl like a normal C "typedef struct foo foo;", expecting
     the type for tag "struct foo" to have a dummy unnamed TYPE_DECL instead
     of the named one we attached above.  It then sets DECL_ORIGINAL_TYPE on
     the supposedly unnamed decl, creating a circularity that upsets
     dwarf2out.

     We don't want to follow the normal C model and create "struct foo"
     tags for tuple types since (a) the types are supposed to be opaque
     and (b) they couldn't be defined as a real struct anyway.  Treating
     the TYPE_DECLs as "typedef struct foo foo;" without creating
     "struct foo" would lead to confusing error messages.  */
  DECL_ORIGINAL_TYPE (decl) = NULL_TREE;

  acle_vector_types[nvectors - 1][type] = tuple_type;
}

/* For every type and predicate combination in GROUP, add one function
   that takes a scalar (pointer) base and a signed vector array index,
   and another that instead takes an unsigned vector array index.
   The vector array index has the same element size as the first
   function type suffix.  SIGNATURE is as for build_all.  */
void
arm_sve_h_builder::build_sv_index (function_signature signature,
				   const function_group &group)
{
  build_32_64 (signature, group, MODE_s32index, MODE_s64index);
  build_32_64 (signature, group, MODE_u32index, MODE_u64index);
}

/* Like build_sv_index, but taking vector byte offsets instead of vector
   array indices.  */
void
arm_sve_h_builder::build_sv_offset (function_signature signature,
				    const function_group &group)
{
  build_32_64 (signature, group, MODE_s32offset, MODE_s64offset);
  build_32_64 (signature, group, MODE_u32offset, MODE_u64offset);
}

/* For every type and predicate combination in GROUP, add a function
   that takes a vector base address and no displacement.  The vector
   base has the same element size as the first type suffix.
   The other arguments are as for build_all.  */
void
arm_sve_h_builder::build_v_base (function_signature signature,
				 const function_group &group,
				 bool force_direct_overloads)
{
  build_32_64 (signature, group, MODE_u32base, MODE_u64base,
	       force_direct_overloads);
}

/* Like build_v_base, but for functions that also take a scalar array
   index.  */
void
arm_sve_h_builder::build_vs_index (function_signature signature,
				   const function_group &group,
				   bool force_direct_overloads)
{
  build_32_64 (signature, group, MODE_u32base_index, MODE_u64base_index,
	       force_direct_overloads);
}

/* Like build_v_base, but for functions that also take a scalar byte
   offset.  */
void
arm_sve_h_builder::build_vs_offset (function_signature signature,
				    const function_group &group,
				    bool force_direct_overloads)
{
  build_32_64 (signature, group, MODE_u32base_offset, MODE_u64base_offset,
	       force_direct_overloads);
}

/* Add a function instance for every type and predicate combination
   in GROUP.  The first function type suffix specifies either a 32-bit
   or a 64-bit type; use MODE32 for the former and MODE64 for the latter.
   The other arguments are as for build_all.  */
void
arm_sve_h_builder::build_32_64 (function_signature signature,
				const function_group &group,
				function_mode mode32, function_mode mode64,
				bool force_direct_overloads)
{
  for (unsigned int pi = 0; group.preds[pi] != NUM_PREDS; ++pi)
    {
      if (group.types[0][0] == NUM_TYPE_SUFFIXES)
	{
	  build_one (signature, group, mode32, 0, pi, force_direct_overloads);
	  build_one (signature, group, mode64, 0, pi, force_direct_overloads);
	}
      for (unsigned int ti = 0; group.types[ti][0] != NUM_TYPE_SUFFIXES; ++ti)
	{
	  unsigned int bits = type_suffixes[group.types[ti][0]].elem_bits;
	  gcc_assert (bits == 32 || bits == 64);
	  function_mode mode = bits == 32 ? mode32 : mode64;
	  build_one (signature, group, mode, ti, pi, force_direct_overloads);
	}
    }
}

/* Add a function instance for every type and predicate combination
   in GROUP.  Take the function base name from GROUP and the mode
   from MODE.  Use SIGNATURE to construct the function signature without
   a governing predicate, then use apply_predication to add in the
   predicate.  FORCE_DIRECT_OVERLOADS is true if there is a one-to-one
   mapping between "short" and "full" names, and if standard overload
   resolution therefore isn't necessary.  */
void
arm_sve_h_builder::build_all (function_signature signature,
			      const function_group &group,
			      function_mode mode,
			      bool force_direct_overloads)
{
  for (unsigned int pi = 0; group.preds[pi] != NUM_PREDS; ++pi)
    for (unsigned int ti = 0;
	 ti == 0 || group.types[ti][0] != NUM_TYPE_SUFFIXES; ++ti)
      build_one (signature, group, mode, ti, pi, force_direct_overloads);
}

/* Add one function instance for GROUP, using mode MODE, the type
   suffixes at index TI and the predication suffix at index PI.
   The other arguments are as for build_all.  */
void
arm_sve_h_builder::build_one (function_signature signature,
			      const function_group &group,
			      function_mode mode,
			      unsigned int ti, unsigned int pi,
			      bool force_direct_overloads)
{
  /* Byte forms of svdupq take 16 arguments.  Add 1 for the return type.  */
  auto_vec<tree, 17> types;
  function_instance instance (group.func, group.shape, mode,
			      group.types[ti], group.preds[pi]);
  (this->*signature) (instance, types);
  apply_predication (instance, types);
  add_function_instance (instance, types, force_direct_overloads);
}

/* Describe one of the signatures:

     sv<m0>_t svfoo[_m0base]_[m1]index(sv<m0>_t, sv<m1>_t)
     sv<m0>_t svfoo[_m0base]_[m1]offset(sv<m0>_t, sv<m1>_t)

   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_adr (const function_instance &instance,
			    vec<tree> &types)
{
  types.quick_push (instance.base_vector_type ());
  types.quick_push (instance.base_vector_type ());
  types.quick_push (instance.displacement_vector_type ());
}

/* Describe the signature "svbool_t svfoo[_t0](sv<t0>_t, sv<t0>_t)"
   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_compare (const function_instance &instance,
				vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.vector_type (0));
}

/* Describe one of the signatures:

     svbool_t svfoo[_n_t0](sv<t0>_t, int64_t)  (for signed t0)
     svbool_t svfoo[_n_t0](sv<t0>_t, uint64_t)  (for unsigned t0)
     svbool_t svfoo[_n_t0](sv<t0>_t, <t0>_t)  (for floating-point t0)

   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_compare_n (const function_instance &instance,
				  vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.wide_scalar_type (0));
}

/* Describe the signature "svbool_t svfoo_t0[_t1](<t1>_t, <t1>_t)"
   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_compare_scalar (const function_instance &instance,
				       vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
  types.quick_push (instance.scalar_type (1));
  types.quick_push (instance.scalar_type (1));
}

/* Describe one of the signatures:

     svbool_t svfoo_wide[_t0](sv<t0>_t, svint64_t)  (for signed t0)
     svbool_t svfoo_wide[_t0](sv<t0>_t, svuint64_t)  (for unsigned t0)

   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_compare_wide (const function_instance &instance,
				     vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.wide_vector_type (0));
}

/* Describe the signature "sv<t0>xM_t svfoo[_t0](sv<t0>_t, ..., sv<t0>_t)"
   for INSTANCE in TYPES.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_create (const function_instance &instance,
			       vec<tree> &types)
{
  types.quick_push (instance.tuple_type (N, 0));
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
}

/* Describe the signature "sv<t0:uint>_t svfoo[_t0](sv<t0>_t, ..., sv<t0>_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_count (const function_instance &instance,
			      vec<tree> &types)
{
  types.quick_push (instance.unsigned_vector_type (0));
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
}

/* Describe the signature "sv<t0>_t svdupq_t0(<t0>_t, ..., <t0>_t)"
   for INSTANCE in TYPES, where there are enough arguments to fill
   128 bits of data (or to control 128 bits of data in the case of
   predicates).  */
void
arm_sve_h_builder::sig_dupq (const function_instance &instance,
			     vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  unsigned int nelts_per_vq = instance.nelts_per_vq (0);
  for (unsigned int i = 0; i < nelts_per_vq; ++i)
    types.quick_push (instance.scalar_type (0));
}

/* Describe the signature "<t0>_t svfoo[_t0](<t0>_t, sv<t0>_t)" for
   INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_fold_left (const function_instance &instance,
				  vec<tree> &types)
{
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.vector_type (0));
}

/* Describe the signature "sv<t0>_t svfoo[_t0](sv<t0>xN_t, uint64_t)"
   for INSTANCE in TYPES.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_get (const function_instance &instance,
			    vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.tuple_type (N, 0));
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe one of the signatures:

     sv<t0>_t svfoo[_t0](sv<t0>_t, uint64_t)
     sv<t0>_t svfoo[_t0](sv<t0>_t, enum svpattern, uint64_t)

   for INSTANCE in TYPES, with the shape deciding between them.  */
void
arm_sve_h_builder::sig_inc_dec (const function_instance &instance,
				vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.vector_type (0));
  if (instance.shape == SHAPE_inc_dec_pattern)
    types.quick_push (acle_svpattern);
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe one of the signatures:

     <t0>_t svfoo[_n_t0](<t0>_t, uint64_t)
     <t0>_t svfoo[_n_t0](<t0>_t, enum svpattern, uint64_t)

   for INSTANCE in TYPES, with the shape deciding between them.  */
void
arm_sve_h_builder::sig_inc_dec_n (const function_instance &instance,
				  vec<tree> &types)
{
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.scalar_type (0));
  if (instance.shape == SHAPE_inc_dec_pattern)
    types.quick_push (acle_svpattern);
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature "sv<t0>_t svfoo[_t0](sv<t0>_t, svbool_t)"
   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_inc_dec_pred (const function_instance &instance,
				     vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.vector_type (0));
  types.quick_push (get_svbool_t ());
}

/* Describe the signature "<t0>_t svfoo[_n_t0]_t1(<t0>_t, svbool_t)"
   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_inc_dec_pred_n (const function_instance &instance,
				       vec<tree> &types)
{
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.scalar_type (0));
  types.quick_push (get_svbool_t ());
}

/* Describe the signature "sv<t0>[xN]_t svfoo_t0()" for INSTANCE
   in TYPES.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_inherent (const function_instance &instance,
				 vec<tree> &types)
{
  types.quick_push (instance.tuple_type (N, 0));
}

/* Describe the signature "uint64_t svfoo()" in TYPES.  */
void
arm_sve_h_builder::sig_inherent_count (const function_instance &,
				       vec<tree> &types)
{
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe one of the signatures:

     sv<t0>xN_t svfoo[_t0](const <t0>_t *)
     sv<t0>xN_t svfoo_vnum[_t0](const <t0>_t *, int64_t)
     sv<t0>_t svfoo_t0(const <X>_t *)
     sv<t0>_t svfoo_vnum_t0(const <X>_t *, int64_t)

   for INSTANCE in TYPES.  The mode of INSTANCE chooses between the vnum and
   non-vnum forms.  The choice of <X> comes from the function base name.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_load (const function_instance &instance,
			     vec<tree> &types)
{
  types.quick_push (instance.tuple_type (N, 0));
  types.quick_push (build_const_pointer (instance.memory_scalar_type ()));
  if (instance.mode == MODE_vnum)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
}

/* Describe one of the signatures:

     sv<t0>_t svfoo_[m1]index[_t0](const <t0>_t *, sv<m1>_t)
     sv<t0>_t svfoo_[m1]offset[_t0](const <t0>_t *, sv<m1>_t)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m1>.  */
void
arm_sve_h_builder::sig_load_gather_sv (const function_instance &instance,
				       vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  types.quick_push (build_const_pointer (instance.memory_scalar_type ()));
  types.quick_push (instance.displacement_vector_type ());
}

/* Describe one of the signatures:

     sv<t0>_t svfoo[_m0base]_t0(sv<m0>_t)
     sv<t0>_t svfoo[_m0base]_index_t0(sv<m0>_t, int64_t)
     sv<t0>_t svfoo[_m0base]_offset_t0(sv<m0>_t, int64_t)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m0>.  */
void
arm_sve_h_builder::sig_load_gather_vs (const function_instance &instance,
				       vec<tree> &types)
{
  types.quick_push (instance.vector_type (0));
  types.quick_push (instance.base_vector_type ());
  if (instance.displacement_units () != UNITS_none)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
}

/* Describe one of the signatures:

     void svfoo(const void *, svprfop)
     void svfoo_vnum(const void *, int64_t, svprfop)

   for INSTANCE in TYPES.  The mode of INSTANCE chooses between the vnum and
   non-vnum forms.  */
void
arm_sve_h_builder::sig_prefetch (const function_instance &instance,
				 vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (const_ptr_type_node);
  if (instance.mode == MODE_vnum)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
  types.quick_push (acle_svprfop);
}

/* Describe one of the signatures:

     void svfoo_[m1]index(const void *, sv<m1>_t, svprfop)
     void svfoo_[m1]offset(const void *, sv<m1>_t, svprfop)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m1>.  */
void
arm_sve_h_builder::sig_prefetch_gather_sv (const function_instance &instance,
					   vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (const_ptr_type_node);
  types.quick_push (instance.displacement_vector_type ());
  types.quick_push (acle_svprfop);
}

/* Describe one of the signatures:

     void svfoo[_m0base](sv<m0>_t, svprfop)
     void svfoo[_m0base]_index(sv<m0>_t, int64_t, svprfop)
     void svfoo[_m0base]_offset(sv<m0>_t, int64_t, svprfop)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m0>.  */
void
arm_sve_h_builder::sig_prefetch_gather_vs (const function_instance &instance,
					   vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (instance.base_vector_type ());
  if (instance.displacement_units () != UNITS_none)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
  types.quick_push (acle_svprfop);
}

/* Describe the signature "sv<t0>_t svfoo[_t0](sv<t0>_t, ...)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary (const function_instance &instance,
			     vec<tree> &types)
{
  for (unsigned int i = 0; i < N + 1; ++i)
    types.quick_push (instance.vector_type (0));
}

/* Describe the signature "sv<t0>_t svfoo(sv<t0>_t, ..., sv<t0:uint>_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_index (const function_instance &instance,
				   vec<tree> &types)
{
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.unsigned_vector_type (0));
}

/* Describe the signature "sv<t0>_t svfoo(sv<t0>_t, ..., <t0:uint>_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_index_n (const function_instance &instance,
				     vec<tree> &types)
{
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.unsigned_scalar_type (0));
}

/* Describe the signature "sv<t0>_t svfoo(sv<t0>_t, ..., uint64_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_index64_n (const function_instance &instance,
				       vec<tree> &types)
{
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature "sv<t0>_t svfoo[_n_t0](sv<t0>_t, ..., <t0>_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_n (const function_instance &instance,
			       vec<tree> &types)
{
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.scalar_type (0));
}

/* Describe the signature:

     sv<t0>_t svfoo[_n_t0](sv<t0>_t, ..., <t0>_t, <t0>_t)

   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_nn (const function_instance &instance,
				vec<tree> &types)
{
  for (unsigned int i = 0; i < N - 1; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.scalar_type (0));
}

/* Describe the signature:

     sv<t0>_t svfoo[_t0](sv<t0>_t, ..., sv<t0.quarter>_t, sv<t0.quarter>_t)

   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_qq (const function_instance &instance,
				vec<tree> &types)
{
  tree quarter_type = instance.quarter_vector_type (0);
  for (unsigned int i = 0; i < N - 1; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (quarter_type);
  types.quick_push (quarter_type);
}

/* Describe the signature:

     sv<t0>_t svfoo[_n_t0](sv<t0>_t, ..., sv<t0.quarter>_t, <t0.quarter>_t,
			   uint64_t)"

   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_qq_u64_n (const function_instance &instance,
				      vec<tree> &types)
{
  for (unsigned int i = 0; i < N - 2; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.quarter_vector_type (0));
  types.quick_push (instance.quarter_vector_type (0));
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature:

     sv<t0>_t svfoo[_n_t0](sv<t0>_t, ..., sv<t0.quarter>_t, <t0.quarter>_t)"

   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_qq_n (const function_instance &instance,
				  vec<tree> &types)
{
  for (unsigned int i = 0; i < N - 1; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (instance.quarter_vector_type (0));
  types.quick_push (instance.quarter_scalar_type (0));
}

/* Describe the signature "sv<t0>_t svfoo[_t0](sv<t0>_t, ..., svuint64_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_u64 (const function_instance& instance,
				 vec<tree> &types)
{
  for (unsigned i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (acle_vector_types[0][VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature "sv<t0>_t svfoo[_t0](sv<t0>_t, ..., uint64_t)"
   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_u64_n (const function_instance &instance,
				   vec<tree> &types)
{
  for (unsigned int i = 0; i < N; ++i)
    types.quick_push (instance.vector_type (0));
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature:

     sv<t0>_t svfoo[_t0](sv<t0>_t, ..., uint64_t, uint64_t)

   for INSTANCE in TYPES, where N is the number of arguments.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_nary_u64_nn (const function_instance &instance,
				    vec<tree> &types)
{
  for (unsigned int i = 0; i < N - 1; ++i)
    types.quick_push (instance.vector_type (0));
  for (unsigned int i = 0; i < 2; ++i)
    types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
}

/* Describe the signature "uint64_t svfoo(enum svpattern)" in TYPES.  */
void
arm_sve_h_builder::sig_pattern_count (const function_instance &,
				      vec<tree> &types)
{
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
  types.quick_push (acle_svpattern);
}

/* Describe the signature "svbool_t svfoo(enum svpattern)" in TYPES.  */
void
arm_sve_h_builder::sig_pattern_pred (const function_instance &,
				     vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
  types.quick_push (acle_svpattern);
}

/* Describe the signature "uint64_t svfoo_t(svbool_t)" in TYPES.  */
void
arm_sve_h_builder::sig_pred_count (const function_instance &, vec<tree> &types)
{
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
  types.quick_push (get_svbool_t ());
}

/* Describe the signature "bool svfoo(svbool_t)" in TYPES.  */
void
arm_sve_h_builder::sig_ptest (const function_instance &, vec<tree> &types)
{
  types.quick_push (boolean_type_node);
  types.quick_push (get_svbool_t ());
}

/* Describe the signature "svbool_t svfoo()" in TYPES.  */
void
arm_sve_h_builder::sig_rdffr (const function_instance &, vec<tree> &types)
{
  types.quick_push (get_svbool_t ());
}

/* Describe the signature "<t0>_t svfoo[_t0](sv<t0>_t)" in TYPES.  */
void
arm_sve_h_builder::sig_reduction (const function_instance &instance,
				  vec<tree> &types)
{
  types.quick_push (instance.scalar_type (0));
  types.quick_push (instance.vector_type (0));
}

/* Describe one of the signatures:

     int64_t svfoo[_t0](sv<t0>_t)  (for signed t0)
     uint64_t svfoo[_t0](sv<t0>_t)  (for unsigned t0)
     <t0>_t svfoo[_t0](sv<t0>_t)  (for floating-point t0)

   for INSTANCE in TYPES.  */
void
arm_sve_h_builder::sig_reduction_wide (const function_instance &instance,
				       vec<tree> &types)
{
  types.quick_push (instance.wide_scalar_type (0));
  types.quick_push (instance.vector_type (0));
}

/* Describe the signature
   "sv<t0>xN_t svfoo[_t0](sv<t0>xN_t, uint64_t, sv<t0>_t)"
   for INSTANCE in TYPES.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_set (const function_instance &instance,
			    vec<tree> &types)
{
  types.quick_push (instance.tuple_type (N, 0));
  types.quick_push (instance.tuple_type (N, 0));
  types.quick_push (scalar_types[VECTOR_TYPE_svuint64_t]);
  types.quick_push (instance.vector_type (0));
}

/* Describe the signature "void svfoo()" in TYPES.  */
void
arm_sve_h_builder::sig_setffr (const function_instance &, vec<tree> &types)
{
  types.quick_push (void_type_node);
}

/* Describe one of the signatures:

     void svfoo[_t0](<t0>_t *, sv<t0>xN_t)
     void svfoo_vnum[_t0](<t0>_t *, int64_t, sv<t0>xN_t)

   for INSTANCE in TYPES.  The mode of INSTANCE chooses between the vnum and
   non-vnum forms.  */
template<unsigned int N>
void
arm_sve_h_builder::sig_store (const function_instance &instance,
			      vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (build_pointer_type (instance.memory_scalar_type ()));
  if (instance.mode == MODE_vnum)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
  types.quick_push (instance.tuple_type (N, 0));
}

/* Describe one of the signatures:

     void svfoo_[m1]index[_t0](<t0>_t *, sv<m1>_t, sv<t0>_t)
     void svfoo_[m1]offset[_t0](<t0>_t *, sv<m1>_t, sv<t0>_t)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m1>.  */
void
arm_sve_h_builder::sig_store_scatter_sv (const function_instance &instance,
					 vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (build_pointer_type (instance.memory_scalar_type ()));
  types.quick_push (instance.displacement_vector_type ());
  types.quick_push (instance.vector_type (0));
}

/* Describe one of the signatures:

     void svfoo[_m0base_t0](sv<m0>_t, sv<t0>_t)
     void svfoo[_m0base]_index[_t0](sv<m0>_t, int64_t, sv<t0>_t)
     void svfoo[_m0base]_offset[_t0](sv<m0>_t, int64_t, sv<t0>_t)

   for INSTANCE in TYPES.  The mode of INSTANCE determines <m0>.  */
void
arm_sve_h_builder::sig_store_scatter_vs (const function_instance &instance,
					 vec<tree> &types)
{
  types.quick_push (void_type_node);
  types.quick_push (instance.base_vector_type ());
  if (instance.displacement_units () != UNITS_none)
    types.quick_push (scalar_types[VECTOR_TYPE_svint64_t]);
  types.quick_push (instance.vector_type (0));
}

/* Return a representation of "const T *".  */
tree
arm_sve_h_builder::build_const_pointer (tree t)
{
  return build_pointer_type (build_qualified_type (t, TYPE_QUAL_CONST));
}

/* If INSTANCE has a governing predicate, add it to the type signature
   in TYPES, where TYPES[0] is the return type, TYPES[1] is the first
   argument type, etc.  */
void
arm_sve_h_builder::apply_predication (const function_instance &instance,
				      vec<tree> &types)
{
  if (instance.pred != PRED_none)
    {
      types.quick_insert (1, get_svbool_t ());
      /* For unary merge operations, the first argument is a vector with
	 the same type as the result.  */
      if (types.length () == 3 && instance.pred == PRED_m)
	types.quick_insert (1, types[0]);
    }
}

/* Add a built-in function for INSTANCE, with the function signature
   given by TYPES.  TYPES[0] is the return type, TYPES[1] is the first
   argument type, etc.  FORCE_DIRECT_OVERLOADS is true if there is a
   one-to-one mapping between "short" and "full" names, and if standard
   overload resolution therefore isn't necessary.  */
void
arm_sve_h_builder::add_function_instance (const function_instance &instance,
					  vec<tree> &types,
					  bool force_direct_overloads)
{
  /* Add the function under its full (unique) name.  */
  char *name = get_name (instance, false);
  tree fntype = build_function_type_array (types[0], types.length () - 1,
					   types.address () + 1);
  tree attrs = get_attributes (instance);
  registered_function &rfn = add_function (instance, name, fntype,
					   attrs, false);

  /* Enter the function into the hash table.  */
  hashval_t hash = instance.hash ();
  registered_function **rfn_slot
    = function_table->find_slot_with_hash (instance, hash, INSERT);
  gcc_assert (!*rfn_slot);
  *rfn_slot = &rfn;

  /* Also add the function under its overloaded alias, if we want
     a separate decl for each instance of an overloaded function.  */
  if (m_direct_overloads || force_direct_overloads)
    {
      char *overload_name = get_name (instance, true);
      if (strcmp (name, overload_name) != 0)
	add_function (instance, overload_name, fntype, attrs, true);
    }

  obstack_free (&m_string_obstack, name);
}

/* If we are using manual overload resolution, add one function decl
   for each overloaded function in GROUP.  Take the function base name
   from GROUP and the mode from MODE.  */
void
arm_sve_h_builder::add_overloaded_functions (const function_group &group,
					     function_mode mode)
{
  if (m_direct_overloads)
    return;

  unsigned int explicit_types = get_explicit_types (group.shape);
  for (unsigned int pi = 0; group.preds[pi] != NUM_PREDS; ++pi)
    {
      type_suffix_pair prev_types = { NUM_TYPE_SUFFIXES, NUM_TYPE_SUFFIXES };
      if (explicit_types == 0)
	{
	  /* One overloaded function for all type combinations.  */
	  function_instance instance (group.func, group.shape, mode,
				      prev_types, group.preds[pi]);
	  add_overloaded_function (instance);
	}
      else
	for (unsigned int ti = 0; group.types[ti][0] != NUM_TYPE_SUFFIXES;
	     ++ti)
	  {
	    /* Stub out the types that are determined by overload resolution.
	       The list of type suffixes is lexicographically ordered,
	       so we can check for duplicates by comparing this pair
	       against the previous one.  */
	    type_suffix_pair types = {
	      explicit_types & 1 ? group.types[ti][0] : NUM_TYPE_SUFFIXES,
	      explicit_types & 2 ? group.types[ti][1] : NUM_TYPE_SUFFIXES
	    };
	    if (ti > 0
		&& types[0] == prev_types[0]
		&& types[1] == prev_types[1])
	      continue;
	    function_instance instance (group.func, group.shape, mode, types,
					group.preds[pi]);
	    add_overloaded_function (instance);
	    memcpy (prev_types, types, sizeof (types));
	  }
    }
}

/* Add one function decl for INSTANCE, to be used with manual overloading.  */
void
arm_sve_h_builder::add_overloaded_function (const function_instance &instance)
{
  char *name = get_name (instance, true);
  add_function (instance, name, m_overload_type, NULL_TREE, true);
  obstack_free (&m_string_obstack, name);
}

/* Add a function NAME with type FNTYPE and attributes ATTRS.  INSTANCE
   describes what the function does and OVERLOADED_P indicates whether
   it is overloaded.  */
registered_function &
arm_sve_h_builder::add_function (const function_instance &instance,
				 const char *name, tree fntype, tree attrs,
				 bool overloaded_p)
{
  unsigned int code = vec_safe_length (registered_functions);
  code = (code << AARCH64_BUILTIN_SHIFT) | AARCH64_BUILTIN_SVE;
  tree decl = add_builtin_function_global_md (input_location, name, fntype,
					      code, NULL, attrs);

  registered_function &rfn = *ggc_alloc <registered_function> ();
  rfn.instance = instance;
  rfn.overloaded_p = overloaded_p;
  rfn.decl = decl;
  vec_safe_push (registered_functions, &rfn);

  return rfn;
}

/* Add attribute NAME to ATTRS.  */
static tree
add_attribute (const char *name, tree attrs)
{
  return tree_cons (get_identifier (name), NULL_TREE, attrs);
}

/* Return the appropriate function attributes for INSTANCE.  */
tree
arm_sve_h_builder::get_attributes (const function_instance &instance)
{
  tree attrs = NULL_TREE;
  switch (instance.func)
    {
    case FUNC_svabd:
    case FUNC_svabs:
    case FUNC_svacge:
    case FUNC_svacgt:
    case FUNC_svacle:
    case FUNC_svaclt:
    case FUNC_svadd:
    case FUNC_svadda:
    case FUNC_svaddv:
    case FUNC_svand:
    case FUNC_svandv:
    case FUNC_svasr:
    case FUNC_svasr_wide:
    case FUNC_svasrd:
    case FUNC_svbic:
    case FUNC_svcadd:
    case FUNC_svcmla:
    case FUNC_svcmla_lane:
    case FUNC_svcmpeq:
    case FUNC_svcmpeq_wide:
    case FUNC_svcmpge:
    case FUNC_svcmpge_wide:
    case FUNC_svcmpgt:
    case FUNC_svcmpgt_wide:
    case FUNC_svcmple:
    case FUNC_svcmple_wide:
    case FUNC_svcmplt:
    case FUNC_svcmplt_wide:
    case FUNC_svcmpne:
    case FUNC_svcmpne_wide:
    case FUNC_svcmpuo:
    case FUNC_svdiv:
    case FUNC_svdivr:
    case FUNC_svdot:
    case FUNC_svdot_lane:
    case FUNC_sveor:
    case FUNC_sveorv:
    case FUNC_svindex:
    case FUNC_svlsl:
    case FUNC_svlsl_wide:
    case FUNC_svlsr:
    case FUNC_svlsr_wide:
    case FUNC_svmad:
    case FUNC_svmax:
    case FUNC_svmaxnm:
    case FUNC_svmaxnmv:
    case FUNC_svmaxv:
    case FUNC_svmin:
    case FUNC_svminnm:
    case FUNC_svminnmv:
    case FUNC_svminv:
    case FUNC_svmla:
    case FUNC_svmla_lane:
    case FUNC_svmls:
    case FUNC_svmls_lane:
    case FUNC_svmsb:
    case FUNC_svmul:
    case FUNC_svmul_lane:
    case FUNC_svmulh:
    case FUNC_svmulx:
    case FUNC_svnand:
    case FUNC_svneg:
    case FUNC_svnmad:
    case FUNC_svnmla:
    case FUNC_svnmls:
    case FUNC_svnmsb:
    case FUNC_svnor:
    case FUNC_svnot:
    case FUNC_svorn:
    case FUNC_svorr:
    case FUNC_svorv:
    case FUNC_svqadd:
    case FUNC_svqdecb:
    case FUNC_svqdecb_pat:
    case FUNC_svqdecd:
    case FUNC_svqdecd_pat:
    case FUNC_svqdech:
    case FUNC_svqdech_pat:
    case FUNC_svqdecp:
    case FUNC_svqdecw:
    case FUNC_svqdecw_pat:
    case FUNC_svqincb:
    case FUNC_svqincb_pat:
    case FUNC_svqincd:
    case FUNC_svqincd_pat:
    case FUNC_svqinch:
    case FUNC_svqinch_pat:
    case FUNC_svqincp:
    case FUNC_svqincw:
    case FUNC_svqincw_pat:
    case FUNC_svqsub:
    case FUNC_svsqrt:
    case FUNC_svsub:
    case FUNC_svsubr:
      if (type_suffixes[instance.types[0]].integer_p)
	{
	  attrs = add_attribute ("const", attrs);
	  attrs = add_attribute ("nothrow", attrs);
	}
      else if (flag_non_call_exceptions && flag_trapping_math)
	/* Exceptions have to be preserved exactly.  */;
      else
	{
	  /* Only pure, not const, because it depends on the rounding mode.  */
	  attrs = add_attribute ("pure", attrs);
	  attrs = add_attribute ("nothrow", attrs);
	}
      break;

    case FUNC_svadrb:
    case FUNC_svadrd:
    case FUNC_svadrh:
    case FUNC_svadrw:
    case FUNC_svbrka:
    case FUNC_svbrkb:
    case FUNC_svbrkn:
    case FUNC_svbrkpa:
    case FUNC_svbrkpb:
    case FUNC_svcls:
    case FUNC_svclz:
    case FUNC_svcnt:
    case FUNC_svcntb:
    case FUNC_svcntb_pat:
    case FUNC_svcntd:
    case FUNC_svcntd_pat:
    case FUNC_svcnth:
    case FUNC_svcnth_pat:
    case FUNC_svcntp:
    case FUNC_svcntw:
    case FUNC_svcntw_pat:
    case FUNC_svcompact:
    case FUNC_svcreate2:
    case FUNC_svcreate3:
    case FUNC_svcreate4:
    case FUNC_svdup:
    case FUNC_svdup_lane:
    case FUNC_svdupq:
    case FUNC_svdupq_lane:
    case FUNC_svext:
    case FUNC_svextb:
    case FUNC_svexth:
    case FUNC_svextw:
    case FUNC_svget2:
    case FUNC_svget3:
    case FUNC_svget4:
    case FUNC_svinsr:
    case FUNC_svpfalse:
    case FUNC_svptest_any:
    case FUNC_svptest_first:
    case FUNC_svptest_last:
    case FUNC_svptrue:
    case FUNC_svptrue_pat:
    case FUNC_svrbit:
    case FUNC_svrev:
    case FUNC_svsel:
    case FUNC_svset2:
    case FUNC_svset3:
    case FUNC_svset4:
    case FUNC_svsplice:
    case FUNC_svtbl:
    case FUNC_svtrn1:
    case FUNC_svtrn2:
    case FUNC_svundef:
    case FUNC_svundef2:
    case FUNC_svundef3:
    case FUNC_svundef4:
    case FUNC_svuzp1:
    case FUNC_svuzp2:
    case FUNC_svwhilele:
    case FUNC_svwhilelt:
    case FUNC_svzip1:
    case FUNC_svzip2:
      attrs = add_attribute ("const", attrs);
      attrs = add_attribute ("nothrow", attrs);
      break;

    case FUNC_svld1:
    case FUNC_svld1_gather:
    case FUNC_svld1rq:
    case FUNC_svld1sb:
    case FUNC_svld1sb_gather:
    case FUNC_svld1sh:
    case FUNC_svld1sh_gather:
    case FUNC_svld1sw:
    case FUNC_svld1sw_gather:
    case FUNC_svld1ub:
    case FUNC_svld1ub_gather:
    case FUNC_svld1uh:
    case FUNC_svld1uh_gather:
    case FUNC_svld1uw:
    case FUNC_svld1uw_gather:
    case FUNC_svld2:
    case FUNC_svld3:
    case FUNC_svld4:
    case FUNC_svldnt1:
      attrs = add_attribute ("pure", attrs);
      if (!flag_non_call_exceptions)
	attrs = add_attribute ("nothrow", attrs);
      break;

    case FUNC_svldff1:
    case FUNC_svldff1_gather:
    case FUNC_svldff1sb:
    case FUNC_svldff1sb_gather:
    case FUNC_svldff1sh:
    case FUNC_svldff1sh_gather:
    case FUNC_svldff1sw:
    case FUNC_svldff1sw_gather:
    case FUNC_svldff1ub:
    case FUNC_svldff1ub_gather:
    case FUNC_svldff1uh:
    case FUNC_svldff1uh_gather:
    case FUNC_svldff1uw:
    case FUNC_svldff1uw_gather:
    case FUNC_svldnf1:
    case FUNC_svldnf1sb:
    case FUNC_svldnf1sh:
    case FUNC_svldnf1sw:
    case FUNC_svldnf1ub:
    case FUNC_svldnf1uh:
    case FUNC_svldnf1uw:
    case FUNC_svprfb:
    case FUNC_svprfb_gather:
    case FUNC_svprfd:
    case FUNC_svprfd_gather:
    case FUNC_svprfh:
    case FUNC_svprfh_gather:
    case FUNC_svprfw:
    case FUNC_svprfw_gather:
    case FUNC_svrdffr:
    case FUNC_svsetffr:
    case FUNC_svst1:
    case FUNC_svst1_scatter:
    case FUNC_svst1b:
    case FUNC_svst1b_scatter:
    case FUNC_svst1h:
    case FUNC_svst1h_scatter:
    case FUNC_svst1w:
    case FUNC_svst1w_scatter:
    case FUNC_svst2:
    case FUNC_svst3:
    case FUNC_svst4:
    case FUNC_svstnt1:
    case FUNC_svwrffr:
      attrs = add_attribute ("nothrow", attrs);
      break;
    }
  return add_attribute ("leaf", attrs);
}

/* Return the bitmask of types that are always explicit in functions
   with shape SHAPE.  Bit 0 is set if the first type is always explicit
   and bit 1 is set if the second type is always explicit.  */
unsigned int
arm_sve_h_builder::get_explicit_types (function_shape shape)
{
  switch (shape)
    {
    case SHAPE_adr_index:
    case SHAPE_adr_offset:
    case SHAPE_binary:
    case SHAPE_binary_extract:
    case SHAPE_binary_index:
    case SHAPE_binary_index_n:
    case SHAPE_binary_index64_n:
    case SHAPE_binary_lane:
    case SHAPE_binary_n:
    case SHAPE_binary_opt_n:
    case SHAPE_binary_rotate:
    case SHAPE_binary_wide:
    case SHAPE_compare_opt_n:
    case SHAPE_compare_wide:
    case SHAPE_create2:
    case SHAPE_create3:
    case SHAPE_create4:
    case SHAPE_fold_left:
    case SHAPE_get2:
    case SHAPE_get3:
    case SHAPE_get4:
    case SHAPE_inc_dec:
    case SHAPE_inc_dec_pattern:
    case SHAPE_inc_dec_pred:
    case SHAPE_inherent_b:
    case SHAPE_inherent_count:
    case SHAPE_load:
    case SHAPE_load2:
    case SHAPE_load3:
    case SHAPE_load4:
    case SHAPE_load_gather_sv:
    case SHAPE_load_scalar:
    case SHAPE_pattern_count:
    case SHAPE_prefetch:
    case SHAPE_prefetch_gather_index:
    case SHAPE_prefetch_gather_offset:
    case SHAPE_ptest:
    case SHAPE_rdffr:
    case SHAPE_reduction:
    case SHAPE_reduction_wide:
    case SHAPE_set2:
    case SHAPE_set3:
    case SHAPE_set4:
    case SHAPE_setffr:
    case SHAPE_shift_opt_n:
    case SHAPE_shift_right_imm:
    case SHAPE_store:
    case SHAPE_store2:
    case SHAPE_store3:
    case SHAPE_store4:
    case SHAPE_store_scatter_index:
    case SHAPE_store_scatter_offset:
    case SHAPE_ternary_lane:
    case SHAPE_ternary_lane_rotate:
    case SHAPE_ternary_opt_n:
    case SHAPE_ternary_qq_lane:
    case SHAPE_ternary_qq_opt_n:
    case SHAPE_ternary_rotate:
    case SHAPE_unary:
    case SHAPE_unary_count:
      return 0;
    case SHAPE_binary_pred:
    case SHAPE_binary_scalar:
    case SHAPE_compare_scalar:
    case SHAPE_dupq:
    case SHAPE_inherent:
    case SHAPE_inherent2:
    case SHAPE_inherent3:
    case SHAPE_inherent4:
    case SHAPE_load_ext:
    case SHAPE_load_ext_gather_index:
    case SHAPE_load_ext_gather_offset:
    case SHAPE_load_gather_vs:
    case SHAPE_pattern_pred:
    case SHAPE_pred_count:
    case SHAPE_unary_n:
    case SHAPE_unary_pred:
      return 1;
    case SHAPE_inc_dec_pred_n:
      return 2;
    }
  gcc_unreachable ();
}

/* Return the overloaded or full function name for INSTANCE; OVERLOADED_P
   selects which.  */
char *
arm_sve_h_builder::get_name (const function_instance &instance,
			     bool overloaded_p)
{
  unsigned int explicit_types
    = (overloaded_p ? get_explicit_types (instance.shape) : 3);

  append_name (base_names[instance.func]);
  if (overloaded_p)
    switch (instance.displacement_units ())
      {
      case UNITS_none:
	break;

      case UNITS_bytes:
	append_name ("_offset");
	break;

      case UNITS_elements:
	append_name ("_index");
	break;

      case UNITS_vectors:
	append_name ("_vnum");
	break;
      }
  else
    append_name (modes[instance.mode].suffix);
  if (explicit_types & 1)
    append_name (type_suffixes[instance.types[0]].suffix);
  if (explicit_types & 2)
    append_name (type_suffixes[instance.types[1]].suffix);
  append_name (pred_suffixes[instance.pred]);
  return finish_name ();
}

/* Add NAME to the end of the function name being built.  */
void
arm_sve_h_builder::append_name (const char *name)
{
  obstack_grow (&m_string_obstack, name, strlen (name));
}

/* Zero-terminate and complete the function name being built.  */
char *
arm_sve_h_builder::finish_name ()
{
  obstack_1grow (&m_string_obstack, 0);
  return (char *) obstack_finish (&m_string_obstack);
}

function_resolver::function_resolver (location_t location,
				      const registered_function &rfn,
				      vec<tree, va_gc> &arglist)
  : m_location (location), m_rfn (rfn), m_fi (rfn.instance),
    m_arglist (arglist)
{
}

/* Try to resolve the overloaded call.  Return:

   - the non-overloaded function decl on success
   - error_mark_node on failure or
   - NULL_TREE if there is a one-to-one mapping between "overloaded"
     (short) and non-overloaded (long) function names, so that no
     resolution is necessary.  */
tree
function_resolver::resolve ()
{
  switch (m_fi.shape)
    {
    case SHAPE_adr_index:
    case SHAPE_adr_offset:
      return resolve_adr ();
    case SHAPE_binary:
    case SHAPE_binary_opt_n:
    case SHAPE_compare_opt_n:
    case SHAPE_shift_opt_n:
      return resolve_uniform (2);
    case SHAPE_binary_extract:
    case SHAPE_binary_lane:
    case SHAPE_binary_rotate:
      return resolve_uniform_imm (3, 1);
    case SHAPE_binary_index:
    case SHAPE_binary_index_n:
    case SHAPE_binary_index64_n:
    case SHAPE_binary_n:
      return resolve_multitype (2, 1);
    case SHAPE_binary_pred:
    case SHAPE_binary_scalar:
    case SHAPE_load_ext:
    case SHAPE_inherent:
    case SHAPE_inherent2:
    case SHAPE_inherent3:
    case SHAPE_inherent4:
    case SHAPE_inherent_count:
    case SHAPE_pattern_count:
    case SHAPE_pattern_pred:
    case SHAPE_pred_count:
    case SHAPE_prefetch:
    case SHAPE_ptest:
    case SHAPE_rdffr:
    case SHAPE_setffr:
    case SHAPE_unary_pred:
      break;
    case SHAPE_binary_wide:
      return resolve_binary_wide ();
    case SHAPE_compare_scalar:
      return resolve_compare_scalar ();
    case SHAPE_compare_wide:
      return resolve_compare_wide ();
    case SHAPE_create2:
      return resolve_create (2);
    case SHAPE_create3:
      return resolve_create (3);
    case SHAPE_create4:
      return resolve_create (4);
    case SHAPE_dupq:
    case SHAPE_unary_n:
      /* All overloading does here is make the "_n" implicit.  */
      return NULL_TREE;
    case SHAPE_fold_left:
      return resolve_fold_left ();
    case SHAPE_get2:
      return resolve_get (2);
    case SHAPE_get3:
      return resolve_get (3);
    case SHAPE_get4:
      return resolve_get (4);
    case SHAPE_inc_dec:
      return resolve_inc_dec (2);
    case SHAPE_inc_dec_pattern:
      return resolve_inc_dec (3);
    case SHAPE_inc_dec_pred:
      return resolve_inc_dec_pred ();
    case SHAPE_inc_dec_pred_n:
      return resolve_inc_dec_pred_n ();
    case SHAPE_inherent_b:
      /* All overloading does here is make the "_b" implicit.  */
      return NULL_TREE;
    case SHAPE_load:
    case SHAPE_load2:
    case SHAPE_load3:
    case SHAPE_load4:
    case SHAPE_load_scalar:
      return resolve_pointer ();
    case SHAPE_load_ext_gather_index:
    case SHAPE_load_ext_gather_offset:
      return resolve_load_gather_sv_or_vs ();
    case SHAPE_load_gather_sv:
      return resolve_load_gather_sv ();
    case SHAPE_load_gather_vs:
      /* The type of the vector base is uniquely determined by the explicit
	 type suffix.  There is no ambiguity with SHAPE_load_gather_sv
	 because the latter uses an implicit type suffix.  */
      return NULL_TREE;
    case SHAPE_prefetch_gather_index:
    case SHAPE_prefetch_gather_offset:
      return resolve_prefetch_gather ();
    case SHAPE_set2:
      return resolve_set (2);
    case SHAPE_set3:
      return resolve_set (3);
    case SHAPE_set4:
      return resolve_set (4);
    case SHAPE_shift_right_imm:
      return resolve_uniform_imm (2, 1);
    case SHAPE_store:
      return resolve_store (1);
    case SHAPE_store2:
      return resolve_store (2);
    case SHAPE_store3:
      return resolve_store (3);
    case SHAPE_store4:
      return resolve_store (4);
    case SHAPE_store_scatter_index:
    case SHAPE_store_scatter_offset:
      return resolve_store_scatter ();
    case SHAPE_ternary_lane:
    case SHAPE_ternary_rotate:
      return resolve_uniform_imm (4, 1);
    case SHAPE_ternary_lane_rotate:
      return resolve_uniform_imm (5, 2);
    case SHAPE_ternary_opt_n:
      return resolve_uniform (3);
    case SHAPE_ternary_qq_lane:
      return resolve_multitype (4, 1);
    case SHAPE_ternary_qq_opt_n:
      return resolve_multitype (3, 1);
    case SHAPE_unary:
    case SHAPE_unary_count:
    case SHAPE_reduction:
    case SHAPE_reduction_wide:
      return resolve_uniform (1);
    }
  gcc_unreachable ();
}

/* Resolve a call based purely on a pointer argument.  The other arguments
   are a governing predicate and (for MODE_vnum) a vnum offset.  */
tree
function_resolver::resolve_pointer ()
{
  vector_type type;

  gcc_assert (m_fi.pred != PRED_none
	      && (m_fi.mode == MODE_none || m_fi.mode == MODE_vnum));
  if (!check_num_arguments (m_fi.mode == MODE_vnum ? 3 : 2)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (type = require_pointer_type (1)) == NUM_VECTOR_TYPES
      || (m_fi.mode == MODE_vnum && !require_scalar_argument (2, "int64_t")))
    return error_mark_node;

  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Resolve a function in which all vector and scalar types in the signature
   have the same element type.  NOPS is the number of vector and scalar
   arguments before predication is applied to the signature (1 for unary
   ops, 2 for binary, etc.).  */
tree
function_resolver::resolve_uniform (unsigned int nops)
{
  /* Check that we have the right number of arguments.  */
  unsigned int i, nargs;
  vector_type type;
  if (!check_first_vector_argument (nops, i, nargs, type))
    return error_mark_node;

  /* Handle subsequent arguments.  */
  for (; i < nargs; ++i)
    {
      /* Allow the final argument to be scalar, if an _n form exists.  */
      if (i == nargs - 1 && scalar_argument_p (i))
	return require_n_form (get_type_suffix (type));
      if (!require_matching_type (i, type))
	return error_mark_node;
    }
  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_adr_{index,offset}.  */
tree
function_resolver::resolve_adr ()
{
  vector_type type1, type2;
  if (!check_num_arguments (2)
      || (type1 = require_vector_base (0)) == NUM_VECTOR_TYPES
      || (type2 = require_integer_vector_type (1)) == NUM_VECTOR_TYPES)
    return error_mark_node;

  function_mode mode = find_function_mode (type1, type2,
					   m_fi.displacement_units ());
  if (mode == MODE_none)
    {
      if (m_fi.mode == MODE_offset)
	error_at (m_location, "cannot combine a base of type %qT with"
		  " an offset of type %qT",
		  get_argument_type (0), get_argument_type (1));
      else
	error_at (m_location, "cannot combine a base of type %qT with"
		  " an index of type %qT",
		  get_argument_type (0), get_argument_type (1));
      return error_mark_node;
    }

  return require_form (mode);
}

/* Resolve a function that has SHAPE_create<NUM_VECTORS>.  */
tree
function_resolver::resolve_create (unsigned int num_vectors)
{
  vector_type type;
  if (!check_num_arguments (num_vectors)
      || (type = require_vector_type (0)) == NUM_VECTOR_TYPES)
    return error_mark_node;

  for (unsigned int i = 1; i < num_vectors; ++i)
    if (!require_matching_type (i, type))
      return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_fold_left.  */
tree
function_resolver::resolve_fold_left ()
{
  vector_type type;
  if (!check_num_arguments (3)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      /* Leave the frontend to check argument 1.  */
      || (type = require_vector_type (2)) == NUM_VECTOR_TYPES)
    return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_inc_dec or SHAPE_inc_dec_pattern.
   NARGS is the expected number of arguments.  */
tree
function_resolver::resolve_inc_dec (unsigned int nargs)
{
  if (!check_num_arguments (nargs))
    return error_mark_node;

  /* Resolve based on the first argument only, which must be either a scalar
     or a vector.  If it's a scalar, it must be a 32-bit or 64-bit integer.  */
  if (scalar_argument_p (0))
    {
      vector_type type = require_integer_scalar_type (0);
      if (type == NUM_VECTOR_TYPES)
	return error_mark_node;
      return require_form (MODE_n, get_type_suffix (type));
    }

  vector_type type = require_vector_type (0);
  if (type == NUM_VECTOR_TYPES)
    return error_mark_node;
  return require_form (MODE_none, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_inc_dec_pred.  */
tree
function_resolver::resolve_inc_dec_pred ()
{
  vector_type type;
  if (!check_num_arguments (2)
      || (type = require_vector_type (0)) == NUM_VECTOR_TYPES
      || !check_argument (1, VECTOR_TYPE_svbool_t))
    return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_inc_dec_pred_n.  */
tree
function_resolver::resolve_inc_dec_pred_n ()
{
  vector_type type;
  if (!check_num_arguments (2)
      || (type = require_integer_scalar_type (0)) == NUM_VECTOR_TYPES
      || !check_argument (1, VECTOR_TYPE_svbool_t))
    return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type),
		       m_fi.types[1]);
}

/* Resolve a gather load that takes a pointer base and a vector displacement.
   The type of the pointer target determines the type of the loaded data.  */
tree
function_resolver::resolve_load_gather_sv ()
{
  function_mode mode;
  vector_type type;
  if (!check_num_arguments (3)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (type = require_pointer_type (1, true)) == NUM_VECTOR_TYPES
      || (mode = require_vector_displacement (2, type, true)) == MODE_none)
    return error_mark_node;

  return require_form (mode, get_type_suffix (type));
}

/* Resolve a gather load that takes one of:

   - a scalar pointer base and a vector displacement
   - a vector base with no displacement or
   - a vector base and a scalar displacement

   The function has an explicit type suffix that determines the type
   of the loaded data.  */
tree
function_resolver::resolve_load_gather_sv_or_vs ()
{
  /* No resolution is needed for a vector base with no displacement;
     there's a one-to-one mapping between short and long names.  */
  if (m_fi.displacement_units () == UNITS_none)
    return NULL_TREE;

  function_mode mode;
  vector_type type = type_suffixes[m_fi.types[0]].type;
  if (!check_num_arguments (3)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (mode = require_gather_address (1, type, true)) == MODE_none)
    return error_mark_node;

  return require_form (mode, get_type_suffix (type));
}

/* Resolve a function that takes NOPS arguments plus any governing
   predicate, consisting of NFIRST arguments of one type followed by
   arguments of other types.  The resolution can be done using only the
   first NFIRST arguments, so it is possible to test the rest against
   the resolved function's signature.  */
tree
function_resolver::resolve_multitype (unsigned int nops, unsigned int nfirst)
{
  /* Check that we have the right number of arguments.  */
  unsigned int i, nargs;
  vector_type type;
  if (!check_first_vector_argument (nops, i, nargs, type))
    return error_mark_node;

  /* Handle subsequent arguments.  */
  for (unsigned int j = 1; j < nfirst; ++j)
    if (!require_matching_type (i++, type))
      return error_mark_node;

  /* Check for a suitable function before checking the final argument,
     so that we don't try to enforce non-existent combinations below.  */
  tree res = require_form (m_fi.mode, get_type_suffix (type));
  if (res == error_mark_node)
    return error_mark_node;

  /* Check that the other arguments have the right type.  */
  unsigned int unchecked = i;
  function_args_iterator iter;
  tree expected;
  i = 0;
  FOREACH_FUNCTION_ARGS (TREE_TYPE (res), expected, iter)
    {
      if (expected == void_type_node)
	{
	  gcc_assert (i == nargs);
	  break;
	}
      /* Leave the frontend to check scalar types, since they
	 don't participate in resolution.  */
      if (i >= unchecked && VECTOR_TYPE_P (expected))
	{
	  if (i + 1 == nargs && scalar_argument_p (i))
	    return require_n_form (get_type_suffix (type));

	  if (!check_argument_against_decl (res, i, expected))
	    return error_mark_node;
	}
      i += 1;
    }

  return res;
}

/* Resolve a function that has SHAPE_get<NUM_VECTORS>.  */
tree
function_resolver::resolve_get (unsigned int num_vectors)
{
  vector_type type;
  if (!check_num_arguments (2)
      || (type = require_tuple_type (0, num_vectors)) == NUM_VECTOR_TYPES
      || !require_integer_immediate (1))
    return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type));
}

/* Resolve a gather prefetch that takes one of:

   - a scalar pointer base (const void *) and a vector displacement
   - a vector base with no displacement or
   - a vector base and a scalar displacement

   The prefetch operation is the final argument.  This is purely a
   mode-based resolution; there are no type suffixes.  */
tree
function_resolver::resolve_prefetch_gather ()
{
  function_mode mode;
  vector_type type = NUM_VECTOR_TYPES;
  unsigned int nargs = m_fi.displacement_units () == UNITS_none ? 3 : 4;
  if (!check_num_arguments (nargs)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (mode = require_gather_address (1, type, false)) == MODE_none)
    return error_mark_node;

  /* Don't check the prefetch operation here.  */
  return require_form (mode);
}

/* Resolve a function that has SHAPE_set<NUM_VECTORS>.  */
tree
function_resolver::resolve_set (unsigned int num_vectors)
{
  vector_type type;
  if (!check_num_arguments (3)
      || (type = require_tuple_type (0, num_vectors)) == NUM_VECTOR_TYPES
      || !require_integer_immediate (1)
      || !check_argument (2, type))
    return error_mark_node;

  return require_form (m_rfn.instance.mode, get_type_suffix (type));
}

/* Resolve a call based on the final vector argument, which is a tuple
   of NUM_VECTORS vectors.  The other arguments are a governing predicate,
   a pointer and (for MODE_vnum) a vnum offset.  */
tree
function_resolver::resolve_store (unsigned int num_vectors)
{
  vector_type type;

  gcc_assert (m_fi.pred != PRED_none
	      && (m_fi.mode == MODE_none || m_fi.mode == MODE_vnum));
  unsigned int nargs = (m_fi.mode == MODE_vnum ? 4 : 3);
  if (!check_num_arguments (nargs)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      /* Don't check pointer argument 1, since it doesn't participate
	 in resolution.  Leave that to the frontend.  */
      || (m_fi.mode == MODE_vnum && !require_scalar_argument (2, "int64_t"))
      || ((type = require_tuple_type (nargs - 1, num_vectors))
	  == NUM_VECTOR_TYPES))
    return error_mark_node;

  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Resolve a scatter store that takes one of:

   - a scalar pointer base and a vector displacement
   - a vector base with no displacement or
   - a vector base and a scalar displacement

   The stored data is the final argument, and it determines the
   type suffix.  */
tree
function_resolver::resolve_store_scatter ()
{
  function_mode mode;
  vector_type type;
  unsigned int nargs = m_fi.displacement_units () == UNITS_none ? 3 : 4;
  if (!check_num_arguments (nargs)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (type = require_sd_vector_type (nargs - 1)) == NUM_VECTOR_TYPES
      || (mode = require_gather_address (1, type, false)) == MODE_none)
    return error_mark_node;

  return require_form (mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_binary_wide.  */
tree
function_resolver::resolve_binary_wide ()
{
  unsigned i, nargs;
  vector_type type;

  if (!check_first_vector_argument (2, i, nargs, type)
      || !check_argument (i, VECTOR_TYPE_svuint64_t))
    return error_mark_node;

  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Resolve a function that has SHAPE_compare_scalar.  */
tree
function_resolver::resolve_compare_scalar ()
{
  vector_type type1, type2;
  if (!check_num_arguments (2)
      || (type1 = require_integer_scalar_type (0)) == NUM_VECTOR_TYPES
      || (type2 = require_integer_scalar_type (1)) == NUM_VECTOR_TYPES
      || !require_matching_scalar_types (0, 1, type1, type2))
    return error_mark_node;

  return require_form (m_fi.mode, m_fi.types[0], get_type_suffix (type1));
}

/* Resolve a function that has SHAPE_compare_wide.  */
tree
function_resolver::resolve_compare_wide ()
{
  vector_type type;
  if (!check_num_arguments (3)
      || !check_argument (0, VECTOR_TYPE_svbool_t)
      || (type = require_integer_vector_type (1)) == NUM_VECTOR_TYPES
      || !check_argument (2, wide_type_for (type)))
    return error_mark_node;

  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Like resolve_uniform, except that the final NIMM arguments have
   type uint64_t and must be integer constant expressions.  */
tree
function_resolver::resolve_uniform_imm (unsigned int nops, unsigned int nimm)
{
  /* Check that we have the right number of arguments.  Also check the
     first vector argument and governing predicate.  */
  unsigned int i, nargs;
  vector_type type;
  if (!check_first_vector_argument (nops, i, nargs, type))
    return error_mark_node;

  /* Handle subsequent vector arguments.  */
  for (; i < nargs - nimm; ++i)
    if (!require_matching_type (i, type))
      return error_mark_node;

  /* Handle the final immediate arguments.  */
  for (; i < nargs; ++i)
    if (!require_integer_immediate (i))
      return error_mark_node;

  return require_form (m_fi.mode, get_type_suffix (type));
}

/* Check that the function is passed NOPS arguments plus the governing
   predicate (if applicable) and that the first argument besides the
   governing predicate is a vector.  Return true if so, otherwise
   report a suitable error.

   When returning true:
   - set I to the number of the first unchecked argument (past the first
     vector and past any governing predicate).
   - set NARGS to the number of arguments including any governing
     predicate.
   - set TYPE to the type of the vector argument.  */
bool
function_resolver::check_first_vector_argument (unsigned int nops,
						unsigned int &i,
						unsigned int &nargs,
						vector_type &type)
{
  i = 0;
  nargs = nops;
  type = NUM_VECTOR_TYPES;

  /* For unary merge operations, the first argument is a vector with
     the same type as the result.  */
  if (nops == 1 && m_fi.pred == PRED_m)
    nargs += 1;
  if (m_fi.pred != PRED_none)
    nargs += 1;
  if (!check_num_arguments (nargs))
    return false;

  /* For unary merge operations, the first argument is a vector with
     the same type as the result.  We'll check that it has the right
     type after the function has been resolved.  */
  if (nops == 1 && m_fi.pred == PRED_m)
    {
      if (require_vector_type (i) == NUM_VECTOR_TYPES)
	return false;
      i += 1;
    }

  /* Check the predicate argument.  */
  if (m_fi.pred != PRED_none)
    {
      if (!check_argument (i, VECTOR_TYPE_svbool_t))
	return false;
      i += 1;
    }

  /* The next argument is always a vector.  */
  type = require_vector_type (i);
  if (type == NUM_VECTOR_TYPES)
    return false;
  i += 1;

  return true;
}

/* Require the function to have exactly EXPECTED arguments.  Return true
   if it does.  */
bool
function_resolver::check_num_arguments (unsigned int expected)
{
  if (m_arglist.length () < expected)
    error_at (m_location, "too few arguments to function %qE", m_rfn.decl);
  else if (m_arglist.length () > expected)
    error_at (m_location, "too many arguments to function %qE", m_rfn.decl);
  return m_arglist.length () == expected;
}

/* Require argument I to be a pointer to a scalar type that has a corresponding
   vector type.  Return that vector type on success and NUM_VECTOR_TYPES
   on failure.  GATHER_SCATTER_P is true if the function is a gather/scatter
   operation, and so requires a pointer to 32-bit or 64-bit data.  */
vector_type
function_resolver::require_pointer_type (unsigned int i, bool gather_scatter_p)
{
  tree actual = get_argument_type (i);
  if (actual == error_mark_node)
    return NUM_VECTOR_TYPES;
  if (TREE_CODE (actual) != POINTER_TYPE)
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects a pointer type", actual, i + 1, m_rfn.decl);
      if (VECTOR_TYPE_P (actual) && gather_scatter_p)
	inform (m_location, "an explicit type suffix is needed"
		" when using a vector of base addresses");
      return NUM_VECTOR_TYPES;
    }
  tree target = TREE_TYPE (actual);
  vector_type type = find_vector_type_from_scalar_type (target);
  if (type == NUM_VECTOR_TYPES)
    {
      error_at (m_location, "passing %qT to argument %d of %qE, but %qT"
		" is not a valid SVE element type", actual, i + 1,
		m_rfn.decl, target);
      return NUM_VECTOR_TYPES;
    }
  unsigned int bits = get_element_bits (type);
  if (gather_scatter_p && bits != 32 && bits != 64)
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects a pointer to 32-bit or 64-bit elements",
		actual, i + 1, m_rfn.decl);
      return NUM_VECTOR_TYPES;
    }
  return type;
}

/* Require argument I to be a tuple of NUM_VECTORS vectors, with
   NUM_VECTORS == 1 selecting a single vector.  Return the type of
   the vector on success and NUM_VECTOR_TYPES on failure.  */
vector_type
function_resolver::require_tuple_type (unsigned int i,
				       unsigned int num_vectors)
{
  tree actual = get_argument_type (i);
  if (actual == error_mark_node)
    return NUM_VECTOR_TYPES;
  /* A linear search should be OK here, since the code isn't hot and
     the number of types is only small.  */
  for (unsigned int size_i = 0; size_i < MAX_TUPLE_SIZE; ++size_i)
    for (unsigned int type_i = 0; type_i < NUM_VECTOR_TYPES; ++type_i)
      if (tree type = acle_vector_types[size_i][type_i])
	if (TYPE_MAIN_VARIANT (actual) == TYPE_MAIN_VARIANT (type))
	  {
	    if (size_i + 1 == num_vectors)
	      return (vector_type) type_i;
	    if (num_vectors == 1)
	      error_at (m_location, "passing %qT to argument %d of %qE, which"
			" expects a single SVE vector rather than a tuple",
			actual, i + 1, m_rfn.decl);
	    else if (size_i == 0 && type_i != VECTOR_TYPE_svbool_t)
	      error_at (m_location, "passing single vector %qT to argument %d"
			" of %qE, which expects a tuple of %d vectors",
			actual, i + 1, m_rfn.decl, num_vectors);
	    else
	      error_at (m_location, "passing %qT to argument %d of %qE, which"
			" expects a tuple of %d vectors", actual, i + 1,
			m_rfn.decl, num_vectors);
	    return NUM_VECTOR_TYPES;
	  }
  if (num_vectors == 1)
    error_at (m_location, "passing %qT to argument %d of %qE, which"
	      " expects an SVE vector type", actual, i + 1, m_rfn.decl);
  else
    error_at (m_location, "passing %qT to argument %d of %qE, which"
	      " expects an SVE tuple type", actual, i + 1, m_rfn.decl);
  return NUM_VECTOR_TYPES;
}

/* Require argument I to have some form of vector type.  Return the
   type of the vector on success and NUM_VECTOR_TYPES on failure.  */
vector_type
function_resolver::require_vector_type (unsigned int i)
{
  return require_tuple_type (i, 1);
}

/* Likewise, but also require the type to be integral.  */
vector_type
function_resolver::require_integer_vector_type (unsigned int i)
{
  vector_type type = require_tuple_type (i, 1);
  if (type == NUM_VECTOR_TYPES)
    return type;

  if (!integral_type_p (type))
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects a vector of integers", get_argument_type (i),
		i + 1, m_rfn.decl);
      return NUM_VECTOR_TYPES;
    }

  return type;
}

/* Likewise, but also require the element size to be 32 or 64 bits.  */
vector_type
function_resolver::require_sd_vector_type (unsigned int i)
{
  vector_type type = require_tuple_type (i, 1);
  if (type == NUM_VECTOR_TYPES)
    return type;

  unsigned int bits = get_element_bits (type);
  if (bits != 32 && bits != 64)
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects a vector of 32-bit or 64-bit elements",
		get_argument_type (i), i + 1, m_rfn.decl);
      return NUM_VECTOR_TYPES;
    }

  return type;
}

/* Require argument I to match EXPECTED, which is the type of argument I
   of DECL.  Return true if the argument has the right form.  */
bool
function_resolver::check_argument_against_decl (tree decl, unsigned int i,
						tree expected)
{
  gcc_assert (VECTOR_TYPE_P (expected));
  tree actual = get_argument_type (i);
  if (actual != error_mark_node
      && TYPE_MAIN_VARIANT (expected) != TYPE_MAIN_VARIANT (actual))
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects %qT", actual, i + 1, decl, expected);
      return false;
    }
  return true;
}

/* Require argument I to have the vector type associated with TYPE,
   in cases where this requirement holds for all uses of the function.
   Return true if the argument has the right form.  */
bool
function_resolver::check_argument (unsigned int i, vector_type type)
{
  return check_argument_against_decl (m_rfn.decl, i, get_vector_type (type));
}

/* Likewise, but TYPE is determined by matching previous arguments
   rather than being a fixed part of the function signature.  This changes
   the nature of the error messages.  */
bool
function_resolver::require_matching_type (unsigned int i, vector_type type)
{
  vector_type new_type = require_vector_type (i);
  if (new_type == NUM_VECTOR_TYPES)
    return false;

  if (type != new_type)
    {
      error_at (m_location, "passing %qT to argument %d of %qE, but"
		" previous arguments had type %qT",
		get_vector_type (new_type), i + 1, m_rfn.decl,
		get_vector_type (type));
      return false;
    }
  return true;
}

/* Return true if argument I is a scalar integer or floating-point value.  */
bool
function_resolver::scalar_argument_p (unsigned int i)
{
  tree type = get_argument_type (i);
  return (INTEGRAL_TYPE_P (type)
	  /* Allow pointer types, leaving the frontend to warn where
	     necessary.  */
	  || POINTER_TYPE_P (type)
	  || SCALAR_FLOAT_TYPE_P (type));
}

/* Require argument I to be a scalar, using EXPECTED as the name of its
   expected type.  Return true if the argument has the right form.  */
bool
function_resolver::require_scalar_argument (unsigned int i,
					    const char *expected)
{
  if (!scalar_argument_p (i))
    {
      error_at (m_location, "passing %qT to argument %d of %qE, which"
		" expects %qs", get_argument_type (i), i + 1, m_rfn.decl,
		expected);
      return false;
    }
  return true;
}

/* Check that argument I has a suitable form for an integer constant
   expression.  function_checker checks whether the argument is
   actually constant and has a suitable range.  */
bool
function_resolver::require_integer_immediate (unsigned int i)
{
  if (!scalar_argument_p (i))
    {
      report_non_ice (m_location, m_rfn.decl, i);
      return false;
    }
  return true;
}

/* Check that argument I has a 32-bit or 64-bit scalar integer type.
   Return the associated vector type on success, otherwise return
   NUM_VECTOR_TYPES.  */
vector_type
function_resolver::require_integer_scalar_type (unsigned int i)
{
  tree actual = get_argument_type (i);
  if (actual == error_mark_node)
    return NUM_VECTOR_TYPES;

  /* Allow enums and booleans to decay to integers, for compatibility
     with C++ overloading rules.  */
  if (INTEGRAL_TYPE_P (actual))
    {
      bool uns_p = TYPE_UNSIGNED (actual);
      /* Honor the usual integer promotions, so that resolution works
	 in the same way as for C++.  */
      if (TYPE_PRECISION (actual) < 32)
	return VECTOR_TYPE_svint32_t;
      if (TYPE_PRECISION (actual) == 32)
	return uns_p ? VECTOR_TYPE_svuint32_t : VECTOR_TYPE_svint32_t;
      if (TYPE_PRECISION (actual) == 64)
	return uns_p ? VECTOR_TYPE_svuint64_t : VECTOR_TYPE_svint64_t;
    }

  error_at (m_location, "passing %qT to argument %d of %qE, which"
	    " expects a 32-bit or 64-bit integer type", actual, i + 1,
	    m_rfn.decl);
  return NUM_VECTOR_TYPES;
}

/* Argument I1 is a scalar argument that matches the element type of
   TYPE1 and argument I2 is a scalar argument that matches the element
   type of TYPE2.  Return true if they are compatible and report an
   error if not.  */
bool
function_resolver::require_matching_scalar_types (unsigned int i1,
						  unsigned int i2,
						  vector_type type1,
						  vector_type type2)
{
  if (type1 == type2)
    return true;

  error_at (m_location, "call to %qE is ambiguous; argument %d has type"
	    " %qs but argument %d has type %qs", m_rfn.decl,
	    i1 + 1, get_scalar_type_name (type1),
	    i2 + 1, get_scalar_type_name (type2));
  return false;
}

/* Require argument I to be a vector base in a gather-style address.
   Return its type on success, otherwise return NUM_VECTOR_TYPES.  */
vector_type
function_resolver::require_vector_base (unsigned int i)
{
  vector_type type = require_vector_type (i);
  if (type == NUM_VECTOR_TYPES
      || type == VECTOR_TYPE_svuint32_t
      || type == VECTOR_TYPE_svuint64_t)
    return type;

  error_at (m_location, "passing %qT to argument %d of %qE, which"
	    " expects %qs or %qs", get_argument_type (i), i + 1, m_rfn.decl,
	    "svuint32_t", "svuint64_t");
  return NUM_VECTOR_TYPES;
}

/* Require argument I to be a vector offset or index in a gather-style
   address.  There are three possible uses:

   - for loading DATA_TYPE (when LOAD_P is true)
   - for storing DATA_TYPE (when LOAD_P is false)
   - for prefetching data (when DATA_TYPE is NUM_VECTOR_TYPES)

   Return the associated mode on success, otherwise return MODE_none.  */
function_mode
function_resolver::require_vector_displacement (unsigned int i,
						vector_type data_type,
						bool load_p)
{
  vector_type displacement_type = require_vector_type (i);
  if (displacement_type == NUM_VECTOR_TYPES)
    return MODE_none;

  if (data_type == NUM_VECTOR_TYPES)
    {
      function_mode mode = find_function_mode (NUM_VECTOR_TYPES,
					       displacement_type,
					       m_fi.displacement_units ());
      if (mode == MODE_none)
	error_at (m_location, "passing %qT to argument %d of %qE,"
		  " which expects a vector of 32-bit of 64-bit integers",
		  get_argument_type (i), i + 1, m_rfn.decl);
      return mode;
    }

  unsigned int required_bits = get_element_bits (data_type);
  if (get_element_bits (displacement_type) == required_bits)
    {
      function_mode mode = find_function_mode (NUM_VECTOR_TYPES,
					       displacement_type,
					       m_fi.displacement_units ());
      if (mode != MODE_none)
	return mode;
    }

  if (m_fi.types[0] == NUM_TYPE_SUFFIXES)
    {
      if (load_p)
	error_at (m_location, "passing %qT to argument %d of %qE, which when"
		  " loading %qT expects a vector of %d-bit integers",
		  get_argument_type (i), i + 1, m_rfn.decl,
		  get_vector_type (data_type), required_bits);
      else
	error_at (m_location, "passing %qT to argument %d of %qE, which when"
		  " storing %qT expects a vector of %d-bit integers",
		  get_argument_type (i), i + 1, m_rfn.decl,
		  get_vector_type (data_type), required_bits);
    }
  else
    error_at (m_location, "passing %qT to argument %d of %qE, which"
	      " expects a vector of %d-bit integers",
	      get_argument_type (i), i + 1, m_rfn.decl, required_bits);
  return MODE_none;
}

/* Require arguments I and I + 1 to form a gather-style address.
   There are three possible uses:

   - for loading DATA_TYPE (when LOAD_P is true)
   - for storing DATA_TYPE (when LOAD_P is false)
   - for prefetching data (when DATA_TYPE is NUM_VECTOR_TYPES)

   The two possible addresses are a vector base and a scalar
   displacement or a scalar (pointer) base and a vector displacement.
   The mode of the overloaded function determines the units of the
   displacement (bytes for "_offset", elements for "_index").

   Return the mode of the non-overloaded function on success, otherwise
   return MODE_none.  */
function_mode
function_resolver::require_gather_address (unsigned int i,
					   vector_type data_type,
					   bool load_p)
{
  tree actual = get_argument_type (i);
  if (actual == error_mark_node)
    return MODE_none;

  if (m_fi.displacement_units () != UNITS_none)
    {
      if (POINTER_TYPE_P (actual))
	/* Don't check the pointer type here, since there's only one valid
	   choice.  Leave that to the frontend.  */
	return require_vector_displacement (i + 1, data_type, load_p);

      if (!VECTOR_TYPE_P (actual))
	{
	  error_at (m_location, "passing %qT to argument %d of %qE,"
		    " which expects a vector or pointer base address",
		    actual, i + 1, m_rfn.decl);
	  return MODE_none;
	}
    }

  /* Check for the correct choice of vector base type.  */
  vector_type base_type;
  if (data_type == NUM_VECTOR_TYPES)
    {
      base_type = require_vector_base (i);
      if (base_type == NUM_VECTOR_TYPES)
	return MODE_none;
    }
  else
    {
      base_type = require_vector_type (i);
      if (base_type == NUM_VECTOR_TYPES)
	return MODE_none;

      unsigned int required_bits = get_element_bits (data_type);
      gcc_assert (required_bits == 32 || required_bits == 64);
      vector_type required_type = (required_bits == 32
				   ? VECTOR_TYPE_svuint32_t
				   : VECTOR_TYPE_svuint64_t);
      if (required_type != base_type)
	{
	  error_at (m_location, "passing %qT to argument %d of %qE,"
		    " which expects %qT", actual, i + 1, m_rfn.decl,
		    get_vector_type (required_type));
	  return MODE_none;
	}
    }

  for (unsigned int mode = 0; mode < ARRAY_SIZE (modes); ++mode)
    if (modes[mode].base_vector_type == base_type
	&& modes[mode].displacement_vector_type == NUM_VECTOR_TYPES
	&& modes[mode].displacement_units == m_fi.displacement_units ())
      return function_mode (mode);

  gcc_unreachable ();
}

/* Require there to be an _n instance of the function with the type suffixes
   given by TYPE0 and TYPE1.  Return its function decl on success and
   error_mark_node on failure.  */
tree
function_resolver::require_n_form (type_suffix type0, type_suffix type1)
{
  tree res = lookup_form (MODE_n, type0, type1);
  if (res)
    return res;

  res = require_form (MODE_none, type0, type1);
  if (res)
    error_at (m_location, "%qE has no %qs form", res, "_n");
  return error_mark_node;
}

/* Require there to be an instance of the function with the mode given
   by MODE and with the type suffixes given by TYPE0 and TYPE1.  Return
   its function decl on success and error_mark_node on failure.  */
tree
function_resolver::require_form (function_mode mode, type_suffix type0,
				 type_suffix type1)
{
  tree res = lookup_form (mode, type0, type1);
  if (!res)
    {
      if (type1 == NUM_TYPE_SUFFIXES)
	error_at (m_location, "%qE has no form that takes %qT arguments",
		  m_rfn.decl, get_vector_type (type0));
      else
	/* To be filled in when we have other cases.  */
	gcc_unreachable ();
      return error_mark_node;
    }
  return res;
}

/* See whether there is an instance of the function with the mode given
   by MODE and the type suffixes given by TYPE0 and TYPE1.  Return its
   function decl on success, otherwise return null.  */
tree
function_resolver::lookup_form (function_mode mode, type_suffix type0,
				type_suffix type1)
{
  type_suffix_pair types = { type0, type1 };
  function_instance instance (m_fi.func, m_fi.shape, mode, types, m_fi.pred);
  registered_function *rfn
    = function_table->find_with_hash (instance, instance.hash ());
  return rfn ? rfn->decl : NULL_TREE;
}

/* Return the vector type associated with TYPE.  */
tree
function_resolver::get_vector_type (vector_type type)
{
  return acle_vector_types[0][type];
}

/* Return the vector type associated with TYPE.  */
tree
function_resolver::get_vector_type (type_suffix type)
{
  return get_vector_type (type_suffixes[type].type);
}

/* Return the number of bits in each element of TYPE.  */
unsigned int
function_resolver::get_element_bits (vector_type type)
{
  return tree_to_uhwi (TYPE_SIZE (TREE_TYPE (get_vector_type (type))));
}

/* Return the <stdint.h> name for TYPE's element type.  Using the <stdint.h>
   name should be more user-friendly than the underlying canonical type,
   since it makes the signedness and bitwidth explicit.  */
const char *
function_resolver::get_scalar_type_name (vector_type type)
{
  return vector_types[type].acle_name + 2;
}

/* Return the type of argument I, or error_mark_node if it isn't
   well-formed.  */
tree
function_resolver::get_argument_type (unsigned int i)
{
  tree arg = m_arglist[i];
  return arg == error_mark_node ? arg : TREE_TYPE (arg);
}

/* Return the type suffix associated with vector type TYPE, using _b
   in the case of svbool_t.  */
type_suffix
function_resolver::get_type_suffix (vector_type type)
{
  for (unsigned int i = 0; i < NUM_TYPE_SUFFIXES; ++i)
    if (type_suffixes[i].type == type)
      return type_suffix (i);
  gcc_unreachable ();
}

function_checker::function_checker (location_t location,
				    const registered_function &rfn, tree decl,
				    unsigned int nargs, tree *args)
  : m_location (location), m_fi (rfn.instance),
    m_fntype (TREE_TYPE (rfn.decl)), m_decl (decl),
    m_nargs (nargs), m_args (args)
{
}

/* Perform semantic checks on the call.  Return true if the call is valid,
   otherwise report a suitable error.  */
bool
function_checker::check ()
{
  if (!TARGET_SVE)
    {
      report_missing_feature (m_location, m_decl, "+sve");
      return false;
    }

  unsigned int pg_args = (m_fi.pred != PRED_none);
  switch (m_fi.shape)
    {
    case SHAPE_adr_index:
    case SHAPE_adr_offset:
    case SHAPE_binary:
    case SHAPE_binary_index:
    case SHAPE_binary_index_n:
    case SHAPE_binary_index64_n:
    case SHAPE_binary_n:
    case SHAPE_binary_opt_n:
    case SHAPE_binary_pred:
    case SHAPE_binary_scalar:
    case SHAPE_binary_wide:
    case SHAPE_compare_opt_n:
    case SHAPE_compare_scalar:
    case SHAPE_compare_wide:
    case SHAPE_create2:
    case SHAPE_create3:
    case SHAPE_create4:
    case SHAPE_dupq:
    case SHAPE_fold_left:
    case SHAPE_inc_dec_pred:
    case SHAPE_inc_dec_pred_n:
    case SHAPE_inherent:
    case SHAPE_inherent2:
    case SHAPE_inherent3:
    case SHAPE_inherent4:
    case SHAPE_inherent_b:
    case SHAPE_inherent_count:
    case SHAPE_load:
    case SHAPE_load2:
    case SHAPE_load3:
    case SHAPE_load4:
    case SHAPE_load_ext:
    case SHAPE_load_ext_gather_index:
    case SHAPE_load_ext_gather_offset:
    case SHAPE_load_gather_sv:
    case SHAPE_load_gather_vs:
    case SHAPE_load_scalar:
    case SHAPE_pred_count:
    case SHAPE_ptest:
    case SHAPE_rdffr:
    case SHAPE_reduction:
    case SHAPE_reduction_wide:
    case SHAPE_setffr:
    case SHAPE_shift_opt_n:
    case SHAPE_store:
    case SHAPE_store2:
    case SHAPE_store3:
    case SHAPE_store4:
    case SHAPE_store_scatter_index:
    case SHAPE_store_scatter_offset:
    case SHAPE_ternary_opt_n:
    case SHAPE_ternary_qq_opt_n:
    case SHAPE_unary:
    case SHAPE_unary_count:
    case SHAPE_unary_n:
    case SHAPE_unary_pred:
      return true;

    case SHAPE_binary_extract:
      return require_immediate_extract_index (pg_args + 2);

    case SHAPE_binary_lane:
      return require_immediate_lane_index (pg_args + 2);

    case SHAPE_binary_rotate:
      return require_immediate_rotate (pg_args + 2, false);

    case SHAPE_get2:
    case SHAPE_set2:
      return require_immediate_range (1, 0, 1);

    case SHAPE_get3:
    case SHAPE_set3:
      return require_immediate_range (1, 0, 2);

    case SHAPE_get4:
    case SHAPE_set4:
      return require_immediate_range (1, 0, 3);

    case SHAPE_inc_dec:
      return require_immediate_range (1, 1, 16);

    case SHAPE_inc_dec_pattern:
      return (require_immediate_svpattern (1)
	      && require_immediate_range (2, 1, 16));

    case SHAPE_pattern_count:
    case SHAPE_pattern_pred:
      return require_immediate_svpattern (0);

    case SHAPE_prefetch:
    case SHAPE_prefetch_gather_index:
    case SHAPE_prefetch_gather_offset:
      if (m_fi.displacement_units () != UNITS_none)
	return require_immediate_svprfop (pg_args + 2);
      return require_immediate_svprfop (pg_args + 1);

    case SHAPE_shift_right_imm:
      return check_shift_right_imm ();

    case SHAPE_ternary_lane:
      return require_immediate_lane_index (pg_args + 3);

    case SHAPE_ternary_lane_rotate:
      return (require_immediate_lane_index (pg_args + 3, 2)
	      && require_immediate_rotate (pg_args + 4, true));

    case SHAPE_ternary_qq_lane:
      return require_immediate_lane_index (pg_args + 3, 4);

    case SHAPE_ternary_rotate:
      return require_immediate_rotate (pg_args + 3, true);
    }
  gcc_unreachable ();
}

/* Check a SHAPE_shift_right_imm call.  */
bool
function_checker::check_shift_right_imm ()
{
  unsigned int bits = type_suffixes[m_fi.types[0]].elem_bits;
  return require_immediate_range (2, 1, bits);
}

/* Check that argument ARGNO is an integer constant expression and
   store its value in VALUE_OUT if so.  The caller should first
   check that argument ARGNO exists.  */
bool
function_checker::require_immediate (unsigned int argno,
				     HOST_WIDE_INT &value_out)
{
  tree arg = m_args[argno];

  /* The type and range are unsigned, so read the argument as an
     unsigned rather than signed HWI.  */
  if (!tree_fits_uhwi_p (arg))
    {
      report_non_ice (m_location, m_decl, argno);
      return false;
    }

  /* ...but treat VALUE_OUT as signed for error reporting, since printing
     -1 is more user-friendly than the maximum uint64_t value.  */
  value_out = tree_to_uhwi (arg);
  return true;
}

/* Check that argument ARGNO, when multiplied by the number of bytes
   in an element of type suffix 0, is an integer constant expression
   in the range [0, 255].  */
bool
function_checker::require_immediate_extract_index (unsigned int argno)
{
  unsigned int bytes = type_suffixes[m_fi.types[0]].elem_bytes;
  return require_immediate_range (argno, 0, 256 / bytes - 1);
}

/* Check that argument ARGNO is suitable for indexing argument ARGNO - 1,
   in groups of GROUP_SIZE elements.  */
bool
function_checker::require_immediate_lane_index (unsigned int argno,
						unsigned int group_size)
{
  /* Get the type of the previous argument.  tree_argument_type wants a
     1-based number, whereas ARGNO is 0-based.  */
  machine_mode mode = TYPE_MODE (type_argument_type (m_fntype, argno));
  gcc_assert (VECTOR_MODE_P (mode));
  unsigned int nlanes = 128 / (group_size * GET_MODE_UNIT_BITSIZE (mode));
  return require_immediate_range (argno, 0, nlanes - 1);
}

/* Check that argument ARGNO is an integer constant expression in the
   range [MIN, MAX].  */
bool
function_checker::require_immediate_range (unsigned int argno,
					   HOST_WIDE_INT min,
					   HOST_WIDE_INT max)
{
  if (m_nargs <= argno)
    return true;

  /* Required because of the tree_to_uhwi -> HOST_WIDE_INT conversion
     in require_immediate.  */
  gcc_assert (min >= 0 && min <= max);
  HOST_WIDE_INT actual;
  if (!require_immediate (argno, actual))
    return false;

  if (!IN_RANGE (actual, min, max))
    {
      report_out_of_range (m_location, m_decl, argno, actual, min, max);
      return false;
    }

  return true;
}

/* Check that argument ARGNO is an integer constant expression that
   specifies a rotation value, as a multiple of 90 degrees.  90 and 270
   are always valid; ALLOW_0_180_P says whether 0 and 180 are too.  */
bool
function_checker::require_immediate_rotate (unsigned int argno,
					    bool allow_0_180_p)
{
  if (m_nargs <= argno)
    return true;

  HOST_WIDE_INT actual;
  if (!require_immediate (argno, actual))
    return false;

  if (actual != 90 && actual != 270)
    {
      if (!allow_0_180_p)
	{
	  report_neither_nor (m_location, m_decl, argno, actual, 90, 270);
	  return false;
	}
      if (actual != 0 && actual != 180)
	{
	  report_not_one_of (m_location, m_decl, argno, actual,
			     0, 90, 180, 270);
	  return false;
	}
    }

  return true;
}

/* Check that argument ARGNO is an integer constant expression that
   has a valid svpattern value.  */
bool
function_checker::require_immediate_svpattern (unsigned int argno)
{
  if (m_nargs <= argno)
    return true;

  HOST_WIDE_INT actual;
  if (!require_immediate (argno, actual))
    return false;

  if (!aarch64_svpattern_immediate_p (actual))
    {
      report_not_enum (m_location, m_decl, argno, actual, acle_svpattern);
      return false;
    }

  return true;
}

/* Check that argument ARGNO is an integer constant expression that
   has a valid svprfop value.  */
bool
function_checker::require_immediate_svprfop (unsigned int argno)
{
  if (m_nargs <= argno)
    return true;

  HOST_WIDE_INT actual;
  if (!require_immediate (argno, actual))
    return false;

  if (!aarch64_svprfop_immediate_p (actual))
    {
      report_not_enum (m_location, m_decl, argno, actual, acle_svprfop);
      return false;
    }

  return true;
}

/* Register the built-in SVE ABI types, such as __SVBool_t.  */
static void
register_builtin_types ()
{
#define DEF_SVE_TYPE(ACLE_NAME, NCHARS, ABI_NAME, SCALAR_TYPE) \
  scalar_types[VECTOR_TYPE_ ## ACLE_NAME] = SCALAR_TYPE;
#include "aarch64-sve-builtins.def"

  for (unsigned int i = 0; i < NUM_VECTOR_TYPES; ++i)
    {
      tree eltype = scalar_types[i];
      tree vectype;
      if (eltype == boolean_type_node)
	{
	  vectype = build_truth_vector_type (BYTES_PER_SVE_VECTOR,
					     BYTES_PER_SVE_VECTOR);
	  gcc_assert (GET_MODE_CLASS (TYPE_MODE (vectype)) == MODE_VECTOR_BOOL
		      && TYPE_MODE (vectype) == TYPE_MODE_RAW (vectype)
		      && TYPE_ALIGN (vectype) == 16
		      && known_eq (wi::to_poly_offset (TYPE_SIZE (vectype)),
				   BYTES_PER_SVE_VECTOR));
	}
      else
	{
	  unsigned int elbytes = tree_to_uhwi (TYPE_SIZE_UNIT (eltype));
	  poly_uint64 nunits = exact_div (BYTES_PER_SVE_VECTOR, elbytes);
	  vectype = build_vector_type (eltype, nunits);
	  gcc_assert (VECTOR_MODE_P (TYPE_MODE (vectype))
		      && TYPE_MODE (vectype) == TYPE_MODE_RAW (vectype)
		      && TYPE_ALIGN (vectype) == 128
		      && known_eq (wi::to_poly_offset (TYPE_SIZE (vectype)),
				   BITS_PER_SVE_VECTOR));
	}
      vectype = build_distinct_type_copy (vectype);
      SET_TYPE_STRUCTURAL_EQUALITY (vectype);
      TYPE_ARTIFICIAL (vectype) = 1;
      TYPE_SIZELESS_P (vectype) = 1;
      abi_vector_types[i] = vectype;
      lang_hooks.types.register_builtin_type (vectype,
					      vector_types[i].abi_name);
    }
}

/* Initialize all compiler built-ins related to SVE.  */
void
init_builtins ()
{
  sve_switcher sve;
  register_builtin_types ();
}

/* Return true if TYPE is a built-in type defined by the SVE ABI.  */
bool
builtin_type_p (const_tree type)
{
  return find_vector_type (type) != NUM_VECTOR_TYPES;
}

/* If TYPE is a builtin-type defined by the SVE ABI, return the mangled name,
   otherwise return NULL.  */
const char *
mangle_builtin_type (const_tree type)
{
  vector_type vtype = find_vector_type (type);
  if (vtype != NUM_VECTOR_TYPES)
    return vector_types[vtype].mangled_name;
  return NULL;
}

/* Implement #pragma GCC aarch64 "arm_sve.h".  */
void
handle_arm_sve_h ()
{
  if (function_table)
    {
      error ("duplicate definition of %qs", "arm_sve.h");
      return;
    }

  sve_switcher sve;
  function_table = new hash_table<registered_function_hasher> (1023);
  arm_sve_h_builder builder;
  for (unsigned int i = 0; i < NUM_VECTOR_TYPES; ++i)
    builder.register_type (vector_type (i));
  builder.register_svpattern ();
  builder.register_svprfop ();
  for (unsigned int i = 0; i < ARRAY_SIZE (function_groups); ++i)
    builder.build (function_groups[i]);
}

/* Return the function decl with SVE function subcode CODE, or error_mark_node
   if no such function exists.  */
tree
builtin_decl (unsigned int code, bool)
{
  if (code >= vec_safe_length (registered_functions))
    return error_mark_node;
  return (*registered_functions)[code]->decl;
}

/* If we're implementing manual overloading, check whether the SVE
   function with subcode CODE is overloaded, and if so attempt to
   determine the corresponding non-overloaded function.  The call
   occurs at location LOCATION and has the arguments given by ARGLIST.

   If the call is erroneous, report an appropriate error and return
   error_mark_node.  Otherwise, if the function is overloaded, return
   the decl of the non-overloaded function.  Return NULL_TREE otherwise,
   indicating that the call should be processed in the normal way.  */
tree
resolve_overloaded_builtin (location_t location, unsigned int code,
			    vec<tree, va_gc> *arglist)
{
  if (code >= vec_safe_length (registered_functions))
    return NULL_TREE;

  registered_function &rfn = *(*registered_functions)[code];
  if (rfn.overloaded_p)
    return function_resolver (location, rfn, *arglist).resolve ();
  return NULL_TREE;
}

/* Perform any semantic checks needed for a call to the SVE function with
   subcode CODE, such as testing for integer constant expressions.
   The call occurs at location LOCATION and has NARGS arguments.
   ARGS gives the value of each argument and ARG_LOCATION gives
   their location.  FNDECL is the original function decl, before
   overload resolution.

   Return true if the call is valid, otherwise report a suitable error.  */
bool
check_builtin_call (location_t location, vec<location_t>, unsigned int code,
		    tree fndecl, unsigned int nargs, tree *args)
{
  registered_function &rfn = *(*registered_functions)[code];
  return function_checker (location, rfn, fndecl, nargs, args).check ();
}

/* Construct a folder for CALL, which calls the SVE function with
   subcode CODE.  Insert any other new statements at GSI.  */
gimple_folder::gimple_folder (unsigned int code, gimple_stmt_iterator *gsi,
			      gcall *call)
  : m_fi ((*registered_functions)[code]->instance),
    m_gsi (gsi), m_call (call), m_lhs (gimple_call_lhs (call))
{
}

/* Try to fold the call.  Return the new statement on success and null
   on failure.  */
gimple *
gimple_folder::fold ()
{
  /* Don't fold anything when SVE is disabled; emit an error during
     expansion instead.  */
  if (!TARGET_SVE)
    return NULL;

  /* Punt if the function has a return type and no result location is
     provided.  The attributes should allow target-independent code to
     remove the calls.  */
  if (!m_lhs && TREE_TYPE (gimple_call_fntype (m_call)) != void_type_node)
    return NULL;

  switch (m_fi.func)
    {
    case FUNC_svabd:
    case FUNC_svabs:
    case FUNC_svacge:
    case FUNC_svacgt:
    case FUNC_svacle:
    case FUNC_svaclt:
    case FUNC_svadd:
    case FUNC_svadda:
    case FUNC_svaddv:
    case FUNC_svadrb:
    case FUNC_svadrd:
    case FUNC_svadrh:
    case FUNC_svadrw:
    case FUNC_svand:
    case FUNC_svandv:
    case FUNC_svasr:
    case FUNC_svasr_wide:
    case FUNC_svasrd:
    case FUNC_svbic:
    case FUNC_svbrka:
    case FUNC_svbrkb:
    case FUNC_svbrkn:
    case FUNC_svbrkpa:
    case FUNC_svbrkpb:
    case FUNC_svcadd:
    case FUNC_svcls:
    case FUNC_svclz:
    case FUNC_svcmla:
    case FUNC_svcmla_lane:
    case FUNC_svcmpeq:
    case FUNC_svcmpeq_wide:
    case FUNC_svcmpge:
    case FUNC_svcmpge_wide:
    case FUNC_svcmpgt:
    case FUNC_svcmpgt_wide:
    case FUNC_svcmple:
    case FUNC_svcmple_wide:
    case FUNC_svcmplt:
    case FUNC_svcmplt_wide:
    case FUNC_svcmpne:
    case FUNC_svcmpne_wide:
    case FUNC_svcmpuo:
    case FUNC_svcnt:
    case FUNC_svcntp:
    case FUNC_svcompact:
    case FUNC_svdiv:
    case FUNC_svdivr:
    case FUNC_svdot:
    case FUNC_svdot_lane:
    case FUNC_svdup_lane:
    case FUNC_svdupq_lane:
    case FUNC_sveor:
    case FUNC_sveorv:
    case FUNC_svext:
    case FUNC_svextb:
    case FUNC_svexth:
    case FUNC_svextw:
    case FUNC_svindex:
    case FUNC_svld1_gather:
    case FUNC_svld1rq:
    case FUNC_svld1sb:
    case FUNC_svld1sb_gather:
    case FUNC_svld1sh:
    case FUNC_svld1sh_gather:
    case FUNC_svld1sw:
    case FUNC_svld1sw_gather:
    case FUNC_svld1ub:
    case FUNC_svld1ub_gather:
    case FUNC_svld1uh:
    case FUNC_svld1uh_gather:
    case FUNC_svld1uw:
    case FUNC_svld1uw_gather:
    case FUNC_svldff1:
    case FUNC_svldff1_gather:
    case FUNC_svldff1sb:
    case FUNC_svldff1sb_gather:
    case FUNC_svldff1sh:
    case FUNC_svldff1sh_gather:
    case FUNC_svldff1sw:
    case FUNC_svldff1sw_gather:
    case FUNC_svldff1ub:
    case FUNC_svldff1ub_gather:
    case FUNC_svldff1uh:
    case FUNC_svldff1uh_gather:
    case FUNC_svldff1uw:
    case FUNC_svldff1uw_gather:
    case FUNC_svldnf1:
    case FUNC_svldnf1sb:
    case FUNC_svldnf1sh:
    case FUNC_svldnf1sw:
    case FUNC_svldnf1ub:
    case FUNC_svldnf1uh:
    case FUNC_svldnf1uw:
    case FUNC_svldnt1:
    case FUNC_svlsl:
    case FUNC_svlsl_wide:
    case FUNC_svlsr:
    case FUNC_svlsr_wide:
    case FUNC_svmad:
    case FUNC_svmax:
    case FUNC_svmaxnm:
    case FUNC_svmaxnmv:
    case FUNC_svmaxv:
    case FUNC_svmin:
    case FUNC_svminnm:
    case FUNC_svminnmv:
    case FUNC_svminv:
    case FUNC_svmla:
    case FUNC_svmla_lane:
    case FUNC_svmls:
    case FUNC_svmls_lane:
    case FUNC_svmsb:
    case FUNC_svmul:
    case FUNC_svmul_lane:
    case FUNC_svmulh:
    case FUNC_svmulx:
    case FUNC_svnand:
    case FUNC_svneg:
    case FUNC_svnmad:
    case FUNC_svnmla:
    case FUNC_svnmls:
    case FUNC_svnmsb:
    case FUNC_svnor:
    case FUNC_svnot:
    case FUNC_svorn:
    case FUNC_svorr:
    case FUNC_svorv:
    case FUNC_svprfb:
    case FUNC_svprfb_gather:
    case FUNC_svprfd:
    case FUNC_svprfd_gather:
    case FUNC_svprfh:
    case FUNC_svprfh_gather:
    case FUNC_svprfw:
    case FUNC_svprfw_gather:
    case FUNC_svptest_any:
    case FUNC_svptest_first:
    case FUNC_svptest_last:
    case FUNC_svqadd:
    case FUNC_svqdecb:
    case FUNC_svqdecb_pat:
    case FUNC_svqdecd:
    case FUNC_svqdecd_pat:
    case FUNC_svqdech:
    case FUNC_svqdech_pat:
    case FUNC_svqdecp:
    case FUNC_svqdecw:
    case FUNC_svqdecw_pat:
    case FUNC_svqincb:
    case FUNC_svqincb_pat:
    case FUNC_svqincd:
    case FUNC_svqincd_pat:
    case FUNC_svqinch:
    case FUNC_svqinch_pat:
    case FUNC_svqincp:
    case FUNC_svqincw:
    case FUNC_svqincw_pat:
    case FUNC_svqsub:
    case FUNC_svrbit:
    case FUNC_svrdffr:
    case FUNC_svsetffr:
    case FUNC_svsplice:
    case FUNC_svsqrt:
    case FUNC_svst1_scatter:
    case FUNC_svst1b:
    case FUNC_svst1b_scatter:
    case FUNC_svst1h:
    case FUNC_svst1h_scatter:
    case FUNC_svst1w:
    case FUNC_svst1w_scatter:
    case FUNC_svstnt1:
    case FUNC_svsub:
    case FUNC_svsubr:
    case FUNC_svtbl:
    /* Don't fold svundef at the gimple level.  There's no exact
       correspondence for SSA_NAMEs, and we explicitly don't want
       to generate a specific value (like an all-zeros vector).  */
    case FUNC_svundef:
    case FUNC_svwrffr:
      return NULL;

    case FUNC_svcntb:
    case FUNC_svcntd:
    case FUNC_svcnth:
    case FUNC_svcntw:
      return fold_cnt_bhwd ();

    case FUNC_svcntb_pat:
    case FUNC_svcntd_pat:
    case FUNC_svcnth_pat:
    case FUNC_svcntw_pat:
      return fold_cnt_bhwd_pat ();

    case FUNC_svcreate2:
    case FUNC_svcreate3:
    case FUNC_svcreate4:
      return fold_create ();

    case FUNC_svdup:
      return fold_dup ();

    case FUNC_svdupq:
      return fold_dupq ();

    case FUNC_svget2:
    case FUNC_svget3:
    case FUNC_svget4:
      return fold_get ();

    case FUNC_svinsr:
      return fold_insr ();

    case FUNC_svld1:
      return fold_ld1 ();

    case FUNC_svld2:
    case FUNC_svld3:
    case FUNC_svld4:
      return fold_ld234 ();

    case FUNC_svpfalse:
      return fold_pfalse ();

    case FUNC_svptrue:
      return fold_ptrue ();

    case FUNC_svptrue_pat:
      return fold_ptrue_pat ();

    case FUNC_svrev:
      return fold_rev ();

    case FUNC_svsel:
      return fold_sel ();

    case FUNC_svset2:
    case FUNC_svset3:
    case FUNC_svset4:
      return fold_set ();

    case FUNC_svst1:
      return fold_st1 ();

    case FUNC_svst2:
    case FUNC_svst3:
    case FUNC_svst4:
      return fold_st234 ();

    case FUNC_svtrn1:
    case FUNC_svtrn2:
      return fold_trn ();

    case FUNC_svundef2:
    case FUNC_svundef3:
    case FUNC_svundef4:
      return fold_undef ();

    case FUNC_svuzp1:
    case FUNC_svuzp2:
      return fold_uzp ();

    case FUNC_svwhilele:
      return fold_while (true);

    case FUNC_svwhilelt:
      return fold_while (false);

    case FUNC_svzip1:
    case FUNC_svzip2:
      return fold_zip ();
    }
  gcc_unreachable ();
}

/* Fold a call to svcnt[bhwd].  */
gimple *
gimple_folder::fold_cnt_bhwd ()
{
  machine_mode ref_mode = m_fi.bhwd_vector_mode ();
  tree count = build_int_cstu (TREE_TYPE (m_lhs), GET_MODE_NUNITS (ref_mode));
  return gimple_build_assign (m_lhs, count);
}

/* Fold a call to svcnt[bhwd]_pat.  */
gimple *
gimple_folder::fold_cnt_bhwd_pat ()
{
  tree pattern_arg = gimple_call_arg (m_call, 0);
  if (!tree_fits_shwi_p (pattern_arg)
      || !aarch64_svpattern_immediate_p (tree_to_shwi (pattern_arg)))
    return NULL;

  aarch64_svpattern pattern = (aarch64_svpattern) tree_to_shwi (pattern_arg);
  if (pattern == AARCH64_SV_ALL)
    return fold_cnt_bhwd ();

  unsigned int nelts_per_vq = m_fi.bhwd_nelts_per_vq ();
  HOST_WIDE_INT value = aarch64_fold_sve_cnt_pat (pattern, nelts_per_vq);
  if (value >= 0)
    {
      tree count = build_int_cstu (TREE_TYPE (m_lhs), value);
      return gimple_build_assign (m_lhs, count);
    }

  return NULL;
}

/* Fold a call to svcreate*.  */
gimple *
gimple_folder::fold_create ()
{
  unsigned int nargs = gimple_call_num_args (m_call);
  tree lhs_type = TREE_TYPE (m_lhs);
  gassign *assign = gimple_build_assign (m_lhs, build_clobber (lhs_type));

  for (unsigned int i = nargs; i-- > 0; )
    {
      tree vector = gimple_call_arg (m_call, i);
      tree field = tuple_type_field (TREE_TYPE (m_lhs));
      tree ref = build3 (COMPONENT_REF, TREE_TYPE (field),
			 unshare_expr (m_lhs), field, NULL_TREE);
      ref = build4 (ARRAY_REF, TREE_TYPE (vector),
		    ref, size_int (i), NULL_TREE, NULL_TREE);
      gsi_insert_after (m_gsi, gimple_build_assign (ref, vector),
			GSI_SAME_STMT);
    }
  return assign;
}

/* Fold a call to svdup.  */
gimple *
gimple_folder::fold_dup ()
{
  tree vec_type = TREE_TYPE (m_lhs);
  tree elt = gimple_call_arg (m_call, m_fi.pred == PRED_none ? 0 : 1);

  if ((m_fi.pred == PRED_none || m_fi.pred == PRED_x)
      && CONSTANT_CLASS_P (elt))
    return gimple_build_assign (m_lhs, build_vector_from_val (vec_type, elt));

  /* Avoid folding away the predicate for _x, since we'll need it later.  */
  if (m_fi.pred == PRED_none)
    return gimple_build_assign (m_lhs, VEC_DUPLICATE_EXPR, elt);

  return NULL;
}

/* Fold a call to svdupq.  */
gimple *
gimple_folder::fold_dupq ()
{
  tree vec_type = TREE_TYPE (m_lhs);
  unsigned int nargs = gimple_call_num_args (m_call);
  /* For predicates, pad out each argument so that we have one element
     per bit.  */
  unsigned int factor = (type_suffixes[m_fi.types[0]].bool_p
			 ? type_suffixes[m_fi.types[0]].elem_bytes : 1);
  tree_vector_builder builder (vec_type, nargs * factor, 1);
  for (unsigned int i = 0; i < nargs; ++i)
    {
      tree elt = gimple_call_arg (m_call, i);
      if (!CONSTANT_CLASS_P (elt))
	return NULL;
      builder.quick_push (elt);
      for (unsigned int j = 1; j < factor; ++j)
	builder.quick_push (build_zero_cst (TREE_TYPE (vec_type)));
    }
  return gimple_build_assign (m_lhs, builder.build ());
}

/* Fold a call to svget.  */
gimple *
gimple_folder::fold_get ()
{
  tree tuple = gimple_call_arg (m_call, 0);
  tree index = gimple_call_arg (m_call, 1);
  tree field = tuple_type_field (TREE_TYPE (tuple));
  tree ref = build3 (COMPONENT_REF, TREE_TYPE (field),
		     tuple, field, NULL_TREE);
  ref = build4 (ARRAY_REF, TREE_TYPE (m_lhs),
		ref, index, NULL_TREE, NULL_TREE);
  return gimple_build_assign (m_lhs, ref);
}

/* Fold a call to svinsr.  */
gimple *
gimple_folder::fold_insr ()
{
  gcall *new_call = gimple_build_call_internal (IFN_VEC_SHL_INSERT, 2,
						gimple_call_arg (m_call, 0),
						gimple_call_arg (m_call, 1));
  gimple_call_set_lhs (new_call, m_lhs);
  return new_call;
}

/* Fold a call to svld1.  */
gimple *
gimple_folder::fold_ld1 ()
{
  tree vectype = m_fi.vector_type (0);

  gimple_seq stmts = NULL;
  tree pred = convert_pred (stmts, vectype, 0);
  tree base = fold_contiguous_base (stmts, vectype);
  gsi_insert_seq_before (m_gsi, stmts, GSI_SAME_STMT);

  tree cookie = load_store_cookie (TREE_TYPE (vectype));
  gcall *new_call = gimple_build_call_internal (IFN_MASK_LOAD, 3,
						base, cookie, pred);
  gimple_call_set_lhs (new_call, m_lhs);
  return new_call;
}

/* Fold a call to svld[234].  */
gimple *
gimple_folder::fold_ld234 ()
{
  tree vectype = m_fi.vector_type (0);

  gimple_seq stmts = NULL;
  tree pred = convert_pred (stmts, vectype, 0);
  tree base = fold_contiguous_base (stmts, vectype);
  gsi_insert_seq_before (m_gsi, stmts, GSI_SAME_STMT);

  tree field = tuple_type_field (TREE_TYPE (m_lhs));
  tree lhs_array = build1 (VIEW_CONVERT_EXPR, TREE_TYPE (field),
			   unshare_expr (m_lhs));

  tree cookie = load_store_cookie (TREE_TYPE (vectype));
  gcall *new_call = gimple_build_call_internal (IFN_MASK_LOAD_LANES, 3,
						base, cookie, pred);
  gimple_call_set_lhs (new_call, lhs_array);
  gsi_insert_after (m_gsi, new_call, GSI_SAME_STMT);

  return gimple_build_assign (m_lhs, build_clobber (TREE_TYPE (m_lhs)));
}

/* Fold a call to svpfalse.  */
gimple *
gimple_folder::fold_pfalse ()
{
  return gimple_build_assign (m_lhs, build_zero_cst (TREE_TYPE (m_lhs)));
}

/* Fold a call to svptrue.  */
gimple *
gimple_folder::fold_ptrue ()
{
  tree vectype = m_fi.vector_type (0);
  tree elttype = TREE_TYPE (vectype);
  unsigned int num_bytes = type_suffixes[m_fi.types[0]].elem_bytes;

  /* The type is svbool_t regardless of TYPE, thus for b8 we want
     { 1, 1, 1, 1, ... }, for b16 we want { 1, 0, 1, 0, ... }, etc.  */
  tree_vector_builder builder (vectype, num_bytes, 1);
  builder.quick_push (build_all_ones_cst (elttype));
  for (unsigned int i = 1; i < num_bytes; ++i)
    builder.quick_push (build_zero_cst (elttype));
  return gimple_build_assign (m_lhs, builder.build ());
}

/* Fold a call to svptrue_pat.  */
gimple *
gimple_folder::fold_ptrue_pat ()
{
  tree pattern_arg = gimple_call_arg (m_call, 0);
  if (!tree_fits_shwi_p (pattern_arg)
      || !aarch64_svpattern_immediate_p (tree_to_shwi (pattern_arg)))
    return NULL;

  aarch64_svpattern pattern = (aarch64_svpattern) tree_to_shwi (pattern_arg);
  if (pattern == AARCH64_SV_ALL)
    return fold_ptrue ();

  int num_bytes = type_suffixes[m_fi.types[0]].elem_bytes;
  HOST_WIDE_INT value = aarch64_fold_sve_cnt_pat (pattern, 16 / num_bytes);
  if (value >= 0)
    return fold_to_vl_pred (num_bytes, value);

  return NULL;
}

/* Fold a call to svrev.  */
gimple *
gimple_folder::fold_rev ()
{
  poly_int64 nelts = TYPE_VECTOR_SUBPARTS (TREE_TYPE (m_lhs));
  vec_perm_builder builder (nelts, 1, 3);
  for (int i = 0; i < 3; ++i)
    builder.quick_push (nelts - i - 1);
  return fold_permute (builder);
}

/* Fold a call to svsel.  */
gimple *
gimple_folder::fold_sel ()
{
  gimple_seq stmts = NULL;
  tree pred = convert_pred (stmts, m_fi.vector_type (0), 0);
  gsi_insert_seq_before (m_gsi, stmts, GSI_SAME_STMT);
  return gimple_build_assign (m_lhs, VEC_COND_EXPR, pred,
			      gimple_call_arg (m_call, 1),
			      gimple_call_arg (m_call, 2));
}

/* Fold a call to svset*.  */
gimple *
gimple_folder::fold_set ()
{
  tree tuple = gimple_call_arg (m_call, 0);
  tree index = gimple_call_arg (m_call, 1);
  tree vector = gimple_call_arg (m_call, 2);
  gassign *assign = gimple_build_assign (unshare_expr (m_lhs), tuple);

  tree field = tuple_type_field (TREE_TYPE (m_lhs));
  tree ref = build3 (COMPONENT_REF, TREE_TYPE (field),
		     m_lhs, field, NULL_TREE);
  ref = build4 (ARRAY_REF, TREE_TYPE (vector),
		ref, index, NULL_TREE, NULL_TREE);
  gsi_insert_after (m_gsi, gimple_build_assign (ref, vector), GSI_SAME_STMT);
  return assign;
}

/* Fold a call to svst1.  */
gimple *
gimple_folder::fold_st1 ()
{
  unsigned int num_args = gimple_call_num_args (m_call);
  tree vectype = m_fi.vector_type (0);
  tree store_data = gimple_call_arg (m_call, num_args - 1);

  gimple_seq stmts = NULL;
  tree pred = convert_pred (stmts, vectype, 0);
  tree base = fold_contiguous_base (stmts, vectype);
  gsi_insert_seq_before (m_gsi, stmts, GSI_SAME_STMT);

  tree cookie = load_store_cookie (TREE_TYPE (vectype));
  return gimple_build_call_internal (IFN_MASK_STORE, 4,
				     base, cookie, pred, store_data);
}

/* Fold a call to svst[234].  */
gimple *
gimple_folder::fold_st234 ()
{
  unsigned int num_args = gimple_call_num_args (m_call);
  tree vectype = m_fi.vector_type (0);
  tree store_data = gimple_call_arg (m_call, num_args - 1);

  gimple_seq stmts = NULL;
  tree pred = convert_pred (stmts, vectype, 0);
  tree base = fold_contiguous_base (stmts, vectype);
  gsi_insert_seq_before (m_gsi, stmts, GSI_SAME_STMT);

  tree field = tuple_type_field (TREE_TYPE (store_data));
  tree rhs_array = build1 (VIEW_CONVERT_EXPR, TREE_TYPE (field), store_data);

  tree cookie = load_store_cookie (TREE_TYPE (vectype));
  return gimple_build_call_internal (IFN_MASK_STORE_LANES, 4,
				     base, cookie, pred, rhs_array);
}

/* Fold a call to svtrn1 or svtrn2.  */
gimple *
gimple_folder::fold_trn ()
{
  poly_uint64 nelts = TYPE_VECTOR_SUBPARTS (TREE_TYPE (m_lhs));
  int base = m_fi.func == FUNC_svtrn1 ? 0 : 1;
  vec_perm_builder builder (nelts, 2, 3);
  for (unsigned int i = 0; i < 3; ++i)
    {
      builder.quick_push (base + i * 2);
      builder.quick_push (base + i * 2 + nelts);
    }
  return fold_permute (builder);
}

/* Fold a call to svundef*.  */
gimple *
gimple_folder::fold_undef ()
{
  return gimple_build_assign (m_lhs, build_clobber (TREE_TYPE (m_lhs)));
}

/* Fold a call to svuzp1 or svuzp2.  */
gimple *
gimple_folder::fold_uzp ()
{
  poly_uint64 nelts = TYPE_VECTOR_SUBPARTS (TREE_TYPE (m_lhs));
  int base = m_fi.func == FUNC_svuzp1 ? 0 : 1;
  vec_perm_builder builder (nelts, 1, 3);
  for (unsigned int i = 0; i < 3; ++i)
    builder.quick_push (base + i * 2);
  return fold_permute (builder);
}

/* Fold a call to svwhilele or svwhilelt; eq_p says which.  T is the
   type to use for constant arguments, chosen based on their signdness.  */
template<typename T>
gimple *
gimple_folder::fold_while_type (bool eq_p)
{
  T arg0, arg1;
  if (!poly_int_tree_p (gimple_call_arg (m_call, 0), &arg0)
      || !poly_int_tree_p (gimple_call_arg (m_call, 1), &arg1))
    return NULL;

  if (eq_p ? known_gt (arg0, arg1) : known_ge (arg0, arg1))
    return fold_pfalse ();

  if (eq_p ? maybe_gt (arg0, arg1) : maybe_ge (arg0, arg1))
    return NULL;

  int elt_size = type_suffixes[m_fi.types[0]].elem_bytes;
  poly_uint64 diff = arg1 - arg0;
  poly_uint64 nelts = exact_div (BYTES_PER_SVE_VECTOR, elt_size);
  /* Subtract from NELTS rather than adding to DIFF, to prevent overflow.  */
  if (eq_p)
    nelts -= 1;
  if (known_ge (diff, nelts))
    return fold_ptrue ();

  /* Conditional equality is fine.  */
  if (maybe_gt (diff, nelts))
    return NULL;

  unsigned HOST_WIDE_INT vl;
  if (diff.is_constant (&vl))
    /* Overflow is no longer possible.  */
    return fold_to_vl_pred (elt_size, eq_p ? vl + 1 : vl);

  return NULL;
}

/* Fold a call to svwhilele or svwhilelt; eq_p says which.  */
gimple *
gimple_folder::fold_while (bool eq_p)
{
  if (type_suffixes[m_fi.types[1]].unsigned_p)
    return fold_while_type<poly_uint64> (eq_p);
  else
    return fold_while_type<poly_int64> (eq_p);
}

/* Fold a call to svzip1 or svzip2.  */
gimple *
gimple_folder::fold_zip ()
{
  poly_uint64 nelts = TYPE_VECTOR_SUBPARTS (TREE_TYPE (m_lhs));
  poly_uint64 base = m_fi.func == FUNC_svzip1 ? 0 : exact_div (nelts, 2);
  vec_perm_builder builder (nelts, 2, 3);
  for (unsigned int i = 0; i < 3; ++i)
    {
      builder.quick_push (base + i);
      builder.quick_push (base + i + nelts);
    }
  return fold_permute (builder);
}

/* Fold a binary permute with the permute vector given by BUILDER.  */
gimple *
gimple_folder::fold_permute (const vec_perm_builder &builder)
{
  /* Punt for now on _b16 and wider; we'd need more complex evpc logic
     to rerecognize the result.  */
  if (type_suffixes[m_fi.types[0]].bool_p
      && type_suffixes[m_fi.types[0]].elem_bits > 8)
    return NULL;

  unsigned int nargs = gimple_call_num_args (m_call);
  poly_uint64 nelts = TYPE_VECTOR_SUBPARTS (TREE_TYPE (m_lhs));
  vec_perm_indices indices (builder, nargs, nelts);
  tree perm_type = build_vector_type (ssizetype, nelts);
  return gimple_build_assign (m_lhs, VEC_PERM_EXPR,
			      gimple_call_arg (m_call, 0),
			      gimple_call_arg (m_call, nargs - 1),
			      vec_perm_indices_to_tree (perm_type, indices));
}

/* Fold an operation to a constant predicate in which:

   (a) each element controls ELT_SIZE bytes of data
   (b) the first VL bits are set and the rest are clear.  */
gimple *
gimple_folder::fold_to_vl_pred (unsigned int elt_size, unsigned int vl)
{
  tree vec_type = TREE_TYPE (m_lhs);
  tree elt_type = TREE_TYPE (vec_type);
  tree minus_one = build_all_ones_cst (elt_type);
  tree zero = build_zero_cst (elt_type);

  /* Construct COUNT elements that contain the ptrue followed by
     a repeating sequence of COUNT elements.  */
  unsigned int count = constant_lower_bound (TYPE_VECTOR_SUBPARTS (vec_type));
  gcc_assert (vl * elt_size <= count);
  tree_vector_builder builder (vec_type, count, 2);
  for (unsigned int i = 0; i < count * 2; ++i)
    {
      bool bit = (i & (elt_size - 1)) == 0 && i < vl * elt_size;
      builder.quick_push (bit ? minus_one : zero);
    }
  return gimple_build_assign (m_lhs, builder.build ());
}

/* Convert predicate argument I so that it has the type appropriate for
   an operation on VECTYPE.  Add any new statements to STMTS.  */
tree
gimple_folder::convert_pred (gimple_seq &stmts, tree vectype, unsigned int i)
{
  tree predtype = build_same_sized_truth_vector_type (vectype);
  tree pred = gimple_call_arg (m_call, i);
  return gimple_build (&stmts, VIEW_CONVERT_EXPR, predtype, pred);
}

/* Return a pointer to the address in a contiguous load or store,
   given that each memory vector has type VECTYPE.  Add any new
   statements to STMTS.  */
tree
gimple_folder::fold_contiguous_base (gimple_seq &stmts, tree vectype)
{
  tree base = gimple_call_arg (m_call, 1);
  if (m_fi.mode == MODE_vnum)
    {
      tree offset = gimple_call_arg (m_call, 2);
      offset = gimple_convert (&stmts, sizetype, offset);
      offset = gimple_build (&stmts, MULT_EXPR, sizetype, offset,
			     TYPE_SIZE_UNIT (vectype));
      base = gimple_build (&stmts, POINTER_PLUS_EXPR, TREE_TYPE (base),
			   base, offset);
    }
  return base;
}

/* Return the alignment and TBAA argument to an internal load or store
   function, given that it accesses memory elements of type TYPE.  */
tree
gimple_folder::load_store_cookie (tree type)
{
 return build_int_cst (build_pointer_type (type), TYPE_ALIGN_UNIT (type));
}

/* Attempt to fold STMT, given that it's a call to the SVE function
   with subcode CODE.  Return the new statement on success and null
   on failure.  Insert any other new statements at GSI.  */
gimple *
gimple_fold_builtin (unsigned int code, gimple_stmt_iterator *gsi, gcall *stmt)
{
  return gimple_folder (code, gsi, stmt).fold ();
}

/* Construct an expander for a call to the SVE function with subcode CODE.
   EXP is the call expression and TARGET is the preferred location for
   the result.  */
function_expander::function_expander (unsigned int code, tree exp, rtx target)
  : m_rfn (*(*registered_functions)[code]), m_fi (m_rfn.instance),
    m_exp (exp), m_location (EXPR_LOCATION (exp)), m_target (target)
{
}

/* Expand the call and return its lhs.  */
rtx
function_expander::expand ()
{
  if (!TARGET_SVE)
    {
      report_missing_feature (m_location, m_rfn.decl, "+sve");
      return m_target;
    }

  unsigned int nargs = call_expr_nargs (m_exp);
  m_args.reserve (nargs);
  for (unsigned int i = 0; i < nargs; ++i)
    m_args.quick_push (expand_normal (CALL_EXPR_ARG (m_exp, i)));

  switch (m_fi.func)
    {
    case FUNC_svabd:
      return expand_abd ();

    case FUNC_svabs:
      return expand_abs ();

    case FUNC_svacge:
      return expand_ac (UNSPEC_COND_GE);

    case FUNC_svacgt:
      return expand_ac (UNSPEC_COND_GT);

    case FUNC_svacle:
      return expand_ac (UNSPEC_COND_LE);

    case FUNC_svaclt:
      return expand_ac (UNSPEC_COND_LT);

    case FUNC_svadd:
      return expand_add (1);

    case FUNC_svadda:
      return expand_adda ();

    case FUNC_svadrb:
      return expand_adrb ();

    case FUNC_svadrd:
    case FUNC_svadrh:
    case FUNC_svadrw:
      return expand_adrhwd ();

    case FUNC_svaddv:
      return expand_reduction (UNSPEC_SADDV, UNSPEC_UADDV, UNSPEC_FADDV);

    case FUNC_svand:
      return expand_and ();

    case FUNC_svandv:
      return expand_reduction (UNSPEC_ANDV, UNSPEC_ANDV, -1);

    case FUNC_svasr:
      return expand_shift (ASHIFTRT, UNSPEC_ASHIFTRT_WIDE);

    case FUNC_svasr_wide:
      return expand_shift_wide (UNSPEC_ASHIFTRT_WIDE);

    case FUNC_svasrd:
      return expand_asrd ();

    case FUNC_svbrka:
      return expand_brk_unary (UNSPEC_BRKA);

    case FUNC_svbrkb:
      return expand_brk_unary (UNSPEC_BRKB);

    case FUNC_svbrkn:
      return expand_brk_binary (UNSPEC_BRKN);

    case FUNC_svbrkpa:
      return expand_brk_binary (UNSPEC_BRKPA);

    case FUNC_svbrkpb:
      return expand_brk_binary (UNSPEC_BRKPB);

    case FUNC_svbic:
      return expand_bic ();

    case FUNC_svcadd:
      return expand_cadd ();

    case FUNC_svcls:
      return expand_unary_count (CLRSB);

    case FUNC_svclz:
      return expand_unary_count (CLZ);

    case FUNC_svcmla:
      return expand_cmla ();

    case FUNC_svcmla_lane:
      return expand_cmla_lane ();

    case FUNC_svcmpeq:
      return expand_cmp (EQ, EQ, UNSPEC_COND_EQ, UNSPEC_COND_EQ);

    case FUNC_svcmpeq_wide:
      return expand_cmp_wide (UNSPEC_COND_EQ, UNSPEC_COND_EQ);

    case FUNC_svcmpge:
      return expand_cmp (GE, GEU, UNSPEC_COND_GE, UNSPEC_COND_GEU);

    case FUNC_svcmpge_wide:
      return expand_cmp_wide (UNSPEC_COND_GE, UNSPEC_COND_GEU);

    case FUNC_svcmpgt:
      return expand_cmp (GT, GTU, UNSPEC_COND_GT, UNSPEC_COND_GTU);

    case FUNC_svcmpgt_wide:
      return expand_cmp_wide (UNSPEC_COND_GT, UNSPEC_COND_GTU);

    case FUNC_svcmple:
      return expand_cmp (LE, LEU, UNSPEC_COND_LE, UNSPEC_COND_LEU);

    case FUNC_svcmple_wide:
      return expand_cmp_wide (UNSPEC_COND_LE, UNSPEC_COND_LEU);

    case FUNC_svcmplt:
      return expand_cmp (LT, LTU, UNSPEC_COND_LT, UNSPEC_COND_LTU);

    case FUNC_svcmplt_wide:
      return expand_cmp_wide (UNSPEC_COND_LT, UNSPEC_COND_LTU);

    case FUNC_svcmpne:
      return expand_cmp (NE, NE, UNSPEC_COND_NE, UNSPEC_COND_NE);

    case FUNC_svcmpne_wide:
      return expand_cmp_wide (UNSPEC_COND_NE, UNSPEC_COND_NE);

    case FUNC_svcmpuo:
      return expand_cmpuo ();

    case FUNC_svcnt:
      return expand_unary_count (POPCOUNT);

    case FUNC_svcntb:
    case FUNC_svcntd:
    case FUNC_svcnth:
    case FUNC_svcntw:
      return expand_cnt_bhwd ();

    case FUNC_svcntb_pat:
    case FUNC_svcntd_pat:
    case FUNC_svcnth_pat:
    case FUNC_svcntw_pat:
      return expand_cnt_bhwd_pat ();

    case FUNC_svcntp:
      return expand_cntp ();

    case FUNC_svcompact:
      return expand_compact ();

    case FUNC_svcreate2:
    case FUNC_svcreate3:
    case FUNC_svcreate4:
      return expand_create ();

    case FUNC_svdiv:
      return expand_div (false);

    case FUNC_svdivr:
      return expand_div (true);

    case FUNC_svdot:
      return expand_dot ();

    case FUNC_svdot_lane:
      return expand_dot_lane ();

    case FUNC_svdup:
      return expand_dup ();

    case FUNC_svdup_lane:
      return expand_dup_lane ();

    case FUNC_svdupq:
      return expand_dupq ();

    case FUNC_svdupq_lane:
      return expand_dupq_lane ();

    case FUNC_sveor:
      return expand_eor ();

    case FUNC_sveorv:
      return expand_reduction (UNSPEC_XORV, UNSPEC_XORV, -1);

    case FUNC_svext:
      return expand_ext ();

    case FUNC_svextb:
    case FUNC_svexth:
    case FUNC_svextw:
      return expand_ext_bhw ();

    case FUNC_svget2:
    case FUNC_svget3:
    case FUNC_svget4:
      return expand_get ();

    case FUNC_svindex:
      return expand_index ();

    case FUNC_svinsr:
      return expand_insr ();

    case FUNC_svld1:
      return expand_ld1 ();

    case FUNC_svld1_gather:
      return expand_ld1_gather ();

    case FUNC_svld1rq:
      return expand_ld1rq ();

    case FUNC_svld1sb:
    case FUNC_svld1sh:
    case FUNC_svld1sw:
      return expand_ld1_ext (SIGN_EXTEND);

    case FUNC_svld1sb_gather:
    case FUNC_svld1sh_gather:
    case FUNC_svld1sw_gather:
      return expand_ld1_ext_gather (SIGN_EXTEND);

    case FUNC_svld1ub:
    case FUNC_svld1uh:
    case FUNC_svld1uw:
      return expand_ld1_ext (ZERO_EXTEND);

    case FUNC_svld1ub_gather:
    case FUNC_svld1uh_gather:
    case FUNC_svld1uw_gather:
      return expand_ld1_ext_gather (ZERO_EXTEND);

    case FUNC_svld2:
    case FUNC_svld3:
    case FUNC_svld4:
      return expand_ld234 ();

    case FUNC_svldff1:
      return expand_ldff1 ();

    case FUNC_svldff1_gather:
      return expand_ldff1_gather ();

    case FUNC_svldff1sb:
    case FUNC_svldff1sh:
    case FUNC_svldff1sw:
      return expand_ldff1_ext (SIGN_EXTEND);

    case FUNC_svldff1sb_gather:
    case FUNC_svldff1sh_gather:
    case FUNC_svldff1sw_gather:
      return expand_ldff1_ext_gather (SIGN_EXTEND);

    case FUNC_svldff1ub:
    case FUNC_svldff1uh:
    case FUNC_svldff1uw:
      return expand_ldff1_ext (ZERO_EXTEND);

    case FUNC_svldff1ub_gather:
    case FUNC_svldff1uh_gather:
    case FUNC_svldff1uw_gather:
      return expand_ldff1_ext_gather (ZERO_EXTEND);

    case FUNC_svldnf1:
      return expand_ldff1 (UNSPEC_LDNF1);

    case FUNC_svldnf1sb:
    case FUNC_svldnf1sh:
    case FUNC_svldnf1sw:
      return expand_ldff1_ext (SIGN_EXTEND, UNSPEC_LDNF1);

    case FUNC_svldnf1ub:
    case FUNC_svldnf1uh:
    case FUNC_svldnf1uw:
      return expand_ldff1_ext (ZERO_EXTEND, UNSPEC_LDNF1);

    case FUNC_svldnt1:
      return expand_ldnt1 ();

    case FUNC_svlsl:
      return expand_shift (ASHIFT, UNSPEC_ASHIFT_WIDE);

    case FUNC_svlsl_wide:
      return expand_shift_wide (UNSPEC_ASHIFT_WIDE);

    case FUNC_svlsr:
      return expand_shift (LSHIFTRT, UNSPEC_LSHIFTRT_WIDE);

    case FUNC_svlsr_wide:
      return expand_shift_wide (UNSPEC_LSHIFTRT_WIDE);

    case FUNC_svmad:
      return expand_mad (1);

    case FUNC_svmax:
      return expand_max ();

    case FUNC_svmaxnm:
      return expand_maxnm ();

    case FUNC_svmaxnmv:
      return expand_reduction (-1, -1, UNSPEC_FMAXNMV);

    case FUNC_svmaxv:
      return expand_reduction (UNSPEC_SMAXV, UNSPEC_UMAXV, UNSPEC_FMAXV);

    case FUNC_svmin:
      return expand_min ();

    case FUNC_svminnm:
      return expand_minnm ();

    case FUNC_svminnmv:
      return expand_reduction (-1, -1, UNSPEC_FMINNMV);

    case FUNC_svminv:
      return expand_reduction (UNSPEC_SMINV, UNSPEC_UMINV, UNSPEC_FMINV);

    case FUNC_svmla:
      return expand_mla ();

    case FUNC_svmla_lane:
      return expand_mla_mls_lane (UNSPEC_FMLA);

    case FUNC_svmls:
      return expand_mls ();

    case FUNC_svmls_lane:
      return expand_mla_mls_lane (UNSPEC_FMLS);

    case FUNC_svmsb:
      return expand_msb (1);

    case FUNC_svmul:
      return expand_mul ();

    case FUNC_svmul_lane:
      return expand_mul_lane ();

    case FUNC_svmulh:
      return expand_mulh ();

    case FUNC_svmulx:
      return expand_mulx ();

    case FUNC_svnand:
      return expand_nand ();

    case FUNC_svneg:
      return expand_neg ();

    case FUNC_svnmad:
      return expand_nmad ();

    case FUNC_svnmla:
      return expand_nmla ();

    case FUNC_svnmls:
      return expand_nmls ();

    case FUNC_svnmsb:
      return expand_nmsb ();

    case FUNC_svnor:
      return expand_nor ();

    case FUNC_svnot:
      return expand_not ();

    case FUNC_svorn:
      return expand_orn ();

    case FUNC_svorr:
      return expand_orr ();

    case FUNC_svorv:
      return expand_reduction (UNSPEC_IORV, UNSPEC_IORV, -1);

    case FUNC_svpfalse:
      return expand_pfalse ();

    case FUNC_svprfb:
    case FUNC_svprfd:
    case FUNC_svprfh:
    case FUNC_svprfw:
      return expand_prf ();

    case FUNC_svprfb_gather:
    case FUNC_svprfd_gather:
    case FUNC_svprfh_gather:
    case FUNC_svprfw_gather:
      return expand_prf_gather ();

    case FUNC_svptest_any:
      return expand_ptest (NE);

    case FUNC_svptest_first:
      return expand_ptest (LT);

    case FUNC_svptest_last:
      return expand_ptest (LTU);

    case FUNC_svptrue:
      return expand_ptrue ();

    case FUNC_svptrue_pat:
      return expand_ptrue_pat ();

    case FUNC_svqadd:
      return expand_qadd ();

    case FUNC_svqdecb:
    case FUNC_svqdecb_pat:
    case FUNC_svqdecd:
    case FUNC_svqdecd_pat:
    case FUNC_svqdech:
    case FUNC_svqdech_pat:
    case FUNC_svqdecw:
    case FUNC_svqdecw_pat:
      return expand_qinc_qdec (SS_MINUS, US_MINUS);

    case FUNC_svqdecp:
      return expand_qincp_qdecp (SS_MINUS, US_MINUS);

    case FUNC_svqincb:
    case FUNC_svqincb_pat:
    case FUNC_svqincd:
    case FUNC_svqincd_pat:
    case FUNC_svqinch:
    case FUNC_svqinch_pat:
    case FUNC_svqincw:
    case FUNC_svqincw_pat:
      return expand_qinc_qdec (SS_PLUS, US_PLUS);

    case FUNC_svqincp:
      return expand_qincp_qdecp (SS_PLUS, US_PLUS);

    case FUNC_svqsub:
      return expand_qsub ();

    case FUNC_svrbit:
      return expand_rbit ();

    case FUNC_svrdffr:
      return expand_rdffr ();

    case FUNC_svrev:
      return expand_rev ();

    case FUNC_svsel:
      return expand_sel ();

    case FUNC_svset2:
    case FUNC_svset3:
    case FUNC_svset4:
      return expand_set ();

    case FUNC_svsetffr:
      return expand_setffr ();

    case FUNC_svsplice:
      return expand_splice ();

    case FUNC_svsqrt:
      return expand_sqrt ();

    case FUNC_svst1:
      return expand_st1 ();

    case FUNC_svst1_scatter:
      return expand_st1_scatter ();

    case FUNC_svst1b:
    case FUNC_svst1h:
    case FUNC_svst1w:
      return expand_st1_trunc ();

    case FUNC_svst1b_scatter:
    case FUNC_svst1h_scatter:
    case FUNC_svst1w_scatter:
      return expand_st1_scatter_trunc ();

    case FUNC_svst2:
    case FUNC_svst3:
    case FUNC_svst4:
      return expand_st234 ();

    case FUNC_svstnt1:
      return expand_stnt1 ();

    case FUNC_svsub:
      return expand_sub (false);

    case FUNC_svsubr:
      return expand_sub (true);

    case FUNC_svtbl:
      return expand_tbl ();

    case FUNC_svtrn1:
      return expand_permute (UNSPEC_TRN1);

    case FUNC_svtrn2:
      return expand_permute (UNSPEC_TRN2);

    case FUNC_svundef:
    case FUNC_svundef2:
    case FUNC_svundef3:
    case FUNC_svundef4:
      return expand_undef ();

    case FUNC_svuzp1:
      return expand_permute (UNSPEC_UZP1);

    case FUNC_svuzp2:
      return expand_permute (UNSPEC_UZP2);

    case FUNC_svwhilele:
      return expand_while (UNSPEC_WHILE_LE, UNSPEC_WHILE_LS);

    case FUNC_svwhilelt:
      return expand_while (UNSPEC_WHILE_LT, UNSPEC_WHILE_LO);

    case FUNC_svwrffr:
      return expand_wrffr ();

    case FUNC_svzip1:
      return expand_permute (UNSPEC_ZIP1);

    case FUNC_svzip2:
      return expand_permute (UNSPEC_ZIP2);
    }
  gcc_unreachable ();
}

/* Expand a call to svabd.  */
rtx
function_expander::expand_abd ()
{
  return expand_signed_pred_op (UNSPEC_SABD, UNSPEC_UABD, UNSPEC_COND_FABD);
}

/* Expand a call to svabs.  */
rtx
function_expander::expand_abs ()
{
  return expand_pred_op (ABS, UNSPEC_COND_FABS);
}

/* Expand a call to svac{ge,gt,le,lt}.  UNSPEC_CODE is the unspec code
   for the comparison.  */
rtx
function_expander::expand_ac (int unspec_code)
{
  insn_code icode = code_for_aarch64_pred_fac (unspec_code, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svadd, or svsub(r) with a negated operand.
   MERGE_ARGNO is the argument that should be used as the fallback
   value in a merging operation.  */
rtx
function_expander::expand_add (unsigned int merge_argno)
{
  if (m_fi.pred == PRED_x)
    {
      if (type_suffixes[m_fi.types[0]].integer_p)
	return expand_via_unpred_direct_optab (add_optab);
      else
	{
	  /* Try to take advantage of unpredicated FP addition, rather than
	     simply treating _x as _m.  */
	  insn_code icode = code_for_aarch64_pred_add (get_mode (0));
	  return expand_via_pred_x_insn (icode);
	}
    }
  return expand_via_pred_direct_optab (cond_add_optab, merge_argno);
}

/* Expand a call to svadda.  */
rtx
function_expander::expand_adda ()
{
  insn_code icode = code_for_aarch64_pred_fold_left_plus (get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svadrb.  */
rtx
function_expander::expand_adrb ()
{
  return expand_via_exact_insn (code_for_aarch64_adr (GET_MODE (m_args[0])));
}

/* Expand a call to svadr[hwd].  */
rtx
function_expander::expand_adrhwd ()
{
  machine_mode mode = GET_MODE (m_args[0]);

  /* Turn the access size into an extra shift argument.  */
  unsigned int bytes = GET_MODE_UNIT_SIZE (m_fi.bhwd_scalar_mode ());
  rtx shift = gen_int_mode (exact_log2 (bytes), GET_MODE_INNER (mode));
  m_args.quick_push (expand_vector_broadcast (mode, shift));

  return expand_via_exact_insn (code_for_aarch64_adr_shift (mode));
}

/* Expand a call to svand.  Make the _m forms merge with argument
   MERGE_ARGNO.  */
rtx
function_expander::expand_and (unsigned int merge_argno)
{
  if (m_fi.types[0] == TYPE_SUFFIX_b)
    return expand_via_exact_insn (CODE_FOR_aarch64_pred_andvnx16bi_z);
  else if (m_fi.pred == PRED_x)
    return expand_via_unpred_direct_optab (and_optab);
  else
    return expand_via_pred_direct_optab (cond_and_optab, merge_argno);
}

/* Expand a call to svasrd.  */
rtx
function_expander::expand_asrd ()
{
  return expand_pred_shift_right_imm (code_for_cond_asrd (get_mode (0)));
}

/* Expand a call to svbrk[ab].  UNSPEC_CODE is the unspec code for the
   operation.  */
rtx
function_expander::expand_brk_unary (int unspec_code)
{
  return expand_via_pred_insn (code_for_aarch64_brk (unspec_code));
}

/* Expand a call to svbrk{n,pa,pb}.  UNSPEC_CODE is the unspec code for the
   operation.  */
rtx
function_expander::expand_brk_binary (int unspec_code)
{
  return expand_via_exact_insn (code_for_aarch64_brk (unspec_code));
}

/* Expand a call to svbic.  */
rtx
function_expander::expand_bic ()
{
  if (CONST_INT_P (m_args[2]))
    {
      machine_mode mode = GET_MODE_INNER (get_mode (0));
      m_args[2] = simplify_unary_operation (NOT, mode, m_args[2], mode);
      return expand_and ();
    }

  if (m_fi.types[0] == TYPE_SUFFIX_b)
    return expand_via_exact_insn (CODE_FOR_aarch64_pred_bicvnx16bi_z);
  else if (m_fi.pred == PRED_x)
    {
      insn_code icode = code_for_aarch64_bic (get_mode (0));
      return expand_via_unpred_insn (icode);
    }
  else
    {
      insn_code icode = code_for_cond_bic (get_mode (0));
      return expand_via_pred_insn (icode);
    }
}

/* Expand a call to svcadd.  */
rtx
function_expander::expand_cadd ()
{
  int rot = get_immediate_rotate (3, false);
  m_args.ordered_remove (3);
  int unspec_code = rot == 90 ? UNSPEC_COND_FCADD90 : UNSPEC_COND_FCADD270;
  return expand_pred_op (UNKNOWN, unspec_code);
}

/* Expand a call to svcmla.  */
rtx
function_expander::expand_cmla ()
{
  int rot = get_immediate_rotate (4, true);
  m_args.ordered_remove (4);
  int unspec_code = (rot == 0 ? UNSPEC_COND_FCMLA
		     : rot == 90 ? UNSPEC_COND_FCMLA90
		     : rot == 180 ? UNSPEC_COND_FCMLA180
		     : UNSPEC_COND_FCMLA270);

  /* Make the operand order the same as the one used by the fma optabs,
     with the accumulator last.  */
  rotate_inputs_left (1, 4);
  return expand_pred_op (UNKNOWN, unspec_code, 3);
}

/* Expand a call to svcmla.  */
rtx
function_expander::expand_cmla_lane ()
{
  require_immediate_lane_index (3, 2);

  int rot = get_immediate_rotate (4, true);
  m_args.ordered_remove (4);
  int unspec_code = (rot == 0 ? UNSPEC_FCMLA
		     : rot == 90 ? UNSPEC_FCMLA90
		     : rot == 180 ? UNSPEC_FCMLA180
		     : UNSPEC_FCMLA270);

  /* Make the operand order the same as the one used by the fma optabs,
     with the accumulator last.  */
  rotate_inputs_left (0, 4);
  insn_code icode = code_for_aarch64_lane (unspec_code, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svcmp*.  CODE_FOR_SINT and CODE_FOR_UINT are the rtx
   comparison codes for signed and unsigned operands respectively.
   UNSPEC_FOR_SINT and UNSPEC_FOR_UINT are the corresponding unspec
   codes for svcmp*_wide, with UNSPEC_FOR_SINT doubling as the unspec
   code for predicated floating-point comparisons.  */
rtx
function_expander::expand_cmp (rtx_code code_for_sint,
			       rtx_code code_for_uint,
			       int unspec_for_sint,
			       int unspec_for_uint)
{
  machine_mode mode = get_mode (0);
  scalar_mode elt_mode = GET_MODE_INNER (mode);
  if (type_suffixes[m_fi.types[0]].integer_p)
    {
      rtx_code code = (type_suffixes[m_fi.types[0]].unsigned_p
		       ? code_for_uint
		       : code_for_sint);
      insn_code icode = code_for_aarch64_pred_cmp (code, mode);
      if (m_fi.mode == MODE_n && mode != VNx2DImode)
	{
	  if (!CONSTANT_P (m_args[2])
	      || !insn_data[icode].operand[3].predicate (m_args[2], DImode))
	    return expand_cmp_wide (unspec_for_sint, unspec_for_uint);

	  m_args[2] = lowpart_subreg (elt_mode, m_args[2], DImode);
	}
      return expand_via_exact_insn (icode);
    }
  else
    {
      insn_code icode = code_for_aarch64_pred_fcm (unspec_for_sint, mode);
      return expand_via_exact_insn (icode);
    }
}

/* Expand a call to svcmp*_wide.  UNSPEC_FOR_SINT and UNSPEC_FOR_UINT
   are the unspec codes for signed and unsigned operands respectively.  */
rtx
function_expander::expand_cmp_wide (int unspec_for_sint, int unspec_for_uint)
{
  int unspec = (type_suffixes[m_fi.types[0]].unsigned_p
		? unspec_for_uint
		: unspec_for_sint);
  insn_code icode = code_for_aarch64_pred_cmp_wide (unspec, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svcmpuo.  */
rtx
function_expander::expand_cmpuo ()
{
  return expand_via_exact_insn (code_for_aarch64_fcmuo_and (get_mode (0)));
}

/* Expand a call to svcnt[bhwd].  */
rtx
function_expander::expand_cnt_bhwd ()
{
  return gen_int_mode (GET_MODE_NUNITS (m_fi.bhwd_vector_mode ()), DImode);
}

/* Expand a call to svcnt[bhwd]_pat.  */
rtx
function_expander::expand_cnt_bhwd_pat ()
{
  require_immediate_svpattern (0);
  m_args.quick_push (gen_int_mode (m_fi.bhwd_nelts_per_vq (), DImode));
  m_args.quick_push (const1_rtx);
  return expand_via_exact_insn (CODE_FOR_aarch64_sve_cnt_pat);
}

/* Expand a call to svcntp.  */
rtx
function_expander::expand_cntp ()
{
  machine_mode mode = get_mode (0);
  /* Add a hint operand to say whether the gp is a ptrue.  */
  bool ptrue_p = gen_lowpart (mode, m_args[0]) == CONSTM1_RTX (mode);
  m_args.quick_push (ptrue_p ? const1_rtx : const0_rtx);
  return expand_via_exact_insn (code_for_aarch64_pred_cntp (mode));
}

/* Expand a call to svcompact.  */
rtx
function_expander::expand_compact ()
{
  return expand_via_exact_insn (code_for_aarch64_sve_compact (get_mode (0)));
}

/* Expand a call to svcreate*.  */
rtx
function_expander::expand_create ()
{
  machine_mode tuple_mode = TYPE_MODE (TREE_TYPE (m_exp));

  if (!m_target || !REG_P (m_target) || overlaps_input_p (m_target))
    m_target = gen_reg_rtx (tuple_mode);

  for (unsigned int i = 0; i < m_args.length (); ++i)
    {
      rtx piece = simplify_gen_subreg (GET_MODE (m_args[i]),
				       m_target, tuple_mode,
				       i * BYTES_PER_SVE_VECTOR);
      emit_move_insn (piece, m_args[i]);
    }

  return m_target;
}

/* Expand a call to svdiv.  */
rtx
function_expander::expand_div (bool reversed_p)
{
  unsigned int merge_argno = 1;
  if (reversed_p)
    {
      std::swap (m_args[1], m_args[2]);
      merge_argno = 2;
    }

  return expand_signed_pred_op (DIV, UDIV, UNSPEC_COND_DIV, merge_argno);
}

/* Expand a call to svdot.  */
rtx
function_expander::expand_dot ()
{
  /* In the optab, the multiplication operands come before the
     accumulator operand.  */
  rotate_inputs_left (0, 3);
  machine_mode mode = get_quarter_mode (0);
  if (type_suffixes[m_fi.types[0]].unsigned_p)
    return expand_via_unpred_direct_optab (udot_prod_optab, mode);
  else
    return expand_via_unpred_direct_optab (sdot_prod_optab, mode);
}

/* Expand a call to svdot_lane.  */
rtx
function_expander::expand_dot_lane ()
{
  require_immediate_lane_index (3, 4);
  /* Use the same ordering as the optab, with the accumulator last.  */
  rotate_inputs_left (0, 4);
  int unspec_code = (type_suffixes[m_fi.types[0]].unsigned_p
		     ? UNSPEC_UDOT : UNSPEC_SDOT);
  insn_code icode = code_for_aarch64_dot_prod_lane (unspec_code, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svdup.  */
rtx
function_expander::expand_dup ()
{
  if (m_fi.pred == PRED_none
      || m_fi.pred == PRED_x)
    return expand_via_unpred_direct_optab (vec_duplicate_optab);
  else
    {
      insn_code icode;
      machine_mode mode = get_mode (0);
      if (valid_for_const_vector_p (GET_MODE_INNER (mode), m_args.last ()))
	icode = code_for_vcond_mask (get_mode (0), get_mode (0));
      else
	icode = code_for_aarch64_sel_dup (get_mode (0));
      return expand_via_sel_insn (icode);
    }
}

/* Expand a call to svdup_lane.  */
rtx
function_expander::expand_dup_lane ()
{
  machine_mode mode = get_mode (0);
  if (CONST_INT_P (m_args[1])
      && IN_RANGE (INTVAL (m_args[1]) * GET_MODE_UNIT_SIZE (mode), 0, 63))
    return expand_via_exact_insn (code_for_aarch64_sve_dup_lane (mode));
  return expand_tbl ();
}

/* Expand a call to svdup.  */
rtx
function_expander::expand_dupq ()
{
  machine_mode mode = get_mode (0);
  unsigned int nelts_per_vq = m_args.length ();
  if (GET_MODE_CLASS (mode) == MODE_VECTOR_BOOL)
    {
      /* Construct a vector of integers so that we can compare them against
	 zero below.  Zero vs. nonzero is the only distinction that
	 matters.  */
      mode = aarch64_sve_int_mode (mode);
      for (unsigned int i = 0; i < nelts_per_vq; ++i)
	m_args[i] = simplify_gen_unary (ZERO_EXTEND, GET_MODE_INNER (mode),
					m_args[i], QImode);
    }

  /* Get the 128-bit Advanced SIMD vector for this data size.  */
  scalar_mode elt_mode = GET_MODE_INNER (mode);
  machine_mode vq_mode = aarch64_vq_mode (elt_mode).require ();
  gcc_assert (known_eq (nelts_per_vq, GET_MODE_NUNITS (vq_mode)));

  /* Put the arguments into a 128-bit Advanced SIMD vector.  We want
     argument N to go into architectural lane N, whereas Advanced SIMD
     vectors are loaded memory lsb to register lsb.  We therefore need
     to reverse the elements for big-endian targets.  */
  rtx tmp = gen_reg_rtx (vq_mode);
  rtvec vec = rtvec_alloc (nelts_per_vq);
  for (unsigned int i = 0; i < nelts_per_vq; ++i)
    RTVEC_ELT (vec, i) = m_args[BYTES_BIG_ENDIAN ? nelts_per_vq - i - 1 : i];
  aarch64_expand_vector_init (tmp, gen_rtx_PARALLEL (vq_mode, vec));

  /* Build the SVE data vector.  */
  rtx res = aarch64_expand_sve_dupq (m_target, mode, tmp);

  /* If the result is a boolean, compare the data vector against zero.  */
  if (mode != get_mode (0))
    res = aarch64_convert_sve_data_to_pred (m_target, get_mode (0), res);
  return res;
}

/* Expand a call to svdupq_lane.  */
rtx
function_expander::expand_dupq_lane ()
{
  machine_mode mode = get_mode (0);
  rtx index = m_args[1];
  if (CONST_INT_P (index) && IN_RANGE (INTVAL (index), 0, 3))
    {
      /* Use the .Q form of DUP.  */
      insn_code icode = code_for_aarch64_sve_dupq_lane (mode);
      add_output_operand (icode);
      add_input_operand (icode, m_args[0]);
      add_fixed_operand (prepare_dupq_lane_index (INTVAL (index)));
      return generate_insn (icode);
    }

  /* Build a .D TBL index for the pairs of doublewords that we want to
     duplicate.  */
  if (CONST_INT_P (index))
    {
      /* The index vector is a constant.  */
      rtx_vector_builder builder (VNx2DImode, 2, 1);
      builder.quick_push (gen_int_mode (INTVAL (index) * 2, DImode));
      builder.quick_push (gen_int_mode (INTVAL (index) * 2 + 1, DImode));
      index = builder.build ();
    }
  else
    {
      /* Duplicate INDEX * 2 to fill a DImode vector.  */
      index = force_reg (DImode, simplify_gen_binary (ASHIFT, DImode,
						      index, const1_rtx));
      index = expand_vector_broadcast (VNx2DImode, index);

      /* Get an alternating 0, 1 predicate.  */
      rtx_vector_builder builder (VNx2BImode, 2, 1);
      builder.quick_push (const0_rtx);
      builder.quick_push (constm1_rtx);
      rtx pg = force_reg (VNx2BImode, builder.build ());

      /* Add one to the odd elements of the index.  */
      rtx one = force_reg (VNx2DImode, CONST1_RTX (VNx2DImode));
      rtx target = gen_reg_rtx (VNx2DImode);
      emit_insn (gen_cond_addvnx2di (target, pg, index, one, index));
      index = target;
    }

  insn_code icode = CODE_FOR_aarch64_sve_tblvnx2di;
  add_output_operand (icode);
  add_input_operand (icode, gen_lowpart (VNx2DImode, m_args[0]));
  add_input_operand (icode, index);
  return generate_insn (icode);
}

/* Expand a call to sveor.  */
rtx
function_expander::expand_eor ()
{
  if (m_fi.types[0] == TYPE_SUFFIX_b)
    return expand_via_exact_insn (CODE_FOR_aarch64_pred_xorvnx16bi_z);
  else if (m_fi.pred == PRED_x)
    return expand_via_unpred_direct_optab (xor_optab);
  else
    return expand_via_pred_direct_optab (cond_xor_optab);
}

/* Expand a call to svext.  */
rtx
function_expander::expand_ext ()
{
  require_immediate_extract_index (2);
  return expand_via_exact_insn (code_for_aarch64_sve_ext (get_mode (0)));
}

/* Expand a call to svext[bhw].  */
rtx
function_expander::expand_ext_bhw ()
{
  scalar_mode suffix_mode = m_fi.bhwd_scalar_mode ();
  if (type_suffixes[m_fi.types[0]].unsigned_p)
    {
      /* Convert to an AND.  */
      m_args.quick_push (GEN_INT (GET_MODE_MASK (suffix_mode)));
      if (m_fi.pred == PRED_m)
	/* We now have arguments "(inactive, pg, op, mask)".  Convert this
	   to "(pg, op, mask, inactive)" so that the order matches svand_m
	   with an extra argument on the end.  Take the inactive elements
	   from this extra argument.  */
	rotate_inputs_left (0, 4);
      return expand_and (3);
    }

  machine_mode wide_mode = get_mode (0);
  poly_uint64 nunits = GET_MODE_NUNITS (wide_mode);
  machine_mode narrow_mode
    = aarch64_sve_data_mode (suffix_mode, nunits).require ();
  if (m_fi.pred == PRED_x)
    {
      insn_code icode = code_for_aarch64_pred_sxt (wide_mode, narrow_mode);
      return expand_via_pred_x_insn (icode);
    }
  else
    {
      insn_code icode = code_for_aarch64_cond_sxt (wide_mode, narrow_mode);
      return expand_via_pred_insn (icode);
    }
}

/* Expand a call to svget[234].  */
rtx
function_expander::expand_get ()
{
  return simplify_gen_subreg (get_mode (0), m_args[0], GET_MODE (m_args[0]),
			      INTVAL (m_args[1]) * BYTES_PER_SVE_VECTOR);
}

/* Expand a call to svindex.  */
rtx
function_expander::expand_index ()
{
  return expand_via_unpred_direct_optab (vec_series_optab);
}

/* Expand a call to svinsr.  */
rtx
function_expander::expand_insr ()
{
  insn_code icode = direct_optab_handler (vec_shl_insert_optab, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svld1.  */
rtx
function_expander::expand_ld1 ()
{
  machine_mode mode = get_mode (0);
  machine_mode pred_mode = get_pred_mode (0);
  insn_code icode = convert_optab_handler (maskload_optab, mode, pred_mode);
  return expand_via_load_insn (icode);
}

/* Expand a call to svld1s[bhw] or svld1u[bhw].  CODE is the type of
   extension, either SIGN_EXTEND or ZERO_EXTEND.  */
rtx
function_expander::expand_ld1_ext (rtx_code code)
{
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = code_for_aarch64_load (code, reg_mode, mem_mode);
  return expand_via_load_insn (icode);
}

/* Expand a call to svld1s[bhw]_gather or svld1u[bhw]_gather.  CODE is
   the type of extension, either SIGN_EXTEND or ZERO_EXTEND.  */
rtx
function_expander::expand_ld1_ext_gather (rtx_code code)
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 5);
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = code_for_aarch64_gather_load (code, reg_mode, mem_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svld1_gather.  */
rtx
function_expander::expand_ld1_gather ()
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 5);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = direct_optab_handler (mask_gather_load_optab, mem_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svld1rq.  */
rtx
function_expander::expand_ld1rq ()
{
  return expand_via_load_insn (code_for_aarch64_sve_ld1rq (get_mode (0)));
}

/* Expand a call to svld[234].  */
rtx
function_expander::expand_ld234 ()
{
  machine_mode tuple_mode = TYPE_MODE (TREE_TYPE (m_exp));
  insn_code icode = convert_optab_handler (vec_mask_load_lanes_optab,
					   tuple_mode, get_mode (0));
  return expand_via_load_insn (icode);
}

/* Expand a call to svldff1 or svldnf1; UNSPEC_CODE is UNSPEC_LDFF1 for
   the former and UNSPEC_LDNF1 for the latter.  */
rtx
function_expander::expand_ldff1 (int code)
{
  emit_insn (gen_aarch64_update_ffr_for_load ());
  return expand_via_load_insn (code_for_aarch64_ldf1 (code, get_mode (0)));
}

/* Expand a call to svldff1[su][bhw] or svldnf1[su][bhw]; UNSPEC_CODE is
   UNSPEC_LDFF1 for the former and UNSPEC_LDNF1 for the latter.  EXT_CODE
   is the type of extension, either SIGN_EXTEND or ZERO_EXTEND.  */
rtx
function_expander::expand_ldff1_ext (rtx_code ext_code, int unspec_code)
{
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  emit_insn (gen_aarch64_update_ffr_for_load ());
  insn_code icode = code_for_aarch64_ldf1 (unspec_code, ext_code,
					   reg_mode, mem_mode);
  return expand_via_load_insn (icode);
}

/* Expand a call to svldff1s[bhw]_gather or svldff1u[bhw]_gather.  CODE is
   the type of extension, either SIGN_EXTEND or ZERO_EXTEND.  */
rtx
function_expander::expand_ldff1_ext_gather (rtx_code code)
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 5);
  emit_insn (gen_aarch64_update_ffr_for_load ());
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = code_for_aarch64_ldff1_gather (code, reg_mode, mem_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svldff1_gather.  */
rtx
function_expander::expand_ldff1_gather ()
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 5);
  emit_insn (gen_aarch64_update_ffr_for_load ());
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  return expand_via_exact_insn (code_for_aarch64_ldff1_gather (mem_mode));
}

/* Expand a call to svldnt1.  */
rtx
function_expander::expand_ldnt1 ()
{
  return expand_via_load_insn (code_for_aarch64_ldnt1 (get_mode (0)));
}

/* Expand a call to svmad.
   svmad (pg, a, b, c) maps directly to cond_fma_optab and aarch64_pred_fma
   with the same operand order:
   op2 -> a
   op3 -> b
   op4 -> c
   and then:
   op5 -> a
   MERGE_ARGNO is the argument that should be used as the fallback
   value in a merging operation.  */
rtx
function_expander::expand_mad (unsigned int merge_argno)
{
  if (m_fi.pred == PRED_x)
    {
      insn_code icode;
      if (type_suffixes[m_fi.types[0]].integer_p)
	icode = code_for_aarch64_pred_fma (get_mode (0));
      else
	icode = code_for_aarch64_pred (UNSPEC_COND_FMLA, get_mode (0));
      return expand_via_pred_x_insn (icode);
    }
  else
    return expand_via_pred_direct_optab (cond_fma_optab, merge_argno);
}

/* Expand a call to svmax.  */
rtx
function_expander::expand_max ()
{
  return expand_signed_pred_op (SMAX, UMAX, UNSPEC_COND_FMAX);
}

/* Expand a call to svmaxnm.  */
rtx
function_expander::expand_maxnm ()
{
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FMAXNM);
}

/* Expand a call to svmin.  */
rtx
function_expander::expand_min ()
{
  return expand_signed_pred_op (SMIN, UMIN, UNSPEC_COND_FMIN);
}

/* Expand a call to svminnm.  */
rtx
function_expander::expand_minnm ()
{
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FMINNM);
}

/* Expand a call to svmla.
   svmla (pg, a, b, c) reorders the operands so that the
   accumulator comes last, and then uses cond_fma_optab/aarch64_pred_fma:
   op2 -> b
   op3 -> c
   op4 -> a
   and then:
   op5 -> a.  */
rtx
function_expander::expand_mla ()
{
  rotate_inputs_left (1, 4);
  return expand_mad (3);
}

/* Expand a call to svmla_lane or svmls_lane; UNSPEC_CODE says which.  */
rtx
function_expander::expand_mla_mls_lane (int unspec_code)
{
  require_immediate_lane_index (3);
  /* Put the operands in the normal (fma ...) order, with the accumulator
     last.  This fits naturally since that's also the unprinted operand
     in the asm output.  */
  rotate_inputs_left (0, 4);
  insn_code icode = code_for_aarch64_lane (unspec_code, get_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svmls.
   svmls (pg, a, b, c) reorders the operands so that the
   accumulator comes last, and then uses cond_fnma_optab/aacrh64_pred_fnma:
   op2 -> b
   op3 -> c
   op4 -> a
   and then:
   op5 -> a.  */
rtx
function_expander::expand_mls ()
{
  rotate_inputs_left (1, 4);
  return expand_msb (3);
}

/* Expand a call to svmsb.
   svmsb (pg, a, b, c) maps directly to cond_fnma_optab and aarch64_pred_fnma
   with the same operand order:
   op2 -> a
   op3 -> b
   op4 -> c
   and then:
   op5 -> a
   MERGE_ARGNO is the argument that should be used as the fallback
   value in a merging operation.  */
rtx
function_expander::expand_msb (unsigned int merge_argno)
{
  if (m_fi.pred == PRED_x)
    {
      insn_code icode;
      if (type_suffixes[m_fi.types[0]].integer_p)
	icode = code_for_aarch64_pred_fnma (get_mode (0));
      else
	icode = code_for_aarch64_pred (UNSPEC_COND_FMLS, get_mode (0));
      return expand_via_pred_x_insn (icode);
    }
  else
    return expand_via_pred_direct_optab (cond_fnma_optab, merge_argno);
}

/* Expand a call to svmul.  */
rtx
function_expander::expand_mul ()
{
  return expand_pred_op (MULT, UNSPEC_COND_MUL);
}

/* Expand a call to svmul_lane.  */
rtx
function_expander::expand_mul_lane ()
{
  require_immediate_lane_index (2);
  return expand_via_exact_insn (code_for_aarch64_mul_lane (get_mode (0)));
}

/* Expand a call to svmulh.  */
rtx
function_expander::expand_mulh ()
{
  return expand_signed_pred_op (UNSPEC_SMUL_HIGHPART, UNSPEC_UMUL_HIGHPART, 0);
}

/* Expand a call to svmulx.  */
rtx
function_expander::expand_mulx ()
{
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FMULX);
}

/* Expand a call to svnand.  */
rtx
function_expander::expand_nand ()
{
  return expand_via_exact_insn (CODE_FOR_aarch64_pred_nandvnx16bi_z);
}

/* Expand a call to svneg.  */
rtx
function_expander::expand_neg ()
{
  return expand_pred_op (NEG, UNSPEC_COND_FNEG);
}

/* Expand a call to svnmad.  */
rtx
function_expander::expand_nmad ()
{
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FNMLA);
}

/* Expand a call to svnmla.  */
rtx
function_expander::expand_nmla ()
{
  /* Put the accumulator at the end (argument 3), but keep it as the
     merge input for _m functions.  */
  rotate_inputs_left (1, 4);
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FNMLA, 3);
}

/* Expand a call to svnmls.  */
rtx
function_expander::expand_nmls ()
{
  /* Put the accumulator at the end (argument 3), but keep it as the
     merge input for _m functions.  */
  rotate_inputs_left (1, 4);
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FNMLS, 3);
}

/* Expand a call to svnmsb.  */
rtx
function_expander::expand_nmsb ()
{
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FNMLS);
}

/* Expand a call to svnor.  */
rtx
function_expander::expand_nor ()
{
  return expand_via_exact_insn (CODE_FOR_aarch64_pred_norvnx16bi_z);
}

/* Expand a call to svnot.  */
rtx
function_expander::expand_not ()
{
  return expand_pred_op (NOT, -1);
}

/* Expand a call to sqadd.  */
rtx
function_expander::expand_qadd ()
{
  return expand_via_signed_unpred_insn (SS_PLUS, US_PLUS);
}

/* Expand a call to svqinc or svqdec.  CODE_FOR_SINT and CODE_FOR_UINT
   are the corresponding saturating signed and unsigned operations
   respectively.  */
rtx
function_expander::expand_qinc_qdec (rtx_code code_for_sint,
				     rtx_code code_for_uint)
{
  /* Check or create the three UNSPEC_SVE_CNT_PAT operands; see
     aarch64_sve_cnt_pat for details.  */
  if (m_fi.shape == SHAPE_inc_dec)
    {
      require_immediate_range (1, 1, 16);
      m_args.quick_insert (1, gen_int_mode (AARCH64_SV_ALL, DImode));
    }
  else
    {
      require_immediate_svpattern (1);
      require_immediate_range (2, 1, 16);
    }
  rtx nelts_per_vq = gen_int_mode (m_fi.bhwd_nelts_per_vq (), DImode);
  m_args.quick_insert (2, nelts_per_vq);

  rtx_code code = (type_suffixes[m_fi.types[0]].unsigned_p
		   ? code_for_uint
		   : code_for_sint);
  machine_mode mode = get_mode (0);
  if (m_fi.mode == MODE_n)
    mode = GET_MODE_INNER (mode);
  else if (mode == VNx8HImode)
    /* Add the subreg_lowpart_operator operand.  */
    m_args.quick_push (gen_rtx_TRUNCATE (HImode, NULL_RTX));

  insn_code icode = code_for_aarch64_sve_pat (code, mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svqincp or svqdecp.  CODE_FOR_SINT and CODE_FOR_UINT
   are the corresponding saturating signed and unsigned operations
   respectively.  */
rtx
function_expander::expand_qincp_qdecp (rtx_code code_for_sint,
				       rtx_code code_for_uint)
{
  /* svcntp is predicated but these functions aren't.  Hide the difference
     by creating an implicit ptrue gp.  */
  m_args.quick_insert (1, CONSTM1_RTX (VNx16BImode));

  machine_mode mode0 = get_mode (0);
  rtx_code code = (type_suffixes[m_fi.types[0]].unsigned_p
		   ? code_for_uint
		   : code_for_sint);
  insn_code icode;
  if (m_fi.mode == MODE_n)
    {
      mode0 = GET_MODE_INNER (mode0);
      icode = code_for_aarch64_sve_cntp (code, mode0, get_mode (1));
    }
  else
    {
      icode = code_for_aarch64_sve_cntp (code, mode0);
      if (mode0 == VNx8HImode)
	/* Add the subreg_lowpart_operator operand.  */
	m_args.quick_push (gen_rtx_TRUNCATE (HImode, NULL_RTX));
    }
  return expand_via_exact_insn (icode);
}

/* Expand a call to sqsub.  */
rtx
function_expander::expand_qsub ()
{
  return expand_via_signed_unpred_insn (SS_MINUS, US_MINUS);
}

/* Expand a call to svorn.  */
rtx
function_expander::expand_orn ()
{
  return expand_via_exact_insn (CODE_FOR_aarch64_pred_ornvnx16bi_z);
}

/* Expand a call to svorr.  */
rtx
function_expander::expand_orr ()
{
  if (m_fi.types[0] == TYPE_SUFFIX_b)
    return expand_via_exact_insn (CODE_FOR_aarch64_pred_iorvnx16bi_z);
  else if (m_fi.pred == PRED_x)
    return expand_via_unpred_direct_optab (ior_optab);
  else
    return expand_via_pred_direct_optab (cond_ior_optab);
}

/* Expand a call to a binary permute function such as svzip1.  UNSPEC_CODE is
   the associated unspec code.  */
rtx
function_expander::expand_permute (int unspec_code)
{
  machine_mode mode = get_mode (0);
  return expand_via_unpred_insn (code_for_aarch64_sve (unspec_code, mode));
}

/* Expand a call to svpfalse.  */
rtx
function_expander::expand_pfalse ()
{
  return CONST0_RTX (VNx16BImode);
}

/* Expand a call to svprf[bhwd].  */
rtx
function_expander::expand_prf ()
{
  prepare_prefetch_operands ();
  scalar_mode int_mode = m_fi.bhwd_scalar_mode ();
  machine_mode mode = aarch64_full_sve_mode (int_mode).require ();
  return expand_via_prefetch_insn (code_for_aarch64_sve_prefetch (mode));
}

/* Expand a call to svprf[bhwd]_gather.  */
rtx
function_expander::expand_prf_gather ()
{
  prepare_prefetch_operands ();
  prepare_gather_address_operands (1);

  machine_mode mem_mode = m_fi.memory_vector_mode ();
  machine_mode reg_mode = GET_MODE (m_args[2]);

  /* Insert a zero operand to identify the mode of the memory being
     accessed.  This goes between the gather operands and prefetch
     operands created above.  */
  m_args.quick_insert (5, CONST0_RTX (mem_mode));

  insn_code icode = code_for_aarch64_sve_gather_prefetch (mem_mode, reg_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svptest_*.  CODE is the comparison code for a
   true return.  */
rtx
function_expander::expand_ptest (rtx_code code)
{
  machine_mode mode;
  rtx is_ptrue = const0_rtx;
  if (aarch64_ptrue_all_mode (m_args[0]).exists (&mode))
    {
      is_ptrue = const1_rtx;
      m_args[1] = gen_lowpart (mode, m_args[1]);
    }

  rtx pg = force_reg (VNx16BImode, m_args[0]);
  rtx op = force_reg (GET_MODE (m_args[1]), m_args[1]);
  emit_insn (gen_aarch64_ptestvnx16bi (pg, pg, op, is_ptrue));

  rtx target = m_target;
  if (!target
      || !REG_P (target)
      || (GET_MODE (target) != SImode && GET_MODE (target) != DImode))
    target = gen_reg_rtx (DImode);

  rtx cc_reg = gen_rtx_REG (CC_NZCmode, CC_REGNUM);
  rtx compare = gen_rtx_fmt_ee (code, GET_MODE (target), cc_reg, const0_rtx);
  emit_insn (gen_rtx_SET (target, compare));
  return target;
}

/* Expand a call to svptrue.  */
rtx
function_expander::expand_ptrue ()
{
  return aarch64_ptrue_all (type_suffixes[m_fi.types[0]].elem_bytes);
}

/* Expand a call to svptrue_pat.  */
rtx
function_expander::expand_ptrue_pat ()
{
  require_immediate_svpattern (0);
  rtvec vec = gen_rtvec (2, m_args[0], CONST0_RTX (get_mode (0)));
  return gen_rtx_CONST (VNx16BImode,
			gen_rtx_UNSPEC (VNx16BImode, vec, UNSPEC_PTRUE));
}

/* Expand a call to svrbit.  */
rtx
function_expander::expand_rbit ()
{
  return expand_pred_op (UNSPEC_RBIT, -1);
}

/* Expand a call to svsel.  */
rtx
function_expander::expand_sel ()
{
  rotate_inputs_left (0, 3);
  insn_code icode = convert_optab_handler (vcond_mask_optab,
					   get_mode (0), get_pred_mode (0));
  return expand_via_exact_insn (icode);
}

/* Expand a call to svset[234].  */
rtx
function_expander::expand_set ()
{
  rtx tuple = m_args[0];
  unsigned int index = INTVAL (m_args[1]);
  rtx vector = m_args[2];

  if (!m_target
      || !REG_P (m_target)
      || reg_overlap_mentioned_p (m_target, vector))
    m_target = gen_reg_rtx (GET_MODE (tuple));
  emit_move_insn (m_target, tuple);

  rtx piece = simplify_gen_subreg (GET_MODE (vector),
				   m_target, GET_MODE (m_target),
				   index * BYTES_PER_SVE_VECTOR);
  emit_move_insn (piece, vector);
  return m_target;
}

/* Expand a call to svrdffr.  */
rtx
function_expander::expand_rdffr ()
{
  emit_insn (gen_aarch64_copy_ffr_to_ffrt ());
  rtx result = expand_via_exact_insn (m_fi.pred == PRED_z
				      ? CODE_FOR_aarch64_rdffr_z
				      : CODE_FOR_aarch64_rdffr);
  emit_insn (gen_aarch64_update_ffrt ());
  return result;
}

/* Expand a call to a vector-to-scalar reduction like svminv.
   UNSPEC_FOR_SINT, UNSPEC_FOR_UINT and UNSPEC_FOR_FP are the
   unspec codes for signed integer, unsigned integer, and floating-point
   reductions respectively, with -1 for unsupported operations.  */
rtx
function_expander::expand_reduction (int unspec_for_sint, int unspec_for_uint,
				     int unspec_for_fp)
{
  int unspec = (!type_suffixes[m_fi.types[0]].integer_p ? unspec_for_fp
		: type_suffixes[m_fi.types[0]].unsigned_p ? unspec_for_uint
		: unspec_for_sint);
  machine_mode mode = get_mode (0);
  /* There's no signed/unsigned distinction for 64-bit elements, so SADDV
     only exists for narrower elements.  */
  if (GET_MODE_UNIT_BITSIZE (mode) == 64 && unspec == UNSPEC_SADDV)
    unspec = UNSPEC_UADDV;
  return expand_via_exact_insn (code_for_aarch64_pred_reduc (unspec, mode));
}

/* Expand a call to svrev.  */
rtx
function_expander::expand_rev ()
{
  return expand_via_exact_insn (code_for_aarch64_sve_rev (get_mode (0)));
}

/* Expand a call to svsetffr.  */
rtx
function_expander::expand_setffr ()
{
  m_args.quick_push (CONSTM1_RTX (VNx16BImode));
  return expand_wrffr ();
}

/* Expand a call to sv{lsl,lsr,asr}.  CODE is the associated shift code
   and WIDE_UNSPEC is the unspec code for the wide variant.  */
rtx
function_expander::expand_shift (rtx_code code, int wide_unspec)
{
  machine_mode mode = get_mode (0);
  machine_mode elem_mode = GET_MODE_INNER (mode);

  if (m_fi.mode == MODE_n
      && mode != VNx2DImode
      && !aarch64_simd_shift_imm_p (m_args[2], elem_mode, code == ASHIFT))
    return expand_shift_wide (wide_unspec);

  if (m_fi.pred == PRED_x)
    return expand_via_pred_x_insn (code_for_aarch64_pred (code, mode));
  else
    return expand_via_pred_insn (code_for_cond (code, mode));
}

/* Expand a call to sv{lsl,lsr,asr}_wide.  UNSPEC is the associated
   unspec code.  */
rtx
function_expander::expand_shift_wide (int unspec)
{
  machine_mode mode = get_mode (0);
  if (m_fi.pred == PRED_x)
    return expand_via_unpred_insn (code_for_aarch64_wide (unspec, mode));
  else
    return expand_via_pred_insn (code_for_cond (unspec, mode));
}

/* Expand a call to svsplice.  */
rtx
function_expander::expand_splice ()
{
  return expand_via_exact_insn (code_for_aarch64_sve_splice (get_mode (0)));
}

/* Expand a call to svsqrt.  */
rtx
function_expander::expand_sqrt ()
{
  gcc_checking_assert (!type_suffixes[m_fi.types[0]].integer_p);
  return expand_pred_op (UNKNOWN, UNSPEC_COND_FSQRT);
}

/* Expand a call to svst1.  */
rtx
function_expander::expand_st1 ()
{
  machine_mode mode = get_mode (0);
  machine_mode pred_mode = get_pred_mode (0);
  insn_code icode = convert_optab_handler (maskstore_optab, mode, pred_mode);
  return expand_via_store_insn (icode);
}

/* Expand a call to svst1_scatter.  */
rtx
function_expander::expand_st1_scatter ()
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 6);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = direct_optab_handler (mask_scatter_store_optab, mem_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svst1[bhw]_scatter.  */
rtx
function_expander::expand_st1_scatter_trunc ()
{
  prepare_gather_address_operands (1);
  rotate_inputs_left (0, 6);
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = code_for_aarch64_scatter_store_trunc (mem_mode, reg_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svst1[bhw].  */
rtx
function_expander::expand_st1_trunc ()
{
  machine_mode reg_mode = get_mode (0);
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  insn_code icode = code_for_aarch64_store_trunc (mem_mode, reg_mode);
  return expand_via_store_insn (icode);
}

/* Expand a call to svst[234].  */
rtx
function_expander::expand_st234 ()
{
  machine_mode tuple_mode = GET_MODE (m_args.last ());
  insn_code icode = convert_optab_handler (vec_mask_store_lanes_optab,
					   tuple_mode, get_mode (0));
  return expand_via_store_insn (icode);
}

/* Expand a call to svstnt1.  */
rtx
function_expander::expand_stnt1 ()
{
  return expand_via_store_insn (code_for_aarch64_stnt1 (get_mode (0)));
}

/* Expand a call to svsub or svsubr; REVERSED_P says which.  */
rtx
function_expander::expand_sub (bool reversed_p)
{
  unsigned int merge_argno = 1;
  if (reversed_p)
    {
      std::swap (m_args[1], m_args[2]);
      merge_argno = 2;
    }

  machine_mode mode = get_mode (0);
  if (try_negating_argument (2, mode))
    return expand_add (merge_argno);

  if (m_fi.pred == PRED_x)
    {
      if (type_suffixes[m_fi.types[0]].integer_p)
	return expand_via_unpred_direct_optab (sub_optab);
      else
	{
	  /* Try to take advantage of unpredicated FP addition, rather than
	     simply treating _x as _m.  */
	  insn_code icode = code_for_aarch64_pred_sub (mode);
	  return expand_via_pred_x_insn (icode);
	}
    }
  return expand_via_pred_direct_optab (cond_sub_optab, merge_argno);
}

/* Expand a call to svtbl.  */
rtx
function_expander::expand_tbl ()
{
  return expand_via_exact_insn (code_for_aarch64_sve_tbl (get_mode (0)));
}

/* Expand a call to sv{cls,clz,cnt}; CODE says which.  */
rtx
function_expander::expand_unary_count (rtx_code code)
{
  return expand_pred_op (code, BITCAST_TO_INT);
}

/* Expand a call to svundef*.  */
rtx
function_expander::expand_undef ()
{
  emit_clobber (copy_rtx (m_target));
  return m_target;
}

/* Expand a call to svwhile*.  UNSPEC_FOR_SINT is the unspec code to
   use for signed integer inputs and UNSPEC_FOR_UINT is the unspec code
   to use for unsigned integer inputs.  */
rtx
function_expander::expand_while (int unspec_for_sint, int unspec_for_uint)
{
  int unspec = (type_suffixes[m_fi.types[1]].unsigned_p
		? unspec_for_uint
		: unspec_for_sint);
  machine_mode pred_mode = get_mode (0);
  scalar_mode reg_mode = GET_MODE_INNER (get_mode (1));
  insn_code icode = code_for_while (unspec, reg_mode, pred_mode);
  return expand_via_exact_insn (icode);
}

/* Expand a call to svwrffr.  */
rtx
function_expander::expand_wrffr ()
{
  return expand_via_exact_insn (CODE_FOR_aarch64_wrffr);
}

/* Implement the call using optab OP, which is an unpredicated direct
   (i.e. single-mode) optab.  MODE is the mode of the operation,
   or VOIDmode if the mode associated with type suffix 0 is correct.  */
rtx
function_expander::expand_via_unpred_direct_optab (optab op,
						   machine_mode mode)
{
  if (mode == VOIDmode)
    mode = get_mode (0);
  insn_code icode = direct_optab_handler (op, mode);
  return expand_via_unpred_insn (icode);
}

/* Implement the call using instruction ICODE, with a 1:1 mapping between
   arguments and operands.  */
rtx
function_expander::expand_via_exact_insn (insn_code icode)
{
  unsigned int nops = insn_data[icode].n_operands;
  if (!function_returns_void_p ())
    {
      add_output_operand (icode);
      nops -= 1;
    }
  for (unsigned int i = 0; i < nops; ++i)
    add_input_operand (icode, m_args[i]);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which loads memory operand 1
   into register operand 0 under the control of predicate operand 2.  */
rtx
function_expander::expand_via_load_insn (insn_code icode)
{
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  rtx base = get_contiguous_base (mem_mode);

  add_output_operand (icode);
  add_mem_operand (mem_mode, base);
  add_input_operand (icode, m_args[0]);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which prefetches from
   address operand 1 under the control of predicate operand 0.  */
rtx
function_expander::expand_via_prefetch_insn (insn_code icode)
{
  add_input_operand (icode, m_args[0]);
  add_address_operand (get_contiguous_base (VNx16QImode));
  for (unsigned int i = m_args.length () - 3; i < m_args.length (); ++i)
    add_input_operand (icode, m_args[i]);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which stores register operand 1
   into memory operand 0 under the control of predicate operand 2.  */
rtx
function_expander::expand_via_store_insn (insn_code icode)
{
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  rtx base = get_contiguous_base (mem_mode);

  add_mem_operand (mem_mode, base);
  add_input_operand (icode, m_args.last ());
  add_input_operand (icode, m_args[0]);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which does not use a
   governing predicate.  */
rtx
function_expander::expand_via_unpred_insn (insn_code icode)
{
  /* Can't drop the predicate for _z and _m.  */
  gcc_assert (m_fi.pred == PRED_x || m_fi.pred == PRED_none);
  /* Discount the output operand.  */
  unsigned int nops = insn_data[icode].n_operands - 1;
  /* Drop the predicate argument in the case of _x predication.  */
  unsigned int bias = (m_fi.pred == PRED_x ? 1 : 0);
  unsigned int i = 0;

  add_output_operand (icode);
  for (; i < nops; ++i)
    add_input_operand (icode, m_args[i + bias]);

  return generate_insn (icode);
}

/* Implement the call using optab OP, which is a predicated direct
   (i.e. single-mode) optab.  Merging forms of OP use argument MERGE_ARGNO
   as the fallback value.  */
rtx
function_expander::expand_via_pred_direct_optab (optab op,
						 unsigned int merge_argno)
{
  machine_mode mode = get_mode (0);
  insn_code icode = direct_optab_handler (op, mode);
  return expand_via_pred_insn (icode, merge_argno);
}

/* Implement the call using instruction ICODE, which is a select-like
   operation with the following operands:

   0: output
   1: true value
   2: false value
   3: predicate.  */
rtx
function_expander::expand_via_sel_insn (insn_code icode)
{
  machine_mode mode = get_mode (0);

  unsigned int opno = 0;
  rtx false_arg = get_fallback_value (mode, 1, 0, opno);
  rtx pred_arg = m_args[opno++];
  rtx true_arg = m_args[opno++];

  add_output_operand (icode);
  add_input_operand (icode, true_arg);
  add_input_operand (icode, false_arg);
  add_input_operand (icode, pred_arg);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which does the equivalent of:

     OUTPUT = COND ? FN (INPUTS) : FALLBACK;

   The operands are in the order above: OUTPUT, COND, INPUTS and FALLBACK.
   Merging forms use argument MERGE_ARGNO as the fallback value.  */
rtx
function_expander::expand_via_pred_insn (insn_code icode,
					 unsigned int merge_argno)
{
  /* Discount the output, predicate, and fallback value.  */
  unsigned int nops = insn_data[icode].n_operands - 3;
  machine_mode mode = insn_data[icode].operand[0].mode;

  unsigned int opno = 0;
  rtx fallback_arg = get_fallback_value (mode, nops, merge_argno, opno);
  rtx pred_arg = m_args[opno++];

  add_output_operand (icode);
  add_input_operand (icode, pred_arg);
  for (unsigned int i = 0; i < nops; ++i)
    add_input_operand (icode, m_args[opno + i]);
  add_input_operand (icode, fallback_arg);
  return generate_insn (icode);
}

/* Implement the call using instruction ICODE, which is a predicated
   operation that returns arbitrary values for inactive lanes.  */
rtx
function_expander::expand_via_pred_x_insn (insn_code icode)
{
  unsigned int nops = m_args.length () - 1;
  machine_mode mode = insn_data[icode].operand[0].mode;
  machine_mode pred_mode = get_pred_mode (0);

  /* Add the normal operands.  */
  add_output_operand (icode);
  add_input_operand (icode, m_args[0]);
  for (unsigned int i = 0; i < nops; ++i)
    add_input_operand (icode, m_args[i + 1]);

  /* Add a flag that indicates whether unpredicated instructions
     are allowed.  */
  rtx pred = m_ops[1].value;
  if (FLOAT_MODE_P (mode))
    {
      if (flag_trapping_math
	  && pred != CONST1_RTX (pred_mode))
	add_integer_operand (SVE_FORBID_NEW_FAULTS);
      else
	add_integer_operand (SVE_ALLOW_NEW_FAULTS);
    }

  return generate_insn (icode);
}

/* Like expand_signed_pred_op, except that CODE works for both signed
   and unsigned integers.  */
rtx
function_expander::expand_pred_op (rtx_code code, int unspec_cond,
				   unsigned int merge_argno)
{
  return expand_signed_pred_op (code, code, unspec_cond, merge_argno);
}

/* Like expand_signed_pred_op, except that UNSPEC_FOR_INT works for both
   signed and unsigned integers.  */
rtx
function_expander::expand_pred_op (int unspec_for_int, int unspec_for_fp)
{
  return expand_signed_pred_op (unspec_for_int, unspec_for_int,
				unspec_for_fp);
}

/* Implement the call using an @aarch64_cond instruction for _x
   predication and a @cond instruction for _z and _m predication.
   The integer instructions are parameterized by an rtx_code while
   the floating-point instructions are parameterized by an unspec code.
   CODE_FOR_SINT is the rtx_code for signed integer operations,
   CODE_FOR_UINT is the rtx_code for unsigned integer operations
   and UNSPEC_COND is the unspec code for floating-point operations.
   UNSPEC_COND can be BITCAST_TO_INT if floating-point types should
   be bitcast to integer types.

   MERGE_ARGNO is the argument that should be used as the fallback
   value in a merging operation.  */
rtx
function_expander::expand_signed_pred_op (rtx_code code_for_sint,
					  rtx_code code_for_uint,
					  int unspec_cond,
					  unsigned int merge_argno)
{
  insn_code icode;

  machine_mode mode = get_mode (0);
  bool integer_p = type_suffixes[m_fi.types[0]].integer_p;
  if (!integer_p && unspec_cond == BITCAST_TO_INT)
    {
      mode = mode_for_int_vector (mode).require ();
      for (unsigned int i = 0; i < m_args.length (); ++i)
	if (GET_MODE (m_args[i]) == get_mode (0))
	  m_args[i] = gen_lowpart (mode, m_args[i]);
      integer_p = true;
    }

  if (m_fi.pred == PRED_x)
    {
      if (integer_p)
	{
	  if (type_suffixes[m_fi.types[0]].unsigned_p)
	    icode = code_for_aarch64_pred (code_for_uint, mode);
	  else
	    icode = code_for_aarch64_pred (code_for_sint, mode);
	}
      else
	icode = code_for_aarch64_pred (unspec_cond, mode);
      return expand_via_pred_x_insn (icode);
    }
  else
    {
      if (integer_p)
	{
	  if (type_suffixes[m_fi.types[0]].unsigned_p)
	    icode = code_for_cond (code_for_uint, mode);
	  else
	    icode = code_for_cond (code_for_sint, mode);
	}
      else
	icode = code_for_cond (unspec_cond, mode);
      return expand_via_pred_insn (icode, merge_argno);
    }
}

/* Implement the call using an @aarch64_cond instruction for _x
   predication and a @cond instruction for _z and _m predication.
   The instructions are parameterized by an unspec.
   UNSPEC_FOR_SINT is the unspec code for signed integer operations,
   UNSPEC_FOR_UINT is the unspec code for unsigned integer operations
   and UNSPEC_FOR_FP is the unspec code for floating-point operations.  */
rtx
function_expander::expand_signed_pred_op (int unspec_for_sint,
					  int unspec_for_uint,
					  int unspec_for_fp)
{
  insn_code icode;

  if (m_fi.pred == PRED_x)
    {
      if (type_suffixes[m_fi.types[0]].integer_p)
	{
	  if (type_suffixes[m_fi.types[0]].unsigned_p)
	    icode = code_for_aarch64_pred (unspec_for_uint, get_mode (0));
	  else
	    icode = code_for_aarch64_pred (unspec_for_sint, get_mode (0));
	}
      else
	icode = code_for_aarch64_pred (unspec_for_fp, get_mode (0));
      return expand_via_pred_x_insn (icode);
    }
  else
    {
      if (type_suffixes[m_fi.types[0]].integer_p)
	{
	  if (type_suffixes[m_fi.types[0]].unsigned_p)
	    icode = code_for_cond (unspec_for_uint, get_mode (0));
	  else
	    icode = code_for_cond (unspec_for_sint, get_mode (0));
	}
      else
	icode = code_for_cond (unspec_for_fp, get_mode (0));
      return expand_via_pred_insn (icode);
    }
}

/* Implement the call using an @aarch64 instruction and the
   instructions are parameterized by an rtx_code.  CODE_FOR_SINT
   is the rtx_code for signed integer operations, CODE_FOR_UINT
   is the rtx_code for unsigned integer operations.  */
rtx
function_expander::expand_via_signed_unpred_insn (rtx_code code_for_sint,
						  rtx_code code_for_uint)
{
  insn_code icode;
  if (type_suffixes[m_fi.types[0]].unsigned_p)
    icode = code_for_aarch64 (code_for_uint, code_for_uint, get_mode (0));
  else
    icode = code_for_aarch64 (code_for_sint, code_for_sint, get_mode (0));
  return expand_via_unpred_insn (icode);
}

/* Expand a call to a SHAPE_shift_right_imm function using predicated
   instruction ICODE, which has the same operand order as conditional
   optabs like cond_add_optab.  */
rtx
function_expander::expand_pred_shift_right_imm (insn_code icode)
{
  require_immediate_range (2, 1, GET_MODE_UNIT_BITSIZE (get_mode (0)));
  return expand_via_pred_insn (icode);
}

/* Check that argument ARGNO, when multiplied by the number of bytes in
   an element of type suffix 0, is a constant integer in the range [0, 255].
   Report an appropriate error if it isn't and set the argument to zero.  */
void
function_expander::require_immediate_extract_index (unsigned int argno)
{
  unsigned int bytes = type_suffixes[m_fi.types[0]].elem_bytes;
  require_immediate_range (argno, 0, 256 / bytes - 1);
}

/* Check that argument ARGNO is suitable for indexing argument ARGNO - 1
   in groups of GROUP_SIZE elements.  Report an appropriate error if it
   isn't and set the argument to a safe in-range value.  */
void
function_expander::require_immediate_lane_index (unsigned int argno,
						 unsigned int group_size)
{
  machine_mode mode = GET_MODE (m_args[argno - 1]);
  gcc_assert (VECTOR_MODE_P (mode));
  unsigned int nlanes = 128 / (group_size * GET_MODE_UNIT_BITSIZE (mode));
  return require_immediate_range (argno, 0, nlanes - 1);
}

/* Require that argument ARGNO is a constant integer in the range
   [MIN, MAX].  Report an appropriate error if it isn't and set
   the argument to a safe in-range value.  */
void
function_expander::require_immediate_range (unsigned int argno,
					    HOST_WIDE_INT min,
					    HOST_WIDE_INT max)
{
  if (!CONST_INT_P (m_args[argno]))
    report_non_ice (m_location, m_rfn.decl, argno);
  else
    {
      HOST_WIDE_INT actual = INTVAL (m_args[argno]);
      if (IN_RANGE (actual, min, max))
	return;
      report_out_of_range (m_location, m_rfn.decl, argno, actual, min, max);
    }
  m_args[argno] = GEN_INT (min);
}

/* Check that argument ARGNO is an integer constant expression that
   specifies a rotation value, as a multiple of 90 degrees, and return
   its value.  Report an appropriate error if the argument isn't valid
   and return a safe value instead.

   90 and 270 are always valid; ALLOW_0_180_P says whether 0 and 180
   are too.  */
int
function_expander::get_immediate_rotate (unsigned int argno,
					 bool allow_0_180_p)
{
  if (!CONST_INT_P (m_args[argno]))
    report_non_ice (m_location, m_rfn.decl, argno);
  else
    {
      HOST_WIDE_INT actual = INTVAL (m_args[argno]);
      if (actual == 90 || actual == 270)
	return actual;
      if (!allow_0_180_p)
	report_neither_nor (m_location, m_rfn.decl, argno, actual, 90, 270);
      else if (actual == 0 || actual == 180)
	return actual;
      else
	report_not_one_of (m_location, m_rfn.decl, argno, actual,
			   0, 90, 180, 270);
    }
  return 90;
}

/* Check that argument ARGNO is an integer constant expression that
   has a valid svpattern value.  Report an error if it doesn't and
   change it to SV_ALL.  */
void
function_expander::require_immediate_svpattern (unsigned int argno)
{
  if (!CONST_INT_P (m_args[argno]))
    report_non_ice (m_location, m_rfn.decl, argno);
  else
    {
      HOST_WIDE_INT actual = INTVAL (m_args[argno]);
      if (aarch64_svpattern_immediate_p (actual))
	return;
      report_not_enum (m_location, m_rfn.decl, argno, actual, acle_svpattern);
    }
  m_args[argno] = gen_int_mode (AARCH64_SV_ALL, DImode);
}

/* Check that argument ARGNO is an integer constant expression that
   has a valid svprfop value.  Report an error if it doesn't and
   change it to SV_PLDL1KEEP.  */
void
function_expander::require_immediate_svprfop (unsigned int argno)
{
  if (!CONST_INT_P (m_args[argno]))
    report_non_ice (m_location, m_rfn.decl, argno);
  else
    {
      HOST_WIDE_INT actual = INTVAL (m_args[argno]);
      if (aarch64_svprfop_immediate_p (actual))
	return;
      report_not_enum (m_location, m_rfn.decl, argno, actual, acle_svprfop);
    }
  m_args[argno] = gen_int_mode (AARCH64_SV_PLDL1KEEP, DImode);
}

/* Return the base address for a contiguous load or store function.
   MODE is the mode of the addressed memory.  */
rtx
function_expander::get_contiguous_base (machine_mode mode)
{
  rtx base = m_args[1];
  if (m_fi.mode == MODE_vnum)
    {
      /* Use the size of the memory mode for extending loads and truncating
	 stores.  Use the size of a full vector for non-extending loads
	 and non-truncating stores (including svld[234] and svst[234]).  */
      poly_int64 size = ordered_min (GET_MODE_SIZE (mode),
				     BYTES_PER_SVE_VECTOR);
      rtx offset = gen_int_mode (size, Pmode);
      offset = simplify_gen_binary (MULT, Pmode, m_args[2], offset);
      base = simplify_gen_binary (PLUS, Pmode, base, offset);
    }
  return base;
}

/* Convert the arguments for a gather/scatter-style operation into
   the associated md operands.  Argument I is the scalar or vector base
   and argument I + 1 is the scalar or vector displacement (if applicable).
   The md pattern expects:

   - a scalar base
   - a vector displacement
   - a const_int that is 1 if the displacement is zero-extended from 32 bits
   - a scaling multiplier (1 for bytes, 2 for .h indices, etc.).  */
void
function_expander::prepare_gather_address_operands (unsigned int i)
{
  machine_mode mem_mode = m_fi.memory_vector_mode ();
  tree vector_type = m_fi.base_vector_type ();
  units units = m_fi.displacement_units ();
  if (units == UNITS_none)
    {
      /* Vector base, no displacement.  Convert to a zero base and a
	 vector byte offset.  */
      m_args.quick_insert (i, const0_rtx);
      units = UNITS_bytes;
    }
  else if (vector_type)
    {
      /* Vector base, scalar displacement.  Convert to a scalar base and
	 a vector byte offset.  */
      std::swap (m_args[i], m_args[i + 1]);
      if (units == UNITS_elements)
	{
	  /* Convert the original scalar array index to a byte offset.  */
	  rtx size = gen_int_mode (GET_MODE_UNIT_SIZE (mem_mode), DImode);
	  m_args[i] = simplify_gen_binary (MULT, DImode, m_args[i], size);
	  units = UNITS_bytes;
	}
    }
  else
    {
      /* Scalar base, vector displacement.  This is what the md pattern wants,
	 so we just need to make sure that the scalar base has DImode.  */
      if (Pmode == SImode)
	m_args[1] = simplify_gen_unary (ZERO_EXTEND, DImode,
					m_args[1], SImode);
      vector_type = m_fi.displacement_vector_type ();
    }
  tree scalar_type = TREE_TYPE (vector_type);

  bool uxtw_p = (TYPE_PRECISION (scalar_type) < 64
		 && TYPE_UNSIGNED (scalar_type));
  unsigned int scale = (units == UNITS_bytes
			? 1 : GET_MODE_UNIT_SIZE (mem_mode));

  m_args.quick_insert (i + 2, GEN_INT (uxtw_p));
  m_args.quick_insert (i + 3, GEN_INT (scale));
}

/* Require the final argument to be an immediate svprfop value.  Add two fake
   arguments to represent the rw and locality operands of a PREFETCH rtx.  */
void
function_expander::prepare_prefetch_operands ()
{
  unsigned int i = m_args.length () - 1;
  require_immediate_svprfop (i);
  unsigned int prfop = INTVAL (m_args[i]);
  /* Bit 3 of the prfop selects stores over loads.  */
  m_args.quick_push (GEN_INT ((prfop & 8) != 0));
  /* Bits 1 and 2 specify the locality; 0-based for svprfop but
     1-based for PREFETCH.  */
  m_args.quick_push (GEN_INT (((prfop >> 1) & 3) + 1));
}

/* Return a PARALLEL that selects a quadword of elements, starting at
   index START.  */
rtx
function_expander::prepare_dupq_lane_index (unsigned int start)
{
  unsigned int nelts_per_vq = m_fi.nelts_per_vq (0);
  return aarch64_gen_stepped_int_parallel (nelts_per_vq,
					   start * nelts_per_vq, 1);
}

/* Rotate inputs m_args[START:END] one position to the left, so that
   m_args[START] becomes m_args[END - 1].  */
void
function_expander::rotate_inputs_left (unsigned int start, unsigned int end)
{
  rtx new_last = m_args[start];
  for (unsigned int i = start; i < end - 1; ++i)
    m_args[i] = m_args[i + 1];
  m_args[end - 1] = new_last;
}

/* Return true if argument I is a constant argument that can be negated
   at compile time, replacing it with the negated value if so.  MODE is the
   associated vector mode, but the argument could be a single element.  */
bool
function_expander::try_negating_argument (unsigned int i, machine_mode mode)
{
  rtx x = m_args[i];
  if (!VECTOR_MODE_P (GET_MODE (x)))
    mode = GET_MODE_INNER (mode);

  x = simplify_unary_operation (NEG, mode, x, mode);
  if (!x)
    return false;

  m_args[i] = x;
  return true;
}

/* Return true if X overlaps any input.  */
bool
function_expander::overlaps_input_p (rtx x)
{
  for (unsigned int i = 0; i < m_args.length (); ++i)
    if (reg_overlap_mentioned_p (x, m_args[i]))
      return true;
  return false;
}

/* Return true if the function has no return value.  */
bool
function_expander::function_returns_void_p ()
{
  return TREE_TYPE (TREE_TYPE (m_rfn.decl)) == void_type_node;
}

/* Return the mode associated with type suffix I.  */
machine_mode
function_expander::get_mode (unsigned int i)
{
  return type_suffixes[m_fi.types[i]].mode;
}

/* Return the vector mode for elements that are a quarter the size of integer
   type suffix I.  */
machine_mode
function_expander::get_quarter_mode (unsigned int i)
{
  return TYPE_MODE (m_fi.quarter_vector_type (i));
}

/* Return the predicate mode associated with type suffix I.  */
machine_mode
function_expander::get_pred_mode (unsigned int i)
{
  unsigned int elem_bytes = type_suffixes[m_fi.types[i]].elem_bytes;
  return aarch64_sve_pred_mode (elem_bytes).require ();
}

/* For a function that does the equivalent of:

     OUTPUT = COND ? FN (INPUTS) : FALLBACK;

   return the value of FALLBACK.

   MODE is the mode of the value.  NOPS is the number of operands
   in INPUTS.  MERGE_ARGNO is member of m_args to use for _m functions,
   or DEFAULT_MERGE_ARGNO if we should apply the usual rules.

   OPNO is the caller's index into m_args.  If the returned value is
   argument 0 (as for unary _m operations), increment OPNO past the
   returned argument.  */
rtx
function_expander::get_fallback_value (machine_mode mode, unsigned int nops,
				       unsigned int merge_argno,
				       unsigned int &opno)
{
  if (m_fi.pred == PRED_z)
    return CONST0_RTX (mode);

  if (merge_argno == DEFAULT_MERGE_ARGNO)
    merge_argno = nops == 1 && m_fi.pred == PRED_m ? 0 : 1;

  if (merge_argno == 0)
    return m_args[opno++];
  return m_args[merge_argno];
}

/* Return the mode of the next operand of the instruction we're building,
   which has code ICODE.  */
machine_mode
function_expander::get_next_operand_mode (insn_code icode)
{
  machine_mode mode = insn_data[icode].operand[m_ops.length ()].mode;
  /* SVE ACLE patterns must specify a mode for every operand.  */
  gcc_assert (mode != VOIDmode);
  return mode;
}

/* Add an output operand to the instruction we're building, which has
   code ICODE.  Bind the output to the preferred target rtx if possible.  */
void
function_expander::add_output_operand (insn_code icode)
{
  machine_mode mode = get_next_operand_mode (icode);
  m_ops.safe_grow (m_ops.length () + 1);
  create_output_operand (&m_ops.last (), m_target, mode);
}

/* Add an input operand to the instruction we're building, which has
   code ICODE.  Calculate the value of the operand as follows:

   - If the operand is a vector and X is not, broadcast X to fill a
     vector of the appropriate mode.

   - Otherwise, if the operand is a predicate, coerce X to have the
     mode that the instruction expects.  In this case X is known to be
     VNx16BImode (the mode of svbool_t).

   - Otherwise use X directly.  The expand machinery checks that X has
     the right mode for the instruction.  */
void
function_expander::add_input_operand (insn_code icode, rtx x)
{
  machine_mode mode = get_next_operand_mode (icode);
  if (!VECTOR_MODE_P (GET_MODE (x)) && VECTOR_MODE_P (mode))
    x = expand_vector_broadcast (mode, x);
  else if (GET_MODE_CLASS (mode) == MODE_VECTOR_BOOL)
    {
      gcc_assert (GET_MODE (x) == VNx16BImode);
      x = gen_lowpart (mode, x);
    }
  m_ops.safe_grow (m_ops.length () + 1);
  create_input_operand (&m_ops.last (), x, mode);
}

/* Add an integer operand with value X to the instruction.  */
void
function_expander::add_integer_operand (HOST_WIDE_INT x)
{
  m_ops.safe_grow (m_ops.length () + 1);
  create_integer_operand (&m_ops.last (), x);
}

/* Add a memory operand with mode MODE and address ADDR.  */
void
function_expander::add_mem_operand (machine_mode mode, rtx addr)
{
  gcc_assert (VECTOR_MODE_P (mode));
  rtx mem = gen_rtx_MEM (mode, memory_address (mode, addr));
  /* The memory is only guaranteed to be element-aligned.  */
  set_mem_align (mem, GET_MODE_ALIGNMENT (GET_MODE_INNER (mode)));
  add_fixed_operand (mem);
}

/* Add an address operand with value X.  The static operand data says
   what mode and form the address must have.  */
void
function_expander::add_address_operand (rtx x)
{
  m_ops.safe_grow (m_ops.length () + 1);
  create_address_operand (&m_ops.last (), x);
}

/* Add an operand that must be X.  The only way of legitimizing an
   invalid X is to reload the address of a MEM.  */
void
function_expander::add_fixed_operand (rtx x)
{
  m_ops.safe_grow (m_ops.length () + 1);
  create_fixed_operand (&m_ops.last (), x);
}

/* Generate instruction ICODE, given that its operands have already
   been added to M_OPS.  Return the value of the first operand.  */
rtx
function_expander::generate_insn (insn_code icode)
{
  expand_insn (icode, m_ops.length (), m_ops.address ());
  return function_returns_void_p () ? const0_rtx : m_ops[0].value;
}

/* Expand a call to the SVE function with subcode CODE.  EXP is the call
   expression and TARGET is the preferred location for the result.
   Return the value of the lhs.  */
rtx
expand_builtin (unsigned int code, tree exp, rtx target)
{
  return function_expander (code, exp, target).expand ();
}

}

using namespace aarch64_sve;

#include "gt-aarch64-sve-builtins.h"
