
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_TreeMap$4__
#define __java_util_TreeMap$4__

#pragma interface

#include <java/lang/Object.h>

class java::util::TreeMap$4 : public ::java::lang::Object
{

public: // actually package-private
  TreeMap$4(::java::util::TreeMap$3 *);
public:
  virtual jboolean hasNext();
  virtual ::java::lang::Object * next();
  virtual void remove();
private:
  ::java::util::Map$Entry * __attribute__((aligned(__alignof__( ::java::lang::Object)))) last;
  ::java::util::Map$Entry * next__;
public: // actually package-private
  ::java::util::TreeMap$3 * this$2;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_TreeMap$4__
