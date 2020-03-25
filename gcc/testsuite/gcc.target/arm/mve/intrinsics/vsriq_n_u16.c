/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint16x8_t
foo (uint16x8_t a, uint16x8_t b)
{
  return vsriq_n_u16 (a, b, 4);
}

/* { dg-final { scan-assembler "vsri.16"  }  } */

uint16x8_t
foo1 (uint16x8_t a, uint16x8_t b)
{
  return vsriq (a, b, 4);
}

/* { dg-final { scan-assembler "vsri.16"  }  } */
