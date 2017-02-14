/* PR target/78227 */
/* { dg-do compile } */
/* { dg-options "-mavx512f -O0 -Wno-psabi" } */

typedef int V __attribute__((vector_size (64)));
typedef long long int W __attribute__((vector_size (64)));

V
foo1 (V v)
{
  return v > 0;
}

V
bar1 (V v)
{
  return v != 0;
}

W
foo2 (W w)
{
  return w > 0;
}

W
bar2 (W w)
{
  return w != 0;
}
