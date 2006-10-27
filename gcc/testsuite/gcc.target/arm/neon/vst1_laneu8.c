/* Test the `vst1_laneu8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vst1_laneu8 (void)
{
  uint8_t *arg0_uint8_t;
  uint8x8_t arg1_uint8x8_t;

  vst1_lane_u8 (arg0_uint8_t, arg1_uint8x8_t, 0);
}

/* { dg-final { scan-assembler "vst1\.8\[ 	\]+((\\\{\[dD\]\[0-9\]+\\\[0\\\]\\\})|(\[dD\]\[0-9\]+\\\[0\\\])), \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

