/* Test the `vreinterpretQs16_s32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretQs16_s32 (void)
{
  int16x8_t out_int16x8_t;
  int32x4_t arg0_int32x4_t;

  out_int16x8_t = vreinterpretq_s16_s32 (arg0_int32x4_t);
}


