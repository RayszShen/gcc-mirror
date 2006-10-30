/* Test the `vmlsQ_laneu32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmlsQ_laneu32 (void)
{
  uint32x4_t out_uint32x4_t;
  uint32x4_t arg0_uint32x4_t;
  uint32x4_t arg1_uint32x4_t;
  uint32x2_t arg2_uint32x2_t;

  out_uint32x4_t = vmlsq_lane_u32 (arg0_uint32x4_t, arg1_uint32x4_t, arg2_uint32x2_t, 1);
}

/* { dg-final { scan-assembler "vmls\.i32\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, \[dD\]\[0-9\]+\\\[\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

