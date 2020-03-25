/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int32x4_t
foo (int16x8_t a)
{
  return vmovlbq_s16 (a);
}

/* { dg-final { scan-assembler "vmovlb.s16"  }  } */

int32x4_t
foo1 (int16x8_t a)
{
  return vmovlbq (a);
}

/* { dg-final { scan-assembler "vmovlb.s16"  }  } */
