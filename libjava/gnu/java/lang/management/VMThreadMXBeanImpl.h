
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_lang_management_VMThreadMXBeanImpl__
#define __gnu_java_lang_management_VMThreadMXBeanImpl__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace lang
      {
        namespace management
        {
            class VMThreadMXBeanImpl;
        }
      }
    }
  }
}

class gnu::java::lang::management::VMThreadMXBeanImpl : public ::java::lang::Object
{

public: // actually package-private
  VMThreadMXBeanImpl();
  static JArray< jlong > * findMonitorDeadlockedThreads();
  static JArray< ::java::lang::Thread * > * getAllThreads();
  static JArray< jlong > * getAllThreadIds();
  static jlong getCurrentThreadCpuTime();
  static jlong getCurrentThreadUserTime();
  static jint getDaemonThreadCount();
  static jint getPeakThreadCount();
  static jint getThreadCount();
  static jlong getThreadCpuTime(jlong);
  static ::java::lang::management::ThreadInfo * getThreadInfoForId(jlong, jint);
  static jlong getThreadUserTime(jlong);
  static jlong getTotalStartedThreadCount();
  static void resetPeakThreadCount();
private:
  static jint filled;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_lang_management_VMThreadMXBeanImpl__
