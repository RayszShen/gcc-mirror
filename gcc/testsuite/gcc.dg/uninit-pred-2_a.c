/* { dg-do compile } */
/* { dg-options "-Wuninitialized -O2" } */

int g;
void bar (void);
void blah (int);

int foo (int n, int m, int r)
{
  int flag = 0;
  int v;

  if (n)
    {
      v = r;
      flag = 1;
    }

  if (m)
    g++;
  else 
    bar();

  if (flag)
    blah(v); /* { dg-bogus "uninitialized" "bogus uninitialized var warning" } */ 

  return 0;
}
