
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_border_LineBorder__
#define __javax_swing_border_LineBorder__

#pragma interface

#include <javax/swing/border/AbstractBorder.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Color;
        class Component;
        class Graphics;
        class Insets;
    }
  }
  namespace javax
  {
    namespace swing
    {
      namespace border
      {
          class Border;
          class LineBorder;
      }
    }
  }
}

class javax::swing::border::LineBorder : public ::javax::swing::border::AbstractBorder
{

public:
  LineBorder(::java::awt::Color *);
  LineBorder(::java::awt::Color *, jint);
  LineBorder(::java::awt::Color *, jint, jboolean);
  static ::javax::swing::border::Border * createBlackLineBorder();
  static ::javax::swing::border::Border * createGrayLineBorder();
  virtual void paintBorder(::java::awt::Component *, ::java::awt::Graphics *, jint, jint, jint, jint);
  virtual ::java::awt::Insets * getBorderInsets(::java::awt::Component *);
  virtual ::java::awt::Insets * getBorderInsets(::java::awt::Component *, ::java::awt::Insets *);
  virtual ::java::awt::Color * getLineColor();
  virtual jint getThickness();
  virtual jboolean getRoundedCorners();
  virtual jboolean isBorderOpaque();
public: // actually package-private
  static const jlong serialVersionUID = -787563427772288970LL;
private:
  static ::javax::swing::border::LineBorder * blackLineBorder;
  static ::javax::swing::border::LineBorder * grayLineBorder;
public: // actually protected
  jint __attribute__((aligned(__alignof__( ::javax::swing::border::AbstractBorder)))) thickness;
  ::java::awt::Color * lineColor;
  jboolean roundedCorners;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_border_LineBorder__
