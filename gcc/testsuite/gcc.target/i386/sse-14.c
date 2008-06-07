/* { dg-do compile } */
/* { dg-options "-O0 -Werror-implicit-function-declaration -march=k8 -m3dnow -msse4 -msse5 -maes -mpclmul" } */

#include <mm_malloc.h>

/* Test that the intrinsics compile without optimization.  All of them are
   defined as inline functions in {,x,e,p,t,s,w,a,b}mmintrin.h  and mm3dnow.h
   that reference the proper builtin functions.  Defining away "extern" and
   "__inline" results in all of them being compiled as proper functions.  */

#define extern
#define __inline

#include <wmmintrin.h>
#include <bmmintrin.h>
#include <smmintrin.h>
#include <mm3dnow.h>

#define _CONCAT(x,y) x ## y

#define test_1(func, type, op1_type, imm)				\
  type _CONCAT(_,func) (op1_type A, int const I)			\
  { return func (A, imm); }

#define test_1x(func, type, op1_type, imm1, imm2)			\
  type _CONCAT(_,func) (op1_type A, int const I, int const L)		\
  { return func (A, imm1, imm2); }

#define test_2(func, type, op1_type, op2_type, imm)			\
  type _CONCAT(_,func) (op1_type A, op2_type B, int const I)		\
  { return func (A, B, imm); }

#define test_2x(func, type, op1_type, op2_type, imm1, imm2)		\
  type _CONCAT(_,func) (op1_type A, op2_type B, int const I, int const L) \
  { return func (A, B, imm1, imm2); }

#define test_4(func, type, op1_type, op2_type, op3_type, op4_type, imm)	\
  type _CONCAT(_,func) (op1_type A, op2_type B,				\
			op3_type C, op4_type D, int const I)		\
  { return func (A, B, C, D, imm); }


/* Following intrinsics require immediate arguments.  They
   are defined as macros for non-optimized compilations. */

/* ammintrin.h */
test_1x (_mm_extracti_si64, __m128i, __m128i, 1, 1)
test_2x (_mm_inserti_si64, __m128i, __m128i, __m128i, 1, 1)

/* wmmintrin.h */
test_1 (_mm_aeskeygenassist_si128, __m128i, __m128i, 1)
test_2 (_mm_clmulepi64_si128, __m128i, __m128i, __m128i, 1)

/* mmintrin-common.h */
test_1 (_mm_round_pd, __m128d, __m128d, 1)
test_1 (_mm_round_ps, __m128, __m128, 1)
test_2 (_mm_round_sd, __m128d, __m128d, __m128d, 1)
test_2 (_mm_round_ss, __m128, __m128, __m128, 1)

/* smmintrin.h */
test_2 (_mm_blend_epi16, __m128i, __m128i, __m128i, 1)
test_2 (_mm_blend_ps, __m128, __m128, __m128, 1)
test_2 (_mm_blend_pd, __m128d, __m128d, __m128d, 1)
test_2 (_mm_dp_ps, __m128, __m128, __m128, 1)
test_2 (_mm_dp_pd, __m128d, __m128d, __m128d, 1)
test_2 (_mm_insert_ps, __m128, __m128, __m128, 1)
test_1 (_mm_extract_ps, int, __m128, 1)
test_2 (_mm_insert_epi8, __m128i, __m128i, int, 1)
test_2 (_mm_insert_epi32, __m128i, __m128i, int, 1)
#ifdef __x86_64__
test_2 (_mm_insert_epi64, __m128i, __m128i, long long, 1)
#endif
test_1 (_mm_extract_epi8, int, __m128i, 1)
test_1 (_mm_extract_epi32, int, __m128i, 1)
#ifdef __x86_64__
test_1 (_mm_extract_epi64, long long, __m128i, 1)
#endif
test_2 (_mm_mpsadbw_epu8, __m128i, __m128i, __m128i, 1)
test_2 (_mm_cmpistrm, __m128i, __m128i, __m128i, 1)
test_2 (_mm_cmpistri, int, __m128i, __m128i, 1)
test_4 (_mm_cmpestrm, __m128i, __m128i, int, __m128i, int, 1)
test_4 (_mm_cmpestri, int, __m128i, int, __m128i, int, 1)
test_2 (_mm_cmpistra, int, __m128i, __m128i, 1)
test_2 (_mm_cmpistrc, int, __m128i, __m128i, 1)
test_2 (_mm_cmpistro, int, __m128i, __m128i, 1)
test_2 (_mm_cmpistrs, int, __m128i, __m128i, 1)
test_2 (_mm_cmpistrz, int, __m128i, __m128i, 1)
test_4 (_mm_cmpestra, int, __m128i, int, __m128i, int, 1)
test_4 (_mm_cmpestrc, int, __m128i, int, __m128i, int, 1)
test_4 (_mm_cmpestro, int, __m128i, int, __m128i, int, 1)
test_4 (_mm_cmpestrs, int, __m128i, int, __m128i, int, 1)
test_4 (_mm_cmpestrz, int, __m128i, int, __m128i, int, 1)

/* tmmintrin.h */
test_2 (_mm_alignr_epi8, __m128i, __m128i, __m128i, 1)
test_2 (_mm_alignr_pi8, __m64, __m64, __m64, 1)

/* emmintrin.h */
test_2 (_mm_shuffle_pd, __m128d, __m128d, __m128d, 1)
test_1 (_mm_srli_si128, __m128i, __m128i, 1)
test_1 (_mm_slli_si128, __m128i, __m128i, 1)
test_1 (_mm_extract_epi16, int, __m128i, 1)
test_2 (_mm_insert_epi16, __m128i, __m128i, int, 1)
test_1 (_mm_shufflehi_epi16, __m128i, __m128i, 1)
test_1 (_mm_shufflelo_epi16, __m128i, __m128i, 1)
test_1 (_mm_shuffle_epi32, __m128i, __m128i, 1)

/* xmmintrin.h */
test_2 (_mm_shuffle_ps, __m128, __m128, __m128, 1)
test_1 (_mm_extract_pi16, int, __m64, 1)
test_1 (_m_pextrw, int, __m64, 1)
test_2 (_mm_insert_pi16, __m64, __m64, int, 1)
test_2 (_m_pinsrw, __m64, __m64, int, 1)
test_1 (_mm_shuffle_pi16, __m64, __m64, 1)
test_1 (_m_pshufw, __m64, __m64, 1)
test_1 (_mm_prefetch, void, void *, _MM_HINT_NTA)

/* bmmintrin.h */
test_1 (_mm_roti_epi8, __m128i, __m128i, 1)
test_1 (_mm_roti_epi16, __m128i, __m128i, 1)
test_1 (_mm_roti_epi32, __m128i, __m128i, 1)
test_1 (_mm_roti_epi64, __m128i, __m128i, 1)
