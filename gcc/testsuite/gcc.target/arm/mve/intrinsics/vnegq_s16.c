/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int16x8_t
foo (int16x8_t a)
{
  return vnegq_s16 (a);
}

/* { dg-final { scan-assembler "vneg.s16"  }  } */

int16x8_t
foo1 (int16x8_t a)
{
  return vnegq (a);
}

/* { dg-final { scan-assembler "vneg.s16"  }  } */
