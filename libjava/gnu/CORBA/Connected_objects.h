
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_Connected_objects__
#define __gnu_CORBA_Connected_objects__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
        class Connected_objects;
        class Connected_objects$cObject;
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
    }
  }
}

class gnu::CORBA::Connected_objects : public ::java::lang::Object
{

public:
  Connected_objects();
  virtual ::gnu::CORBA::Connected_objects$cObject * getKey(::org::omg::CORBA::Object *);
  virtual ::gnu::CORBA::Connected_objects$cObject * add(::org::omg::CORBA::Object *, jint);
  virtual ::gnu::CORBA::Connected_objects$cObject * add(JArray< jbyte > *, ::org::omg::CORBA::Object *, jint, ::java::lang::Object *);
  virtual ::gnu::CORBA::Connected_objects$cObject * get(JArray< jbyte > *);
  virtual ::java::util::Set * entrySet();
  virtual void remove(::org::omg::CORBA::Object *);
  virtual void remove(JArray< jbyte > *);
public: // actually protected
  virtual JArray< jbyte > * generateObjectKey(::org::omg::CORBA::Object *);
private:
  static jlong getFreeInstanceNumber();
public:
  virtual jint size();
private:
  static jlong free_object_number;
  ::java::util::Map * __attribute__((aligned(__alignof__( ::java::lang::Object)))) objects;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_Connected_objects__
