
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_DelayQueue$Itr__
#define __java_util_concurrent_DelayQueue$Itr__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>


class java::util::concurrent::DelayQueue$Itr : public ::java::lang::Object
{

public: // actually package-private
  DelayQueue$Itr(::java::util::concurrent::DelayQueue *, JArray< ::java::lang::Object * > *);
public:
  virtual jboolean hasNext();
  virtual ::java::util::concurrent::Delayed * DelayQueue$Itr$next();
  virtual void remove();
  virtual ::java::lang::Object * next();
public: // actually package-private
  JArray< ::java::lang::Object * > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) array;
  jint cursor;
  jint lastRet;
  ::java::util::concurrent::DelayQueue * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_DelayQueue$Itr__
