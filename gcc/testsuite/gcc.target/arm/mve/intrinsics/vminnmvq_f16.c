/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

float16_t
foo (float16_t a, float16x8_t b)
{
  return vminnmvq_f16 (a, b);
}

/* { dg-final { scan-assembler "vminnmv.f16"  }  } */

float16_t
foo1 (float16_t a, float16x8_t b)
{
  return vminnmvq (a, b);
}

/* { dg-final { scan-assembler "vminnmv.f16"  }  } */
