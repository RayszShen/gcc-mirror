/* { dg-do compile { target { powerpc*-*-* && lp64 } } } */
/* { dg-skip-if "" { powerpc*-*-darwin* } } */
/* { dg-require-effective-target powerpc_p8vector_ok } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power8" } } */
/* { dg-options "-mcpu=power8 -O2" } */

#include <stdint.h>

typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

float
mask_and_float_var (float f, uint32_t mask)
{ 
  ieee_float_shape_type u;

  u.value = f;
  u.word &= mask;

  return u.value;
}

/* { dg-final { scan-assembler     {\mxxland\M}  } } */
/* { dg-final { scan-assembler-not {\mand\M}     } } */
/* { dg-final { scan-assembler-not {\mmfvsrd\M}  } } */
/* { dg-final { scan-assembler-not {\mstxv\M}    } } */
/* { dg-final { scan-assembler-not {\mlxv\M}     } } */
/* { dg-final { scan-assembler-not {\msrdi\M}    } } */
