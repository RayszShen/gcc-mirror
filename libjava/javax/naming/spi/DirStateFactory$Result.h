
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_naming_spi_DirStateFactory$Result__
#define __javax_naming_spi_DirStateFactory$Result__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace naming
    {
      namespace directory
      {
          class Attributes;
      }
      namespace spi
      {
          class DirStateFactory$Result;
      }
    }
  }
}

class javax::naming::spi::DirStateFactory$Result : public ::java::lang::Object
{

public:
  DirStateFactory$Result(::java::lang::Object *, ::javax::naming::directory::Attributes *);
  virtual ::java::lang::Object * getObject();
  virtual ::javax::naming::directory::Attributes * getAttributes();
private:
  ::java::lang::Object * __attribute__((aligned(__alignof__( ::java::lang::Object)))) obj;
  ::javax::naming::directory::Attributes * outAttrs;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_naming_spi_DirStateFactory$Result__
