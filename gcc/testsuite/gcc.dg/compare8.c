/* { dg-do compile } */
/* { dg-options "-Wsign-compare" } */

int
f(unsigned short a1, unsigned short a2, unsigned int b)
{
  return ((a1+a2)|5) > b ? 2 : 3;  /* { dg-bogus "signed and unsigned" } */
}

int
g(unsigned short a1, unsigned short a2, unsigned int b)
{
  return ((a1+a2)&5) > b ? 2 : 3;  /* { dg-bogus "signed and unsigned" } */
}

int
h(unsigned short a1, unsigned short a2, unsigned int b)
{
  return ((a1+a2)^5) > b ? 2 : 3;  /* { dg-bogus "signed and unsigned" } */
}

