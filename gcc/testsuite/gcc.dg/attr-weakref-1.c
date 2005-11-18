// { dg-do run }
// { dg-require-weak "" }
// { dg-options "-O2" }
// { dg-additional-sources "attr-weakref-1a.c" }

// Copyright 2005 Free Software Foundation, Inc.
// Contributed by Alexandre Oliva <aoliva@redhat.com>

// Torture test for weakrefs.  The first letter of an identifier
// indicates whether/how it is defined; the second letter indicates
// whether it is part of a variable or function test; the number that
// follows is a test counter, and a letter that may follow enables
// multiple identifiers within the same test (e.g., multiple weakrefs
// or pointers to the same identifier).

// Identifiers starting with W are weakrefs; those with p are
// pointers; those with g are global definitions; those with l are
// local definitions; those with w are expected to be weak undefined
// in the symbol table; those with u are expected to be marked as
// non-weak undefined in the symbol table.

#include <stdlib.h>

#define USED __attribute__((used))

typedef int vtype;

extern vtype wv1;
extern vtype Wv1a __attribute__((weakref ("wv1")));
static vtype *pv1a USED = &Wv1a;
extern vtype Wv1b __attribute__((weak, weakref, alias ("wv1")));
static vtype *pv1b USED = &Wv1b;
extern vtype Wv1c __attribute__((weakref));
extern vtype Wv1c __attribute__((alias ("wv1")));
static vtype *pv1c USED = &Wv1c;

vtype gv2;
extern vtype Wv2a __attribute__((weakref ("gv2")));
static vtype *pv2a USED = &Wv2a;

static vtype lv3;
extern vtype Wv3a __attribute__((weakref ("lv3")));
static vtype *pv3a USED = &Wv3a;

extern vtype uv4;
extern vtype Wv4a __attribute__((weakref ("uv4")));
static vtype *pv4a USED = &Wv4a;
static vtype *pv4 USED = &uv4;

extern vtype Wv5a __attribute__((weakref ("uv5")));
static vtype *pv5a USED = &Wv5a;
extern vtype uv5;
static vtype *pv5 USED = &uv5;

extern vtype Wv6a __attribute__((weakref ("wv6")));
static vtype *pv6a USED = &Wv6a;
extern vtype wv6;

extern vtype Wv7a __attribute__((weakref ("uv7")));
static vtype* USED fv7 (void) {
  return &Wv7a;
}
extern vtype uv7;
static vtype* USED fv7a (void) {
  return &uv7;
}

extern vtype uv8;
static vtype* USED fv8a (void) {
  return &uv8;
}
extern vtype Wv8a __attribute__((weakref ("uv8")));
static vtype* USED fv8 (void) {
  return &Wv8a;
}

extern vtype wv9 __attribute__((weak));
extern vtype Wv9a __attribute__((weakref ("wv9")));
static vtype *pv9a USED = &Wv9a;

extern vtype Wv10a __attribute__((weakref ("Wv10b")));
extern vtype Wv10b __attribute__((weakref ("Wv10c")));
extern vtype Wv10c __attribute__((weakref ("Wv10d")));
extern vtype Wv10d __attribute__((weakref ("wv10")));
extern vtype wv10;

extern vtype wv11;
extern vtype Wv11d __attribute__((weakref ("wv11")));
extern vtype Wv11c __attribute__((weakref ("Wv11d")));
extern vtype Wv11b __attribute__((weakref ("Wv11c")));
extern vtype Wv11a __attribute__((weakref ("Wv11b")));

extern vtype Wv12 __attribute__((weakref ("wv12")));
extern vtype wv12 __attribute__((weak));

extern vtype Wv13 __attribute__((weakref ("wv13")));
extern vtype wv13 __attribute__((weak));

extern vtype Wv14a __attribute__((weakref ("wv14")));
extern vtype Wv14b __attribute__((weakref ("wv14")));
extern vtype wv14 __attribute__((weak));

typedef void ftype(void);

