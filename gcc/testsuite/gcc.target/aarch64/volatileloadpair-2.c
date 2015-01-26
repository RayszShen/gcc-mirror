/* { dg-do compile } */
/* { dg-options "-O2" } */
/* volatile references should not produce load pair. */
/* { dg-final { scan-assembler-not "ldp\t" } } */

int f0(volatile int *a)
{
  int b = a[0];
  int c = a[1];
  int d = a[2];
  int e = a[3];
  return b + c + d + e;
}

int f1(volatile int *a)
{
  int b = a[1];
  int c = a[0];
  int d = a[2];
  int e = a[3];
  return b + c + d + e;
}

int f2(volatile int *a)
{
  int b = a[1];
  int c = a[0];
  int d = a[3];
  int e = a[2];
  return b + c + d + e;
}

int f3(volatile int *a)
{
  int b = a[1];
  int c = a[3];
  int d = a[0];
  int e = a[2];
  return b + c + d + e;
}

int f4(volatile int *a)
{
  int b = a[1];
  int c = a[3];
  int d = a[2];
  int e = a[0];
  return b + c + d + e;
}
