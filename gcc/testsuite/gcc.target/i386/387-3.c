/* Verify that 387 mathematical constants are recognized.  */
/* { dg-do compile } */
/* { dg-options "-O2 -mfpmath=387 -mfancy-math-387" } */
/* { dg-final { scan-assembler "fldpi" } } */

long double add_pi(long double x)
{
  return x + 3.1415926535897932385128089594061862044L;
}

