
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_rmi_rmic_CompilerProcess$1__
#define __gnu_java_rmi_rmic_CompilerProcess$1__

#pragma interface

#include <java/lang/Thread.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace rmi
      {
        namespace rmic
        {
            class CompilerProcess;
            class CompilerProcess$1;
        }
      }
    }
  }
}

class gnu::java::rmi::rmic::CompilerProcess$1 : public ::java::lang::Thread
{

public: // actually package-private
  CompilerProcess$1(::gnu::java::rmi::rmic::CompilerProcess *, ::java::io::InputStream *);
public:
  void run();
public: // actually package-private
  ::gnu::java::rmi::rmic::CompilerProcess * __attribute__((aligned(__alignof__( ::java::lang::Thread)))) this$0;
private:
  ::java::io::InputStream * val$procin;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_rmi_rmic_CompilerProcess$1__
