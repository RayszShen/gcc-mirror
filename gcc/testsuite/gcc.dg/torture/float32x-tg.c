/* Test _Float32x type-generic built-in functions.  */
/* { dg-do run } */
/* { dg-options "" } */
/* { dg-add-options float32x } */
/* { dg-require-effective-target float32x_runtime } */

#define WIDTH 32
#define EXT 1
#include "floatn-tg.h"
