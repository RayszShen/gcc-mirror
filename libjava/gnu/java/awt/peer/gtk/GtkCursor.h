
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_gtk_GtkCursor__
#define __gnu_java_awt_peer_gtk_GtkCursor__

#pragma interface

#include <java/awt/Cursor.h>
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
              class GtkCursor;
              class GtkImage;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Image;
        class Point;
    }
  }
}

class gnu::java::awt::peer::gtk::GtkCursor : public ::java::awt::Cursor
{

public: // actually package-private
  GtkCursor(::java::awt::Image *, ::java::awt::Point *, ::java::lang::String *);
  virtual ::gnu::java::awt::peer::gtk::GtkImage * getGtkImage();
  virtual ::java::awt::Point * getHotspot();
private:
  ::gnu::java::awt::peer::gtk::GtkImage * __attribute__((aligned(__alignof__( ::java::awt::Cursor)))) image;
  ::java::awt::Point * hotspot;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_gtk_GtkCursor__
