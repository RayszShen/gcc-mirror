/* Test the `vmla_lanef32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmla_lanef32 (void)
{
  float32x2_t out_float32x2_t;
  float32x2_t arg0_float32x2_t;
  float32x2_t arg1_float32x2_t;
  float32x2_t arg2_float32x2_t;

  out_float32x2_t = vmla_lane_f32 (arg0_float32x2_t, arg1_float32x2_t, arg2_float32x2_t, 0);
}

/* { dg-final { scan-assembler "vmla\.f32\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+\\\[0\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

