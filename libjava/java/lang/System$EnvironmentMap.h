
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_lang_System$EnvironmentMap__
#define __java_lang_System$EnvironmentMap__

#pragma interface

#include <java/util/HashMap.h>

class java::lang::System$EnvironmentMap : public ::java::util::HashMap
{

public: // actually package-private
  System$EnvironmentMap();
public:
  virtual jboolean containsKey(::java::lang::Object *);
  virtual jboolean containsValue(::java::lang::Object *);
  virtual ::java::util::Set * entrySet();
  virtual ::java::lang::String * target$get(::java::lang::Object *);
  virtual ::java::util::Set * keySet();
  virtual ::java::lang::String * target$remove(::java::lang::Object *);
  virtual ::java::util::Collection * values();
  virtual ::java::lang::Object * get(::java::lang::Object *);
  virtual ::java::lang::Object * remove(::java::lang::Object *);
private:
  ::java::util::Set * __attribute__((aligned(__alignof__( ::java::util::HashMap)))) entries;
  ::java::util::Set * keys;
  ::java::util::Collection * values__;
public:
  static ::java::lang::Class class$;
};

#endif // __java_lang_System$EnvironmentMap__
