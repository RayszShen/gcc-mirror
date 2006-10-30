/* Test the `vdupQ_lanep8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vdupQ_lanep8 (void)
{
  poly8x16_t out_poly8x16_t;
  poly8x8_t arg0_poly8x8_t;

  out_poly8x16_t = vdupq_lane_p8 (arg0_poly8x8_t, 1);
}

/* { dg-final { scan-assembler "vdup\.8\[ 	\]+\[qQ\]\[0-9\]+, \[dD\]\[0-9\]+\\\[\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

