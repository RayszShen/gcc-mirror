
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_nio_charset_ISO_8859_1$Encoder__
#define __gnu_java_nio_charset_ISO_8859_1$Encoder__

#pragma interface

#include <java/nio/charset/CharsetEncoder.h>
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
            class ByteEncodeLoopHelper;
            class ISO_8859_1$Encoder;
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
          class Charset;
          class CoderResult;
      }
    }
  }
}

class gnu::java::nio::charset::ISO_8859_1$Encoder : public ::java::nio::charset::CharsetEncoder
{

public: // actually package-private
  ISO_8859_1$Encoder(::java::nio::charset::Charset *);
public:
  jboolean canEncode(jchar);
  jboolean canEncode(::java::lang::CharSequence *);
public: // actually protected
  ::java::nio::charset::CoderResult * encodeLoop(::java::nio::CharBuffer *, ::java::nio::ByteBuffer *);
private:
  static ::gnu::java::nio::charset::ByteEncodeLoopHelper * helper;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_nio_charset_ISO_8859_1$Encoder__
