
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_security_jce_sig_EncodedKeyFactory__
#define __gnu_java_security_jce_sig_EncodedKeyFactory__

#pragma interface

#include <java/security/KeyFactorySpi.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace security
      {
        namespace jce
        {
          namespace sig
          {
              class EncodedKeyFactory;
          }
        }
        namespace key
        {
          namespace dss
          {
              class DSSPublicKey;
          }
          namespace rsa
          {
              class GnuRSAPublicKey;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace security
    {
        class Key;
        class PrivateKey;
        class PublicKey;
      namespace spec
      {
          class DSAPrivateKeySpec;
          class DSAPublicKeySpec;
          class KeySpec;
          class RSAPrivateCrtKeySpec;
          class RSAPublicKeySpec;
      }
    }
  }
  namespace javax
  {
    namespace crypto
    {
      namespace interfaces
      {
          class DHPrivateKey;
          class DHPublicKey;
      }
      namespace spec
      {
          class DHPrivateKeySpec;
          class DHPublicKeySpec;
      }
    }
  }
}

class gnu::java::security::jce::sig::EncodedKeyFactory : public ::java::security::KeyFactorySpi
{

public:
  EncodedKeyFactory();
private:
  static ::java::lang::Object * invokeConstructor(::java::lang::String *, JArray< ::java::lang::Object * > *);
  static ::java::lang::Class * getConcreteClass(::java::lang::String *);
  static ::java::lang::reflect::Constructor * getConcreteCtor(::java::lang::Class *);
  static ::java::lang::Object * invokeValueOf(::java::lang::String *, JArray< jbyte > *);
  static ::java::lang::reflect::Method * getValueOfMethod(::java::lang::Class *);
public: // actually protected
  virtual ::java::security::PublicKey * engineGeneratePublic(::java::security::spec::KeySpec *);
  virtual ::java::security::PrivateKey * engineGeneratePrivate(::java::security::spec::KeySpec *);
  virtual ::java::security::spec::KeySpec * engineGetKeySpec(::java::security::Key *, ::java::lang::Class *);
  virtual ::java::security::Key * engineTranslateKey(::java::security::Key *);
private:
  ::gnu::java::security::key::dss::DSSPublicKey * decodeDSSPublicKey(::java::security::spec::DSAPublicKeySpec *);
  ::gnu::java::security::key::rsa::GnuRSAPublicKey * decodeRSAPublicKey(::java::security::spec::RSAPublicKeySpec *);
  ::javax::crypto::interfaces::DHPublicKey * decodeDHPublicKey(::javax::crypto::spec::DHPublicKeySpec *);
  ::javax::crypto::interfaces::DHPublicKey * decodeDHPublicKey(JArray< jbyte > *);
  ::java::security::PrivateKey * decodeDSSPrivateKey(::java::security::spec::DSAPrivateKeySpec *);
  ::java::security::PrivateKey * decodeRSAPrivateKey(::java::security::spec::RSAPrivateCrtKeySpec *);
  ::javax::crypto::interfaces::DHPrivateKey * decodeDHPrivateKey(::javax::crypto::spec::DHPrivateKeySpec *);
  ::javax::crypto::interfaces::DHPrivateKey * decodeDHPrivateKey(JArray< jbyte > *);
  static ::java::util::logging::Logger * log;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_security_jce_sig_EncodedKeyFactory__
