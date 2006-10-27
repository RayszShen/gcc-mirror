/* Test the `vmov_nu8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vmov_nu8 (void)
{
  uint8x8_t out_uint8x8_t;
  uint8_t arg0_uint8_t;

  out_uint8x8_t = vmov_n_u8 (arg0_uint8_t);
}

/* { dg-final { scan-assembler "vdup\.8\[ 	\]+\[dD\]\[0-9\]+, \[rR\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

