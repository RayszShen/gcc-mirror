/* { dg-do compile { target { ! ia32 } } } */
/* { dg-options "-O2 -mbmi2 -dp" } */

#include "bmi2-sarx64-1.c"

/* { dg-final { scan-assembler-times "bmi2_ashrdi3" 1 } } */
