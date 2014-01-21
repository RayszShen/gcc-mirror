/* Test the `vreinterpretQf32_p64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_crypto_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_crypto } */

#include "arm_neon.h"

void test_vreinterpretQf32_p64 (void)
{
  float32x4_t out_float32x4_t;
  poly64x2_t arg0_poly64x2_t;

  out_float32x4_t = vreinterpretq_f32_p64 (arg0_poly64x2_t);
}

/* { dg-final { cleanup-saved-temps } } */
