/* { dg-do compile } */
/* { dg-require-effective-target ilp32 } */
/* { dg-options "-mpreferred-stack-boundary=2" } */

/* This compile only test is to detect an assertion failure in stack branch
   development.  */
void baz (void);
                       
double foo (void)
{
  baz ();
  return;
}

double bar (void)
{
  baz ();
}
