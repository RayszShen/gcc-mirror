
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_imageio_ImageIO$1__
#define __javax_imageio_ImageIO$1__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace imageio
    {
        class ImageIO$1;
        class ImageTranscoder;
    }
  }
}

class javax::imageio::ImageIO$1 : public ::java::lang::Object
{

public: // actually package-private
  ImageIO$1(::java::util::Iterator *);
public:
  virtual jboolean hasNext();
  virtual ::javax::imageio::ImageTranscoder * ImageIO$1$next();
  virtual void remove();
  virtual ::java::lang::Object * next();
private:
  ::java::util::Iterator * __attribute__((aligned(__alignof__( ::java::lang::Object)))) val$spiIterator;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_imageio_ImageIO$1__
