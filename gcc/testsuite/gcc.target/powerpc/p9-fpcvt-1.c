/* { dg-do compile { target { powerpc64*-*-* && lp64 } } } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power9" } } */
/* { dg-require-effective-target powerpc_p9vector_ok } */
/* { dg-options "-mcpu=power9 -O2" } */

void sc (signed char    *p, double x) { *p = x; }
void uc (unsigned char  *p, double x) { *p = x; }
void ss (signed short   *p, double x) { *p = x; }
void us (unsigned short *p, double x) { *p = x; }
void si (signed int     *p, double x) { *p = x; }
void ui (unsigned int   *p, double x) { *p = x; }

/* { dg-final { scan-assembler     "stxsibx" } } */
/* { dg-final { scan-assembler     "stxsihx" } } */
/* { dg-final { scan-assembler     "stxsiwx" } } */
/* { dg-final { scan-assembler-not "mfvsrd"  } } */
/* { dg-final { scan-assembler-not "mfvsrwz" } } */
/* { dg-final { scan-assembler-not "mtvsrd"  } } */
/* { dg-final { scan-assembler-not "mtvsrwa" } } */
/* { dg-final { scan-assembler-not "mtvsrwz" } } */
