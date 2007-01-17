
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_event_filters_ExceptionOnlyFilter__
#define __gnu_classpath_jdwp_event_filters_ExceptionOnlyFilter__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace classpath
    {
      namespace jdwp
      {
        namespace event
        {
            class Event;
          namespace filters
          {
              class ExceptionOnlyFilter;
          }
        }
        namespace id
        {
            class ReferenceTypeId;
        }
      }
    }
  }
}

class gnu::classpath::jdwp::event::filters::ExceptionOnlyFilter : public ::java::lang::Object
{

public:
  ExceptionOnlyFilter(::gnu::classpath::jdwp::id::ReferenceTypeId *, jboolean, jboolean);
  virtual ::gnu::classpath::jdwp::id::ReferenceTypeId * getType();
  virtual jboolean matches(::gnu::classpath::jdwp::event::Event *);
private:
  ::gnu::classpath::jdwp::id::ReferenceTypeId * __attribute__((aligned(__alignof__( ::java::lang::Object)))) _refId;
  jboolean _caught;
  jboolean _uncaught;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_event_filters_ExceptionOnlyFilter__
