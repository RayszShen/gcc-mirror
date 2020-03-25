/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int32x4_t
foo (int32x4_t a)
{
  return vrshrq_n_s32 (a, 32);
}

/* { dg-final { scan-assembler "vrshr.s32"  }  } */

int32x4_t
foo1 (int32x4_t a)
{
  return vrshrq (a, 32);
}

/* { dg-final { scan-assembler "vrshr.s32"  }  } */
