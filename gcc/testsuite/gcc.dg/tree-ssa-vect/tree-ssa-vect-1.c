/* { dg-do compile { target powerpc*-*-* i?86-*-* } } */
/* { dg-options "-O2 -ftree-vectorize -fdump-tree-vect -maltivec" { target powerpc*-*-* } } */
/* { dg-options "-O2 -ftree-vectorize -fdump-tree-vect -msse" { target i?86-*-* } } */

#define N 16

void fbar (float *);
void ibar (int *);
void sbar (short *);

foo (int n)
{
  float a[N+1];
  float b[N];
  float c[N];
  float d[N];
  int ia[N];
  int ib[N];
  int ic[N];
  short sa[N];
  short sb[N];
  short sc[N];
  int i,j;
  int diff = 0;
  char cb[N];
  char cc[N];
  char image[N][N];
  char block[N][N];

  /* Not vetorizable yet (unknown loop bound).  */
  for (i = 0; i < n; i++){
    a[i] = b[i];
  }
  fbar (a);

  /* Vectorizable.  */
  for (i = 0; i < N; i++){
    a[i] = b[i];
  }
  fbar (a);

  /* Not vetorizable yet (constant assignment).  */
  for (i = 0; i < N; i++){
    a[i] = 5;
  }
  fbar (a);

  /* Vectorizable.  */
  for (i = 0; i < N; i++){
    a[i] = b[i] + c[i] + d[i];
  }
  fbar (a);

  /* Vectorizable.  */
  for (i = 0; i < N; i++){
    a[i] = b[i] * c[i];
  }
  fbar (a);

  /* Not vetorizable yet (access pattern).  */
  for (i = 0; i < N/2; i++){
    a[i] = b[2*i+1] * c[2*i+1] - b[2*i] * c[2*i];
    d[i] = b[2*i] * c[2*i+1] + b[2*i+1] * c[2*i];
  }
  fbar (a);

  /* Not vetorizable yet (no support for integer mult).  */
  for (i = 0; i < N; i++){
    ia[i] = ib[i] * ic[i];
  }
  ibar (ia);

  /* Vectorizable.  */
  for (i = 0; i < N; i++){
    a[i] = b[i] + c[i];
    d[i] = b[i] + c[i];
    ia[i] = ib[i] + ic[i];
  }
  ibar (ia);
  fbar (a);
  fbar (d);

  /* Not vectorizable yet (two types with different nunits in vector).  */
  for (i = 0; i < N; i++){
    ia[i] = ib[i] + ic[i];
    sa[i] = sb[i] + sc[i];
  }
  ibar (ia);
  sbar (sa);

  /* Not vetorizable yet (too conservative dependence test).  */
  for (i = 0; i < N; i++){
    a[i] = b[i] + c[i];
    a[i+1] = b[i] + c[i];
  }
  fbar (a);

  /* Not vetorizable yet (condition in loop).  */
  for (i = 0; i < N; i++){
    a[i] = (b[i] > 0 ? b[i] : 0);
  }
  fbar (a);

  /* Not vetorizable yet (cross-iteration cycle).  */
  diff = 0;
  for (i = 0; i < N; i++) {
    diff += (cb[i] - cc[i]);
  }
  ibar (&diff);

  /* Not vetorizable yet (outer-loop: not attempted. 
     inner-loop: cross iteration cycle; multi-dimensional arrays).  */
  diff = 0;
  for (i = 0; i < N; i++) {
    for (i = 0; i < N; i++) {
      diff += (image[i][j] - block[i][j]);
    }
  }
  ibar (&diff);

  /* Not vectorizable yet (induction)  */
  for ( i = 0; i < N; i++) {
    a[i] = i;
  }
  fbar (a);
}

/* { dg-final { scan-tree-dump-times "vectorized 5 loops" 1 "vect"} } */
