/* Test the `vqdmlsl_lanes32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vqdmlsl_lanes32 (void)
{
  int64x2_t out_int64x2_t;
  int64x2_t arg0_int64x2_t;
  int32x2_t arg1_int32x2_t;
  int32x2_t arg2_int32x2_t;

  out_int64x2_t = vqdmlsl_lane_s32 (arg0_int64x2_t, arg1_int32x2_t, arg2_int32x2_t, 0);
}

/* { dg-final { scan-assembler "vqdmlsl\.s32\[ 	\]+\[qQ\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+\\\[0\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

