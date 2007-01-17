
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_keyring_BinaryDataEntry__
#define __gnu_javax_crypto_keyring_BinaryDataEntry__

#pragma interface

#include <gnu/javax/crypto/keyring/PrimitiveEntry.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace crypto
      {
        namespace keyring
        {
            class BinaryDataEntry;
            class Properties;
        }
      }
    }
  }
}

class gnu::javax::crypto::keyring::BinaryDataEntry : public ::gnu::javax::crypto::keyring::PrimitiveEntry
{

public:
  BinaryDataEntry(::java::lang::String *, JArray< jbyte > *, ::java::util::Date *, ::gnu::javax::crypto::keyring::Properties *);
private:
  BinaryDataEntry();
public:
  static ::gnu::javax::crypto::keyring::BinaryDataEntry * decode(::java::io::DataInputStream *);
  virtual ::java::lang::String * getContentType();
  virtual JArray< jbyte > * getData();
public: // actually protected
  virtual void encodePayload();
public:
  static const jint TYPE = 9;
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_keyring_BinaryDataEntry__
