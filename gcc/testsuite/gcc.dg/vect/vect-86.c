/* { dg-require-effective-target vect_int } */

#include <stdarg.h>
#include "tree-vect.h"

#define N 16

int main1 (int n)
{
  int i, j, k;
  int a[N], b[N];

  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
        {
	  k = i + n;
          a[j] = k;
        }
      b[i] = k;
    }


  for (j = 0; j < n; j++)
    if (a[j] != i + n - 1)
      abort();	

  for (i = 0; i < n; i++)
    if (b[i] != i + n)
      abort();	

  return 0;
}

int main (void)
{ 
  check_vect ();

  main1 (N);
  main1 (0);
  main1 (1);
  main1 (2);
  main1 (N-1);

  return 0;
}

/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect" } } */
