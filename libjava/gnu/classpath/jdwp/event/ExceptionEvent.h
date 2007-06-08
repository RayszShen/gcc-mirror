
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_event_ExceptionEvent__
#define __gnu_classpath_jdwp_event_ExceptionEvent__

#pragma interface

#include <gnu/classpath/jdwp/event/Event.h>
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
            class ExceptionEvent;
        }
        namespace util
        {
            class Location;
        }
      }
    }
  }
}

class gnu::classpath::jdwp::event::ExceptionEvent : public ::gnu::classpath::jdwp::event::Event
{

public:
  ExceptionEvent(::java::lang::Throwable *, ::java::lang::Thread *, ::gnu::classpath::jdwp::util::Location *, ::gnu::classpath::jdwp::util::Location *, ::java::lang::Class *, ::java::lang::Object *);
  virtual ::java::lang::Object * getParameter(jint);
  virtual void setCatchLoc(::gnu::classpath::jdwp::util::Location *);
public: // actually protected
  virtual void _writeData(::java::io::DataOutputStream *);
private:
  ::java::lang::Object * __attribute__((aligned(__alignof__( ::gnu::classpath::jdwp::event::Event)))) _instance;
  ::java::lang::Throwable * _exception;
  ::java::lang::Thread * _thread;
  ::gnu::classpath::jdwp::util::Location * _location;
  ::gnu::classpath::jdwp::util::Location * _catchLocation;
  ::java::lang::Class * _klass;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_event_ExceptionEvent__
