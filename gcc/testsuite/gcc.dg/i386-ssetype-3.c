/* { dg-do compile { target i?86-*-* x86_64-*-* } } */
/* { dg-options "-O2 -msse2 -march=k8" } */
/* { dg-final { scan-assembler "andps\[^\\n\]*magic" } } */
/* { dg-final { scan-assembler "andnps\[^\\n\]*magic" } } */
/* { dg-final { scan-assembler "xorps\[^\\n\]*magic" } } */
/* { dg-final { scan-assembler "orps\[^\\n\]*magic" } } */
/* ??? All of the backend patters are WAY too fragile.  */
/* { dg-final { scan-assembler-not "movdqa" { xfail *-*-* } } } */
/* { dg-final { scan-assembler "movaps\[^\\n\]*magic" } } */

/* Verify that we generate proper instruction with memory operand.  */

#include <xmmintrin.h>

__m128 magic_a, magic_b;
__m128
t1(void)
{
return _mm_and_ps (magic_a,magic_b);
}
__m128
t2(void)
{
return _mm_andnot_ps (magic_a,magic_b);
}
__m128
t3(void)
{
return _mm_or_ps (magic_a,magic_b);
}
__m128
t4(void)
{
return _mm_xor_ps (magic_a,magic_b);
}
