/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

mve_pred16_t
foo (int16x8_t a, int16_t b)
{
  return vcmpeqq_n_s16 (a, b);
}

/* { dg-final { scan-assembler "vcmp.i16"  }  } */

mve_pred16_t
foo1 (int16x8_t a, int16_t b)
{
  return vcmpeqq (a, b);
}

/* { dg-final { scan-assembler "vcmp.i16"  }  } */
