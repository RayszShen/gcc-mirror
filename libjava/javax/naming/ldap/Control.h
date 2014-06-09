
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_naming_ldap_Control__
#define __javax_naming_ldap_Control__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace naming
    {
      namespace ldap
      {
          class Control;
      }
    }
  }
}

class javax::naming::ldap::Control : public ::java::lang::Object
{

public:
  virtual ::java::lang::String * getID() = 0;
  virtual jboolean isCritical() = 0;
  virtual JArray< jbyte > * getEncodedValue() = 0;
  static const jboolean CRITICAL = 1;
  static const jboolean NONCRITICAL = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __javax_naming_ldap_Control__
