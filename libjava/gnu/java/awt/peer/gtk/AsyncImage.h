
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_gtk_AsyncImage__
#define __gnu_java_awt_peer_gtk_AsyncImage__

#pragma interface

#include <java/awt/Image.h>
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
              class AsyncImage;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Graphics;
        class Image;
      namespace image
      {
          class ImageObserver;
          class ImageProducer;
      }
    }
    namespace net
    {
        class URL;
    }
  }
}

class gnu::java::awt::peer::gtk::AsyncImage : public ::java::awt::Image
{

public: // actually package-private
  AsyncImage(::java::net::URL *);
public:
  virtual void flush();
  virtual ::java::awt::Graphics * getGraphics();
  virtual jint getHeight(::java::awt::image::ImageObserver *);
  virtual ::java::lang::Object * getProperty(::java::lang::String *, ::java::awt::image::ImageObserver *);
  virtual ::java::awt::image::ImageProducer * getSource();
  virtual jint getWidth(::java::awt::image::ImageObserver *);
public: // actually package-private
  virtual void addObserver(::java::awt::image::ImageObserver *);
  static ::java::awt::Image * realImage(::java::awt::Image *, ::java::awt::image::ImageObserver *);
  virtual void notifyObservers(jint);
  virtual jint checkImage(::java::awt::image::ImageObserver *);
  ::java::awt::Image * __attribute__((aligned(__alignof__( ::java::awt::Image)))) realImage__;
  ::java::util::HashSet * observers;
  static jboolean $assertionsDisabled;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_gtk_AsyncImage__
