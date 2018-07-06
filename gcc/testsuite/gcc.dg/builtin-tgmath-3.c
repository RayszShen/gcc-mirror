/* Test __builtin_tgmath: integer arguments mapped to _Float64.  */
/* { dg-do run } */
/* { dg-options "" } */
/* { dg-add-options float32 } */
/* { dg-add-options float64 } */
/* { dg-require-effective-target float32_runtime } */
/* { dg-require-effective-target float64_runtime } */

extern void abort (void);
extern void exit (int);

#define CHECK_CALL(C, E, V)			\
  do						\
    {						\
      if ((C) != (E))				\
	abort ();				\
      extern __typeof (C) V;			\
    }						\
  while (0)

extern _Float32 var_f32;

_Float32 t1f (float x) { return x + 1; }
_Float32 t1d (double x) { return x + 2; }
_Float32 t1l (long double x) { return x + 3; }
_Float32 t1f64 (_Float64 x) { return x + 4; }

#define t1v(x) __builtin_tgmath (t1f, t1d, t1l, t1f64, x)

static void
test_1 (void)
{
  float f = 1;
  double d = 2;
  long double ld = 3;
  _Float64 f64 = 4;
  int i = 5;
  CHECK_CALL (t1v (f), 2, var_f32);
  CHECK_CALL (t1v (d), 4, var_f32);
  CHECK_CALL (t1v (ld), 6, var_f32);
  CHECK_CALL (t1v (f64), 8, var_f32);
  CHECK_CALL (t1v (i), 9, var_f32);
}

int
main (void)
{
  test_1 ();
  exit (0);
}
