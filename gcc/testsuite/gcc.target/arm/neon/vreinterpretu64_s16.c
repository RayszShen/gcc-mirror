/* Test the `vreinterpretu64_s16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretu64_s16 (void)
{
  uint64x1_t out_uint64x1_t;
  int16x4_t arg0_int16x4_t;

  out_uint64x1_t = vreinterpret_u64_s16 (arg0_int16x4_t);
}


