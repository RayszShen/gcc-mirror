
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_gtk_CairoGraphics2D__
#define __gnu_java_awt_peer_gtk_CairoGraphics2D__

#pragma interface

#include <java/awt/Graphics2D.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace peer
        {
          namespace gtk
          {
              class CairoGraphics2D;
              class CairoSurface;
              class GdkFontPeer;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class BasicStroke;
        class Color;
        class Composite;
        class CompositeContext;
        class Font;
        class FontMetrics;
        class Graphics;
        class GraphicsConfiguration;
        class Image;
        class Paint;
        class Rectangle;
        class RenderingHints;
        class RenderingHints$Key;
        class Shape;
        class Stroke;
      namespace font
      {
          class FontRenderContext;
          class GlyphVector;
      }
      namespace geom
      {
          class AffineTransform;
          class PathIterator;
          class Rectangle2D;
      }
      namespace image
      {
          class BufferedImage;
          class BufferedImageOp;
          class ColorModel;
          class ImageObserver;
          class Raster;
          class RenderedImage;
        namespace renderable
        {
            class RenderableImage;
        }
      }
    }
    namespace text
    {
        class AttributedCharacterIterator;
    }
  }
}

class gnu::java::awt::peer::gtk::CairoGraphics2D : public ::java::awt::Graphics2D
{

public:
  CairoGraphics2D();
  virtual void setup(jlong);
  virtual void copy(::gnu::java::awt::peer::gtk::CairoGraphics2D *, jlong);
  virtual void finalize();
  virtual void dispose();
public: // actually protected
  virtual jlong init(jlong);
public:
  virtual ::java::awt::Graphics * create() = 0;
  virtual ::java::awt::GraphicsConfiguration * getDeviceConfiguration() = 0;
public: // actually protected
  virtual void copyAreaImpl(jint, jint, jint, jint, jint, jint) = 0;
  virtual ::java::awt::geom::Rectangle2D * getRealBounds() = 0;
public:
  virtual void disposeNative(jlong);
public: // actually protected
  virtual void drawPixels(jlong, JArray< jint > *, jint, jint, jint, JArray< jdouble > *, jdouble, jint);
  virtual void setGradient(jlong, jdouble, jdouble, jdouble, jdouble, jint, jint, jint, jint, jint, jint, jint, jint, jboolean);
  virtual void setPaintPixels(jlong, JArray< jint > *, jint, jint, jint, jboolean, jint, jint);
  virtual void cairoSetMatrix(jlong, JArray< jdouble > *);
  virtual void cairoScale(jlong, jdouble, jdouble);
  virtual void cairoSetOperator(jlong, jint);
  virtual void cairoSetRGBAColor(jlong, jdouble, jdouble, jdouble, jdouble);
  virtual void cairoSetFillRule(jlong, jint);
  virtual void cairoSetLine(jlong, jdouble, jint, jint, jdouble);
  virtual void cairoSetDash(jlong, JArray< jdouble > *, jint, jdouble);
  virtual void cairoDrawGlyphVector(jlong, ::gnu::java::awt::peer::gtk::GdkFontPeer *, jfloat, jfloat, jint, JArray< jint > *, JArray< jfloat > *, JArray< jlong > *);
  virtual void cairoSetFont(jlong, ::gnu::java::awt::peer::gtk::GdkFontPeer *);
  virtual void cairoRectangle(jlong, jdouble, jdouble, jdouble, jdouble);
  virtual void cairoArc(jlong, jdouble, jdouble, jdouble, jdouble, jdouble);
  virtual void cairoSave(jlong);
  virtual void cairoRestore(jlong);
  virtual void cairoNewPath(jlong);
  virtual void cairoClosePath(jlong);
  virtual void cairoMoveTo(jlong, jdouble, jdouble);
  virtual void cairoLineTo(jlong, jdouble, jdouble);
  virtual void cairoCurveTo(jlong, jdouble, jdouble, jdouble, jdouble, jdouble, jdouble);
  virtual void cairoStroke(jlong);
  virtual void cairoFill(jlong, jdouble);
  virtual void cairoClip(jlong);
  virtual void cairoResetClip(jlong);
  virtual void cairoSetAntialias(jlong, jboolean);
public:
  virtual void setTransform(::java::awt::geom::AffineTransform *);
private:
  void setTransformImpl(::java::awt::geom::AffineTransform *);
public:
  virtual void transform(::java::awt::geom::AffineTransform *);
  virtual void rotate(jdouble);
  virtual void rotate(jdouble, jdouble, jdouble);
  virtual void scale(jdouble, jdouble);
  virtual void translate(jdouble, jdouble);
  virtual void translate(jint, jint);
  virtual void shear(jdouble, jdouble);
  virtual void clip(::java::awt::Shape *);
  virtual ::java::awt::Paint * getPaint();
  virtual ::java::awt::geom::AffineTransform * getTransform();
  virtual void setPaint(::java::awt::Paint *);
public: // actually protected
  virtual void setCustomPaint(::java::awt::Rectangle *);
public:
  virtual ::java::awt::Stroke * getStroke();
  virtual void setStroke(::java::awt::Stroke *);
public: // actually protected
  virtual ::java::awt::Rectangle * findStrokedBounds(::java::awt::Shape *);
public:
  virtual void setPaintMode();
  virtual void setXORMode(::java::awt::Color *);
  virtual void setColor(::java::awt::Color *);
public: // actually package-private
  virtual void updateColor();
public:
  virtual ::java::awt::Color * getColor();
  virtual void clipRect(jint, jint, jint, jint);
  virtual ::java::awt::Shape * getClip();
  virtual ::java::awt::Rectangle * getClipBounds();
public: // actually protected
  virtual ::java::awt::geom::Rectangle2D * getClipInDevSpace();
public:
  virtual void setClip(jint, jint, jint, jint);
  virtual void setClip(::java::awt::Shape *);
  virtual void setBackground(::java::awt::Color *);
  virtual ::java::awt::Color * getBackground();
  virtual ::java::awt::Composite * getComposite();
  virtual void setComposite(::java::awt::Composite *);
public: // actually protected
  virtual ::java::awt::image::ColorModel * getNativeCM() = 0;
  virtual ::java::awt::image::ColorModel * getBufferCM();
public:
  virtual void draw(::java::awt::Shape *);
  virtual void fill(::java::awt::Shape *);
private:
  void createPath(::java::awt::Shape *, jboolean);
public:
  virtual void clearRect(jint, jint, jint, jint);
  virtual void draw3DRect(jint, jint, jint, jint, jboolean);
  virtual void drawArc(jint, jint, jint, jint, jint, jint);
  virtual void drawLine(jint, jint, jint, jint);
  virtual void drawRect(jint, jint, jint, jint);
  virtual void fillArc(jint, jint, jint, jint, jint, jint);
  virtual void fillRect(jint, jint, jint, jint);
  virtual void fillPolygon(JArray< jint > *, JArray< jint > *, jint);
  virtual void drawPolygon(JArray< jint > *, JArray< jint > *, jint);
  virtual void drawPolyline(JArray< jint > *, JArray< jint > *, jint);
  virtual void drawOval(jint, jint, jint, jint);
  virtual void drawRoundRect(jint, jint, jint, jint, jint, jint);
  virtual void fillOval(jint, jint, jint, jint);
  virtual void fillRoundRect(jint, jint, jint, jint, jint, jint);
  virtual void copyArea(jint, jint, jint, jint, jint, jint);
  virtual void setRenderingHint(::java::awt::RenderingHints$Key *, ::java::lang::Object *);
  virtual ::java::lang::Object * getRenderingHint(::java::awt::RenderingHints$Key *);
  virtual void setRenderingHints(::java::util::Map *);
  virtual void addRenderingHints(::java::util::Map *);
  virtual ::java::awt::RenderingHints * getRenderingHints();
private:
  jint getInterpolation();
  void setAntialias(jboolean);
public: // actually protected
  virtual jboolean drawImage(::java::awt::Image *, ::java::awt::geom::AffineTransform *, ::java::awt::Color *, ::java::awt::image::ImageObserver *);
public:
  virtual void drawRenderedImage(::java::awt::image::RenderedImage *, ::java::awt::geom::AffineTransform *);
  virtual void drawRenderableImage(::java::awt::image::renderable::RenderableImage *, ::java::awt::geom::AffineTransform *);
  virtual jboolean drawImage(::java::awt::Image *, ::java::awt::geom::AffineTransform *, ::java::awt::image::ImageObserver *);
  virtual void drawImage(::java::awt::image::BufferedImage *, ::java::awt::image::BufferedImageOp *, jint, jint);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, ::java::awt::image::ImageObserver *);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, ::java::awt::Color *, ::java::awt::image::ImageObserver *);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, jint, jint, ::java::awt::Color *, ::java::awt::image::ImageObserver *);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, jint, jint, ::java::awt::image::ImageObserver *);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, jint, jint, jint, jint, jint, jint, ::java::awt::Color *, ::java::awt::image::ImageObserver *);
  virtual jboolean drawImage(::java::awt::Image *, jint, jint, jint, jint, jint, jint, jint, jint, ::java::awt::image::ImageObserver *);
