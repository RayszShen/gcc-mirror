
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_Popup$JWindowPopup__
#define __javax_swing_Popup$JWindowPopup__

#pragma interface

#include <javax/swing/Popup.h>
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
        class JWindow;
        class Popup$JWindowPopup;
    }
  }
}

class javax::swing::Popup$JWindowPopup : public ::javax::swing::Popup
{

public:
  Popup$JWindowPopup(::java::awt::Component *, ::java::awt::Component *, jint, jint);
  virtual void show();
  virtual void hide();
public: // actually package-private
  ::javax::swing::JWindow * __attribute__((aligned(__alignof__( ::javax::swing::Popup)))) window;
private:
  ::java::awt::Component * contents;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_Popup$JWindowPopup__
