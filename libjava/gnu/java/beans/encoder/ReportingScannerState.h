
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_beans_encoder_ReportingScannerState__
#define __gnu_java_beans_encoder_ReportingScannerState__

#pragma interface

#include <gnu/java/beans/encoder/ScannerState.h>
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
            class ObjectId;
            class ReportingScannerState;
        }
      }
    }
  }
}

class gnu::java::beans::encoder::ReportingScannerState : public ::gnu::java::beans::encoder::ScannerState
{

public: // actually package-private
  ReportingScannerState();
  virtual void methodInvocation(::java::lang::String *);
  virtual void staticMethodInvocation(::java::lang::String *, ::java::lang::String *);
  virtual void staticFieldAccess(::java::lang::String *, ::java::lang::String *);
  virtual void classResolution(::java::lang::String *);
  virtual void objectInstantiation(::java::lang::String *, ::gnu::java::beans::encoder::ObjectId *);
  virtual void primitiveInstantiation(::java::lang::String *, ::java::lang::String *);
  virtual void objectArrayInstantiation(::java::lang::String *, ::java::lang::String *, ::gnu::java::beans::encoder::ObjectId *);
  virtual void primitiveArrayInstantiation(::java::lang::String *, ::java::lang::String *, ::gnu::java::beans::encoder::ObjectId *);
  virtual void arraySet(::java::lang::String *);
  virtual void arrayGet(::java::lang::String *);
  virtual void listGet();
  virtual void listSet();
  virtual void nullObject();
  virtual void stringReference(::java::lang::String *);
  virtual void objectReference(::gnu::java::beans::encoder::ObjectId *);
  virtual void end();
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_beans_encoder_ReportingScannerState__
