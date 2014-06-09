
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_interfaces_DSAPrivateKey__
#define __java_security_interfaces_DSAPrivateKey__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace math
    {
        class BigInteger;
    }
    namespace security
    {
      namespace interfaces
      {
          class DSAParams;
          class DSAPrivateKey;
      }
    }
  }
}

class java::security::interfaces::DSAPrivateKey : public ::java::lang::Object
{

public:
  virtual ::java::math::BigInteger * getX() = 0;
  virtual ::java::security::interfaces::DSAParams * getParams() = 0;
  virtual ::java::lang::String * getAlgorithm() = 0;
  virtual ::java::lang::String * getFormat() = 0;
  virtual JArray< jbyte > * getEncoded() = 0;
  static const jlong serialVersionUID = 7776497482533790279LL;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_security_interfaces_DSAPrivateKey__
