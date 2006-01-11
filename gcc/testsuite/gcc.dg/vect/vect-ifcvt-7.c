/* { dg-require-effective-target vect_condition } */

#include <stdarg.h>
#include <signal.h>

#define N 16
#define MAX 42

extern void abort(void); 

int main ()
{  
  int A[N] = {36,39,42,45,43,32,21,12,23,34,45,56,42,78,89,11};
  int B[N] = {42,42,0,42,42,42,42,42,42,42,42,42,0,42,42,42};
  int i, j;

  for (i = 0; i < 16; i++)
    A[i] = ( A[i] == MAX ? 0 : MAX); 

  /* check results:  */
  for (i = 0; i < N; i++)
    if (A[i] != B[i])
      abort ();

  return 0;
}



/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect" } } */
/* { dg-final { cleanup-tree-dump "vect" } } */
