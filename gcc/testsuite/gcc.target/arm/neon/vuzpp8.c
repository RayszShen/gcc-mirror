/* Test the `vuzpp8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vuzpp8 (void)
{
  poly8x8x2_t out_poly8x8x2_t;
  poly8x8_t arg0_poly8x8_t;
  poly8x8_t arg1_poly8x8_t;

  out_poly8x8x2_t = vuzp_p8 (arg0_poly8x8_t, arg1_poly8x8_t);
}

/* { dg-final { scan-assembler "vuzp\.8\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

