/* { dg-do compile { target { powerpc*-*-* } } } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power9" } } */
/* { dg-require-effective-target powerpc_p9vector_ok } */
/* { dg-options "-mcpu=power9" } */

#include <altivec.h>

__vector long long int
get_exponents (__vector double *p)
{
  __vector double source = *p;

  return vec_extract_exp (source);
}

/* { dg-final { scan-assembler "xvxexpdp" } } */
