/* { dg-do compile } */
/* { dg-options "-fgimple -O -Wmaybe-uninitialized" } */

unsigned int __GIMPLE (ssa,startwith("uninit1"))
foo (unsigned int v)
{
  unsigned int undef;        /* { dg-bogus "may be used uninitialized" } */
  unsigned int _2;
  unsigned int _9;
  unsigned int _10;

  __BB(2):
  if (v_4(D) < 100u)
    goto __BB3;
  else
    goto __BB4;

  /* 'undef' is defined conditionally (under 'v < 100' predicate).  */
  __BB(3):
  undef_8 = 8u;
  goto __BB4;

  /* An undef value flows into a phi.  */
  __BB(4):
  undef_1 = __PHI (__BB2: undef_5(D), __BB3: undef_8);
  if (v_4(D) == 42u)
    goto __BB5;
  else
    goto __BB6;

  /* The phi value is used here (under 'v == 42' predicate).  */
  __BB(5):
  _9 = undef_1;
  goto __BB7;

  __BB(6):
  _10 = v_4(D);
  goto __BB7;

  __BB(7):
  _2 = __PHI (__BB5: _9, __BB6: _10);
  return _2;
}
