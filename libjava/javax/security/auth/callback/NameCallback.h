
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_security_auth_callback_NameCallback__
#define __javax_security_auth_callback_NameCallback__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace security
    {
      namespace auth
      {
        namespace callback
        {
            class NameCallback;
        }
      }
    }
  }
}

class javax::security::auth::callback::NameCallback : public ::java::lang::Object
{

public:
  NameCallback(::java::lang::String *);
  NameCallback(::java::lang::String *, ::java::lang::String *);
  virtual ::java::lang::String * getPrompt();
  virtual ::java::lang::String * getDefaultName();
  virtual void setName(::java::lang::String *);
  virtual ::java::lang::String * getName();
private:
  void setPrompt(::java::lang::String *);
  void setDefaultName(::java::lang::String *);
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::lang::Object)))) prompt;
  ::java::lang::String * defaultName;
  ::java::lang::String * inputName;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_security_auth_callback_NameCallback__
