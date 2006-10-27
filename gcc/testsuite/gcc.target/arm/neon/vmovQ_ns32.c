/* Test the `vmovQ_ns32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmovQ_ns32 (void)
{
  int32x4_t out_int32x4_t;
  int32_t arg0_int32_t;

  out_int32x4_t = vmovq_n_s32 (arg0_int32_t);
}

/* { dg-final { scan-assembler "vdup\.32\[ 	\]+\[qQ\]\[0-9\]+, \[rR\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

