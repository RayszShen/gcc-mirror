/* Test the `vreinterprets8_p8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterprets8_p8 (void)
{
  int8x8_t out_int8x8_t;
  poly8x8_t arg0_poly8x8_t;

  out_int8x8_t = vreinterpret_s8_p8 (arg0_poly8x8_t);
}


