/* This test needs runtime that provides stpcpy function.  */
/* { dg-do run { target *-*-linux* *-*-gnu* } } */
/* { dg-options "-O2" } */

#define USE_GNU
#include "strlenopt-12.c"
