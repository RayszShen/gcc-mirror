
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_TextAction$HorizontalMovementAction__
#define __javax_swing_text_TextAction$HorizontalMovementAction__

#pragma interface

#include <javax/swing/text/TextAction.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class ActionEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
      namespace text
      {
          class Caret;
          class TextAction$HorizontalMovementAction;
      }
    }
  }
}

class javax::swing::text::TextAction$HorizontalMovementAction : public ::javax::swing::text::TextAction
{

public: // actually package-private
  TextAction$HorizontalMovementAction(::java::lang::String *, jint);
public:
  virtual void actionPerformed(::java::awt::event::ActionEvent *);
public: // actually protected
  virtual void actionPerformedImpl(::javax::swing::text::Caret *, jint) = 0;
public: // actually package-private
  jint __attribute__((aligned(__alignof__( ::javax::swing::text::TextAction)))) dir;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_TextAction$HorizontalMovementAction__
