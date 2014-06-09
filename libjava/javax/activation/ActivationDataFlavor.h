
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_activation_ActivationDataFlavor__
#define __javax_activation_ActivationDataFlavor__

#pragma interface

#include <java/awt/datatransfer/DataFlavor.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace datatransfer
      {
          class DataFlavor;
      }
    }
  }
  namespace javax
  {
    namespace activation
    {
        class ActivationDataFlavor;
    }
  }
}

class javax::activation::ActivationDataFlavor : public ::java::awt::datatransfer::DataFlavor
{

public:
  ActivationDataFlavor(::java::lang::Class *, ::java::lang::String *, ::java::lang::String *);
  ActivationDataFlavor(::java::lang::Class *, ::java::lang::String *);
  ActivationDataFlavor(::java::lang::String *, ::java::lang::String *);
  virtual ::java::lang::String * getMimeType();
  virtual ::java::lang::Class * getRepresentationClass();
  virtual ::java::lang::String * getHumanPresentableName();
  virtual void setHumanPresentableName(::java::lang::String *);
  virtual jboolean equals(::java::awt::datatransfer::DataFlavor *);
  virtual jboolean isMimeTypeEqual(::java::lang::String *);
public: // actually protected
  virtual ::java::lang::String * normalizeMimeTypeParameter(::java::lang::String *, ::java::lang::String *);
  virtual ::java::lang::String * normalizeMimeType(::java::lang::String *);
private:
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::awt::datatransfer::DataFlavor)))) mimeType;
  ::java::lang::Class * representationClass;
  ::java::lang::String * humanPresentableName;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_activation_ActivationDataFlavor__
