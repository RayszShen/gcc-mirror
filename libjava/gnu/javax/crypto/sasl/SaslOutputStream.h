
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_sasl_SaslOutputStream__
#define __gnu_javax_crypto_sasl_SaslOutputStream__

#pragma interface

#include <java/io/OutputStream.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace crypto
      {
        namespace sasl
        {
            class SaslOutputStream;
        }
      }
    }
  }
  namespace javax
  {
    namespace security
    {
      namespace sasl
      {
          class SaslClient;
          class SaslServer;
      }
    }
  }
}

class gnu::javax::crypto::sasl::SaslOutputStream : public ::java::io::OutputStream
{

public:
  SaslOutputStream(::javax::security::sasl::SaslClient *, ::java::io::OutputStream *);
  SaslOutputStream(::javax::security::sasl::SaslServer *, ::java::io::OutputStream *);
  virtual void close();
  virtual void flush();
  virtual void write(jint);
  virtual void write(JArray< jbyte > *, jint, jint);
private:
  static ::java::util::logging::Logger * log;
  ::javax::security::sasl::SaslClient * __attribute__((aligned(__alignof__( ::java::io::OutputStream)))) client;
  ::javax::security::sasl::SaslServer * server;
  jint maxRawSendSize;
  ::java::io::OutputStream * dest;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_sasl_SaslOutputStream__
