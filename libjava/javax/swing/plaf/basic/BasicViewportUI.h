
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicViewportUI__
#define __javax_swing_plaf_basic_BasicViewportUI__

#pragma interface

#include <javax/swing/plaf/ViewportUI.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class JComponent;
      namespace plaf
      {
          class ComponentUI;
        namespace basic
        {
            class BasicViewportUI;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicViewportUI : public ::javax::swing::plaf::ViewportUI
{

public:
  BasicViewportUI();
public: // actually protected
  virtual void installDefaults(::javax::swing::JComponent *);
  virtual void uninstallDefaults(::javax::swing::JComponent *);
public:
  static ::javax::swing::plaf::ComponentUI * createUI(::javax::swing::JComponent *);
  virtual void installUI(::javax::swing::JComponent *);
  virtual void uninstallUI(::javax::swing::JComponent *);
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicViewportUI__
