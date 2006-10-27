/* Test the `vqshrun_ns32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vqshrun_ns32 (void)
{
  uint16x4_t out_uint16x4_t;
  int32x4_t arg0_int32x4_t;

  out_uint16x4_t = vqshrun_n_s32 (arg0_int32x4_t, 0);
}

/* { dg-final { scan-assembler "vqshrun\.s32\[ 	\]+\[dD\]\[0-9\]+, \[qQ\]\[0-9\]+, #\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

