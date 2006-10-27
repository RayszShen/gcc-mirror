/* Test the `vreinterpretu64_u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vreinterpretu64_u8 (void)
{
  uint64x1_t out_uint64x1_t;
  uint8x8_t arg0_uint8x8_t;

  out_uint64x1_t = vreinterpret_u64_u8 (arg0_uint8x8_t);
}


