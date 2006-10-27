/* Test the `vst4Q_lanes16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vst4Q_lanes16 (void)
{
  int16_t *arg0_int16_t;
  int16x8x4_t arg1_int16x8x4_t;

  vst4q_lane_s16 (arg0_int16_t, arg1_int16x8x4_t, 0);
}

/* { dg-final { scan-assembler "vst4\.16\[ 	\]+\\\{((\[dD\]\[0-9\]+\\\[0\\\]-\[dD\]\[0-9\]+\\\[0\\\])|(\[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\], \[dD\]\[0-9\]+\\\[0\\\]))\\\}, \\\[\[rR\]\[0-9\]+\\\]!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */

