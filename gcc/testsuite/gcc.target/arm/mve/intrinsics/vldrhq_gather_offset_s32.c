/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

int32x4_t
foo (int16_t const * base, uint32x4_t offset)
{
  return vldrhq_gather_offset_s32 (base, offset);
}

/* { dg-final { scan-assembler "vldrh.s32"  }  } */

int32x4_t
foo1 (int16_t const * base, uint32x4_t offset)
{
  return vldrhq_gather_offset (base, offset);
}

/* { dg-final { scan-assembler "vldrh.s32"  }  } */
