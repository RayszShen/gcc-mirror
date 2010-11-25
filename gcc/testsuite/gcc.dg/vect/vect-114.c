/* { dg-require-effective-target vect_float } */

#include <stdarg.h>
#include "tree-vect.h"

#define N 16

__attribute__ ((noinline)) int
main1 (void)
{
  int i;
  float a[N];
  float b[N] = {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45};

  /* Reverse access and forward access.  */
  for (i = N; i > 0; i--)
    {
      a[N-i] = b[i-1];
    }

  /* Check results.  */
  for (i = 0; i <N; i++)
    {
      if (a[i] != b[N-1-i])
	abort ();
    }

  return 0;
}

int main (void)
{
  check_vect ();
  return main1 ();
}

/* { dg-final { scan-tree-dump-times "vectorized 0 loops" 1 "vect" { target { ! { vect_perm && vect_hw_misalign } } } } } */
/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect" { target { vect_perm && vect_hw_misalign } } } } */
/* { dg-final { cleanup-tree-dump "vect" } } */

