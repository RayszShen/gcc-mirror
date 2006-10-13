
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_swing_SwingFramePeer__
#define __gnu_java_awt_peer_swing_SwingFramePeer__

#pragma interface

#include <gnu/java/awt/peer/swing/SwingWindowPeer.h>
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
          namespace swing
          {
              class SwingFramePeer;
              class SwingMenuBarPeer;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Frame;
        class Graphics;
        class Image;
        class Insets;
        class MenuBar;
        class Point;
        class Rectangle;
      namespace event
      {
          class MouseEvent;
      }
    }
  }
}

class gnu::java::awt::peer::swing::SwingFramePeer : public ::gnu::java::awt::peer::swing::SwingWindowPeer
{

public:
  SwingFramePeer(::java::awt::Frame *);
  virtual void setMenuBar(::java::awt::MenuBar *);
public: // actually protected
  virtual void peerPaint(::java::awt::Graphics *);
public:
  virtual void setBounds(jint, jint, jint, jint);
  virtual ::java::awt::Insets * getInsets();
  virtual ::java::awt::Point * getMenuLocationOnScreen();
public: // actually protected
  virtual void handleMouseEvent(::java::awt::event::MouseEvent *);
  virtual void handleMouseMotionEvent(::java::awt::event::MouseEvent *);
public:
  virtual void setIconImage(::java::awt::Image *) = 0;
  virtual void setResizable(jboolean) = 0;
  virtual void setTitle(::java::lang::String *) = 0;
  virtual jint getState() = 0;
  virtual void setState(jint) = 0;
  virtual void setMaximizedBounds(::java::awt::Rectangle *) = 0;
  virtual void setBoundsPrivate(jint, jint, jint, jint) = 0;
public: // actually package-private
  ::gnu::java::awt::peer::swing::SwingMenuBarPeer * __attribute__((aligned(__alignof__( ::gnu::java::awt::peer::swing::SwingWindowPeer)))) menuBar;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_swing_SwingFramePeer__
