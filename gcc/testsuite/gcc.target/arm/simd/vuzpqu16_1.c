/* Test the `vuzpQu16' ARM Neon intrinsic.  */

/* { dg-do run } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O1 -fno-inline" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"
#include "../../aarch64/simd/vuzpqu16.x"

/* { dg-final { scan-assembler-times "vuzp\.16\[ \t\]+\[qQ\]\[0-9\]+, ?\[qQ\]\[0-9\]+!?\(?:\[ \t\]+@\[a-zA-Z0-9 \]+\)?\n" 1 } } */
/* { dg-final { cleanup-saved-temps } } */
