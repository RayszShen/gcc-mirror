
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicTableUI$MouseInputHandler__
#define __javax_swing_plaf_basic_BasicTableUI$MouseInputHandler__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Point;
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
            class BasicTableUI;
            class BasicTableUI$MouseInputHandler;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicTableUI$MouseInputHandler : public ::java::lang::Object
{

public:
  BasicTableUI$MouseInputHandler(::javax::swing::plaf::basic::BasicTableUI *);
private:
  void updateSelection(jboolean);
public:
  virtual void mouseClicked(::java::awt::event::MouseEvent *);
  virtual void mouseDragged(::java::awt::event::MouseEvent *);
  virtual void mouseEntered(::java::awt::event::MouseEvent *);
  virtual void mouseExited(::java::awt::event::MouseEvent *);
  virtual void mouseMoved(::java::awt::event::MouseEvent *);
  virtual void mousePressed(::java::awt::event::MouseEvent *);
  virtual void mouseReleased(::java::awt::event::MouseEvent *);
public: // actually package-private
  ::java::awt::Point * __attribute__((aligned(__alignof__( ::java::lang::Object)))) begin;
  ::java::awt::Point * curr;
  ::javax::swing::plaf::basic::BasicTableUI * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicTableUI$MouseInputHandler__
