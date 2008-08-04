/* PR middle-end/37009 */
/* { dg-do compile { target { { i?86-*-* x86_64-*-* } && ilp32 } } } */
/* { dg-options "-w -msse2 -mpreferred-stack-boundary=2" } */

#include <stdarg.h>
#include <emmintrin.h>

extern void bar (int *);

__m128
foo(va_list arg) 
{
  int __attribute((aligned(16))) xxx;

  xxx = 2;
  bar (&xxx);
  return va_arg (arg, __m128);
}

/* { dg-final { scan-assembler "and\[l\]\[ \t\]" } } */
