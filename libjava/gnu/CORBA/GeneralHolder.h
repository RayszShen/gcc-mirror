
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_GeneralHolder__
#define __gnu_CORBA_GeneralHolder__

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
          class BufferedCdrOutput;
      }
        class GeneralHolder;
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

class gnu::CORBA::GeneralHolder : public ::java::lang::Object
{

public:
  GeneralHolder(::gnu::CORBA::CDR::BufferedCdrOutput *);
  virtual void _read(::org::omg::CORBA::portable::InputStream *);
  virtual ::org::omg::CORBA::TypeCode * _type();
  virtual void _write(::org::omg::CORBA::portable::OutputStream *);
public: // actually package-private
  virtual ::org::omg::CORBA::portable::InputStream * getInputStream();
public:
  virtual ::gnu::CORBA::GeneralHolder * Clone();
private:
  ::gnu::CORBA::CDR::BufferedCdrOutput * __attribute__((aligned(__alignof__( ::java::lang::Object)))) value;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_GeneralHolder__
