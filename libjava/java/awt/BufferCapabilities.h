
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_BufferCapabilities__
#define __java_awt_BufferCapabilities__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class BufferCapabilities;
        class BufferCapabilities$FlipContents;
        class ImageCapabilities;
    }
  }
}

class java::awt::BufferCapabilities : public ::java::lang::Object
{

public:
  BufferCapabilities(::java::awt::ImageCapabilities *, ::java::awt::ImageCapabilities *, ::java::awt::BufferCapabilities$FlipContents *);
  virtual ::java::awt::ImageCapabilities * getFrontBufferCapabilities();
  virtual ::java::awt::ImageCapabilities * getBackBufferCapabilities();
  virtual jboolean isPageFlipping();
  virtual ::java::awt::BufferCapabilities$FlipContents * getFlipContents();
  virtual jboolean isFullScreenRequired();
  virtual jboolean isMultiBufferAvailable();
  virtual ::java::lang::Object * clone();
private:
  ::java::awt::ImageCapabilities * __attribute__((aligned(__alignof__( ::java::lang::Object)))) front;
  ::java::awt::ImageCapabilities * back;
  ::java::awt::BufferCapabilities$FlipContents * flip;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_BufferCapabilities__
