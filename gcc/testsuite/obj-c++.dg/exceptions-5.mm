/* Contributed by Nicola Pero <nicola.pero@meta-innovation.com>, November 2010.  */
/* { dg-options "-fobjc-exceptions" } */
/* { dg-do compile } */

/* Test that you can use an unnamed argument with @catch.  This test is the same
   as exceptions-3.mm, but with no name for @catch arguments.  */

#include <objc/objc.h>

@interface MyObject
{
  Class isa;
} /* { dg-line interface_MyObject } */
@end

@implementation MyObject
@end

@protocol MyProtocol;

typedef MyObject MyObjectTypedef;
typedef MyObject *MyObjectPtrTypedef;
typedef int intTypedef;

int test (id object)
{
  int dummy = 0;

  @try { @throw object; }
  @catch (int)          /* { dg-error "'@catch' parameter is not a known Objective-C class type" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (intTypedef)   /* { dg-error "'@catch' parameter is not a known Objective-C class type" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (int *)         /* { dg-error "'@catch' parameter is not a known Objective-C class type" } */
    {
      dummy++;
    }  

  @try { @throw object; }
  @catch (id)           /* Ok */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (id <MyProtocol>) /* { dg-error "'@catch' parameter cannot be protocol-qualified" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObject *)    /* Ok */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObject <MyProtocol> *)  /* { dg-error "'@catch' parameter cannot be protocol-qualified" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObject)     /* { dg-error "'@catch' parameter is not a known Objective-C class type" } */
    {                     /* { dg-error "no matching function" "" { target *-*-* } .-1 } */
      dummy++;            /* { dg-message "MyObject" "" { target *-*-* } interface_MyObject } */
    }                     /* { dg-message "candidate" "" { target *-*-* } interface_MyObject } */

  @try { @throw object; }
  @catch (static MyObject *) /* { dg-error "storage class" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObjectTypedef *) /* Ok */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObjectTypedef <MyProtocol> *) /* { dg-error "'@catch' parameter cannot be protocol-qualified" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (MyObjectPtrTypedef) /* Ok */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (Class)   /* { dg-error "'@catch' parameter is not a known Objective-C class type" } */
    {
      dummy++;
    }

  @try { @throw object; }
  @catch (...)            /* Ok */
    {
      dummy++;
    }

  return dummy;
}
