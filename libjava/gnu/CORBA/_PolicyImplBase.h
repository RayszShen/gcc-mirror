
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA__PolicyImplBase__
#define __gnu_CORBA__PolicyImplBase__

#pragma interface

#include <org/omg/CORBA/portable/ObjectImpl.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
        class _PolicyImplBase;
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class Policy;
        namespace portable
        {
            class InputStream;
            class OutputStream;
            class ResponseHandler;
        }
      }
    }
  }
}

class gnu::CORBA::_PolicyImplBase : public ::org::omg::CORBA::portable::ObjectImpl
{

public:
  _PolicyImplBase(jint, ::java::lang::Object *, jint, ::java::lang::String *);
  virtual jint policy_type();
  virtual JArray< ::java::lang::String * > * _ids();
  virtual ::org::omg::CORBA::portable::OutputStream * _invoke(::java::lang::String *, ::org::omg::CORBA::portable::InputStream *, ::org::omg::CORBA::portable::ResponseHandler *);
  virtual ::java::lang::Object * getValue();
  virtual jint getCode();
  virtual void destroy();
  virtual ::java::lang::String * toString();
  virtual ::org::omg::CORBA::Policy * copy();
  virtual jint hashCode();
  virtual jboolean equals(::java::lang::Object *);
private:
  static const jlong serialVersionUID = 1LL;
  JArray< ::java::lang::String * > * __attribute__((aligned(__alignof__( ::org::omg::CORBA::portable::ObjectImpl)))) ids;
  jint type;
  ::java::lang::Object * value;
  jint policyCode;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA__PolicyImplBase__
