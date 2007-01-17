
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_cipher_Rijndael__
#define __gnu_javax_crypto_cipher_Rijndael__

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
            class Rijndael;
        }
      }
    }
  }
}

class gnu::javax::crypto::cipher::Rijndael : public ::gnu::javax::crypto::cipher::BaseCipher
{

public:
  Rijndael();
  static jint getRounds(jint, jint);
private:
  static void rijndaelEncrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *, jint);
  static void rijndaelDecrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *, jint);
  static void aesEncrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *);
  static void aesDecrypt(JArray< jbyte > *, jint, JArray< jbyte > *, jint, ::java::lang::Object *);
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
  static ::java::lang::String * SS;
  static JArray< jbyte > * S;
  static JArray< jbyte > * Si;
  static JArray< jint > * T1;
  static JArray< jint > * T2;
  static JArray< jint > * T3;
  static JArray< jint > * T4;
  static JArray< jint > * T5;
  static JArray< jint > * T6;
  static JArray< jint > * T7;
  static JArray< jint > * T8;
  static JArray< jint > * U1;
  static JArray< jint > * U2;
  static JArray< jint > * U3;
  static JArray< jint > * U4;
  static JArray< jbyte > * rcon;
  static JArray< JArray< JArray< jint > * > * > * shifts;
  static JArray< jbyte > * KAT_KEY;
  static JArray< jbyte > * KAT_CT;
  static ::java::lang::Boolean * valid;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_cipher_Rijndael__
