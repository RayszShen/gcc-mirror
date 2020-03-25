/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint8x16_t
foo (uint8x16_t inactive, uint8x16_t a, mve_pred16_t p)
{
  return vshrq_m_n_u8 (inactive, a, 8, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vshrt.u8"  }  } */

uint8x16_t
foo1 (uint8x16_t inactive, uint8x16_t a, mve_pred16_t p)
{
  return vshrq_m (inactive, a, 8, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vshrt.u8"  }  } */
