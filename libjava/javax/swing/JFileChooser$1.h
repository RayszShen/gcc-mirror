
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JFileChooser$1__
#define __javax_swing_JFileChooser$1__

#pragma interface

#include <java/awt/event/WindowAdapter.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class WindowEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JFileChooser;
        class JFileChooser$1;
    }
  }
}

class javax::swing::JFileChooser$1 : public ::java::awt::event::WindowAdapter
{

public: // actually package-private
  JFileChooser$1(::javax::swing::JFileChooser *);
public:
  virtual void windowClosing(::java::awt::event::WindowEvent *);
public: // actually package-private
  ::javax::swing::JFileChooser * __attribute__((aligned(__alignof__( ::java::awt::event::WindowAdapter)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JFileChooser$1__
