
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_html_ImageView__
#define __javax_swing_text_html_ImageView__

#pragma interface

#include <javax/swing/text/View.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Graphics;
        class Image;
        class Rectangle;
        class Shape;
    }
    namespace net
    {
        class URL;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class Icon;
        class ImageIcon;
      namespace text
      {
          class AttributeSet;
          class Element;
          class Position$Bias;
        namespace html
        {
            class ImageView;
            class StyleSheet;
        }
      }
    }
  }
}

class javax::swing::text::html::ImageView : public ::javax::swing::text::View
{

public:
  ImageView(::javax::swing::text::Element *);
public: // actually package-private
  virtual void reloadImage(jboolean);
public:
  virtual jfloat getAlignment(jint);
  virtual ::java::lang::String * getAltText();
  virtual ::javax::swing::text::AttributeSet * getAttributes();
  virtual ::java::awt::Image * getImage();
  virtual ::java::net::URL * getImageURL();
  virtual ::javax::swing::Icon * getLoadingImageIcon();
  virtual jboolean getLoadsSynchronously();
  virtual ::javax::swing::Icon * getNoImageIcon();
  virtual jfloat getPreferredSpan(jint);
public: // actually protected
  virtual ::javax::swing::text::html::StyleSheet * getStyleSheet();
public:
  virtual ::java::lang::String * getToolTipText(jfloat, jfloat, ::java::awt::Shape *);
  virtual void paint(::java::awt::Graphics *, ::java::awt::Shape *);
private:
  void renderIcon(::java::awt::Graphics *, ::java::awt::Rectangle *, ::javax::swing::Icon *);
public:
  virtual void setLoadsSynchronously(jboolean);
public: // actually protected
  virtual void setPropertiesFromAttributes();
public:
  virtual jint viewToModel(jfloat, jfloat, ::java::awt::Shape *, JArray< ::javax::swing::text::Position$Bias * > *);
  virtual ::java::awt::Shape * modelToView(jint, ::java::awt::Shape *, ::javax::swing::text::Position$Bias *);
  virtual void setSize(jfloat, jfloat);
public: // actually package-private
  jboolean __attribute__((aligned(__alignof__( ::javax::swing::text::View)))) loadOnDemand;
  ::javax::swing::ImageIcon * imageIcon;
  jbyte imageState;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_html_ImageView__
