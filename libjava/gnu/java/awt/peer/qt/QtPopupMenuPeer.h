
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_qt_QtPopupMenuPeer__
#define __gnu_java_awt_peer_qt_QtPopupMenuPeer__

#pragma interface

#include <gnu/java/awt/peer/qt/QtMenuPeer.h>
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
          namespace qt
          {
              class QtPopupMenuPeer;
              class QtToolkit;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Component;
        class Event;
        class PopupMenu;
    }
  }
}

class gnu::java::awt::peer::qt::QtPopupMenuPeer : public ::gnu::java::awt::peer::qt::QtMenuPeer
{

public:
  QtPopupMenuPeer(::gnu::java::awt::peer::qt::QtToolkit *, ::java::awt::PopupMenu *);
private:
  void showNative(jint, jint);
public:
  virtual void show(::java::awt::Component *, jint, jint);
  virtual void show(::java::awt::Event *);
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_qt_QtPopupMenuPeer__
