
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_gcj_runtime_SystemClassLoader__
#define __gnu_gcj_runtime_SystemClassLoader__

#pragma interface

#include <java/net/URLClassLoader.h>
extern "Java"
{
  namespace gnu
  {
    namespace gcj
    {
      namespace runtime
      {
          class SystemClassLoader;
      }
    }
  }
}

class gnu::gcj::runtime::SystemClassLoader : public ::java::net::URLClassLoader
{

public: // actually package-private
  SystemClassLoader(::java::lang::ClassLoader *);
  void addClass(::java::lang::Class *);
public: // actually protected
  ::java::lang::Class * findClass(::java::lang::String *);
public: // actually package-private
  void init();
private:
  ::java::util::HashMap * __attribute__((aligned(__alignof__( ::java::net::URLClassLoader)))) nativeClasses;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_gcj_runtime_SystemClassLoader__
