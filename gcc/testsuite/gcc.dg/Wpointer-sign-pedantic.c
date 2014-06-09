/* This is from PR c/25892.  See Wpointer-sign.c for more details.  */

/* { dg-options "-pedantic" } */

void foo(unsigned long* ulp);/* { dg-message "note: expected '\[^'\n\]*' but argument is of type '\[^'\n\]*'" "note: expected" { target *-*-* } 5 } */

void bar(long* lp) {
  foo(lp); /* { dg-warning "differ in signedness" } */
}
