
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_net_ssl_provider_SSLSocketFactoryImpl__
#define __gnu_javax_net_ssl_provider_SSLSocketFactoryImpl__

#pragma interface

#include <javax/net/ssl/SSLSocketFactory.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace net
      {
        namespace ssl
        {
          namespace provider
          {
              class SSLContextImpl;
              class SSLSocketFactoryImpl;
              class SSLSocketImpl;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace net
    {
        class InetAddress;
        class Socket;
    }
  }
}

class gnu::javax::net::ssl::provider::SSLSocketFactoryImpl : public ::javax::net::ssl::SSLSocketFactory
{

public:
  SSLSocketFactoryImpl(::gnu::javax::net::ssl::provider::SSLContextImpl *);
  virtual ::java::net::Socket * createSocket(::java::net::Socket *, ::java::lang::String *, jint, jboolean);
  virtual JArray< ::java::lang::String * > * getDefaultCipherSuites();
  virtual JArray< ::java::lang::String * > * getSupportedCipherSuites();
  virtual ::gnu::javax::net::ssl::provider::SSLSocketImpl * SSLSocketFactoryImpl$createSocket(::java::lang::String *, jint);
  virtual ::gnu::javax::net::ssl::provider::SSLSocketImpl * SSLSocketFactoryImpl$createSocket(::java::lang::String *, jint, ::java::net::InetAddress *, jint);
  virtual ::gnu::javax::net::ssl::provider::SSLSocketImpl * SSLSocketFactoryImpl$createSocket(::java::net::InetAddress *, jint);
  virtual ::gnu::javax::net::ssl::provider::SSLSocketImpl * SSLSocketFactoryImpl$createSocket(::java::net::InetAddress *, jint, ::java::net::InetAddress *, jint);
  virtual ::java::net::Socket * createSocket();
  virtual ::java::net::Socket * createSocket(::java::net::InetAddress *, jint, ::java::net::InetAddress *, jint);
  virtual ::java::net::Socket * createSocket(::java::net::InetAddress *, jint);
  virtual ::java::net::Socket * createSocket(::java::lang::String *, jint, ::java::net::InetAddress *, jint);
  virtual ::java::net::Socket * createSocket(::java::lang::String *, jint);
private:
  ::gnu::javax::net::ssl::provider::SSLContextImpl * __attribute__((aligned(__alignof__( ::javax::net::ssl::SSLSocketFactory)))) contextImpl;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_net_ssl_provider_SSLSocketFactoryImpl__
