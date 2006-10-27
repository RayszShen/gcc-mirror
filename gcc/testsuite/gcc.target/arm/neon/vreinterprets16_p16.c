/* Test the `vreinterprets16_p16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterprets16_p16 (void)
{
  int16x4_t out_int16x4_t;
  poly16x4_t arg0_poly16x4_t;

  out_int16x4_t = vreinterpret_s16_p16 (arg0_poly16x4_t);
}


