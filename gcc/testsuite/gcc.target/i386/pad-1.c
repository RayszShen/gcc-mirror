/* { dg-do compile } */
/* { dg-options "-O2 -fomit-frame-pointer -mtune=generic -S" } */
/* { dg-final { scan-assembler "rep" } } */
/* { dg-final { scan-assembler-not "nop" } } */

void
foo ()
{
}
