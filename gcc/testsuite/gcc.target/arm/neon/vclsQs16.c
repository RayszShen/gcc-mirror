/* Test the `vclsQs16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vclsQs16 (void)
{
  int16x8_t out_int16x8_t;
  int16x8_t arg0_int16x8_t;

  out_int16x8_t = vclsq_s16 (arg0_int16x8_t);
}

/* { dg-final { scan-assembler "vcls\.s16\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
