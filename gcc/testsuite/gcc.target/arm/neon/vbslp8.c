/* Test the `vbslp8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vbslp8 (void)
{
  poly8x8_t out_poly8x8_t;
  uint8x8_t arg0_uint8x8_t;
  poly8x8_t arg1_poly8x8_t;
  poly8x8_t arg2_poly8x8_t;

  out_poly8x8_t = vbsl_p8 (arg0_uint8x8_t, arg1_poly8x8_t, arg2_poly8x8_t);
}

/* { dg-final { scan-assembler "((vbsl)|(vbit)|(vbif))\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
