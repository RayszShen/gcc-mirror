/* Test for <tgmath.h> in C99. */
/* Origin: Matt Austern <austern@apple.com>
/* { dg-do compile { target c99_runtime } } */
/* { dg-options "-std=iso9899:1999" } */
/* { dg-add-options c99_runtime } */
/* { dg-skip-if "<complex.h> missing" { alpha*-dec-osf5* } } */

/* Test that invoking type-generic pow on complex float invokes cpowf. */
#include <tgmath.h>

complex double foo(complex float x, float y)
{
  return pow(x, y);
}

/* { dg-final { scan-assembler "cpowf" } } */
