
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_EnumMap__
#define __java_util_EnumMap__

#pragma interface

#include <java/util/AbstractMap.h>
#include <gcj/array.h>


class java::util::EnumMap : public ::java::util::AbstractMap
{

public:
  EnumMap(::java::lang::Class *);
  EnumMap(::java::util::EnumMap *);
  EnumMap(::java::util::Map *);
  virtual jint size();
  virtual jboolean containsValue(::java::lang::Object *);
  virtual jboolean containsKey(::java::lang::Object *);
  virtual ::java::lang::Object * get(::java::lang::Object *);
  virtual ::java::lang::Object * target$put(::java::lang::Enum *, ::java::lang::Object *);
  virtual ::java::lang::Object * remove(::java::lang::Object *);
  virtual void putAll(::java::util::Map *);
  virtual void clear();
  virtual ::java::util::Set * keySet();
  virtual ::java::util::Collection * values();
  virtual ::java::util::Set * entrySet();
  virtual jboolean equals(::java::lang::Object *);
  virtual ::java::util::EnumMap * target$clone();
  virtual ::java::lang::Object * clone();
  virtual ::java::lang::Object * put(::java::lang::Object *, ::java::lang::Object *);
private:
  static const jlong serialVersionUID = 458661240069192865LL;
public: // actually package-private
  JArray< ::java::lang::Object * > * __attribute__((aligned(__alignof__( ::java::util::AbstractMap)))) store;
  jint cardinality;
  ::java::lang::Class * enumClass;
  ::java::util::Set * entries;
  static ::java::lang::Object * emptySlot;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_EnumMap__
