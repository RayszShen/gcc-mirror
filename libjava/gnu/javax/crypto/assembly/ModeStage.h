
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_assembly_ModeStage__
#define __gnu_javax_crypto_assembly_ModeStage__

#pragma interface

#include <gnu/javax/crypto/assembly/Stage.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace crypto
      {
        namespace assembly
        {
            class Direction;
            class ModeStage;
        }
        namespace mode
        {
            class IMode;
        }
      }
    }
  }
}

class gnu::javax::crypto::assembly::ModeStage : public ::gnu::javax::crypto::assembly::Stage
{

public: // actually package-private
  ModeStage(::gnu::javax::crypto::mode::IMode *, ::gnu::javax::crypto::assembly::Direction *);
public:
  virtual ::java::util::Set * blockSizes();
public: // actually package-private
  virtual void initDelegate(::java::util::Map *);
public:
  virtual jint currentBlockSize();
public: // actually package-private
  virtual void resetDelegate();
  virtual void updateDelegate(JArray< jbyte > *, jint, JArray< jbyte > *, jint);
public:
  virtual jboolean selfTest();
private:
  ::gnu::javax::crypto::mode::IMode * __attribute__((aligned(__alignof__( ::gnu::javax::crypto::assembly::Stage)))) delegate;
  ::java::util::Set * cachedBlockSizes;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_assembly_ModeStage__
