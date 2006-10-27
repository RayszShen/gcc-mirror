/* Test the `vabalu32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vabalu32 (void)
{
  uint64x2_t out_uint64x2_t;
  uint64x2_t arg0_uint64x2_t;
  uint32x2_t arg1_uint32x2_t;
  uint32x2_t arg2_uint32x2_t;

  out_uint64x2_t = vabal_u32 (arg0_uint64x2_t, arg1_uint32x2_t, arg2_uint32x2_t);
}

/* { dg-final { scan-assembler "vabal\.u32\[ 	\]+\[qQ\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

