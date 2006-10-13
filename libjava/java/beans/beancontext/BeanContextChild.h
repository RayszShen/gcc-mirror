
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_beans_beancontext_BeanContextChild__
#define __java_beans_beancontext_BeanContextChild__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace beans
    {
        class PropertyChangeListener;
        class VetoableChangeListener;
      namespace beancontext
      {
          class BeanContext;
          class BeanContextChild;
      }
    }
  }
}

class java::beans::beancontext::BeanContextChild : public ::java::lang::Object
{

public:
  virtual void setBeanContext(::java::beans::beancontext::BeanContext *) = 0;
  virtual ::java::beans::beancontext::BeanContext * getBeanContext() = 0;
  virtual void addPropertyChangeListener(::java::lang::String *, ::java::beans::PropertyChangeListener *) = 0;
  virtual void removePropertyChangeListener(::java::lang::String *, ::java::beans::PropertyChangeListener *) = 0;
  virtual void addVetoableChangeListener(::java::lang::String *, ::java::beans::VetoableChangeListener *) = 0;
  virtual void removeVetoableChangeListener(::java::lang::String *, ::java::beans::VetoableChangeListener *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_beans_beancontext_BeanContextChild__
