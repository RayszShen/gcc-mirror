
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_locks_LockSupport__
#define __java_util_concurrent_locks_LockSupport__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace sun
  {
    namespace misc
    {
        class Unsafe;
    }
  }
}

class java::util::concurrent::locks::LockSupport : public ::java::lang::Object
{

  LockSupport();
  static void setBlocker(::java::lang::Thread *, ::java::lang::Object *);
public:
  static void unpark(::java::lang::Thread *);
  static void park(::java::lang::Object *);
  static void parkNanos(::java::lang::Object *, jlong);
  static void parkUntil(::java::lang::Object *, jlong);
  static ::java::lang::Object * getBlocker(::java::lang::Thread *);
  static void park();
  static void parkNanos(jlong);
  static void parkUntil(jlong);
private:
  static ::sun::misc::Unsafe * unsafe;
  static jlong parkBlockerOffset;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_locks_LockSupport__
