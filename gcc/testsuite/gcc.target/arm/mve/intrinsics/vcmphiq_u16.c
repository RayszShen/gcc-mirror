/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

mve_pred16_t
foo (uint16x8_t a, uint16x8_t b)
{
  return vcmphiq_u16 (a, b);
}

/* { dg-final { scan-assembler "vcmp.u16"  }  } */

mve_pred16_t
foo1 (uint16x8_t a, uint16x8_t b)
{
  return vcmphiq (a, b);
}

/* { dg-final { scan-assembler "vcmp.u16"  }  } */
