
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_print_attribute_Attribute__
#define __javax_print_attribute_Attribute__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace print
    {
      namespace attribute
      {
          class Attribute;
      }
    }
  }
}

class javax::print::attribute::Attribute : public ::java::lang::Object
{

public:
  virtual ::java::lang::Class * getCategory() = 0;
  virtual ::java::lang::String * getName() = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __javax_print_attribute_Attribute__
