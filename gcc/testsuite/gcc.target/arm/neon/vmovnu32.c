/* Test the `vmovnu32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmovnu32 (void)
{
  uint16x4_t out_uint16x4_t;
  uint32x4_t arg0_uint32x4_t;

  out_uint16x4_t = vmovn_u32 (arg0_uint32x4_t);
}

/* { dg-final { scan-assembler "vmovn\.i32\[ 	\]+\[dD\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

