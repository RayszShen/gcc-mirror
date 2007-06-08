
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CosNaming_NamingContext__
#define __org_omg_CosNaming_NamingContext__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class Context;
          class ContextList;
          class DomainManager;
          class ExceptionList;
          class NVList;
          class NamedValue;
          class Object;
          class Policy;
          class Request;
          class SetOverrideType;
      }
      namespace CosNaming
      {
          class BindingIteratorHolder;
          class BindingListHolder;
          class NameComponent;
          class NamingContext;
      }
    }
  }
}

class org::omg::CosNaming::NamingContext : public ::java::lang::Object
{

public:
  virtual void bind(JArray< ::org::omg::CosNaming::NameComponent * > *, ::org::omg::CORBA::Object *) = 0;
  virtual void bind_context(JArray< ::org::omg::CosNaming::NameComponent * > *, ::org::omg::CosNaming::NamingContext *) = 0;
  virtual ::org::omg::CosNaming::NamingContext * bind_new_context(JArray< ::org::omg::CosNaming::NameComponent * > *) = 0;
  virtual void destroy() = 0;
  virtual void list(jint, ::org::omg::CosNaming::BindingListHolder *, ::org::omg::CosNaming::BindingIteratorHolder *) = 0;
  virtual ::org::omg::CosNaming::NamingContext * new_context() = 0;
  virtual void rebind(JArray< ::org::omg::CosNaming::NameComponent * > *, ::org::omg::CORBA::Object *) = 0;
  virtual void rebind_context(JArray< ::org::omg::CosNaming::NameComponent * > *, ::org::omg::CosNaming::NamingContext *) = 0;
  virtual ::org::omg::CORBA::Object * resolve(JArray< ::org::omg::CosNaming::NameComponent * > *) = 0;
  virtual void unbind(JArray< ::org::omg::CosNaming::NameComponent * > *) = 0;
  virtual ::org::omg::CORBA::Request * _create_request(::org::omg::CORBA::Context *, ::java::lang::String *, ::org::omg::CORBA::NVList *, ::org::omg::CORBA::NamedValue *) = 0;
  virtual ::org::omg::CORBA::Request * _create_request(::org::omg::CORBA::Context *, ::java::lang::String *, ::org::omg::CORBA::NVList *, ::org::omg::CORBA::NamedValue *, ::org::omg::CORBA::ExceptionList *, ::org::omg::CORBA::ContextList *) = 0;
  virtual ::org::omg::CORBA::Object * _duplicate() = 0;
  virtual JArray< ::org::omg::CORBA::DomainManager * > * _get_domain_managers() = 0;
  virtual ::org::omg::CORBA::Object * _get_interface_def() = 0;
  virtual ::org::omg::CORBA::Policy * _get_policy(jint) = 0;
  virtual jint _hash(jint) = 0;
  virtual jboolean _is_a(::java::lang::String *) = 0;
  virtual jboolean _is_equivalent(::org::omg::CORBA::Object *) = 0;
  virtual jboolean _non_existent() = 0;
  virtual void _release() = 0;
  virtual ::org::omg::CORBA::Request * _request(::java::lang::String *) = 0;
  virtual ::org::omg::CORBA::Object * _set_policy_override(JArray< ::org::omg::CORBA::Policy * > *, ::org::omg::CORBA::SetOverrideType *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __org_omg_CosNaming_NamingContext__
