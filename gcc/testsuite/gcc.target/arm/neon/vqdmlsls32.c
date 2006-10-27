/* Test the `vqdmlsls32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vqdmlsls32 (void)
{
  int64x2_t out_int64x2_t;
  int64x2_t arg0_int64x2_t;
  int32x2_t arg1_int32x2_t;
  int32x2_t arg2_int32x2_t;

  out_int64x2_t = vqdmlsl_s32 (arg0_int64x2_t, arg1_int32x2_t, arg2_int32x2_t);
}

/* { dg-final { scan-assembler "vqdmlsl\.s32\[ 	\]+\[qQ\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

