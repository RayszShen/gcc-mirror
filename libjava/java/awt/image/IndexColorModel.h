
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_IndexColorModel__
#define __java_awt_image_IndexColorModel__

#pragma interface

#include <java/awt/image/ColorModel.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace image
      {
          class BufferedImage;
          class IndexColorModel;
          class Raster;
      }
    }
    namespace math
    {
        class BigInteger;
    }
  }
}

class java::awt::image::IndexColorModel : public ::java::awt::image::ColorModel
{

public:
  IndexColorModel(jint, jint, JArray< jbyte > *, JArray< jbyte > *, JArray< jbyte > *);
  IndexColorModel(jint, jint, JArray< jbyte > *, JArray< jbyte > *, JArray< jbyte > *, jint);
  IndexColorModel(jint, jint, JArray< jbyte > *, JArray< jbyte > *, JArray< jbyte > *, JArray< jbyte > *);
  IndexColorModel(jint, jint, JArray< jbyte > *, jint, jboolean);
  IndexColorModel(jint, jint, JArray< jbyte > *, jint, jboolean, jint);
  IndexColorModel(jint, jint, JArray< jint > *, jint, jboolean, jint, jint);
  IndexColorModel(jint, jint, JArray< jint > *, jint, jint, ::java::math::BigInteger *);
  virtual jint getMapSize();
  virtual jint getTransparentPixel();
  virtual void getReds(JArray< jbyte > *);
  virtual void getGreens(JArray< jbyte > *);
  virtual void getBlues(JArray< jbyte > *);
  virtual void getAlphas(JArray< jbyte > *);
  virtual jint getRed(jint);
  virtual jint getGreen(jint);
  virtual jint getBlue(jint);
  virtual jint getAlpha(jint);
  virtual jint getRGB(jint);
  virtual void getRGBs(JArray< jint > *);
  virtual jboolean isValid(jint);
  virtual jboolean isValid();
  virtual ::java::math::BigInteger * getValidPixels();
  virtual ::java::awt::image::BufferedImage * convertToIntDiscrete(::java::awt::image::Raster *, jboolean);
private:
  jint __attribute__((aligned(__alignof__( ::java::awt::image::ColorModel)))) map_size;
  jboolean opaque;
  jint trans;
  JArray< jint > * rgb;
  ::java::math::BigInteger * validBits;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_image_IndexColorModel__
