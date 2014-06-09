
/* { dg-do compile } */
/* { dg-options "-Wuninitialized -O2" } */

int g;
void bar();
void blah(int);

int foo (int n, int l, int m, int r)
{
  int v;

  if ( (n < 10) && (m != 100)  && (r < 20) )
    v = r;

  if (m) g++; 
  else  bar();

  if (l > 100)
    if ( (n <= 9) &&  (m < 100)  && (r < 19) )
      blah(v); /* { dg-bogus "uninitialized" "bogus warning" } */

  if ( (n <= 8) &&  (m < 99)  && (r < 19) )
      blah(v); /* { dg-bogus "uninitialized" "bogus warning" } */

  return 0;
}

int foo_2 (int n, int l, int m, int r)
{
  int v;

  if ( (n < 10) && (m != 100)  && (r < 20) )
    v = r;

  if (m) g++; 
  else  bar();

  if (l > 100)
    if ( (n <= 8) &&  (m < 101)  && (r < 19) )
      blah(v); /* { dg-warning "uninitialized" "real warning" } */

  return 0;
}
