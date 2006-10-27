/* Test the `vreinterpretp16_s8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretp16_s8 (void)
{
  poly16x4_t out_poly16x4_t;
  int8x8_t arg0_int8x8_t;

  out_poly16x4_t = vreinterpret_p16_s8 (arg0_int8x8_t);
}


