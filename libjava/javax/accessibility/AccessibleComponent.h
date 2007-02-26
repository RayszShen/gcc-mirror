
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_accessibility_AccessibleComponent__
#define __javax_accessibility_AccessibleComponent__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Color;
        class Cursor;
        class Dimension;
        class Font;
        class FontMetrics;
        class Point;
        class Rectangle;
      namespace event
      {
          class FocusListener;
      }
    }
  }
  namespace javax
  {
    namespace accessibility
    {
        class Accessible;
        class AccessibleComponent;
    }
  }
}

class javax::accessibility::AccessibleComponent : public ::java::lang::Object
{

public:
  virtual ::java::awt::Color * getBackground() = 0;
  virtual void setBackground(::java::awt::Color *) = 0;
  virtual ::java::awt::Color * getForeground() = 0;
  virtual void setForeground(::java::awt::Color *) = 0;
  virtual ::java::awt::Cursor * getCursor() = 0;
  virtual void setCursor(::java::awt::Cursor *) = 0;
  virtual ::java::awt::Font * getFont() = 0;
  virtual void setFont(::java::awt::Font *) = 0;
  virtual ::java::awt::FontMetrics * getFontMetrics(::java::awt::Font *) = 0;
  virtual jboolean isEnabled() = 0;
  virtual void setEnabled(jboolean) = 0;
  virtual jboolean isVisible() = 0;
  virtual void setVisible(jboolean) = 0;
  virtual jboolean isShowing() = 0;
  virtual jboolean contains(::java::awt::Point *) = 0;
  virtual ::java::awt::Point * getLocationOnScreen() = 0;
  virtual ::java::awt::Point * getLocation() = 0;
  virtual void setLocation(::java::awt::Point *) = 0;
  virtual ::java::awt::Rectangle * getBounds() = 0;
  virtual void setBounds(::java::awt::Rectangle *) = 0;
  virtual ::java::awt::Dimension * getSize() = 0;
  virtual void setSize(::java::awt::Dimension *) = 0;
  virtual ::javax::accessibility::Accessible * getAccessibleAt(::java::awt::Point *) = 0;
  virtual jboolean isFocusTraversable() = 0;
  virtual void requestFocus() = 0;
  virtual void addFocusListener(::java::awt::event::FocusListener *) = 0;
  virtual void removeFocusListener(::java::awt::event::FocusListener *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __javax_accessibility_AccessibleComponent__
