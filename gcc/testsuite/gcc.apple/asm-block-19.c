/* APPLE LOCAL file CW asm blocks */
/* { dg-do assemble { target i?86*-*-darwin* } } */
/* { dg-options { -fasm-blocks -msse3 } } */
/* Radar 4236553 */

void foo()
{
  asm mov eax,eax ?		/* { dg-error "parse error before" } */
}
