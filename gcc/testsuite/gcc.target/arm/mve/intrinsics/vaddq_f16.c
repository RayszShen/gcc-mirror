/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

float16x8_t
foo (float16x8_t a, float16x8_t b)
{
  return vaddq_f16 (a, b);
}

/* { dg-final { scan-assembler "vadd.f16"  }  } */

float16x8_t
foo1 (float16x8_t a, float16x8_t b)
{
  return vaddq (a, b);
}

/* { dg-final { scan-assembler "vadd.f16"  }  } */
