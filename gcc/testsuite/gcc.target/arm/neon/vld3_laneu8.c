/* Test the `vld3_laneu8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vld3_laneu8 (void)
{
  uint8x8x3_t out_uint8x8x3_t;
  uint8x8x3_t arg1_uint8x8x3_t;

  out_uint8x8x3_t = vld3_lane_u8 (0, arg1_uint8x8x3_t, 1);
}

/* { dg-final { scan-assembler "vld3\.8\[ 	\]+\\\{((\[dD\]\[0-9\]+\\\[\[0-9\]+\\\]-\[dD\]\[0-9\]+\\\[\[0-9\]+\\\])|(\[dD\]\[0-9\]+\\\[\[0-9\]+\\\], \[dD\]\[0-9\]+\\\[\[0-9\]+\\\], \[dD\]\[0-9\]+\\\[\[0-9\]+\\\]))\\\}, \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

