/* Test the `vget_laneu64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vget_laneu64 (void)
{
  uint64_t out_uint64_t;
  uint64x1_t arg0_uint64x1_t;

  out_uint64_t = vget_lane_u64 (arg0_uint64x1_t, 0);
}

/* { dg-final { scan-assembler "vmov\[ 	\]+\[rR\]\[0-9\]+, \[rR\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

