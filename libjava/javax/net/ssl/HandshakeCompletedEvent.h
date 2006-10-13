
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_net_ssl_HandshakeCompletedEvent__
#define __javax_net_ssl_HandshakeCompletedEvent__

#pragma interface

#include <java/util/EventObject.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace security
    {
      namespace cert
      {
          class Certificate;
      }
    }
  }
  namespace javax
  {
    namespace net
    {
      namespace ssl
      {
          class HandshakeCompletedEvent;
          class SSLSession;
          class SSLSocket;
      }
    }
    namespace security
    {
      namespace cert
      {
          class X509Certificate;
      }
    }
  }
}

class javax::net::ssl::HandshakeCompletedEvent : public ::java::util::EventObject
{

public:
  HandshakeCompletedEvent(::javax::net::ssl::SSLSocket *, ::javax::net::ssl::SSLSession *);
  virtual ::java::lang::String * getCipherSuite();
  virtual JArray< ::java::security::cert::Certificate * > * getLocalCertificates();
  virtual JArray< ::java::security::cert::Certificate * > * getPeerCertificates();
  virtual JArray< ::javax::security::cert::X509Certificate * > * getPeerCertificateChain();
  virtual ::javax::net::ssl::SSLSession * getSession();
  virtual ::javax::net::ssl::SSLSocket * getSocket();
private:
  static const jlong serialVersionUID = 7914963744257769778LL;
  ::javax::net::ssl::SSLSession * __attribute__((aligned(__alignof__( ::java::util::EventObject)))) session;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_net_ssl_HandshakeCompletedEvent__
