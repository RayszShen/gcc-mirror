/* APPLE LOCAL file Objective-C++ */
/* Test calling super from within a category method.  */
/* { dg-do compile } */
#include <objc/objc.h>

@interface NSObject
@end
@interface NSMenuItem: NSObject
@end

@interface NSObject (Test)
+ (int) test_func;
@end

@implementation NSObject (Test)
+ (int) test_func
{}
@end

@interface NSMenuItem (Test)
+ (int) test_func;
@end

@implementation NSMenuItem (Test)
+ (int) test_func
{
   return [super test_func];  /* { dg-bogus "invalid use of undefined type" } */
}   /* { dg-bogus "forward declaration of" "" { target *-*-* } 27 } */
@end

