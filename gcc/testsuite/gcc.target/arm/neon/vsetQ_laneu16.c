/* Test the `vsetQ_laneu16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vsetQ_laneu16 (void)
{
  uint16x8_t out_uint16x8_t;
  uint16_t arg0_uint16_t;
  uint16x8_t arg1_uint16x8_t;

  out_uint16x8_t = vsetq_lane_u16 (arg0_uint16_t, arg1_uint16x8_t, 1);
}

/* { dg-final { scan-assembler "vmov\.16\[ 	\]+\[dD\]\[0-9\]+\\\[\[0-9\]+\\\], \[rR\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
