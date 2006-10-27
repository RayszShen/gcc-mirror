/* Test the `vmovns16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmovns16 (void)
{
  int8x8_t out_int8x8_t;
  int16x8_t arg0_int16x8_t;

  out_int8x8_t = vmovn_s16 (arg0_int16x8_t);
}

/* { dg-final { scan-assembler "vmovn\.i16\[ 	\]+\[dD\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

