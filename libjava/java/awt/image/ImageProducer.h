
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_ImageProducer__
#define __java_awt_image_ImageProducer__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace image
      {
          class ImageConsumer;
          class ImageProducer;
      }
    }
  }
}

class java::awt::image::ImageProducer : public ::java::lang::Object
{

public:
  virtual void addConsumer(::java::awt::image::ImageConsumer *) = 0;
  virtual jboolean isConsumer(::java::awt::image::ImageConsumer *) = 0;
  virtual void removeConsumer(::java::awt::image::ImageConsumer *) = 0;
  virtual void startProduction(::java::awt::image::ImageConsumer *) = 0;
  virtual void requestTopDownLeftRightResend(::java::awt::image::ImageConsumer *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_awt_image_ImageProducer__
