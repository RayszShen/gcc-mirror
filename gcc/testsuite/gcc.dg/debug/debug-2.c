/* Verify that the scheduler does not discard the lexical block.  */
/* { dg-do compile } */
/* { dg-options "-dA" } */
/* APPLE LOCAL xfail for Apple only, --serena */
/* { dg-final { scan-assembler "xyzzy" { xfail *-apple-darwin* } } } */

long p;

long foo(void)
{
  if (1)
    {
      long xyzzy = 0;
      if (p)
        xyzzy = 2;
      return xyzzy;
    }
  else
    {
      int x = 0;
      return x;
    }
}
