
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_GapContent$Mark__
#define __javax_swing_text_GapContent$Mark__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace text
      {
          class GapContent;
          class GapContent$Mark;
      }
    }
  }
}

class javax::swing::text::GapContent$Mark : public ::java::lang::Object
{

public: // actually package-private
  GapContent$Mark(::javax::swing::text::GapContent *, jint);
  virtual jint getOffset();
public:
  virtual jint compareTo(::java::lang::Object *);
  virtual jboolean equals(::java::lang::Object *);
public: // actually package-private
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) mark;
  jint refCount;
  ::javax::swing::text::GapContent * this$0;
  static jboolean $assertionsDisabled;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_GapContent$Mark__
