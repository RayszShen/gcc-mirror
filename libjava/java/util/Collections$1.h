
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_Collections$1__
#define __java_util_Collections$1__

#pragma interface

#include <java/lang/Object.h>

class java::util::Collections$1 : public ::java::lang::Object
{

public: // actually package-private
  Collections$1(::java::util::Collections$SingletonSet *);
public:
  jboolean hasNext();
  ::java::lang::Object * next();
  void remove();
private:
  jboolean __attribute__((aligned(__alignof__( ::java::lang::Object)))) hasNext__;
public: // actually package-private
  ::java::util::Collections$SingletonSet * this$1;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_Collections$1__
