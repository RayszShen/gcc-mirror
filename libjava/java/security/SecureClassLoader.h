
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_SecureClassLoader__
#define __java_security_SecureClassLoader__

#pragma interface

#include <java/lang/ClassLoader.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace security
    {
        class CodeSource;
        class PermissionCollection;
        class SecureClassLoader;
    }
  }
}

class java::security::SecureClassLoader : public ::java::lang::ClassLoader
{

public: // actually protected
  SecureClassLoader(::java::lang::ClassLoader *);
  SecureClassLoader();
  virtual ::java::lang::Class * defineClass(::java::lang::String *, JArray< jbyte > *, jint, jint, ::java::security::CodeSource *);
  virtual ::java::security::PermissionCollection * getPermissions(::java::security::CodeSource *);
public: // actually package-private
  ::java::util::WeakHashMap * __attribute__((aligned(__alignof__( ::java::lang::ClassLoader)))) protectionDomainCache;
public:
  static ::java::lang::Class class$;
};

#endif // __java_security_SecureClassLoader__
