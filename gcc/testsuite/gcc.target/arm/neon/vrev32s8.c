/* Test the `vrev32s8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vrev32s8 (void)
{
  int8x8_t out_int8x8_t;
  int8x8_t arg0_int8x8_t;

  out_int8x8_t = vrev32_s8 (arg0_int8x8_t);
}

/* { dg-final { scan-assembler "vrev32\.8\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
