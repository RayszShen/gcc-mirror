/* { dg-do compile } */
/* { dg-options "-O2 -mavx512vl" } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %ymm\[0-9\]\[^\{\]" 3 } } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %ymm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %ymm\[0-9\]\{%k\[1-7\]\}\{z\}" 1 } } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %xmm\[0-9\]\[^\{\]" 3 } } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %xmm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vpsraq\[ \\t\]+\[^\n\]*, %xmm\[0-9\]\{%k\[1-7\]\}\{z\}" 1 } } */

#include <immintrin.h>

volatile __m256i x256;
volatile __m128i x128;
#define y 7
volatile __mmask8 m;

void extern
avx512vl_test (void)
{
  x256 = _mm256_srai_epi64 (x256, y);
  x256 = _mm256_mask_srai_epi64 (x256, m, x256, y);
  x256 = _mm256_maskz_srai_epi64 (m, x256, y);
  x128 = _mm_srai_epi64 (x128, y);
  x128 = _mm_mask_srai_epi64 (x128, m, x128, y);
  x128 = _mm_maskz_srai_epi64 (m, x128, y);
}
