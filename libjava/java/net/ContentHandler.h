
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_net_ContentHandler__
#define __java_net_ContentHandler__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace net
    {
        class ContentHandler;
        class URLConnection;
    }
  }
}

class java::net::ContentHandler : public ::java::lang::Object
{

public:
  ContentHandler();
  virtual ::java::lang::Object * getContent(::java::net::URLConnection *) = 0;
  virtual ::java::lang::Object * getContent(::java::net::URLConnection *, JArray< ::java::lang::Class * > *);
  static ::java::lang::Class class$;
};

#endif // __java_net_ContentHandler__
