
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_PortableInterceptor_ObjectReferenceTemplateHelper__
#define __org_omg_PortableInterceptor_ObjectReferenceTemplateHelper__

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
      namespace PortableInterceptor
      {
          class ObjectReferenceTemplate;
          class ObjectReferenceTemplateHelper;
      }
    }
  }
}

class org::omg::PortableInterceptor::ObjectReferenceTemplateHelper : public ::java::lang::Object
{

public:
  ObjectReferenceTemplateHelper();
  static ::org::omg::CORBA::TypeCode * type();
  static void insert(::org::omg::CORBA::Any *, ::org::omg::PortableInterceptor::ObjectReferenceTemplate *);
  static ::org::omg::PortableInterceptor::ObjectReferenceTemplate * extract(::org::omg::CORBA::Any *);
  static ::java::lang::String * id();
  static ::org::omg::PortableInterceptor::ObjectReferenceTemplate * read(::org::omg::CORBA::portable::InputStream *);
  static void write(::org::omg::CORBA::portable::OutputStream *, ::org::omg::PortableInterceptor::ObjectReferenceTemplate *);
  static ::java::lang::Class class$;
};

#endif // __org_omg_PortableInterceptor_ObjectReferenceTemplateHelper__
