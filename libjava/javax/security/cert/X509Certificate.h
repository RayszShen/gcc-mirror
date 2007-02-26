
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_security_cert_X509Certificate__
#define __javax_security_cert_X509Certificate__

#pragma interface

#include <javax/security/cert/Certificate.h>
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
        class Principal;
    }
  }
  namespace javax
  {
    namespace security
    {
      namespace cert
      {
          class X509Certificate;
      }
    }
  }
}

class javax::security::cert::X509Certificate : public ::javax::security::cert::Certificate
{

public:
  X509Certificate();
  static ::javax::security::cert::X509Certificate * getInstance(JArray< jbyte > *);
  static ::javax::security::cert::X509Certificate * getInstance(::java::io::InputStream *);
  virtual void checkValidity() = 0;
  virtual void checkValidity(::java::util::Date *) = 0;
  virtual jint getVersion() = 0;
  virtual ::java::math::BigInteger * getSerialNumber() = 0;
  virtual ::java::security::Principal * getIssuerDN() = 0;
  virtual ::java::security::Principal * getSubjectDN() = 0;
  virtual ::java::util::Date * getNotBefore() = 0;
  virtual ::java::util::Date * getNotAfter() = 0;
  virtual ::java::lang::String * getSigAlgName() = 0;
  virtual ::java::lang::String * getSigAlgOID() = 0;
  virtual JArray< jbyte > * getSigAlgParams() = 0;
  static ::java::lang::Class class$;
};

#endif // __javax_security_cert_X509Certificate__
