/* Test the `vrev64s16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vrev64s16 (void)
{
  int16x4_t out_int16x4_t;
  int16x4_t arg0_int16x4_t;

  out_int16x4_t = vrev64_s16 (arg0_int16x4_t);
}

/* { dg-final { scan-assembler "vrev64\.16\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
