
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_pad_IPad__
#define __gnu_javax_crypto_pad_IPad__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace crypto
      {
        namespace pad
        {
            class IPad;
        }
      }
    }
  }
}

class gnu::javax::crypto::pad::IPad : public ::java::lang::Object
{

public:
  virtual ::java::lang::String * name() = 0;
  virtual void init(jint) = 0;
  virtual void init(::java::util::Map *) = 0;
  virtual JArray< jbyte > * pad(JArray< jbyte > *, jint, jint) = 0;
  virtual jint unpad(JArray< jbyte > *, jint, jint) = 0;
  virtual void reset() = 0;
  virtual jboolean selfTest() = 0;
  static ::java::lang::String * PADDING_BLOCK_SIZE;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __gnu_javax_crypto_pad_IPad__
