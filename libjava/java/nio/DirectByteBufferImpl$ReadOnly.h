
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_nio_DirectByteBufferImpl$ReadOnly__
#define __java_nio_DirectByteBufferImpl$ReadOnly__

#pragma interface

#include <java/nio/DirectByteBufferImpl.h>
extern "Java"
{
  namespace gnu
  {
    namespace gcj
    {
        class RawData;
    }
  }
  namespace java
  {
    namespace nio
    {
        class ByteBuffer;
        class DirectByteBufferImpl$ReadOnly;
    }
  }
}

class java::nio::DirectByteBufferImpl$ReadOnly : public ::java::nio::DirectByteBufferImpl
{

public: // actually package-private
  DirectByteBufferImpl$ReadOnly(::java::lang::Object *, ::gnu::gcj::RawData *, jint, jint, jint);
public:
  ::java::nio::ByteBuffer * put(jbyte);
  ::java::nio::ByteBuffer * put(jint, jbyte);
  jboolean isReadOnly();
  static ::java::lang::Class class$;
};

#endif // __java_nio_DirectByteBufferImpl$ReadOnly__
