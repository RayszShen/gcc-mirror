/* Test the `vmov_nu16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vmov_nu16 (void)
{
  uint16x4_t out_uint16x4_t;
  uint16_t arg0_uint16_t;

  out_uint16x4_t = vmov_n_u16 (arg0_uint16_t);
}

/* { dg-final { scan-assembler "vdup\.16\[ 	\]+\[dD\]\[0-9\]+, (\[rR\]\[0-9\]+|\[dD\]\[0-9\]+\\\[\[0-9\]+\\\])!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
