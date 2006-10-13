
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_font_opentype_Scaler__
#define __gnu_java_awt_font_opentype_Scaler__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace font
        {
          namespace opentype
          {
              class Scaler;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
      namespace geom
      {
          class AffineTransform;
          class GeneralPath;
          class Point2D;
      }
    }
  }
}

class gnu::java::awt::font::opentype::Scaler : public ::java::lang::Object
{

public:
  Scaler();
  virtual ::java::awt::geom::GeneralPath * getOutline(jint, jfloat, ::java::awt::geom::AffineTransform *, jboolean, jboolean) = 0;
  virtual void getAdvance(jint, jfloat, ::java::awt::geom::AffineTransform *, jboolean, jboolean, jboolean, ::java::awt::geom::Point2D *) = 0;
  virtual jfloat getAscent(jfloat, ::java::awt::geom::AffineTransform *, jboolean, jboolean, jboolean) = 0;
  virtual jfloat getDescent(jfloat, ::java::awt::geom::AffineTransform *, jboolean, jboolean, jboolean) = 0;
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_font_opentype_Scaler__
