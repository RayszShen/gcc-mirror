
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_text_ParsePosition__
#define __java_text_ParsePosition__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace text
    {
        class ParsePosition;
    }
  }
}

class java::text::ParsePosition : public ::java::lang::Object
{

public:
  ParsePosition(jint);
  virtual jint getIndex();
  virtual void setIndex(jint);
  virtual jint getErrorIndex();
  virtual void setErrorIndex(jint);
  virtual jboolean equals(::java::lang::Object *);
  virtual jint hashCode();
  virtual ::java::lang::String * toString();
private:
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) index;
  jint error_index;
public:
  static ::java::lang::Class class$;
};

#endif // __java_text_ParsePosition__
