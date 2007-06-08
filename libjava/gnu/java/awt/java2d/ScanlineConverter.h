
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_java2d_ScanlineConverter__
#define __gnu_java_awt_java2d_ScanlineConverter__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace java2d
        {
            class AbstractGraphics2D;
            class ActiveEdges;
            class PolyEdge;
            class Scanline;
            class ScanlineConverter;
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Shape;
      namespace geom
      {
          class AffineTransform;
          class PathIterator;
      }
    }
  }
}

class gnu::java::awt::java2d::ScanlineConverter : public ::java::lang::Object
{

public: // actually package-private
  ScanlineConverter();
  void renderShape(::gnu::java::awt::java2d::AbstractGraphics2D *, ::java::awt::Shape *, ::java::awt::Shape *, ::java::awt::geom::AffineTransform *, jint);
private:
  void clear();
  void doScanline(::gnu::java::awt::java2d::AbstractGraphics2D *, jint, jboolean, jboolean);
  void setResolution(jint);
  void setUpperBounds(jint);
  void addShape(::java::awt::geom::PathIterator *, jboolean);
  void addEdge(::gnu::java::awt::java2d::PolyEdge *);
  jint fit(jint);
  jint scanlineIndex(jint);
  void edgePoolAdd(jint, jint, jint, jint, jboolean);
  static jint FIXED_DIGITS;
  static jint ONE;
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) numScanlines;
  JArray< ::gnu::java::awt::java2d::Scanline * > * scanlines;
  jint upperBounds;
  jint resolution;
  jint halfStep;
  JArray< jfloat > * coords;
  ::gnu::java::awt::java2d::ActiveEdges * activeEdges;
  ::gnu::java::awt::java2d::PolyEdge * edgePool;
  ::gnu::java::awt::java2d::PolyEdge * edgePoolLast;
  jint minY;
  jint maxY;
public: // actually package-private
  static jboolean $assertionsDisabled;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_java2d_ScanlineConverter__
