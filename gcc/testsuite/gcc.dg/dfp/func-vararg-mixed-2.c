/* { dg-do run { target { { i?86-*-* x86_64-*-* } && ilp32 } } } */
/* { dg-options "-std=gnu99 -mpreferred-stack-boundary=2" } */

/* C99 6.5.2.2 Function calls.
   Test passing varargs of the combination of decimal float types and
   other types.  */

#include <stdarg.h>

extern void abort (void);
static int failcnt = 0;
                                                                                
/* Support compiling the test to report individual failures; default is
   to abort as soon as a check fails.  */
#ifdef DBG
#include <stdio.h>
#define FAILURE { printf ("failed at line %d\n", __LINE__); failcnt++; }
#else
#define FAILURE abort ();
#endif

/* Supposing the list of varying number of arguments is:
   unsigned int, _Decimal128, double, _Decimal32, _Decimal64.  */

static _Decimal32
vararg_d32 (unsigned arg, ...)
{
  va_list ap;
  _Decimal32 result;

  va_start (ap, arg);

  va_arg (ap, unsigned int);
  va_arg (ap, _Decimal128);
  va_arg (ap, double);
  result = va_arg (ap, _Decimal32);

  va_end (ap);
  return result;
}

static _Decimal32
vararg_d64 (unsigned arg, ...)
{
  va_list ap;
  _Decimal64 result;

  va_start (ap, arg);

  va_arg (ap, unsigned int);
  va_arg (ap, _Decimal128);
  va_arg (ap, double);
  va_arg (ap, _Decimal32);
  result = va_arg (ap, _Decimal64);

  va_end (ap);
  return result;
}

static _Decimal128
vararg_d128 (unsigned arg, ...)
{
  va_list ap;
  _Decimal128 result;

  va_start (ap, arg);

  va_arg (ap, unsigned int);
  result = va_arg (ap, _Decimal128);

  va_end (ap);
  return result;
}

static unsigned int
vararg_int (unsigned arg, ...)
{
  va_list ap;
  unsigned int result;

  va_start (ap, arg);

  result = va_arg (ap, unsigned int);

  va_end (ap);
  return result;
}

static double
vararg_double (unsigned arg, ...)
{
  va_list ap;
  float result;

  va_start (ap, arg);

  va_arg (ap, unsigned int);
  va_arg (ap, _Decimal128);
  result = va_arg (ap, double);

  va_end (ap);
  return result;
}


int
main ()
{
  if (vararg_d32 (3, 0, 1.0dl, 2.0, 3.0df, 4.0dd) != 3.0df) FAILURE
  if (vararg_d64 (4, 0, 1.0dl, 2.0, 3.0df, 4.0dd) != 4.0dd) FAILURE
  if (vararg_d128 (1, 0, 1.0dl, 2.0, 3.0df, 4.0dd) != 1.0dl) FAILURE
  if (vararg_int (0, 0, 1.0dl, 2.0, 3.0df, 4.0dd) != 0) FAILURE
  if (vararg_double (2, 0, 1.0dl, 2.0, 3.0df, 4.0dd) != 2.0) FAILURE

  if (failcnt != 0)
    abort ();
  return 0;
}
