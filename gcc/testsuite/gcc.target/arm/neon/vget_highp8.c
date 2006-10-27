/* Test the `vget_highp8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vget_highp8 (void)
{
  poly8x8_t out_poly8x8_t;
  poly8x16_t arg0_poly8x16_t;

  out_poly8x8_t = vget_high_p8 (arg0_poly8x16_t);
}


