
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_DoubleSeqHolder__
#define __org_omg_CORBA_DoubleSeqHolder__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
      namespace typecodes
      {
          class ArrayTypeCode;
      }
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class DoubleSeqHolder;
          class TypeCode;
        namespace portable
        {
            class InputStream;
            class OutputStream;
        }
      }
    }
  }
}

class org::omg::CORBA::DoubleSeqHolder : public ::java::lang::Object
{

public:
  DoubleSeqHolder();
  DoubleSeqHolder(JArray< jdouble > *);
  void _read(::org::omg::CORBA::portable::InputStream *);
  ::org::omg::CORBA::TypeCode * _type();
  void _write(::org::omg::CORBA::portable::OutputStream *);
  JArray< jdouble > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) value;
private:
  ::gnu::CORBA::typecodes::ArrayTypeCode * typecode;
public:
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_DoubleSeqHolder__
