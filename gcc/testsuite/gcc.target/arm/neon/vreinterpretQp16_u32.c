/* Test the `vreinterpretQp16_u32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vreinterpretQp16_u32 (void)
{
  poly16x8_t out_poly16x8_t;
  uint32x4_t arg0_uint32x4_t;

  out_poly16x8_t = vreinterpretq_p16_u32 (arg0_uint32x4_t);
}

/* { dg-final { cleanup-saved-temps } } */
