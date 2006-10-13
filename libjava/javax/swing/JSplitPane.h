
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JSplitPane__
#define __javax_swing_JSplitPane__

#pragma interface

#include <javax/swing/JComponent.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Component;
        class Graphics;
    }
  }
  namespace javax
  {
    namespace accessibility
    {
        class AccessibleContext;
    }
    namespace swing
    {
        class JSplitPane;
      namespace plaf
      {
          class SplitPaneUI;
      }
    }
  }
}

class javax::swing::JSplitPane : public ::javax::swing::JComponent
{

public:
  JSplitPane(jint, jboolean, ::java::awt::Component *, ::java::awt::Component *);
  JSplitPane(jint, ::java::awt::Component *, ::java::awt::Component *);
  JSplitPane(jint, jboolean);
  JSplitPane(jint);
  JSplitPane();
public: // actually protected
  virtual void addImpl(::java::awt::Component *, ::java::lang::Object *, jint);
public:
  virtual ::javax::accessibility::AccessibleContext * getAccessibleContext();
  virtual ::java::awt::Component * getBottomComponent();
  virtual jint getDividerLocation();
  virtual jint getDividerSize();
  virtual jint getLastDividerLocation();
  virtual ::java::awt::Component * getLeftComponent();
  virtual jint getMaximumDividerLocation();
  virtual jint getMinimumDividerLocation();
  virtual jint getOrientation();
  virtual jdouble getResizeWeight();
  virtual ::java::awt::Component * getRightComponent();
  virtual ::java::awt::Component * getTopComponent();
  virtual ::javax::swing::plaf::SplitPaneUI * getUI();
  virtual jboolean isContinuousLayout();
  virtual jboolean isOneTouchExpandable();
  virtual jboolean isValidateRoot();
public: // actually protected
  virtual void paintChildren(::java::awt::Graphics *);
  virtual ::java::lang::String * paramString();
public:
  virtual void remove(::java::awt::Component *);
  virtual void remove(jint);
  virtual void removeAll();
  virtual void resetToPreferredSizes();
  virtual void setBottomComponent(::java::awt::Component *);
  virtual void setContinuousLayout(jboolean);
  virtual void setDividerLocation(jdouble);
  virtual void setDividerLocation(jint);
  virtual void setDividerSize(jint);
  virtual void setLastDividerLocation(jint);
  virtual void setLeftComponent(::java::awt::Component *);
  virtual void setOneTouchExpandable(jboolean);
  virtual void setOrientation(jint);
  virtual void setResizeWeight(jdouble);
  virtual void setRightComponent(::java::awt::Component *);
  virtual void setTopComponent(::java::awt::Component *);
  virtual void setUI(::javax::swing::plaf::SplitPaneUI *);
  virtual void updateUI();
  virtual ::java::lang::String * getUIClassID();
public: // actually package-private
  virtual void setUIProperty(::java::lang::String *, ::java::lang::Object *);
private:
  static const jlong serialVersionUID = -5634142046175988380LL;
public:
  static ::java::lang::String * BOTTOM;
  static ::java::lang::String * CONTINUOUS_LAYOUT_PROPERTY;
  static ::java::lang::String * DIVIDER;
  static ::java::lang::String * DIVIDER_LOCATION_PROPERTY;
  static ::java::lang::String * DIVIDER_SIZE_PROPERTY;
  static const jint HORIZONTAL_SPLIT = 1;
  static ::java::lang::String * LAST_DIVIDER_LOCATION_PROPERTY;
  static ::java::lang::String * LEFT;
  static ::java::lang::String * ONE_TOUCH_EXPANDABLE_PROPERTY;
  static ::java::lang::String * ORIENTATION_PROPERTY;
  static ::java::lang::String * RESIZE_WEIGHT_PROPERTY;
  static ::java::lang::String * RIGHT;
  static ::java::lang::String * TOP;
  static const jint VERTICAL_SPLIT = 0;
public: // actually protected
  jboolean __attribute__((aligned(__alignof__( ::javax::swing::JComponent)))) continuousLayout;
  jboolean oneTouchExpandable;
  jint dividerSize;
  jint lastDividerLocation;
  jint orientation;
  ::java::awt::Component * leftComponent;
  ::java::awt::Component * rightComponent;
private:
  jdouble resizeWeight;
  jboolean clientDividerSizeSet;
  jboolean clientOneTouchExpandableSet;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JSplitPane__
