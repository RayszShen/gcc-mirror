
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_text_CharacterIterator__
#define __java_text_CharacterIterator__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace text
    {
        class CharacterIterator;
    }
  }
}

class java::text::CharacterIterator : public ::java::lang::Object
{

public:
  virtual jchar current() = 0;
  virtual jchar next() = 0;
  virtual jchar previous() = 0;
  virtual jchar first() = 0;
  virtual jchar last() = 0;
  virtual jint getIndex() = 0;
  virtual jchar setIndex(jint) = 0;
  virtual jint getBeginIndex() = 0;
  virtual jint getEndIndex() = 0;
  virtual ::java::lang::Object * clone() = 0;
  static const jchar DONE = 65535;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __java_text_CharacterIterator__
