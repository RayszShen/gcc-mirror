
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_CopyOnWriteArrayList$1__
#define __java_util_concurrent_CopyOnWriteArrayList$1__

#pragma interface

#include <java/lang/Object.h>

class java::util::concurrent::CopyOnWriteArrayList$1 : public ::java::lang::Object
{

public: // actually package-private
  CopyOnWriteArrayList$1(::java::util::concurrent::CopyOnWriteArrayList$SubList *, jint);
public:
  jboolean hasNext();
  jboolean hasPrevious();
  ::java::lang::Object * next();
  ::java::lang::Object * previous();
  jint nextIndex();
  jint previousIndex();
  void remove();
  void set(::java::lang::Object *);
  void add(::java::lang::Object *);
private:
  ::java::util::ListIterator * __attribute__((aligned(__alignof__( ::java::lang::Object)))) i;
  jint position;
public: // actually package-private
  ::java::util::concurrent::CopyOnWriteArrayList$SubList * this$1;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_CopyOnWriteArrayList$1__
