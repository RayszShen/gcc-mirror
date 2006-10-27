/* Test the `vld3_laneu16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vld3_laneu16 (void)
{
  uint16x4x3_t out_uint16x4x3_t;
  uint16x4x3_t arg1_uint16x4x3_t;

  out_uint16x4x3_t = vld3_lane_u16 (0, arg1_uint16x4x3_t, 0);
}

/* { dg-final { scan-assembler "vld3\.16\[ 	\]+\\\{((\[dD\]\[0-9\]+\\\[0\\\]-\[dD\]\[0-9\]+\\\[0\\\])|(\[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\]))\\\}, \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

