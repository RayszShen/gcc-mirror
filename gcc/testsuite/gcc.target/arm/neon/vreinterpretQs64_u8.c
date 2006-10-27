/* Test the `vreinterpretQs64_u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretQs64_u8 (void)
{
  int64x2_t out_int64x2_t;
  uint8x16_t arg0_uint8x16_t;

  out_int64x2_t = vreinterpretq_s64_u8 (arg0_uint8x16_t);
}


