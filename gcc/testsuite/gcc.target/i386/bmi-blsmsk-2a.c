/* { dg-do compile } */
/* { dg-options "-O2 -mbmi -fno-inline -dp" } */

#include "bmi-blsmsk-2.c"

/* { dg-final { scan-assembler-times "bmi_blsmsk_si" 1 } } */
