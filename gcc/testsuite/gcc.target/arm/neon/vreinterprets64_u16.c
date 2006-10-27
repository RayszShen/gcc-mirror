/* Test the `vreinterprets64_u16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterprets64_u16 (void)
{
  int64x1_t out_int64x1_t;
  uint16x4_t arg0_uint16x4_t;

  out_int64x1_t = vreinterpret_s64_u16 (arg0_uint16x4_t);
}


