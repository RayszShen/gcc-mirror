
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_Semaphore$NonfairSync__
#define __java_util_concurrent_Semaphore$NonfairSync__

#pragma interface

#include <java/util/concurrent/Semaphore$Sync.h>

class java::util::concurrent::Semaphore$NonfairSync : public ::java::util::concurrent::Semaphore$Sync
{

public: // actually package-private
  Semaphore$NonfairSync(jint);
public: // actually protected
  jint tryAcquireShared(jint);
private:
  static const jlong serialVersionUID = -2694183684443567898LL;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_Semaphore$NonfairSync__
