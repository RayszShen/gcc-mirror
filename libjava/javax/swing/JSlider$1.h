
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JSlider$1__
#define __javax_swing_JSlider$1__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class JSlider;
        class JSlider$1;
      namespace event
      {
          class ChangeEvent;
      }
    }
  }
}

class javax::swing::JSlider$1 : public ::java::lang::Object
{

public: // actually package-private
  JSlider$1(::javax::swing::JSlider *);
public:
  virtual void stateChanged(::javax::swing::event::ChangeEvent *);
public: // actually package-private
  ::javax::swing::JSlider * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JSlider$1__
