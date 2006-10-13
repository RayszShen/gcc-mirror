
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_GraphicsEnvironment__
#define __java_awt_GraphicsEnvironment__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Font;
        class Graphics2D;
        class GraphicsDevice;
        class GraphicsEnvironment;
        class Point;
        class Rectangle;
      namespace image
      {
          class BufferedImage;
      }
    }
  }
}

class java::awt::GraphicsEnvironment : public ::java::lang::Object
{

public: // actually protected
  GraphicsEnvironment();
public:
  static ::java::awt::GraphicsEnvironment * getLocalGraphicsEnvironment();
  static jboolean isHeadless();
  virtual jboolean isHeadlessInstance();
  virtual JArray< ::java::awt::GraphicsDevice * > * getScreenDevices() = 0;
  virtual ::java::awt::GraphicsDevice * getDefaultScreenDevice() = 0;
  virtual ::java::awt::Graphics2D * createGraphics(::java::awt::image::BufferedImage *) = 0;
  virtual JArray< ::java::awt::Font * > * getAllFonts() = 0;
  virtual JArray< ::java::lang::String * > * getAvailableFontFamilyNames() = 0;
  virtual JArray< ::java::lang::String * > * getAvailableFontFamilyNames(::java::util::Locale *) = 0;
  virtual ::java::awt::Point * getCenterPoint();
  virtual ::java::awt::Rectangle * getMaximumWindowBounds();
private:
  static ::java::awt::GraphicsEnvironment * localGraphicsEnvironment;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_GraphicsEnvironment__
