/* Test _Float32 constant types.  */
/* { dg-do compile } */
/* { dg-options "" } */
/* { dg-add-options float32 } */
/* { dg-require-effective-target float32 } */

#define WIDTH 32
#define EXT 0
#include "floatn-typeof.h"
