
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_ComponentColorModel__
#define __java_awt_image_ComponentColorModel__

#pragma interface

#include <java/awt/image/ColorModel.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace color
      {
          class ColorSpace;
      }
      namespace image
      {
          class ColorModel;
          class ComponentColorModel;
          class Raster;
          class SampleModel;
          class WritableRaster;
      }
    }
  }
}

class java::awt::image::ComponentColorModel : public ::java::awt::image::ColorModel
{

  static jint sum(JArray< jint > *);
public:
  ComponentColorModel(::java::awt::color::ColorSpace *, JArray< jint > *, jboolean, jboolean, jint, jint);
  ComponentColorModel(::java::awt::color::ColorSpace *, jboolean, jboolean, jint, jint);
  virtual jint getRed(jint);
  virtual jint getGreen(jint);
  virtual jint getBlue(jint);
  virtual jint getAlpha(jint);
  virtual jint getRGB(jint);
private:
  JArray< jfloat > * getRGBFloat(jint);
  JArray< jfloat > * getRGBFloat(::java::lang::Object *);
public:
  virtual jint getRed(::java::lang::Object *);
  virtual jint getGreen(::java::lang::Object *);
  virtual jint getBlue(::java::lang::Object *);
  virtual jint getAlpha(::java::lang::Object *);
private:
  jint getRGB(JArray< jfloat > *);
public:
  virtual jint getRGB(::java::lang::Object *);
  virtual ::java::lang::Object * getDataElements(jint, ::java::lang::Object *);
  virtual JArray< jint > * getComponents(jint, JArray< jint > *, jint);
  virtual JArray< jint > * getComponents(::java::lang::Object *, JArray< jint > *, jint);
  virtual jint getDataElement(JArray< jint > *, jint);
  virtual ::java::lang::Object * getDataElements(JArray< jint > *, jint, ::java::lang::Object *);
  virtual ::java::awt::image::ColorModel * coerceData(::java::awt::image::WritableRaster *, jboolean);
  virtual jboolean isCompatibleRaster(::java::awt::image::Raster *);
  virtual ::java::awt::image::WritableRaster * createCompatibleWritableRaster(jint, jint);
  virtual ::java::awt::image::SampleModel * createCompatibleSampleModel(jint, jint);
  virtual jboolean isCompatibleSampleModel(::java::awt::image::SampleModel *);
  virtual ::java::awt::image::WritableRaster * getAlphaRaster(::java::awt::image::WritableRaster *);
  virtual jboolean equals(::java::lang::Object *);
  static ::java::lang::Class class$;
};

#endif // __java_awt_image_ComponentColorModel__
