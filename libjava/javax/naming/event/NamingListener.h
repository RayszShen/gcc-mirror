
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_naming_event_NamingListener__
#define __javax_naming_event_NamingListener__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace naming
    {
      namespace event
      {
          class NamingExceptionEvent;
          class NamingListener;
      }
    }
  }
}

class javax::naming::event::NamingListener : public ::java::lang::Object
{

public:
  virtual void namingExceptionThrown(::javax::naming::event::NamingExceptionEvent *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __javax_naming_event_NamingListener__
