/* Test the `vst1s64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vst1s64 (void)
{
  int64_t *arg0_int64_t;
  int64x1_t arg1_int64x1_t;

  vst1_s64 (arg0_int64_t, arg1_int64x1_t);
}

/* { dg-final { scan-assembler "vst1\.64\[ 	\]+((\\\{\[dD\]\[0-9\]+\\\})|(\[dD\]\[0-9\]+)), \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