extern ftype wf1;
extern ftype Wf1a __attribute__((weakref ("wf1")));
static ftype *pf1a USED = &Wf1a;
extern ftype Wf1b __attribute__((weak, weakref, alias ("wf1")));
static ftype *pf1b USED = &Wf1b;
extern ftype Wf1c __attribute__((weakref));
extern ftype Wf1c __attribute__((alias ("wf1")));
static ftype *pf1c USED = &Wf1c;

void gf2(void) {}
extern ftype Wf2a __attribute__((weakref ("gf2")));
static ftype *pf2a USED = &Wf2a;

static void lf3(void) {}
extern ftype Wf3a __attribute__((weakref ("lf3")));
static ftype *pf3a USED = &Wf3a;

extern ftype uf4;
extern ftype Wf4a __attribute__((weakref ("uf4")));
static ftype *pf4a USED = &Wf4a;
static ftype *pf4 USED = &uf4;

extern ftype Wf5a __attribute__((weakref ("uf5")));
static ftype *pf5a USED = &Wf5a;
extern ftype uf5;
static ftype *pf5 USED = &uf5;

extern ftype Wf6a __attribute__((weakref ("wf6")));
static ftype *pf6a USED = &Wf6a;
extern ftype wf6;

extern ftype Wf7a __attribute__((weakref ("uf7")));
static ftype* USED ff7 (void) {
  return &Wf7a;
}
extern ftype uf7;
static ftype* USED ff7a (void) {
  return &uf7;
}

extern ftype uf8;
static ftype* USED ff8a (void) {
  return &uf8;
}
extern ftype Wf8a __attribute__((weakref ("uf8")));
static ftype* USED ff8 (void) {
  return &Wf8a;
}

extern ftype wf9 __attribute__((weak));
extern ftype Wf9a __attribute__((weakref ("wf9")));
static ftype *pf9a USED = &Wf9a;

extern ftype Wf10a __attribute__((weakref ("Wf10b")));
extern ftype Wf10b __attribute__((weakref ("Wf10c")));
extern ftype Wf10c __attribute__((weakref ("Wf10d")));
extern ftype Wf10d __attribute__((weakref ("wf10")));
extern ftype wf10;

extern ftype wf11;
extern ftype Wf11d __attribute__((weakref ("wf11")));
extern ftype Wf11c __attribute__((weakref ("Wf11d")));
extern ftype Wf11b __attribute__((weakref ("Wf11c")));
extern ftype Wf11a __attribute__((weakref ("Wf11b")));

extern ftype Wf12 __attribute__((weakref ("wf12")));
extern ftype wf12 __attribute__((weak));

extern ftype Wf13 __attribute__((weakref ("wf13")));
extern ftype wf13 __attribute__((weak));

extern ftype Wf14a __attribute__((weakref ("wf14")));
extern ftype Wf14b __attribute__((weakref ("wf14")));
extern ftype wf14 __attribute__((weak));

#define chk(p) do { if (!p) abort (); } while (0)

int main () {
  chk (!pv1a);
  chk (!pv1b);
  chk (!pv1c);
  chk (pv2a);
  chk (pv3a);
  chk (pv4a);
  chk (pv4);
  chk (pv5a);
  chk (pv5);
  chk (!pv6a);
  chk (fv7 ());
  chk (fv7a ());
  chk (fv8 ());
  chk (fv8a ());
  chk (!pv9a);
  chk (!&Wv10a);
  chk (!&Wv11a);
  chk (!&Wv12);
  chk (!&wv12);
  chk (!&wv13);
  chk (!&Wv14a);

  chk (!pf1a);
  chk (!pf1b);
  chk (!pf1c);
  chk (pf2a);
  chk (pf3a);
  chk (pf4a);
  chk (pf4);
  chk (pf5a);
  chk (pf5);
  chk (!pf6a);
  chk (ff7 ());
  chk (ff7a ());
  chk (ff8 ());
  chk (ff8a ());
  chk (!pf9a);
  chk (!&Wf10a);
  chk (!&Wf11a);
  chk (!&Wf12);
  chk (!&wf12);
  chk (!&wf13);
  chk (!&Wf14a);

  exit (0);
}
