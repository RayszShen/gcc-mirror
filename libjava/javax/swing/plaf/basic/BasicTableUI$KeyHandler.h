
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicTableUI$KeyHandler__
#define __javax_swing_plaf_basic_BasicTableUI$KeyHandler__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class KeyEvent;
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
            class BasicTableUI$KeyHandler;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicTableUI$KeyHandler : public ::java::lang::Object
{

public:
  BasicTableUI$KeyHandler(::javax::swing::plaf::basic::BasicTableUI *);
  virtual void keyTyped(::java::awt::event::KeyEvent *);
  virtual void keyPressed(::java::awt::event::KeyEvent *);
  virtual void keyReleased(::java::awt::event::KeyEvent *);
public: // actually package-private
  ::javax::swing::plaf::basic::BasicTableUI * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicTableUI$KeyHandler__
