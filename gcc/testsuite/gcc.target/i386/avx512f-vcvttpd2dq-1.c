/* { dg-do compile } */
/* { dg-options "-mavx512f -O2" } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\[^\{\]" 2 } } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 2 } } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\{z\}" 2 } } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*\{sae\}\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*\{sae\}\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vcvttpd2dq\[ \\t\]+\[^\n\]*\{sae\}\[^\n\]*%zmm\[0-9\]\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\{z\}" 1 } } */

#include <immintrin.h>

volatile __m512d s;
volatile __m256i res;
volatile __mmask8 m;

void extern
avx512f_test (void)
{
  res = _mm512_cvttpd_epi32 (s);
  res = _mm512_mask_cvttpd_epi32 (res, m, s);
  res = _mm512_maskz_cvttpd_epi32 (m, s);
  res = _mm512_cvtt_roundpd_epi32 (s, _MM_FROUND_NO_EXC);
  res = _mm512_mask_cvtt_roundpd_epi32 (res, m, s, _MM_FROUND_NO_EXC);
  res = _mm512_maskz_cvtt_roundpd_epi32 (m, s, _MM_FROUND_NO_EXC);
}
