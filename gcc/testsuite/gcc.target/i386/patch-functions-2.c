/* { dg-do run} */
/* { dg-options "-mpatch-functions-for-instrumentation -mno-patch-functions-main-always" } */

/* Function is small to be instrumented with default values. Check there
   aren't any nop-bytes at beginning or end of function.  */

/* { dg-final { scan-assembler-not ".byte\t0xeb,0x09(.*).byte\t0x90" } } */
/* { dg-final { scan-assembler-not "ret(.*).byte\t0x90(.*).byte\t0x90" } } */

void foo() {
  int x = 0;
}

int main() {
  foo();
  return 0;
}
