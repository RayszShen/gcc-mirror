
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_image_renderable_ParameterBlock__
#define __java_awt_image_renderable_ParameterBlock__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace image
      {
          class RenderedImage;
        namespace renderable
        {
            class ParameterBlock;
            class RenderableImage;
        }
      }
    }
  }
}

class java::awt::image::renderable::ParameterBlock : public ::java::lang::Object
{

public:
  ParameterBlock();
  ParameterBlock(::java::util::Vector *);
  ParameterBlock(::java::util::Vector *, ::java::util::Vector *);
  virtual ::java::lang::Object * shallowClone();
  virtual ::java::lang::Object * clone();
  virtual ::java::awt::image::renderable::ParameterBlock * addSource(::java::lang::Object *);
  virtual ::java::lang::Object * getSource(jint);
  virtual ::java::awt::image::renderable::ParameterBlock * setSource(::java::lang::Object *, jint);
  virtual ::java::awt::image::RenderedImage * getRenderedSource(jint);
  virtual ::java::awt::image::renderable::RenderableImage * getRenderableSource(jint);
  virtual jint getNumSources();
  virtual ::java::util::Vector * getSources();
  virtual void setSources(::java::util::Vector *);
  virtual void removeSources();
  virtual jint getNumParameters();
  virtual ::java::util::Vector * getParameters();
  virtual void setParameters(::java::util::Vector *);
  virtual void removeParameters();
  virtual ::java::awt::image::renderable::ParameterBlock * add(::java::lang::Object *);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jbyte);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jchar);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jshort);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jint);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jlong);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jfloat);
  virtual ::java::awt::image::renderable::ParameterBlock * add(jdouble);
  virtual ::java::awt::image::renderable::ParameterBlock * set(::java::lang::Object *, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jbyte, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jchar, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jshort, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jint, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jlong, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jfloat, jint);
  virtual ::java::awt::image::renderable::ParameterBlock * set(jdouble, jint);
  virtual ::java::lang::Object * getObjectParameter(jint);
  virtual jbyte getByteParameter(jint);
  virtual jchar getCharParameter(jint);
  virtual jshort getShortParameter(jint);
  virtual jint getIntParameter(jint);
  virtual jlong getLongParameter(jint);
  virtual jfloat getFloatParameter(jint);
  virtual jdouble getDoubleParameter(jint);
  virtual JArray< ::java::lang::Class * > * getParamClasses();
private:
  static const jlong serialVersionUID = -7577115551785240750LL;
public: // actually protected
  ::java::util::Vector * __attribute__((aligned(__alignof__( ::java::lang::Object)))) sources;
  ::java::util::Vector * parameters;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_image_renderable_ParameterBlock__
