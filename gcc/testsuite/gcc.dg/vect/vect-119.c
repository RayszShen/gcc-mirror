/* { dg-require-effective-target vect_int } */

#include <stdarg.h>
#include "tree-vect.h"

#define N 32

short sa[N];
short sc[N] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
short sb[N] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
int ia[N];
int ic[N] = {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,
	       0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int ib[N] = {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,
	       0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

/* Current peeling-for-alignment scheme will consider the 'sa[i+7]'
   access for peeling, and therefore will examine the option of
   using a peeling factor = VF-7 = 8-7 = 1. This will also
   align the access to 'ia[i+3]'.  */

int main1 (int n)
{
  int i;

  /* Multiple types with different sizes, used in independent
     copmutations. Vectorizable.  */
  for (i = 0; i < n; i++)
    {
      sa[i+7] = sb[i] + sc[i];
      ia[i+3] = ib[i] + ic[i];
    }

  /* check results:  */
  for (i = 0; i < n; i++)
    {
      if (sa[i+7] != sb[i] + sc[i] || ia[i+3] != ib[i] + ic[i])
	abort ();
    }

  return 0;
}

/* Current peeling-for-alignment scheme will consider the 'ia[i+3]'
   access for peeling, and therefore will examine the option of
   using a peeling factor = VF-3 = 8-3 = 5. This will also 
   align the access to 'sa[i+3]'.  */

int main2 (int n)
{
  int i;

  /* Multiple types with different sizes, used in idependent
     copmutations. Vectorizable.  */
  for (i = 0; i < n; i++)
    {
      ia[i+3] = ib[i] + ic[i];
      sa[i+3] = sb[i] + sc[i];
    }

  /* check results:  */
  for (i = 0; i < n; i++)
    {
      if (sa[i+3] != sb[i] + sc[i] || ia[i+3] != ib[i] + ic[i])
        abort ();
    }

  return 0;
}

int main (void)
{ 
  check_vect ();
  
  main1 (N-7);
  main2 (N-3);

  return 0;
}

/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 2 "vect" { xfail vect_no_align } } } */
/* { dg-final { scan-tree-dump-times "Alignment of access forced using peeling" 2 "vect" { xfail vect_no_align } } } */
/* { dg-final { scan-tree-dump-times "Vectorizing an unaligned access" 8 "vect" { xfail vect_no_align } } } */
/* { dg-final { cleanup-tree-dump "vect" } } */

