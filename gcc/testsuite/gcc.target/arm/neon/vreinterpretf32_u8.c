/* Test the `vreinterpretf32_u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretf32_u8 (void)
{
  float32x2_t out_float32x2_t;
  uint8x8_t arg0_uint8x8_t;

  out_float32x2_t = vreinterpret_f32_u8 (arg0_uint8x8_t);
}


