/* Test the `vst1u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vst1u8 (void)
{
  uint8_t *arg0_uint8_t;
  uint8x8_t arg1_uint8x8_t;

  vst1_u8 (arg0_uint8_t, arg1_uint8x8_t);
}

/* { dg-final { scan-assembler "vst1\.8\[ 	\]+((\\\{\[dD\]\[0-9\]+\\\})|(\[dD\]\[0-9\]+)), \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
