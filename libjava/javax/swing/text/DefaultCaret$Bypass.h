
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_DefaultCaret$Bypass__
#define __javax_swing_text_DefaultCaret$Bypass__

#pragma interface

#include <javax/swing/text/NavigationFilter$FilterBypass.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace text
      {
          class Caret;
          class DefaultCaret;
          class DefaultCaret$Bypass;
          class Position$Bias;
      }
    }
  }
}

class javax::swing::text::DefaultCaret$Bypass : public ::javax::swing::text::NavigationFilter$FilterBypass
{

public: // actually package-private
  DefaultCaret$Bypass(::javax::swing::text::DefaultCaret *);
public:
  virtual ::javax::swing::text::Caret * getCaret();
  virtual void moveDot(jint, ::javax::swing::text::Position$Bias *);
  virtual void setDot(jint, ::javax::swing::text::Position$Bias *);
public: // actually package-private
  ::javax::swing::text::DefaultCaret * __attribute__((aligned(__alignof__( ::javax::swing::text::NavigationFilter$FilterBypass)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_DefaultCaret$Bypass__
