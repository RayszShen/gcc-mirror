
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_Popup__
#define __javax_swing_Popup__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Component;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class Popup;
    }
  }
}

class javax::swing::Popup : public ::java::lang::Object
{

public: // actually protected
  Popup(::java::awt::Component *, ::java::awt::Component *, jint, jint);
  Popup();
public:
  virtual void show();
  virtual void hide();
  static ::java::lang::Class class$;
};

#endif // __javax_swing_Popup__
