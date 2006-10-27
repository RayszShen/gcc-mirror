/* Test the `vaddhnu64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vaddhnu64 (void)
{
  uint32x2_t out_uint32x2_t;
  uint64x2_t arg0_uint64x2_t;
  uint64x2_t arg1_uint64x2_t;

  out_uint32x2_t = vaddhn_u64 (arg0_uint64x2_t, arg1_uint64x2_t);
}

/* { dg-final { scan-assembler "vaddhn\.i64\[ 	\]+\[dD\]\[0-9\]+, \[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

