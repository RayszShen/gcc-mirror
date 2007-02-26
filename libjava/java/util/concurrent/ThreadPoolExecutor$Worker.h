
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_ThreadPoolExecutor$Worker__
#define __java_util_concurrent_ThreadPoolExecutor$Worker__

#pragma interface

#include <java/lang/Object.h>

class java::util::concurrent::ThreadPoolExecutor$Worker : public ::java::lang::Object
{

public: // actually package-private
  ThreadPoolExecutor$Worker(::java::util::concurrent::ThreadPoolExecutor *, ::java::lang::Runnable *);
  virtual jboolean isActive();
  virtual void interruptIfIdle();
  virtual void interruptNow();
private:
  void runTask(::java::lang::Runnable *);
public:
  virtual void run();
private:
  ::java::util::concurrent::locks::ReentrantLock * __attribute__((aligned(__alignof__( ::java::lang::Object)))) runLock;
  ::java::lang::Runnable * firstTask;
public: // actually package-private
  volatile jlong completedTasks;
  ::java::lang::Thread * thread;
  ::java::util::concurrent::ThreadPoolExecutor * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_ThreadPoolExecutor$Worker__
