/* { dg-do compile } */

#define N 40
#define M 128
unsigned char in[N+M];
unsigned short out[N];

/* Outer-loop vectorization. */
/* Not vectorized due to multiple-types in the inner-loop.  */

void
foo (){
  int i,j;
  unsigned short diff;

  for (i = 0; i < N; i++) {
    diff = 0;
    for (j = 0; j < M; j+=8) {
      diff += in[j+i];
    }
    out[i]=diff;
  }
}

/* { dg-final { scan-tree-dump-times "OUTER LOOP VECTORIZED" 1 "vect" { xfail *-*-* } } } */
/* { dg-final { cleanup-tree-dump "vect" } } */
