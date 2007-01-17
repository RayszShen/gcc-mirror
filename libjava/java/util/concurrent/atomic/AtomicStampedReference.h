
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_atomic_AtomicStampedReference__
#define __java_util_concurrent_atomic_AtomicStampedReference__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>


class java::util::concurrent::atomic::AtomicStampedReference : public ::java::lang::Object
{

public:
  AtomicStampedReference(::java::lang::Object *, jint);
  virtual ::java::lang::Object * getReference();
  virtual jint getStamp();
  virtual ::java::lang::Object * get(JArray< jint > *);
  virtual jboolean weakCompareAndSet(::java::lang::Object *, ::java::lang::Object *, jint, jint);
  virtual jboolean compareAndSet(::java::lang::Object *, ::java::lang::Object *, jint, jint);
  virtual void set(::java::lang::Object *, jint);
  virtual jboolean attemptStamp(::java::lang::Object *, jint);
private:
  ::java::util::concurrent::atomic::AtomicReference * __attribute__((aligned(__alignof__( ::java::lang::Object)))) atomicRef;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_atomic_AtomicStampedReference__
