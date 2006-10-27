/* Test the `vcltf32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vcltf32 (void)
{
  uint32x2_t out_uint32x2_t;
  float32x2_t arg0_float32x2_t;
  float32x2_t arg1_float32x2_t;

  out_uint32x2_t = vclt_f32 (arg0_float32x2_t, arg1_float32x2_t);
}

/* { dg-final { scan-assembler "vcgt\.f32\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

