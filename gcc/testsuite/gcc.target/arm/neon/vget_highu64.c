/* Test the `vget_highu64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vget_highu64 (void)
{
  uint64x1_t out_uint64x1_t;
  uint64x2_t arg0_uint64x2_t;

  out_uint64x1_t = vget_high_u64 (arg0_uint64x2_t);
}


