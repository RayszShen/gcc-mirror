
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_ItemSelectable__
#define __java_awt_ItemSelectable__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class ItemSelectable;
      namespace event
      {
          class ItemListener;
      }
    }
  }
}

class java::awt::ItemSelectable : public ::java::lang::Object
{

public:
  virtual JArray< ::java::lang::Object * > * getSelectedObjects() = 0;
  virtual void addItemListener(::java::awt::event::ItemListener *) = 0;
  virtual void removeItemListener(::java::awt::event::ItemListener *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_awt_ItemSelectable__
