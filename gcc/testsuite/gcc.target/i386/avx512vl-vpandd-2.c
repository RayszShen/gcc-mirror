/* { dg-do run } */
/* { dg-options "-O2 -mavx512vl -DAVX512VL" } */
/* { dg-require-effective-target avx512vl } */

#define AVX512F_LEN 256
#define AVX512F_LEN_HALF 128
#include "avx512f-vpandd-2.c"

#undef AVX512F_LEN
#undef AVX512F_LEN_HALF

#define AVX512F_LEN 128
#define AVX512F_LEN_HALF 128
#include "avx512f-vpandd-2.c"
