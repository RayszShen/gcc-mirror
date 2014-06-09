
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicRootPaneUI__
#define __javax_swing_plaf_basic_BasicRootPaneUI__

#pragma interface

#include <javax/swing/plaf/RootPaneUI.h>
extern "Java"
{
  namespace java
  {
    namespace beans
    {
        class PropertyChangeEvent;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JComponent;
        class JRootPane;
      namespace plaf
      {
          class ComponentUI;
        namespace basic
        {
            class BasicRootPaneUI;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicRootPaneUI : public ::javax::swing::plaf::RootPaneUI
{

public:
  BasicRootPaneUI();
  static ::javax::swing::plaf::ComponentUI * createUI(::javax::swing::JComponent *);
  virtual void installUI(::javax::swing::JComponent *);
public: // actually protected
  virtual void installDefaults(::javax::swing::JRootPane *);
  virtual void installComponents(::javax::swing::JRootPane *);
  virtual void installListeners(::javax::swing::JRootPane *);
  virtual void installKeyboardActions(::javax::swing::JRootPane *);
public:
  virtual void propertyChange(::java::beans::PropertyChangeEvent *);
  virtual void uninstallUI(::javax::swing::JComponent *);
public: // actually protected
  virtual void uninstallDefaults(::javax::swing::JRootPane *);
  virtual void uninstallComponents(::javax::swing::JRootPane *);
  virtual void uninstallListeners(::javax::swing::JRootPane *);
  virtual void uninstallKeyboardActions(::javax::swing::JRootPane *);
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicRootPaneUI__
