
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_imageio_metadata_IIOInvalidTreeException__
#define __javax_imageio_metadata_IIOInvalidTreeException__

#pragma interface

#include <javax/imageio/IIOException.h>
extern "Java"
{
  namespace javax
  {
    namespace imageio
    {
      namespace metadata
      {
          class IIOInvalidTreeException;
      }
    }
  }
  namespace org
  {
    namespace w3c
    {
      namespace dom
      {
          class Node;
      }
    }
  }
}

class javax::imageio::metadata::IIOInvalidTreeException : public ::javax::imageio::IIOException
{

public:
  IIOInvalidTreeException(::java::lang::String *, ::org::w3c::dom::Node *);
  IIOInvalidTreeException(::java::lang::String *, ::java::lang::Throwable *, ::org::w3c::dom::Node *);
  virtual ::org::w3c::dom::Node * getOffendingNode();
private:
  static const jlong serialVersionUID = -1314083172544132777LL;
public: // actually protected
  ::org::w3c::dom::Node * __attribute__((aligned(__alignof__( ::javax::imageio::IIOException)))) offendingNode;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_imageio_metadata_IIOInvalidTreeException__
