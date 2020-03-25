/* { dg-require-effective-target arm_v8_1m_mve_ok } */
/* { dg-add-options arm_v8_1m_mve } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

uint64_t
foo (uint32x4_t a)
{
    return vaddlvq_u32 (a);
}

/* { dg-final { scan-assembler "vaddlv.u32"  }  } */

uint64_t
foo1 (uint32x4_t a)
{
    return vaddlvq (a);
}

/* { dg-final { scan-assembler "vaddlv.u32"  }  } */
