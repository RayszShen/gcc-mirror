
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_EmbeddedWindow__
#define __gnu_java_awt_EmbeddedWindow__

#pragma interface

#include <java/awt/Frame.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
          class EmbeddedWindow;
      }
    }
  }
}

class gnu::java::awt::EmbeddedWindow : public ::java::awt::Frame
{

public:
  EmbeddedWindow();
  EmbeddedWindow(jlong);
  virtual void addNotify();
  virtual void setHandle(jlong);
  virtual jlong getHandle();
private:
  jlong __attribute__((aligned(__alignof__( ::java::awt::Frame)))) handle;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_EmbeddedWindow__
