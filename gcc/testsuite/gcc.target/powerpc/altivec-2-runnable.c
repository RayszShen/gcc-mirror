/* { dg-do compile { target powerpc*-*-* } } */
/* { dg-require-effective-target powerpc_p8vector_ok } */
/* { dg-options "-mpower8-vector -mvsx" } */

#include <altivec.h>

#ifdef DEBUG
#include <stdio.h>
#endif

void abort (void);

/* Endian considerations: The "high" half of a vector with n elements is the
   first n/2 elements of the vector. For little endian, these elements are in
   the rightmost half of the vector. For big endian, these elements are in the
   leftmost half of the vector.  */

int main ()
{
  int i;
  vector bool int vec_bi_arg;
  vector bool long long vec_bll_result, vec_bll_expected;

  vector signed int vec_si_arg;
  vector signed long long int vec_slli_result, vec_slli_expected;

  /*  use of ‘long long’ in AltiVec types requires -mvsx */
  /* __builtin_altivec_vupkhsw and __builtin_altivec_vupklsw
     requires the -mpower8-vector option */

  vec_bi_arg = (vector bool int){ 0, 1, 1, 0 };

  vec_bll_expected = (vector bool long long){ 0, 1 };

  vec_bll_result = vec_unpackh (vec_bi_arg);

  for (i = 0; i < 2; i++) {
    if (vec_bll_expected[i] != vec_bll_result[i])
#if DEBUG
       printf("ERROR: vec_unpackh, vec_bll_expected[%d] = %d does not match vec_bll_result[%d] = %d\n",
	      i, vec_bll_expected[i], i, vec_bll_result[i]);
#else
       abort();
#endif
  }

  vec_bll_expected = (vector bool long long){ 1, 0 };

  vec_bll_result = vec_unpackl (vec_bi_arg);

  for (i = 0; i < 2; i++) {
    if (vec_bll_expected[i] != vec_bll_result[i])
#if DEBUG
       printf("ERROR: vec_unpackl, vec_bll_expected[%d] = %d does not match vec_bll_result[%d] = %d\n",
	      i, vec_bll_expected[i], i, vec_bll_result[i]);
#else
       abort();
#endif
  }


  vec_si_arg = (vector signed int){ 0, 101, 202, 303 };

  vec_slli_expected = (vector signed long long int){ 0, 101 };

  vec_slli_result = vec_unpackh (vec_si_arg);

  for (i = 0; i < 2; i++) {
    if (vec_slli_expected[i] != vec_slli_result[i])
#if DEBUG
       printf("ERROR: vec_unpackh, vec_slli_expected[%d] = %d does not match vec_slli_result[%d] = %d\n",
	      i, vec_slli_expected[i], i, vec_slli_result[i]);
#else
       abort();
#endif
  }

  vec_slli_result = vec_unpackl (vec_si_arg);
  vec_slli_expected = (vector signed long long int){ 202, 303 };

  for (i = 0; i < 2; i++) {
    if (vec_slli_expected[i] != vec_slli_result[i])
#if DEBUG
       printf("ERROR: vec_unpackl, vec_slli_expected[%d] = %d does not match vec_slli_result[%d] = %d\n",
	      i, vec_slli_expected[i], i, vec_slli_result[i]);
#else
       abort();
#endif
  }



  return 0;
}
