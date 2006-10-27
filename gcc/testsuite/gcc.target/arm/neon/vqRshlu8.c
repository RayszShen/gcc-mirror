/* Test the `vqRshlu8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vqRshlu8 (void)
{
  uint8x8_t out_uint8x8_t;
  uint8x8_t arg0_uint8x8_t;
  int8x8_t arg1_int8x8_t;

  out_uint8x8_t = vqrshl_u8 (arg0_uint8x8_t, arg1_int8x8_t);
}

/* { dg-final { scan-assembler "vqrshl\.u8\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

