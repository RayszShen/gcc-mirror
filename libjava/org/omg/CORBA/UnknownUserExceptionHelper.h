
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_UnknownUserExceptionHelper__
#define __org_omg_CORBA_UnknownUserExceptionHelper__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class Any;
          class TypeCode;
          class UnknownUserException;
          class UnknownUserExceptionHelper;
        namespace portable
        {
            class InputStream;
            class OutputStream;
        }
      }
    }
  }
}

class org::omg::CORBA::UnknownUserExceptionHelper : public ::java::lang::Object
{

public:
  UnknownUserExceptionHelper();
  static ::org::omg::CORBA::TypeCode * type();
  static void insert(::org::omg::CORBA::Any *, ::org::omg::CORBA::UnknownUserException *);
  static ::org::omg::CORBA::UnknownUserException * extract(::org::omg::CORBA::Any *);
  static ::java::lang::String * id();
  static ::org::omg::CORBA::UnknownUserException * read(::org::omg::CORBA::portable::InputStream *);
  static void write(::org::omg::CORBA::portable::OutputStream *, ::org::omg::CORBA::UnknownUserException *);
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_UnknownUserExceptionHelper__
