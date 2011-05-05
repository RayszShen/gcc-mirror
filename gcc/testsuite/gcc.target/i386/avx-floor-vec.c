/* { dg-do run } */
/* { dg-options "-O2 -ffast-math -ftree-vectorize -mavx" } */
/* { dg-require-effective-target avx } */
/* { dg-skip-if "no M_PI" { vxworks_kernel } } */

#include "avx-check.h"

#include <math.h>

extern double floor (double);

#define NUM 64

static void
__attribute__((__target__("fpmath=sse")))
init_src (double *src)
{
  int i, sign = 1;
  double f = rand ();

  for (i = 0; i < NUM; i++)
    {
      src[i] = (i + 1) * f * M_PI * sign;
      if (i < (NUM / 2))
	{
          if ((i % 6) == 0)
	    f = f * src[i];
        }
      else if (i == (NUM / 2))
	f = rand ();
      else if ((i % 6) == 0)
	f = 1 / (f * (i + 1) * src[i] * M_PI * sign);
      sign = -sign;
    }
}

static void
__attribute__((__target__("fpmath=387")))
avx_test (void)
{
  double a[NUM];
  double r[NUM];
  int i;

  init_src (a);

  for (i = 0; i < NUM; i++)
    r[i] = floor (a[i]);

  /* check results:  */
  for (i = 0; i < NUM; i++)
    if (r[i] != floor (a[i]))
      abort();
}
