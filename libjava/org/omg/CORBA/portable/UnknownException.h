
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_portable_UnknownException__
#define __org_omg_CORBA_portable_UnknownException__

#pragma interface

#include <org/omg/CORBA/SystemException.h>
extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
        namespace portable
        {
            class UnknownException;
        }
      }
    }
  }
}

class org::omg::CORBA::portable::UnknownException : public ::org::omg::CORBA::SystemException
{

public:
  UnknownException(::java::lang::Throwable *);
private:
  static const jlong serialVersionUID = 1725238280802233450LL;
public:
  ::java::lang::Throwable * __attribute__((aligned(__alignof__( ::org::omg::CORBA::SystemException)))) originalEx;
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_portable_UnknownException__
