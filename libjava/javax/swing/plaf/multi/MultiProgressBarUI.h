
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_multi_MultiProgressBarUI__
#define __javax_swing_plaf_multi_MultiProgressBarUI__

#pragma interface

#include <javax/swing/plaf/ProgressBarUI.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Dimension;
        class Graphics;
    }
  }
  namespace javax
  {
    namespace accessibility
    {
        class Accessible;
    }
    namespace swing
    {
        class JComponent;
      namespace plaf
      {
          class ComponentUI;
        namespace multi
        {
            class MultiProgressBarUI;
        }
      }
    }
  }
}

class javax::swing::plaf::multi::MultiProgressBarUI : public ::javax::swing::plaf::ProgressBarUI
{

public:
  MultiProgressBarUI();
  static ::javax::swing::plaf::ComponentUI * createUI(::javax::swing::JComponent *);
  virtual void installUI(::javax::swing::JComponent *);
  virtual void uninstallUI(::javax::swing::JComponent *);
  virtual JArray< ::javax::swing::plaf::ComponentUI * > * getUIs();
  virtual jboolean contains(::javax::swing::JComponent *, jint, jint);
  virtual void update(::java::awt::Graphics *, ::javax::swing::JComponent *);
  virtual void paint(::java::awt::Graphics *, ::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getPreferredSize(::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getMinimumSize(::javax::swing::JComponent *);
  virtual ::java::awt::Dimension * getMaximumSize(::javax::swing::JComponent *);
  virtual jint getAccessibleChildrenCount(::javax::swing::JComponent *);
  virtual ::javax::accessibility::Accessible * getAccessibleChild(::javax::swing::JComponent *, jint);
public: // actually protected
  ::java::util::Vector * __attribute__((aligned(__alignof__( ::javax::swing::plaf::ProgressBarUI)))) uis;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_multi_MultiProgressBarUI__
