
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_nio_charset_ByteCharset$Encoder__
#define __gnu_java_nio_charset_ByteCharset$Encoder__

#pragma interface

#include <java/nio/charset/CharsetEncoder.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace nio
      {
        namespace charset
        {
            class ByteCharset;
            class ByteCharset$Encoder;
        }
      }
    }
  }
  namespace java
  {
    namespace nio
    {
        class ByteBuffer;
        class CharBuffer;
      namespace charset
      {
          class CoderResult;
      }
    }
  }
}

class gnu::java::nio::charset::ByteCharset$Encoder : public ::java::nio::charset::CharsetEncoder
{

public: // actually package-private
  ByteCharset$Encoder(::gnu::java::nio::charset::ByteCharset *);
public: // actually protected
  ::java::nio::charset::CoderResult * encodeLoop(::java::nio::CharBuffer *, ::java::nio::ByteBuffer *);
private:
  JArray< jbyte > * __attribute__((aligned(__alignof__( ::java::nio::charset::CharsetEncoder)))) lookup;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_nio_charset_ByteCharset$Encoder__
