/* { dg-do compile } */
/* { dg-options "-mavx512vl -O2" } */
/* { dg-final { scan-assembler "vptestmd\[ \\t\]+\[^\n\]*%xmm\[0-7\]\[^\n\]*k\[1-7\]\[^\{\]" } } */
/* { dg-final { scan-assembler "vptestmd\[ \\t\]+\[^\n\]*%xmm\[0-7\]\[^\n\]*k\[1-7\]\[^\{\]" } } */
/* { dg-final { scan-assembler "vptestmd\[ \\t\]+\[^\n\]*%ymm\[0-7\]\[^\n\]*k\[1-7\]\{%*k\[1-7\]\}" } } */
/* { dg-final { scan-assembler "vptestmd\[ \\t\]+\[^\n\]*%ymm\[0-7\]\[^\n\]*k\[1-7\]\{%*k\[1-7\]\}" } } */

#include <immintrin.h>

volatile __m128i x128;
volatile __m256i x256;
volatile __mmask8 m;

void extern
avx512vl_test (void)
{
  m = _mm_test_epi32_mask (x128, x128);
  m = _mm256_test_epi32_mask (x256, x256);
  m = _mm_mask_test_epi32_mask (3, x128, x128);
  m = _mm256_mask_test_epi32_mask (3, x256, x256);
}
