
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_VMStackWalker__
#define __gnu_classpath_VMStackWalker__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace classpath
    {
        class VMStackWalker;
    }
    namespace gcj
    {
        class RawData;
    }
  }
}

class gnu::classpath::VMStackWalker : public ::java::lang::Object
{

public:
  VMStackWalker();
  static JArray< ::java::lang::Class * > * getClassContext();
  static ::java::lang::Class * getCallingClass();
private:
  static ::java::lang::Class * getCallingClass(::gnu::gcj::RawData *);
public:
  static ::java::lang::ClassLoader * getCallingClassLoader();
private:
  static ::java::lang::ClassLoader * getCallingClassLoader(::gnu::gcj::RawData *);
public:
  static ::java::lang::ClassLoader * getClassLoader(::java::lang::Class *);
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_VMStackWalker__
