/* Test the `vget_laneu32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vget_laneu32 (void)
{
  uint32_t out_uint32_t;
  uint32x2_t arg0_uint32x2_t;

  out_uint32_t = vget_lane_u32 (arg0_uint32x2_t, 0);
}

/* { dg-final { scan-assembler "vmov\.u32\[ 	\]+\[rR\]\[0-9\]+, \[dD\]\[0-9\]+\\\[0\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

