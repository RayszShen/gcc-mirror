/* Test the `vsraQ_ns64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vsraQ_ns64 (void)
{
  int64x2_t out_int64x2_t;
  int64x2_t arg0_int64x2_t;
  int64x2_t arg1_int64x2_t;

  out_int64x2_t = vsraq_n_s64 (arg0_int64x2_t, arg1_int64x2_t, 0);
}

/* { dg-final { scan-assembler "vsra\.s64\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, #\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

