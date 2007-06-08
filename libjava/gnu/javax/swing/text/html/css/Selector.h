
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_swing_text_html_css_Selector__
#define __gnu_javax_swing_text_html_css_Selector__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace swing
      {
        namespace text
        {
          namespace html
          {
            namespace css
            {
                class Selector;
            }
          }
        }
      }
    }
  }
}

class gnu::javax::swing::text::html::css::Selector : public ::java::lang::Object
{

public:
  Selector(::java::lang::String *);
  virtual jboolean matches(JArray< ::java::lang::String * > *, JArray< ::java::util::Map * > *);
  virtual jint getSpecificity();
  virtual ::java::lang::String * toString();
private:
  void calculateSpecificity();
  JArray< ::java::lang::String * > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) selector;
  JArray< ::java::lang::String * > * elements;
  JArray< ::java::lang::String * > * ids;
  JArray< ::java::lang::String * > * classes;
  jint specificity;
  jboolean implicit;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_swing_text_html_css_Selector__
