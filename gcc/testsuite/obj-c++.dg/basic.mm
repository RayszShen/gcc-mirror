// A basic sanity check for Objective-C++.
// { dg-do run }
/* { dg-xfail-run-if "Needs OBJC2 ABI" { *-*-darwin* && { lp64 && { ! objc2 } } } { "-fnext-runtime" } { "" } } */
#include "../objc-obj-c++-shared/Object1.h"
#include <iostream>

@interface Greeter : Object
- (void) greet: (const char *)msg;
@end

@implementation Greeter
- (void) greet: (const char *)msg { std::cout << msg; }
@end

int
main ()
{
  std::cout << "Hello from C++\n";
  Greeter *obj = [Greeter new];
  [obj greet: "Hello from Objective-C\n"];
}
#include "../objc-obj-c++-shared/Object1-implementation.h"
