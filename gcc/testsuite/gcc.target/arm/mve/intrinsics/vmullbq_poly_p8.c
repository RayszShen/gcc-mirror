/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint16x8_t
foo (uint8x16_t a, uint8x16_t b)
{
  return vmullbq_poly_p8 (a, b);
}

/* { dg-final { scan-assembler "vmullb.p8"  }  } */

uint16x8_t
foo1 (uint8x16_t a, uint8x16_t b)
{
  return vmullbq_poly (a, b);
}

/* { dg-final { scan-assembler "vmullb.p8"  }  } */
