
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicTreeUI$MouseInputHandler__
#define __javax_swing_plaf_basic_BasicTreeUI$MouseInputHandler__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Component;
      namespace event
      {
          class MouseEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
      namespace plaf
      {
        namespace basic
        {
            class BasicTreeUI;
            class BasicTreeUI$MouseInputHandler;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicTreeUI$MouseInputHandler : public ::java::lang::Object
{

public:
  BasicTreeUI$MouseInputHandler(::javax::swing::plaf::basic::BasicTreeUI *, ::java::awt::Component *, ::java::awt::Component *, ::java::awt::event::MouseEvent *);
  virtual void mouseClicked(::java::awt::event::MouseEvent *);
  virtual void mousePressed(::java::awt::event::MouseEvent *);
  virtual void mouseReleased(::java::awt::event::MouseEvent *);
  virtual void mouseEntered(::java::awt::event::MouseEvent *);
  virtual void mouseExited(::java::awt::event::MouseEvent *);
  virtual void mouseDragged(::java::awt::event::MouseEvent *);
  virtual void mouseMoved(::java::awt::event::MouseEvent *);
public: // actually protected
  virtual void removeFromSource();
private:
  void dispatch(::java::awt::event::MouseEvent *);
public: // actually protected
  ::java::awt::Component * __attribute__((aligned(__alignof__( ::java::lang::Object)))) source;
  ::java::awt::Component * destination;
public: // actually package-private
  ::javax::swing::plaf::basic::BasicTreeUI * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicTreeUI$MouseInputHandler__
