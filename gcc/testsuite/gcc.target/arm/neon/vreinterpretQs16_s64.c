/* Test the `vreinterpretQs16_s64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretQs16_s64 (void)
{
  int16x8_t out_int16x8_t;
  int64x2_t arg0_int64x2_t;

  out_int16x8_t = vreinterpretq_s16_s64 (arg0_int64x2_t);
}

/* { dg-final { cleanup-saved-temps } } */
