/* { dg-do run } */
/* { dg-options "-O2 -ftree-vectorize -fdump-tree-vect-stats -maltivec" { target powerpc*-*-* } } */


#include <stdarg.h>
#include <signal.h>

#define N 16
#define MAX 42

extern void abort(void); 

int main ()
{  
  int A[N] = {36,39,42,45,43,32,21,12,23,34,45,56,67,78,89,11};
  int B[N] = {36,39,42,45,43,32,21,12,23,34,45,56,67,78,89,11};
  int i, j;

  for (i = 0; i < 16; i++)
    A[i] = ( A[i] != MAX ? MAX : 0); 

  for (i = 0; i < 16; i++)
    B[i] = ( B[i] != MAX ? MAX : 0); 

  /* check results:  */
  for (i = 0; i < N; i++)
    if (!(A[i] != MAX || A[i] != 0))
      abort ();

  /* check results:  */
  for (i = 0; i < N; i++)
    if (!(B[i] != MAX || B[i] != 0))
      abort ();

  return 0;
}



/* { dg-final { scan-tree-dump-times "vectorized 2 loops" 1 "vect" } } */
