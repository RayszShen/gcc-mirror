/* PR target/8340 */
/* { dg-do compile { target i?86-*-* } } */
/* { dg-skip-if "" { i?86-*-* } { "-m64" } { "" } } */
/* { dg-options "-fPIC" } */

int foo ()
{
  static int a;

  __asm__ __volatile__ (  /* { dg-error "PIC register" } */
    "xorl %%ebx, %%ebx\n"
    "movl %%ebx, %0\n"
    : "=m" (a)
    :
    : "%ebx"
  );

  return a;
}
