
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_DefaultCellEditor$JComboBoxDelegate__
#define __javax_swing_DefaultCellEditor$JComboBoxDelegate__

#pragma interface

#include <javax/swing/DefaultCellEditor$EditorDelegate.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class DefaultCellEditor;
        class DefaultCellEditor$JComboBoxDelegate;
    }
  }
}

class javax::swing::DefaultCellEditor$JComboBoxDelegate : public ::javax::swing::DefaultCellEditor$EditorDelegate
{

  DefaultCellEditor$JComboBoxDelegate(::javax::swing::DefaultCellEditor *);
public:
  virtual void setValue(::java::lang::Object *);
  virtual ::java::lang::Object * getCellEditorValue();
  virtual jboolean shouldSelectCell(::java::util::EventObject *);
public: // actually package-private
  DefaultCellEditor$JComboBoxDelegate(::javax::swing::DefaultCellEditor *, ::javax::swing::DefaultCellEditor$JComboBoxDelegate *);
private:
  static const jlong serialVersionUID = 1LL;
public: // actually package-private
  ::javax::swing::DefaultCellEditor * __attribute__((aligned(__alignof__( ::javax::swing::DefaultCellEditor$EditorDelegate)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_DefaultCellEditor$JComboBoxDelegate__
