/* Test the `vreinterpretQp16_p8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretQp16_p8 (void)
{
  poly16x8_t out_poly16x8_t;
  poly8x16_t arg0_poly8x16_t;

  out_poly16x8_t = vreinterpretq_p16_p8 (arg0_poly8x16_t);
}


