/* { dg-do preprocess } */
/* { dg-options "-P -dU" } */
/* { dg-final { scan-file cmdlne-dU-14.i "^\n*B\n+#define A B\n+B\n*$" } } */
#define A B
A
A
