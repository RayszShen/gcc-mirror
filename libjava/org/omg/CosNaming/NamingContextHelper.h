
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CosNaming_NamingContextHelper__
#define __org_omg_CosNaming_NamingContextHelper__

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
          class Object;
          class TypeCode;
        namespace portable
        {
            class InputStream;
            class OutputStream;
        }
      }
      namespace CosNaming
      {
          class NamingContext;
          class NamingContextHelper;
      }
    }
  }
}

class org::omg::CosNaming::NamingContextHelper : public ::java::lang::Object
{

public:
  NamingContextHelper();
  static ::org::omg::CosNaming::NamingContext * extract(::org::omg::CORBA::Any *);
  static ::java::lang::String * id();
  static void insert(::org::omg::CORBA::Any *, ::org::omg::CosNaming::NamingContext *);
  static ::org::omg::CosNaming::NamingContext * narrow(::org::omg::CORBA::Object *);
  static ::org::omg::CosNaming::NamingContext * unchecked_narrow(::org::omg::CORBA::Object *);
  static ::org::omg::CosNaming::NamingContext * read(::org::omg::CORBA::portable::InputStream *);
  static ::org::omg::CORBA::TypeCode * type();
  static void write(::org::omg::CORBA::portable::OutputStream *, ::org::omg::CosNaming::NamingContext *);
private:
  static ::java::lang::String * _id;
public:
  static ::java::lang::Class class$;
};

#endif // __org_omg_CosNaming_NamingContextHelper__
