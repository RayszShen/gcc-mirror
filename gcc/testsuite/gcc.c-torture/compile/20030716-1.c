/* APPLE LOCAL testsuite nested functions */
/* { dg-options "-fnested-functions" } */
void baz(int i);

void foo(int i, int A[i+1])
{
    int j=A[i];
    void bar() { baz(A[i]); }
}
