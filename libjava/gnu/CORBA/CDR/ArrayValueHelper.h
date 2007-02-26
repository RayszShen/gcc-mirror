
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_CDR_ArrayValueHelper__
#define __gnu_CORBA_CDR_ArrayValueHelper__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
      namespace CDR
      {
          class ArrayValueHelper;
      }
    }
  }
  namespace javax
  {
    namespace rmi
    {
      namespace CORBA
      {
          class ValueHandler;
      }
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
        namespace portable
        {
            class InputStream;
            class OutputStream;
        }
      }
    }
  }
}

class gnu::CORBA::CDR::ArrayValueHelper : public ::java::lang::Object
{

public: // actually package-private
  virtual jboolean written_as_object();
  ArrayValueHelper(::java::lang::Class *);
public:
  virtual ::java::lang::String * get_id();
  virtual ::java::io::Serializable * read_value(::org::omg::CORBA::portable::InputStream *);
  virtual void write_value(::org::omg::CORBA::portable::OutputStream *, ::java::io::Serializable *);
public: // actually package-private
  static ::javax::rmi::CORBA::ValueHandler * handler;
  ::java::lang::Class * __attribute__((aligned(__alignof__( ::java::lang::Object)))) arrayClass;
  ::java::lang::Class * component;
  ::java::lang::String * componentId;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_CDR_ArrayValueHelper__
