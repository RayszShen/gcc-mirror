// { dg-do compile }

void foo(int i)
{
  int j;
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp parallel
    { case 0:; }
  }
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp for
    for (j = 0; j < 10; ++ j)
      { case 1:; }
  }
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp critical
    { case 2:; }
  }
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp master
    { case 3:; }
  }
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp sections
    { case 4:;
    #pragma omp section
       { case 5:; }
    }
  }
  switch (i) // { dg-error "invalid entry to OpenMP structured block" }
  {
  #pragma omp ordered
    { default:; }
  }
}
