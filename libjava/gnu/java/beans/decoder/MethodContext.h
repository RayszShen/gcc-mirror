
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_beans_decoder_MethodContext__
#define __gnu_java_beans_decoder_MethodContext__

#pragma interface

#include <gnu/java/beans/decoder/AbstractCreatableObjectContext.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace beans
      {
        namespace decoder
        {
            class Context;
            class MethodContext;
        }
      }
    }
  }
}

class gnu::java::beans::decoder::MethodContext : public ::gnu::java::beans::decoder::AbstractCreatableObjectContext
{

public: // actually package-private
  MethodContext(::java::lang::String *, ::java::lang::String *);
public:
  virtual void addParameterObjectImpl(::java::lang::Object *);
public: // actually protected
  virtual ::java::lang::Object * createObject(::gnu::java::beans::decoder::Context *);
private:
  ::java::util::ArrayList * __attribute__((aligned(__alignof__( ::gnu::java::beans::decoder::AbstractCreatableObjectContext)))) arguments;
  ::java::lang::String * methodName;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_beans_decoder_MethodContext__
