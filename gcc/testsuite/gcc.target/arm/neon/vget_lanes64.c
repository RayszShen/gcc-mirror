/* Test the `vget_lanes64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vget_lanes64 (void)
{
  int64_t out_int64_t;
  int64x1_t arg0_int64x1_t;

  out_int64_t = vget_lane_s64 (arg0_int64x1_t, 0);
}

/* { dg-final { scan-assembler "vmov\[ 	\]+\[rR\]\[0-9\]+, \[rR\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

