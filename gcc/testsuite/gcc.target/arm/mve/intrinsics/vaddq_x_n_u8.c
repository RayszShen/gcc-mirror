/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint8x16_t
foo (uint8x16_t a, uint8_t b, mve_pred16_t p)
{
  return vaddq_x_n_u8 (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vaddt.i8"  }  } */

uint8x16_t
foo1 (uint8x16_t a, uint8_t b, mve_pred16_t p)
{
  return vaddq_x (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vaddt.i8"  }  } */
