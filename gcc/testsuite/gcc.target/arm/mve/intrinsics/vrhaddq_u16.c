/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint16x8_t
foo (uint16x8_t a, uint16x8_t b)
{
  return vrhaddq_u16 (a, b);
}

/* { dg-final { scan-assembler "vrhadd.u16"  }  } */

uint16x8_t
foo1 (uint16x8_t a, uint16x8_t b)
{
  return vrhaddq (a, b);
}

/* { dg-final { scan-assembler "vrhadd.u16"  }  } */
