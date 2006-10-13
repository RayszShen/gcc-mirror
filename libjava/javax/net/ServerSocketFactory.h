
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_net_ServerSocketFactory__
#define __javax_net_ServerSocketFactory__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace net
    {
        class InetAddress;
        class ServerSocket;
    }
  }
  namespace javax
  {
    namespace net
    {
        class ServerSocketFactory;
    }
  }
}

class javax::net::ServerSocketFactory : public ::java::lang::Object
{

public: // actually protected
  ServerSocketFactory();
public:
  static ::javax::net::ServerSocketFactory * getDefault();
  virtual ::java::net::ServerSocket * createServerSocket();
  virtual ::java::net::ServerSocket * createServerSocket(jint) = 0;
  virtual ::java::net::ServerSocket * createServerSocket(jint, jint) = 0;
  virtual ::java::net::ServerSocket * createServerSocket(jint, jint, ::java::net::InetAddress *) = 0;
  static ::java::lang::Class class$;
};

#endif // __javax_net_ServerSocketFactory__
