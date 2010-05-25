/* Test the `vrev64Qf32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vrev64Qf32 (void)
{
  float32x4_t out_float32x4_t;
  float32x4_t arg0_float32x4_t;

  out_float32x4_t = vrev64q_f32 (arg0_float32x4_t);
}

/* { dg-final { scan-assembler "vrev64\.32\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
