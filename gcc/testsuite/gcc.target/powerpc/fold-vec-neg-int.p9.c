/* Verify that overloaded built-ins for vec_neg with int
   inputs produce the right code when -mcpu=power9 is specified.  */

/* { dg-do compile } */
/* { dg-require-effective-target powerpc_altivec_ok } */
/* { dg-options "-maltivec -O2 -mcpu=power9" } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power9" } } */

#include <altivec.h>

vector signed int
test1 (vector signed int x)
{
  return vec_neg (x);
}

/* { dg-final { scan-assembler-times "vnegw" 1 } } */

