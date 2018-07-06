/* { dg-do compile } */
/* { dg-options "-O2 -fwrapv" } */

char *a;
int b(void)
{
  long d;
  if (a) {
      char c;
      while ((c = *a) && !((unsigned)c - '0' <= 9) && c != ',' && c != '-'
	     && c != '+')
	++a;
      d = (long)a;
  }
  if (*a)
    return d;
}
