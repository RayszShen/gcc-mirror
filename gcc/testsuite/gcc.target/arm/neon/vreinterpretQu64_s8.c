/* Test the `vreinterpretQu64_s8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vreinterpretQu64_s8 (void)
{
  uint64x2_t out_uint64x2_t;
  int8x16_t arg0_int8x16_t;

  out_uint64x2_t = vreinterpretq_u64_s8 (arg0_int8x16_t);
}

/* { dg-final { cleanup-saved-temps } } */
