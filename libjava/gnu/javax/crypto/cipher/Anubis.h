
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_cipher_Anubis__
#define __gnu_javax_crypto_cipher_Anubis__

#pragma interface

#include <gnu/javax/crypto/cipher/BaseCipher.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace crypto
      {
        namespace cipher
        {
            class Anubis;
        }
      }
    }
  }
}

class gnu::javax::crypto::cipher::Anubis : public ::gnu::javax::crypto::cipher::BaseCipher
{

public:
  Anubis();
private:
  static void anubis(JArray< jbyte > *, jint, JArray< jbyte > *, jint, JArray< JArray< jint > * > *);
public:
  ::java::lang::Object * clone();
  ::java::util::Iterator * blockSizes();
  ::java::util::Iterator * keySizes();
  ::java::lang::Object * makeKey(JArray< jbyte > *, jint);
  void encrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *, jint);
  void decrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *, jint);
  jboolean selfTest();
private:
  static ::java::util::logging::Logger * log;
  static const jint DEFAULT_BLOCK_SIZE = 16;
  static const jint DEFAULT_KEY_SIZE = 16;
  static ::java::lang::String * Sd;
  static JArray< jbyte > * S;
  static JArray< jint > * T0;
  static JArray< jint > * T1;
  static JArray< jint > * T2;
  static JArray< jint > * T3;
  static JArray< jint > * T4;
  static JArray< jint > * T5;
  static JArray< jint > * rc;
  static JArray< jbyte > * KAT_KEY;
  static JArray< jbyte > * KAT_CT;
  static ::java::lang::Boolean * valid;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_cipher_Anubis__
