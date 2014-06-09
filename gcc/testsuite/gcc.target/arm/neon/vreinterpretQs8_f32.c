/* Test the `vreinterpretQs8_f32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vreinterpretQs8_f32 (void)
{
  int8x16_t out_int8x16_t;
  float32x4_t arg0_float32x4_t;

  out_int8x16_t = vreinterpretq_s8_f32 (arg0_float32x4_t);
}

/* { dg-final { cleanup-saved-temps } } */
