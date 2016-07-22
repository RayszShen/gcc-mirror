/* { dg-do compile { target { powerpc*-*-* } } } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power8" } } */
/* { dg-require-effective-target powerpc_p9vector_ok } */
/* { dg-skip-if "" { powerpc*-*-aix* } } */
/* { dg-options "-mcpu=power8" } */

#include <altivec.h>

__vector float
make_floats (__vector int *significands_p, __vector int *exponents_p)
{
  __vector int significands = *significands_p;
  __vector int exponents = *exponents_p;

  return vec_insert_exp (significands, exponents); /* { dg-error "Builtin function __builtin_vsx_vector_insert_exp requires" } */
}
