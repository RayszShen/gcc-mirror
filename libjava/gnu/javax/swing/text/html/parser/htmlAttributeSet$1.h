
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_swing_text_html_parser_htmlAttributeSet$1__
#define __gnu_javax_swing_text_html_parser_htmlAttributeSet$1__

#pragma interface

#include <java/lang/Object.h>
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
            namespace parser
            {
                class htmlAttributeSet;
                class htmlAttributeSet$1;
            }
          }
        }
      }
    }
  }
}

class gnu::javax::swing::text::html::parser::htmlAttributeSet$1 : public ::java::lang::Object
{

public: // actually package-private
  htmlAttributeSet$1(::gnu::javax::swing::text::html::parser::htmlAttributeSet *, ::java::util::Enumeration *);
public:
  virtual jboolean hasMoreElements();
  virtual ::java::lang::Object * nextElement();
public: // actually package-private
  ::gnu::javax::swing::text::html::parser::htmlAttributeSet * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
private:
  ::java::util::Enumeration * val$enumeration;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_swing_text_html_parser_htmlAttributeSet$1__
