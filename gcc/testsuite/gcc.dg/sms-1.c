/* The same test as loop-3c.c.  It failed on ia64
   due to not handling of subreg in the lhs that is fixed.  */
/* { dg-do run } */
/* { dg-options "-O2 -fmodulo-sched -fmodulo-sched-allow-regmoves -w" } */


#include <limits.h>

void * a[255];

f (m)
{
  int i;
  int sh = 0x100;
  i = m;
  do
    {
      a[sh >>= 1] = ((unsigned)i << 3)  + (char*)a;
      i += 4;
    }
  while (i < INT_MAX/2 + 1 + 4 * 4);
}

main ()
{
  a[0x10] = 0;
  a[0x08] = 0;
  f (INT_MAX/2 + INT_MAX/4 + 2);
  if (a[0x10] || a[0x08])
    abort ();
  a[0x10] = 0;
  a[0x08] = 0;
  f (INT_MAX/2 + 1);
  if (! a[0x10] || a[0x08])
    abort ();
  exit (0);
}

