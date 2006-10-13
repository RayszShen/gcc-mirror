
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_NamingService_NamingMap__
#define __gnu_CORBA_NamingService_NamingMap__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
      namespace NamingService
      {
          class NamingMap;
      }
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class Object;
      }
      namespace CosNaming
      {
          class NameComponent;
      }
    }
  }
}

class gnu::CORBA::NamingService::NamingMap : public ::java::lang::Object
{

public:
  NamingMap();
  virtual void bind(::org::omg::CosNaming::NameComponent *, ::org::omg::CORBA::Object *);
  virtual jboolean containsKey(::org::omg::CosNaming::NameComponent *);
  virtual jboolean containsValue(::org::omg::CORBA::Object *);
  virtual ::java::util::Set * entries();
  virtual ::org::omg::CORBA::Object * get(::org::omg::CosNaming::NameComponent *);
  virtual void rebind(::org::omg::CosNaming::NameComponent *, ::org::omg::CORBA::Object *);
  virtual void remove(::org::omg::CosNaming::NameComponent *);
  virtual jint size();
public: // actually protected
  ::java::util::TreeMap * __attribute__((aligned(__alignof__( ::java::lang::Object)))) map;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_NamingService_NamingMap__
