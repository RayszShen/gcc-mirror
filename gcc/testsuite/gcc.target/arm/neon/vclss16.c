/* Test the `vclss16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vclss16 (void)
{
  int16x4_t out_int16x4_t;
  int16x4_t arg0_int16x4_t;

  out_int16x4_t = vcls_s16 (arg0_int16x4_t);
}

/* { dg-final { scan-assembler "vcls\.s16\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

