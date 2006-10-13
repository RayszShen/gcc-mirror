
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_ColorConvertOp__
#define __java_awt_image_ColorConvertOp__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class RenderingHints;
      namespace color
      {
          class ColorSpace;
          class ICC_Profile;
      }
      namespace geom
      {
          class Point2D;
          class Rectangle2D;
      }
      namespace image
      {
          class BufferedImage;
          class ColorConvertOp;
          class ColorModel;
          class Raster;
          class WritableRaster;
      }
    }
  }
}

class java::awt::image::ColorConvertOp : public ::java::lang::Object
{

public:
  ColorConvertOp(::java::awt::color::ColorSpace *, ::java::awt::RenderingHints *);
  ColorConvertOp(::java::awt::color::ColorSpace *, ::java::awt::color::ColorSpace *, ::java::awt::RenderingHints *);
  ColorConvertOp(JArray< ::java::awt::color::ICC_Profile * > *, ::java::awt::RenderingHints *);
  ColorConvertOp(::java::awt::RenderingHints *);
  virtual ::java::awt::image::BufferedImage * filter(::java::awt::image::BufferedImage *, ::java::awt::image::BufferedImage *);
  virtual ::java::awt::image::BufferedImage * createCompatibleDestImage(::java::awt::image::BufferedImage *, ::java::awt::image::ColorModel *);
  virtual JArray< ::java::awt::color::ICC_Profile * > * getICC_Profiles();
  virtual ::java::awt::RenderingHints * getRenderingHints();
  virtual ::java::awt::image::WritableRaster * filter(::java::awt::image::Raster *, ::java::awt::image::WritableRaster *);
  virtual ::java::awt::image::WritableRaster * createCompatibleDestRaster(::java::awt::image::Raster *);
  virtual ::java::awt::geom::Point2D * getPoint2D(::java::awt::geom::Point2D *, ::java::awt::geom::Point2D *);
  virtual ::java::awt::geom::Rectangle2D * getBounds2D(::java::awt::image::BufferedImage *);
  virtual ::java::awt::geom::Rectangle2D * getBounds2D(::java::awt::image::Raster *);
private:
  void copyimage(::java::awt::image::BufferedImage *, ::java::awt::image::BufferedImage *);
  void copyraster(::java::awt::image::Raster *, ::java::awt::color::ColorSpace *, ::java::awt::image::WritableRaster *, ::java::awt::color::ColorSpace *);
  ::java::awt::color::ColorSpace * __attribute__((aligned(__alignof__( ::java::lang::Object)))) srccs;
  ::java::awt::color::ColorSpace * dstcs;
  ::java::awt::RenderingHints * hints;
  JArray< ::java::awt::color::ICC_Profile * > * profiles;
  JArray< ::java::awt::color::ColorSpace * > * spaces;
  jboolean rasterValid;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_image_ColorConvertOp__
