
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_AbstractList$3__
#define __java_util_AbstractList$3__

#pragma interface

#include <java/lang/Object.h>

class java::util::AbstractList$3 : public ::java::lang::Object
{

public: // actually package-private
  AbstractList$3(::java::util::AbstractList *, jint);
private:
  void checkMod();
public:
  virtual jboolean hasNext();
  virtual jboolean hasPrevious();
  virtual ::java::lang::Object * next();
  virtual ::java::lang::Object * previous();
  virtual jint nextIndex();
  virtual jint previousIndex();
  virtual void remove();
  virtual void set(::java::lang::Object *);
  virtual void add(::java::lang::Object *);
private:
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) knownMod;
  jint position;
  jint lastReturned;
  jint size;
public: // actually package-private
  ::java::util::AbstractList * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_AbstractList$3__
