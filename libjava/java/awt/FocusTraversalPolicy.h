
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_FocusTraversalPolicy__
#define __java_awt_FocusTraversalPolicy__

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
        class FocusTraversalPolicy;
        class Window;
    }
  }
}

class java::awt::FocusTraversalPolicy : public ::java::lang::Object
{

public:
  FocusTraversalPolicy();
  virtual ::java::awt::Component * getComponentAfter(::java::awt::Container *, ::java::awt::Component *) = 0;
  virtual ::java::awt::Component * getComponentBefore(::java::awt::Container *, ::java::awt::Component *) = 0;
  virtual ::java::awt::Component * getFirstComponent(::java::awt::Container *) = 0;
  virtual ::java::awt::Component * getLastComponent(::java::awt::Container *) = 0;
  virtual ::java::awt::Component * getDefaultComponent(::java::awt::Container *) = 0;
  virtual ::java::awt::Component * getInitialComponent(::java::awt::Window *);
  static ::java::lang::Class class$;
};

#endif // __java_awt_FocusTraversalPolicy__
