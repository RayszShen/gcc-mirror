
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_beans_MethodDescriptor__
#define __java_beans_MethodDescriptor__

#pragma interface

#include <java/beans/FeatureDescriptor.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace beans
    {
        class MethodDescriptor;
        class ParameterDescriptor;
    }
  }
}

class java::beans::MethodDescriptor : public ::java::beans::FeatureDescriptor
{

public:
  MethodDescriptor(::java::lang::reflect::Method *);
  MethodDescriptor(::java::lang::reflect::Method *, JArray< ::java::beans::ParameterDescriptor * > *);
  virtual JArray< ::java::beans::ParameterDescriptor * > * getParameterDescriptors();
  virtual ::java::lang::reflect::Method * getMethod();
private:
  ::java::lang::reflect::Method * __attribute__((aligned(__alignof__( ::java::beans::FeatureDescriptor)))) m;
  JArray< ::java::beans::ParameterDescriptor * > * parameterDescriptors;
public:
  static ::java::lang::Class class$;
};

#endif // __java_beans_MethodDescriptor__
