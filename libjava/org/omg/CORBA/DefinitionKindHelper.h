
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_DefinitionKindHelper__
#define __org_omg_CORBA_DefinitionKindHelper__

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
          class DefinitionKind;
          class DefinitionKindHelper;
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

class org::omg::CORBA::DefinitionKindHelper : public ::java::lang::Object
{

public:
  DefinitionKindHelper();
  static void insert(::org::omg::CORBA::Any *, ::org::omg::CORBA::DefinitionKind *);
  static ::org::omg::CORBA::DefinitionKind * extract(::org::omg::CORBA::Any *);
  static ::org::omg::CORBA::TypeCode * type();
  static ::java::lang::String * id();
  static ::org::omg::CORBA::DefinitionKind * read(::org::omg::CORBA::portable::InputStream *);
  static void write(::org::omg::CORBA::portable::OutputStream *, ::org::omg::CORBA::DefinitionKind *);
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_DefinitionKindHelper__
