
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CosNaming_NamingContextPackage_CannotProceedHelper__
#define __org_omg_CosNaming_NamingContextPackage_CannotProceedHelper__

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
        namespace portable
        {
            class InputStream;
            class OutputStream;
        }
      }
      namespace CosNaming
      {
        namespace NamingContextPackage
        {
            class CannotProceed;
            class CannotProceedHelper;
        }
      }
    }
  }
}

class org::omg::CosNaming::NamingContextPackage::CannotProceedHelper : public ::java::lang::Object
{

public:
  CannotProceedHelper();
  static ::org::omg::CosNaming::NamingContextPackage::CannotProceed * extract(::org::omg::CORBA::Any *);
  static ::java::lang::String * id();
  static void insert(::org::omg::CORBA::Any *, ::org::omg::CosNaming::NamingContextPackage::CannotProceed *);
  static ::org::omg::CosNaming::NamingContextPackage::CannotProceed * read(::org::omg::CORBA::portable::InputStream *);
  static ::org::omg::CORBA::TypeCode * type();
  static void write(::org::omg::CORBA::portable::OutputStream *, ::org::omg::CosNaming::NamingContextPackage::CannotProceed *);
private:
  static ::java::lang::String * _id;
public:
  static ::java::lang::Class class$;
};

#endif // __org_omg_CosNaming_NamingContextPackage_CannotProceedHelper__
