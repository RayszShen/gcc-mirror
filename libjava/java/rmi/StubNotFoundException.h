
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_rmi_StubNotFoundException__
#define __java_rmi_StubNotFoundException__

#pragma interface

#include <java/rmi/RemoteException.h>
extern "Java"
{
  namespace java
  {
    namespace rmi
    {
        class StubNotFoundException;
    }
  }
}

class java::rmi::StubNotFoundException : public ::java::rmi::RemoteException
{

public:
  StubNotFoundException(::java::lang::String *);
  StubNotFoundException(::java::lang::String *, ::java::lang::Exception *);
private:
  static const jlong serialVersionUID = -7088199405468872373LL;
public:
  static ::java::lang::Class class$;
};

#endif // __java_rmi_StubNotFoundException__
