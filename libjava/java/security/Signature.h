
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_Signature__
#define __java_security_Signature__

#pragma interface

#include <java/security/SignatureSpi.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace security
    {
        class AlgorithmParameters;
        class PrivateKey;
        class Provider;
        class PublicKey;
        class SecureRandom;
        class Signature;
      namespace cert
      {
          class Certificate;
      }
      namespace spec
      {
          class AlgorithmParameterSpec;
      }
    }
  }
}

class java::security::Signature : public ::java::security::SignatureSpi
{

public: // actually protected
  Signature(::java::lang::String *);
public:
  static ::java::security::Signature * getInstance(::java::lang::String *);
  static ::java::security::Signature * getInstance(::java::lang::String *, ::java::lang::String *);
  static ::java::security::Signature * getInstance(::java::lang::String *, ::java::security::Provider *);
  virtual ::java::security::Provider * getProvider();
  virtual void initVerify(::java::security::PublicKey *);
  virtual void initVerify(::java::security::cert::Certificate *);
  virtual void initSign(::java::security::PrivateKey *);
  virtual void initSign(::java::security::PrivateKey *, ::java::security::SecureRandom *);
  virtual JArray< jbyte > * sign();
  virtual jint sign(JArray< jbyte > *, jint, jint);
  virtual jboolean verify(JArray< jbyte > *);
  virtual jboolean verify(JArray< jbyte > *, jint, jint);
  virtual void update(jbyte);
  virtual void update(JArray< jbyte > *);
  virtual void update(JArray< jbyte > *, jint, jint);
  virtual ::java::lang::String * getAlgorithm();
  virtual ::java::lang::String * toString();
  virtual void setParameter(::java::lang::String *, ::java::lang::Object *);
  virtual void setParameter(::java::security::spec::AlgorithmParameterSpec *);
  virtual ::java::security::AlgorithmParameters * getParameters();
  virtual ::java::lang::Object * getParameter(::java::lang::String *);
  virtual ::java::lang::Object * clone();
private:
  static ::java::lang::String * SIGNATURE;
public: // actually protected
  static const jint UNINITIALIZED = 0;
  static const jint SIGN = 2;
  static const jint VERIFY = 3;
  jint __attribute__((aligned(__alignof__( ::java::security::SignatureSpi)))) state;
private:
  ::java::lang::String * algorithm;
public: // actually package-private
  ::java::security::Provider * provider;
public:
  static ::java::lang::Class class$;
};

#endif // __java_security_Signature__
