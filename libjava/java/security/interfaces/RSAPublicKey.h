
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_interfaces_RSAPublicKey__
#define __java_security_interfaces_RSAPublicKey__

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
          class RSAPublicKey;
      }
    }
  }
}

class java::security::interfaces::RSAPublicKey : public ::java::lang::Object
{

public:
  virtual ::java::math::BigInteger * getPublicExponent() = 0;
  virtual ::java::lang::String * getAlgorithm() = 0;
  virtual ::java::lang::String * getFormat() = 0;
  virtual JArray< jbyte > * getEncoded() = 0;
  virtual ::java::math::BigInteger * getModulus() = 0;
  static const jlong serialVersionUID = -8727434096241101194LL;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_security_interfaces_RSAPublicKey__