public: // actually protected
  virtual void drawCairoSurface(::gnu::java::awt::peer::gtk::CairoSurface *, ::java::awt::geom::AffineTransform *, jdouble, jint);
public:
  virtual void drawString(::java::lang::String *, jfloat, jfloat);
  virtual void drawString(::java::lang::String *, jint, jint);
  virtual void drawString(::java::text::AttributedCharacterIterator *, jint, jint);
  virtual void drawGlyphVector(::java::awt::font::GlyphVector *, jfloat, jfloat);
  virtual void drawString(::java::text::AttributedCharacterIterator *, jfloat, jfloat);
  virtual ::java::awt::font::FontRenderContext * getFontRenderContext();
  virtual ::java::awt::FontMetrics * getFontMetrics();
  virtual ::java::awt::FontMetrics * getFontMetrics(::java::awt::Font *);
  virtual void setFont(::java::awt::Font *);
  virtual ::java::awt::Font * getFont();
  virtual jboolean hit(::java::awt::Rectangle *, ::java::awt::Shape *, jboolean);
  virtual ::java::lang::String * toString();
private:
  jboolean drawRaster(::java::awt::image::ColorModel *, ::java::awt::image::Raster *, ::java::awt::geom::AffineTransform *, ::java::awt::Color *);
  jdouble shiftX(jdouble, jboolean);
  jdouble shiftY(jdouble, jboolean);
  void walkPath(::java::awt::geom::PathIterator *, jboolean);
  ::java::util::Map * getDefaultHints();
