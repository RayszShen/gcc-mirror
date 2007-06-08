
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_event_AncestorEvent__
#define __javax_swing_event_AncestorEvent__

#pragma interface

#include <java/awt/AWTEvent.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Container;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JComponent;
      namespace event
      {
          class AncestorEvent;
      }
    }
  }
}

class javax::swing::event::AncestorEvent : public ::java::awt::AWTEvent
{

public:
  AncestorEvent(::javax::swing::JComponent *, jint, ::java::awt::Container *, ::java::awt::Container *);
  virtual ::java::awt::Container * getAncestor();
  virtual ::java::awt::Container * getAncestorParent();
  virtual ::javax::swing::JComponent * getComponent();
private:
  static const jlong serialVersionUID = -8079801679695605002LL;
public:
  static const jint ANCESTOR_ADDED = 1;
  static const jint ANCESTOR_REMOVED = 2;
  static const jint ANCESTOR_MOVED = 3;
private:
  ::javax::swing::JComponent * __attribute__((aligned(__alignof__( ::java::awt::AWTEvent)))) sourceComponent;
  ::java::awt::Container * ancestor;
  ::java::awt::Container * ancestorParent;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_event_AncestorEvent__
