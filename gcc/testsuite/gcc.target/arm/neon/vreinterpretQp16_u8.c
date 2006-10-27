/* Test the `vreinterpretQp16_u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretQp16_u8 (void)
{
  poly16x8_t out_poly16x8_t;
  uint8x16_t arg0_uint8x16_t;

  out_poly16x8_t = vreinterpretq_p16_u8 (arg0_uint8x16_t);
}


