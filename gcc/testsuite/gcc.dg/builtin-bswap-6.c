/* { dg-do compile { target arm*-*-* alpha*-*-* i?86-*-* powerpc*-*-* rs6000-*-* x86_64-*-* s390*-*-* } } */
/* { dg-require-effective-target stdint_types } */
/* { dg-options "-O -fdump-rtl-combine" } */
/* { dg-options "-O -fdump-rtl-combine -march=z900" { target s390-*-* } } */

#include <stdint.h>

#define BS(X) __builtin_bswap32(X)

int foo1 (uint32_t a)
{
  if (BS (a) == 0xA0000)
    return 1;
  return 0;
}

int foo2 (uint32_t a)
{
  if (BS (a) != 0xA0000)
    return 1;
  return 0;
}

int foo3 (uint32_t a, uint32_t b)
{
  if (BS (a) == BS (b))
    return 1;
  return 0;
}

int foo4 (uint32_t a, uint32_t b)
{
  if (BS (a) != BS (b))
    return 1;
  return 0;
}

/* { dg-final { scan-rtl-dump-not "bswapsi" "combine" } } */
/* { dg-final { cleanup-rtl-dump "combine" } } */
