/* Test the stc2l ACLE intrinsic.  */

/* { dg-do assemble } */
/* { dg-options "-save-temps" } */
/* { dg-require-effective-target arm_coproc2_ok } */

#include "arm_acle.h"

extern void * p;

void test_stc2l (void)
{
  __arm_stc2l (10, 1, p - 120);
  __arm_stc2l (11, 1, p - 122);
}

/* { dg-final { scan-assembler "stc2l\tp10, CR1, \[r\[0-9\]+" } } */
/* { dg-final { scan-assembler "stc2l\tp11, CR1, \[r\[0-9\]+\]\n" } } */
