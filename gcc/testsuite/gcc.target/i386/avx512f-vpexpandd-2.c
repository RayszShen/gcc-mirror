/* { dg-do run } */
/* { dg-options "-O2 -mavx512f" } */
/* { dg-require-effective-target avx512f } */

#define AVX512F

#include "avx512f-helper.h"

#define SIZE (AVX512F_LEN / 32)
#include "avx512f-mask-type.h"

static void
CALC (int *s, int *r, MASK_TYPE mask)
{
  int i, k;

  for (i = 0, k = 0; i < SIZE; i++)
    {
      if (mask & (1 << i))
	r[i] = s[k++];
    }
}

static void
TEST (void)
{
  UNION_TYPE (AVX512F_LEN, i_d) s1, res1, res2, res3, res4;
  MASK_TYPE mask = MASK_VALUE;
  int s2[SIZE];
  int res_ref1[SIZE];
  int res_ref2[SIZE];
  int i, sign = 1;

  for (i = 0; i < SIZE; i++)
    {
      s1.a[i] = 12345 * (i + 200) * sign;
      s2[i] = 67890 * (i + 300) * sign;
      res1.a[i] = DEFAULT_VALUE;
      res3.a[i] = DEFAULT_VALUE;
      sign = -sign;
    }

  res1.x = INTRINSIC (_mask_expand_epi32) (res1.x, mask, s1.x);
  res2.x = INTRINSIC (_maskz_expand_epi32) (mask, s1.x);
  res3.x = INTRINSIC (_mask_expandloadu_epi32) (res3.x, mask, s2);
  res4.x = INTRINSIC (_maskz_expandloadu_epi32) (mask, s2);

  CALC (s1.a, res_ref1, mask);
  CALC (s2, res_ref2, mask);

  MASK_MERGE (i_d) (res_ref1, mask, SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_d) (res1, res_ref1))
    abort ();

  MASK_ZERO (i_d) (res_ref1, mask, SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_d) (res2, res_ref1))
    abort ();

  MASK_MERGE (i_d) (res_ref2, mask, SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_d) (res3, res_ref2))
    abort ();

  MASK_ZERO (i_d) (res_ref2, mask, SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_d) (res4, res_ref2))
    abort ();
}
