/* { dg-do compile } */
/* { dg-options "-fcilkplus" } */

int main (void)
{
  int array[10][10], array2[10];
  
  array2[:] = array2[ : : ] ;  /* { dg-error " expected ']' before ':' token" }  */

  return 0;
}
