/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int64x2_t
foo (int32x4_t a, int32x4_t b, mve_pred16_t p)
{
  return vmulltq_int_x_s32 (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
/* { dg-final { scan-assembler "vmulltt.s32"  }  } */

int64x2_t
foo1 (int32x4_t a, int32x4_t b, mve_pred16_t p)
{
  return vmulltq_int_x (a, b, p);
}

/* { dg-final { scan-assembler "vpst" } } */
