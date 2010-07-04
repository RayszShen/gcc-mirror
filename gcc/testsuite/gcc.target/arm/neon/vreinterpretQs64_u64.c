/* Test the `vreinterpretQs64_u64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vreinterpretQs64_u64 (void)
{
  int64x2_t out_int64x2_t;
  uint64x2_t arg0_uint64x2_t;

  out_int64x2_t = vreinterpretq_s64_u64 (arg0_uint64x2_t);
}

/* { dg-final { cleanup-saved-temps } } */
