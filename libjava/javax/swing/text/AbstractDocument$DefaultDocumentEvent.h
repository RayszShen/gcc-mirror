
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_AbstractDocument$DefaultDocumentEvent__
#define __javax_swing_text_AbstractDocument$DefaultDocumentEvent__

#pragma interface

#include <javax/swing/undo/CompoundEdit.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace event
      {
          class DocumentEvent$ElementChange;
          class DocumentEvent$EventType;
      }
      namespace text
      {
          class AbstractDocument;
          class AbstractDocument$DefaultDocumentEvent;
          class Document;
          class Element;
      }
      namespace undo
      {
          class UndoableEdit;
      }
    }
  }
}

class javax::swing::text::AbstractDocument$DefaultDocumentEvent : public ::javax::swing::undo::CompoundEdit
{

public:
  AbstractDocument$DefaultDocumentEvent(::javax::swing::text::AbstractDocument *, jint, jint, ::javax::swing::event::DocumentEvent$EventType *);
  virtual jboolean addEdit(::javax::swing::undo::UndoableEdit *);
  virtual ::javax::swing::text::Document * getDocument();
  virtual jint getLength();
  virtual jint getOffset();
  virtual ::javax::swing::event::DocumentEvent$EventType * getType();
  virtual ::javax::swing::event::DocumentEvent$ElementChange * getChange(::javax::swing::text::Element *);
  virtual ::java::lang::String * toString();
private:
  static const jlong serialVersionUID = 5230037221564563284LL;
  static const jint THRESHOLD = 10;
  jint __attribute__((aligned(__alignof__( ::javax::swing::undo::CompoundEdit)))) offset;
  jint length;
  ::javax::swing::event::DocumentEvent$EventType * type;
  ::java::util::HashMap * changes;
  jboolean modified;
public: // actually package-private
  ::javax::swing::text::AbstractDocument * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_AbstractDocument$DefaultDocumentEvent__
