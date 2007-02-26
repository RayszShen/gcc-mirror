
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_security_key_dss_DSSKey__
#define __gnu_java_security_key_dss_DSSKey__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace security
      {
        namespace key
        {
          namespace dss
          {
              class DSSKey;
          }
        }
      }
    }
  }
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
      }
    }
  }
}

class gnu::java::security::key::dss::DSSKey : public ::java::lang::Object
{

public: // actually protected
  DSSKey(jint, ::java::math::BigInteger *, ::java::math::BigInteger *, ::java::math::BigInteger *);
public:
  virtual ::java::security::interfaces::DSAParams * getParams();
  virtual ::java::lang::String * getAlgorithm();
  virtual JArray< jbyte > * getEncoded();
  virtual ::java::lang::String * getFormat();
  virtual jboolean equals(::java::lang::Object *);
  virtual ::java::lang::String * toString();
  virtual JArray< jbyte > * getEncoded(jint) = 0;
  virtual jboolean hasInheritedParameters();
public: // actually protected
  ::java::math::BigInteger * __attribute__((aligned(__alignof__( ::java::lang::Object)))) p;
  ::java::math::BigInteger * q;
  ::java::math::BigInteger * g;
  jint defaultFormat;
private:
  ::java::lang::String * str;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_security_key_dss_DSSKey__