public:
  static JArray< jint > * findSimpleIntegerArray(::java::awt::image::ColorModel *, ::java::awt::image::Raster *);
private:
  void updateClip(::java::awt::geom::AffineTransform *);
  static ::java::awt::Rectangle * computeIntersection(jint, jint, jint, jint, ::java::awt::Rectangle *);
public: // actually package-private
  static ::java::awt::geom::Rectangle2D * getTransformedBounds(::java::awt::geom::Rectangle2D *, ::java::awt::geom::AffineTransform *);
  jlong __attribute__((aligned(__alignof__( ::java::awt::Graphics2D)))) nativePointer;
  ::java::awt::Paint * paint;
  jboolean customPaint;
  ::java::awt::Stroke * stroke;
  ::java::awt::Color * fg;
  ::java::awt::Color * bg;
  ::java::awt::Shape * clip__;
  ::java::awt::geom::AffineTransform * transform__;
  ::java::awt::Font * font;
  ::java::awt::Composite * comp;
  ::java::awt::CompositeContext * compCtx;
private:
  ::java::awt::RenderingHints * hints;
  jboolean antialias;
  jboolean ignoreAA;
public: // actually protected
  jboolean shiftDrawCalls;
private:
  jboolean firstClip;
  ::java::awt::Shape * originalClip;
  static ::java::awt::BasicStroke * draw3DRectStroke;
public: // actually package-private
  static ::java::awt::image::ColorModel * rgb32;
  static ::java::awt::image::ColorModel * argb32;
public:
  static const jint INTERPOLATION_NEAREST = 0;
  static const jint INTERPOLATION_BILINEAR = 1;
  static const jint INTERPOLATION_BICUBIC = 5;
  static const jint ALPHA_INTERPOLATION_SPEED = 2;
  static const jint ALPHA_INTERPOLATION_QUALITY = 3;
  static const jint ALPHA_INTERPOLATION_DEFAULT = 4;
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_gtk_CairoGraphics2D__
