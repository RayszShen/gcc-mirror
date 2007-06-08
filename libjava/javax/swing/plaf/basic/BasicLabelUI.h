
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicLabelUI__
#define __javax_swing_plaf_basic_BasicLabelUI__

#pragma interface

#include <javax/swing/plaf/LabelUI.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Dimension;
        class FontMetrics;
        class Graphics;
        class Insets;
        class Rectangle;
    }
    namespace beans
    {
        class PropertyChangeEvent;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class Icon;
        class JComponent;
        class JLabel;
      namespace plaf
      {
          class ComponentUI;
        namespace basic
        {
            class BasicLabelUI;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicLabelUI : public ::javax::swing::plaf::LabelUI
{

public:
  BasicLabelUI();
  static ::javax::swing::plaf::ComponentUI * createUI(::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getPreferredSize(::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getMinimumSize(::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getMaximumSize(::javax::swing::JComponent *);
  virtual void paint(::java::awt::Graphics *, ::javax::swing::JComponent *);
public: // actually protected
  virtual ::java::lang::String * layoutCL(::javax::swing::JLabel *, ::java::awt::FontMetrics *, ::java::lang::String *, ::javax::swing::Icon *, ::java::awt::Rectangle *, ::java::awt::Rectangle *, ::java::awt::Rectangle *);
  virtual void paintDisabledText(::javax::swing::JLabel *, ::java::awt::Graphics *, ::java::lang::String *, jint, jint);
  virtual void paintEnabledText(::javax::swing::JLabel *, ::java::awt::Graphics *, ::java::lang::String *, jint, jint);
public:
  virtual void installUI(::javax::swing::JComponent *);
  virtual void uninstallUI(::javax::swing::JComponent *);
public: // actually protected
  virtual void installComponents(::javax::swing::JLabel *);
  virtual void uninstallComponents(::javax::swing::JLabel *);
  virtual void installDefaults(::javax::swing::JLabel *);
  virtual void uninstallDefaults(::javax::swing::JLabel *);
  virtual void installKeyboardActions(::javax::swing::JLabel *);
  virtual void uninstallKeyboardActions(::javax::swing::JLabel *);
  virtual void installListeners(::javax::swing::JLabel *);
  virtual void uninstallListeners(::javax::swing::JLabel *);
public:
  virtual void propertyChange(::java::beans::PropertyChangeEvent *);
private:
  ::java::awt::FontMetrics * getFontMetrics(::javax::swing::JLabel *);
public: // actually protected
  static ::javax::swing::plaf::basic::BasicLabelUI * labelUI;
private:
  ::java::awt::Rectangle * __attribute__((aligned(__alignof__( ::javax::swing::plaf::LabelUI)))) vr;
  ::java::awt::Rectangle * ir;
  ::java::awt::Rectangle * tr;
  ::java::awt::Insets * cachedInsets;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicLabelUI__
