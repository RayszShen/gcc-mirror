
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_Collections$UnmodifiableListIterator__
#define __java_util_Collections$UnmodifiableListIterator__

#pragma interface

#include <java/util/Collections$UnmodifiableIterator.h>

class java::util::Collections$UnmodifiableListIterator : public ::java::util::Collections$UnmodifiableIterator
{

public: // actually package-private
  Collections$UnmodifiableListIterator(::java::util::ListIterator *);
public:
  void add(::java::lang::Object *);
  jboolean hasPrevious();
  jint nextIndex();
  ::java::lang::Object * previous();
  jint previousIndex();
  void set(::java::lang::Object *);
private:
  ::java::util::ListIterator * __attribute__((aligned(__alignof__( ::java::util::Collections$UnmodifiableIterator)))) li;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_Collections$UnmodifiableListIterator__
