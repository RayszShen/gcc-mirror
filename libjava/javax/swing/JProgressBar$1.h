
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JProgressBar$1__
#define __javax_swing_JProgressBar$1__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class JProgressBar;
        class JProgressBar$1;
      namespace event
      {
          class ChangeEvent;
      }
    }
  }
}

class javax::swing::JProgressBar$1 : public ::java::lang::Object
{

public: // actually package-private
  JProgressBar$1(::javax::swing::JProgressBar *);
public:
  void stateChanged(::javax::swing::event::ChangeEvent *);
public: // actually package-private
  ::javax::swing::JProgressBar * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JProgressBar$1__
