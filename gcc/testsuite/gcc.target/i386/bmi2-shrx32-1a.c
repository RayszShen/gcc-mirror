/* { dg-do compile } */
/* { dg-options "-O2 -mbmi2 -dp" } */

#include "bmi2-shrx32-1.c"

/* { dg-final { scan-assembler-times "bmi2_lshrsi3" 1 } } */
