
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_beans_encoder_elements_MethodInvocation__
#define __gnu_java_beans_encoder_elements_MethodInvocation__

#pragma interface

#include <gnu/java/beans/encoder/elements/Element.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace beans
      {
        namespace encoder
        {
            class Writer;
          namespace elements
          {
              class MethodInvocation;
          }
        }
      }
    }
  }
}

class gnu::java::beans::encoder::elements::MethodInvocation : public ::gnu::java::beans::encoder::elements::Element
{

public:
  MethodInvocation(::java::lang::String *);
  virtual void writeStart(::gnu::java::beans::encoder::Writer *);
public: // actually package-private
  ::java::lang::String * __attribute__((aligned(__alignof__( ::gnu::java::beans::encoder::elements::Element)))) methodName;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_beans_encoder_elements_MethodInvocation__
