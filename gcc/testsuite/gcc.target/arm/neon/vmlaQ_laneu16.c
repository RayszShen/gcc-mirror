/* Test the `vmlaQ_laneu16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmlaQ_laneu16 (void)
{
  uint16x8_t out_uint16x8_t;
  uint16x8_t arg0_uint16x8_t;
  uint16x8_t arg1_uint16x8_t;
  uint16x4_t arg2_uint16x4_t;

  out_uint16x8_t = vmlaq_lane_u16 (arg0_uint16x8_t, arg1_uint16x8_t, arg2_uint16x4_t, 1);
}

/* { dg-final { scan-assembler "vmla\.i16\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, \[dD\]\[0-9\]+\\\[\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

