
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_processor_CommandSet__
#define __gnu_classpath_jdwp_processor_CommandSet__

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
          class VMIdManager;
        namespace processor
        {
            class CommandSet;
        }
      }
    }
  }
  namespace java
  {
    namespace nio
    {
        class ByteBuffer;
    }
  }
}

class gnu::classpath::jdwp::processor::CommandSet : public ::java::lang::Object
{

public:
  CommandSet();
  virtual jboolean runCommand(::java::nio::ByteBuffer *, ::java::io::DataOutputStream *, jbyte) = 0;
public: // actually protected
  ::gnu::classpath::jdwp::VMIdManager * __attribute__((aligned(__alignof__( ::java::lang::Object)))) idMan;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_processor_CommandSet__
