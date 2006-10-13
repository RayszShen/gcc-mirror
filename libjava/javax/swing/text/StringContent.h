
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_StringContent__
#define __javax_swing_text_StringContent__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace text
      {
          class Position;
          class Segment;
          class StringContent;
      }
      namespace undo
      {
          class UndoableEdit;
      }
    }
  }
}

class javax::swing::text::StringContent : public ::java::lang::Object
{

public:
  StringContent();
  StringContent(jint);
public: // actually protected
  ::java::util::Vector * getPositionsInRange(::java::util::Vector *, jint, jint);
public:
  ::javax::swing::text::Position * createPosition(jint);
  jint length();
  ::javax::swing::undo::UndoableEdit * insertString(jint, ::java::lang::String *);
  ::javax::swing::undo::UndoableEdit * remove(jint, jint);
  ::java::lang::String * getString(jint, jint);
  void getChars(jint, jint, ::javax::swing::text::Segment *);
public: // actually protected
  void updateUndoPositions(::java::util::Vector *);
public: // actually package-private
  void checkLocation(jint, jint);
private:
  static const jlong serialVersionUID = 4755994433709540381LL;
public: // actually package-private
  JArray< jchar > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) content;
private:
  jint count;
  ::java::util::Vector * positions;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_StringContent__
