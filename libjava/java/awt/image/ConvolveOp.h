
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_ConvolveOp__
#define __java_awt_image_ConvolveOp__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class RenderingHints;
      namespace geom
      {
          class Point2D;
          class Rectangle2D;
      }
      namespace image
      {
          class BufferedImage;
          class ColorModel;
          class ConvolveOp;
          class Kernel;
          class Raster;
          class WritableRaster;
      }
    }
  }
}

class java::awt::image::ConvolveOp : public ::java::lang::Object
{

public:
  ConvolveOp(::java::awt::image::Kernel *, jint, ::java::awt::RenderingHints *);
  ConvolveOp(::java::awt::image::Kernel *);
  virtual ::java::awt::image::BufferedImage * filter(::java::awt::image::BufferedImage *, ::java::awt::image::BufferedImage *);
  virtual ::java::awt::image::BufferedImage * createCompatibleDestImage(::java::awt::image::BufferedImage *, ::java::awt::image::ColorModel *);
  virtual ::java::awt::RenderingHints * getRenderingHints();
  virtual jint getEdgeCondition();
  virtual ::java::awt::image::Kernel * getKernel();
  virtual ::java::awt::image::WritableRaster * filter(::java::awt::image::Raster *, ::java::awt::image::WritableRaster *);
private:
  void fillEdge(::java::awt::image::Raster *, ::java::awt::image::WritableRaster *, jint, jint, jint, jint, jint);
public:
  virtual ::java::awt::image::WritableRaster * createCompatibleDestRaster(::java::awt::image::Raster *);
  virtual ::java::awt::geom::Rectangle2D * getBounds2D(::java::awt::image::BufferedImage *);
  virtual ::java::awt::geom::Rectangle2D * getBounds2D(::java::awt::image::Raster *);
  virtual ::java::awt::geom::Point2D * getPoint2D(::java::awt::geom::Point2D *, ::java::awt::geom::Point2D *);
  static const jint EDGE_ZERO_FILL = 0;
  static const jint EDGE_NO_OP = 1;
private:
  ::java::awt::image::Kernel * __attribute__((aligned(__alignof__( ::java::lang::Object)))) kernel;
  jint edge;
  ::java::awt::RenderingHints * hints;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_image_ConvolveOp__
