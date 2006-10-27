/* Test the `vst4_laneu32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vst4_laneu32 (void)
{
  uint32_t *arg0_uint32_t;
  uint32x2x4_t arg1_uint32x2x4_t;

  vst4_lane_u32 (arg0_uint32_t, arg1_uint32x2x4_t, 0);
}

/* { dg-final { scan-assembler "vst4\.32\[ 	\]+\\\{((\[dD\]\[0-9\]+\\\[0\\\]-\[dD\]\[0-9\]+\\\[0\\\])|(\[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\]))\\\}, \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

