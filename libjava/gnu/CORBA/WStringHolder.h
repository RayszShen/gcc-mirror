
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_WStringHolder__
#define __gnu_CORBA_WStringHolder__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
        class WStringHolder;
      namespace typecodes
      {
          class StringTypeCode;
      }
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
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

class gnu::CORBA::WStringHolder : public ::java::lang::Object
{

public:
  WStringHolder();
  WStringHolder(::java::lang::String *);
  virtual void _read(::org::omg::CORBA::portable::InputStream *);
  virtual ::org::omg::CORBA::TypeCode * _type();
  virtual void _write(::org::omg::CORBA::portable::OutputStream *);
private:
  static ::gnu::CORBA::typecodes::StringTypeCode * t_string;
public:
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::lang::Object)))) value;
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_WStringHolder__
