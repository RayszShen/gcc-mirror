/* Check if class references (generated for the NeXT runtime) are appropriately 
   folded. */
/* Author: Ziemowit Laski <zlaski@apple.com>.  */
/* The ABI is different for m64 darwin so skip this test for now */
/* { dg-do compile { target { *-*-darwin* && { ! lp64 } } } } */
/* { dg-skip-if "" { *-*-darwin* } { "-fgnu-runtime" } { "" } } */

#include "../objc-obj-c++-shared/Object1.h"

typedef Object ObjectTypedef1;
typedef ObjectTypedef1 ObjectTypedef2;
@compatibility_alias ObjectAlias1 ObjectTypedef2;
@compatibility_alias ObjectAlias2 ObjectAlias1;
typedef ObjectAlias2 ObjectTypedef3;

void foo(void) {
  id obj = [Object new];
  obj = [ObjectTypedef1 new];
  obj = [ObjectTypedef2 new];
  obj = [ObjectTypedef3 new];
  obj = [ObjectAlias1 new];
  obj = [ObjectAlias2 new];
}

/* { dg-final { scan-assembler "_OBJC_CLASS_REFERENCES_0" } } */
/* { dg-final { scan-assembler-not "_OBJC_CLASS_REFERENCES_1" } } */
