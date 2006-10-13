
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_TypeCode__
#define __org_omg_CORBA_TypeCode__

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
          class TCKind;
          class TypeCode;
      }
    }
  }
}

class org::omg::CORBA::TypeCode : public ::java::lang::Object
{

public:
  TypeCode();
  virtual ::org::omg::CORBA::TypeCode * concrete_base_type() = 0;
  virtual ::org::omg::CORBA::TypeCode * content_type() = 0;
  virtual jint default_index() = 0;
  virtual ::org::omg::CORBA::TypeCode * discriminator_type() = 0;
  virtual jboolean equal(::org::omg::CORBA::TypeCode *) = 0;
  virtual jboolean equivalent(::org::omg::CORBA::TypeCode *) = 0;
  virtual jshort fixed_digits() = 0;
  virtual jshort fixed_scale() = 0;
  virtual ::org::omg::CORBA::TypeCode * get_compact_typecode() = 0;
  virtual ::java::lang::String * id() = 0;
  virtual ::org::omg::CORBA::TCKind * kind() = 0;
  virtual jint length() = 0;
  virtual jint member_count() = 0;
  virtual ::org::omg::CORBA::Any * member_label(jint) = 0;
  virtual ::java::lang::String * member_name(jint) = 0;
  virtual ::org::omg::CORBA::TypeCode * member_type(jint) = 0;
  virtual jshort member_visibility(jint) = 0;
  virtual ::java::lang::String * name() = 0;
  virtual jshort type_modifier() = 0;
private:
  static const jlong serialVersionUID = -6521025782489515676LL;
public:
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_TypeCode__
