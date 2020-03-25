/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

float16x8_t
foo (float16x8_t a, float16x8_t b, float16x8_t c)
{
  return vfmsq_f16 (a, b, c);
}

/* { dg-final { scan-assembler "vfms.f16"  }  } */

float16x8_t
foo1 (float16x8_t a, float16x8_t b, float16x8_t c)
{
  return vfmsq (a, b, c);
}

/* { dg-final { scan-assembler "vfms.f16"  }  } */
