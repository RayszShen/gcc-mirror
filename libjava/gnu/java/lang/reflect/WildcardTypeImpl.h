
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_lang_reflect_WildcardTypeImpl__
#define __gnu_java_lang_reflect_WildcardTypeImpl__

#pragma interface

#include <gnu/java/lang/reflect/TypeImpl.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace lang
      {
        namespace reflect
        {
            class WildcardTypeImpl;
        }
      }
    }
  }
}

class gnu::java::lang::reflect::WildcardTypeImpl : public ::gnu::java::lang::reflect::TypeImpl
{

public: // actually package-private
  WildcardTypeImpl(::java::lang::reflect::Type *, ::java::lang::reflect::Type *);
  ::java::lang::reflect::Type * resolve();
public:
  JArray< ::java::lang::reflect::Type * > * getUpperBounds();
  JArray< ::java::lang::reflect::Type * > * getLowerBounds();
  jboolean equals(::java::lang::Object *);
  jint hashCode();
  ::java::lang::String * toString();
private:
  ::java::lang::reflect::Type * __attribute__((aligned(__alignof__( ::gnu::java::lang::reflect::TypeImpl)))) lower;
  ::java::lang::reflect::Type * upper;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_lang_reflect_WildcardTypeImpl__
