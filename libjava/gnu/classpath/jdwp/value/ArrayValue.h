
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_value_ArrayValue__
#define __gnu_classpath_jdwp_value_ArrayValue__

#pragma interface

#include <gnu/classpath/jdwp/value/Value.h>
extern "Java"
{
  namespace gnu
  {
    namespace classpath
    {
      namespace jdwp
      {
        namespace value
        {
            class ArrayValue;
        }
      }
    }
  }
}

class gnu::classpath::jdwp::value::ArrayValue : public ::gnu::classpath::jdwp::value::Value
{

public:
  ArrayValue(::java::lang::Object *);
public: // actually protected
  virtual ::java::lang::Object * getObject();
  virtual void write(::java::io::DataOutputStream *);
public: // actually package-private
  ::java::lang::Object * __attribute__((aligned(__alignof__( ::gnu::classpath::jdwp::value::Value)))) _value;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_value_ArrayValue__
