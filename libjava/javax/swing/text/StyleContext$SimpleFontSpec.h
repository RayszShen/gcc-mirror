
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_StyleContext$SimpleFontSpec__
#define __javax_swing_text_StyleContext$SimpleFontSpec__

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
          class StyleContext$SimpleFontSpec;
      }
    }
  }
}

class javax::swing::text::StyleContext$SimpleFontSpec : public ::java::lang::Object
{

public:
  StyleContext$SimpleFontSpec(::java::lang::String *, jint, jint);
  virtual jboolean equals(::java::lang::Object *);
  virtual jint hashCode();
public: // actually package-private
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::lang::Object)))) family;
  jint style;
  jint size;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_StyleContext$SimpleFontSpec__
