/* Test the `vzipQs8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vzipQs8 (void)
{
  int8x16x2_t out_int8x16x2_t;
  int8x16_t arg0_int8x16_t;
  int8x16_t arg1_int8x16_t;

  out_int8x16x2_t = vzipq_s8 (arg0_int8x16_t, arg1_int8x16_t);
}

/* { dg-final { scan-assembler "vzip\.8\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

