
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_ViewportLayout__
#define __javax_swing_ViewportLayout__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Component;
        class Container;
        class Dimension;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class ViewportLayout;
    }
  }
}

class javax::swing::ViewportLayout : public ::java::lang::Object
{

public:
  ViewportLayout();
  virtual void addLayoutComponent(::java::lang::String *, ::java::awt::Component *);
  virtual void removeLayoutComponent(::java::awt::Component *);
  virtual ::java::awt::Dimension * preferredLayoutSize(::java::awt::Container *);
  virtual ::java::awt::Dimension * minimumLayoutSize(::java::awt::Container *);
  virtual void layoutContainer(::java::awt::Container *);
private:
  static const jlong serialVersionUID = -788225906076097229LL;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_ViewportLayout__
