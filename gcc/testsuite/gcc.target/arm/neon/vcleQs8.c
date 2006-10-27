/* Test the `vcleQs8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vcleQs8 (void)
{
  uint8x16_t out_uint8x16_t;
  int8x16_t arg0_int8x16_t;
  int8x16_t arg1_int8x16_t;

  out_uint8x16_t = vcleq_s8 (arg0_int8x16_t, arg1_int8x16_t);
}

/* { dg-final { scan-assembler "vcge\.s8\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

