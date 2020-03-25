/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int16x8_t
foo (int8x16_t a, int8x16_t b, mve_pred16_t p)
{
  return vmullbq_int_x_s8 (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vmullbt.s8"  }  } */

int16x8_t
foo1 (int8x16_t a, int8x16_t b, mve_pred16_t p)
{
  return vmullbq_int_x (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
