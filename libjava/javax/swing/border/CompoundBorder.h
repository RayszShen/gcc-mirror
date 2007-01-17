
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_border_CompoundBorder__
#define __javax_swing_border_CompoundBorder__

#pragma interface

#include <javax/swing/border/AbstractBorder.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
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
          class CompoundBorder;
      }
    }
  }
}

class javax::swing::border::CompoundBorder : public ::javax::swing::border::AbstractBorder
{

public:
  CompoundBorder();
  CompoundBorder(::javax::swing::border::Border *, ::javax::swing::border::Border *);
  virtual jboolean isBorderOpaque();
  virtual void paintBorder(::java::awt::Component *, ::java::awt::Graphics *, jint, jint, jint, jint);
  virtual ::java::awt::Insets * getBorderInsets(::java::awt::Component *, ::java::awt::Insets *);
  virtual ::java::awt::Insets * getBorderInsets(::java::awt::Component *);
  virtual ::javax::swing::border::Border * getOutsideBorder();
  virtual ::javax::swing::border::Border * getInsideBorder();
public: // actually package-private
  static const jlong serialVersionUID = 9054540377030555103LL;
public: // actually protected
  ::javax::swing::border::Border * __attribute__((aligned(__alignof__( ::javax::swing::border::AbstractBorder)))) insideBorder;
  ::javax::swing::border::Border * outsideBorder;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_border_CompoundBorder__
