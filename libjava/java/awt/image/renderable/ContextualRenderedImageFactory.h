
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_renderable_ContextualRenderedImageFactory__
#define __java_awt_image_renderable_ContextualRenderedImageFactory__

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
      namespace geom
      {
          class Rectangle2D;
      }
      namespace image
      {
          class RenderedImage;
        namespace renderable
        {
            class ContextualRenderedImageFactory;
            class ParameterBlock;
            class RenderContext;
            class RenderableImage;
        }
      }
    }
  }
}

class java::awt::image::renderable::ContextualRenderedImageFactory : public ::java::lang::Object
{

public:
  virtual ::java::awt::image::renderable::RenderContext * mapRenderContext(jint, ::java::awt::image::renderable::RenderContext *, ::java::awt::image::renderable::ParameterBlock *, ::java::awt::image::renderable::RenderableImage *) = 0;
  virtual ::java::awt::image::RenderedImage * create(::java::awt::image::renderable::RenderContext *, ::java::awt::image::renderable::ParameterBlock *) = 0;
  virtual ::java::awt::geom::Rectangle2D * getBounds2D(::java::awt::image::renderable::ParameterBlock *) = 0;
  virtual ::java::lang::Object * getProperty(::java::awt::image::renderable::ParameterBlock *, ::java::lang::String *) = 0;
  virtual JArray< ::java::lang::String * > * getPropertyNames() = 0;
  virtual jboolean isDynamic() = 0;
  virtual ::java::awt::image::RenderedImage * create(::java::awt::image::renderable::ParameterBlock *, ::java::awt::RenderingHints *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_awt_image_renderable_ContextualRenderedImageFactory__
