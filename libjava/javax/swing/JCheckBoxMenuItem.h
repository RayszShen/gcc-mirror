
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JCheckBoxMenuItem__
#define __javax_swing_JCheckBoxMenuItem__

#pragma interface

#include <javax/swing/JMenuItem.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace accessibility
    {
        class AccessibleContext;
    }
    namespace swing
    {
        class Action;
        class Icon;
        class JCheckBoxMenuItem;
    }
  }
}

class javax::swing::JCheckBoxMenuItem : public ::javax::swing::JMenuItem
{

public:
  JCheckBoxMenuItem();
  JCheckBoxMenuItem(::javax::swing::Icon *);
  JCheckBoxMenuItem(::java::lang::String *);
  JCheckBoxMenuItem(::javax::swing::Action *);
  JCheckBoxMenuItem(::java::lang::String *, ::javax::swing::Icon *);
  JCheckBoxMenuItem(::java::lang::String *, jboolean);
  JCheckBoxMenuItem(::java::lang::String *, ::javax::swing::Icon *, jboolean);
  virtual ::java::lang::String * getUIClassID();
  virtual jboolean getState();
  virtual void setState(jboolean);
  virtual JArray< ::java::lang::Object * > * getSelectedObjects();
  virtual void requestFocus();
public: // actually protected
  virtual ::java::lang::String * paramString();
public:
  virtual ::javax::accessibility::AccessibleContext * getAccessibleContext();
private:
  static const jlong serialVersionUID = -6676402307973384715LL;
  static ::java::lang::String * uiClassID;
  jboolean __attribute__((aligned(__alignof__( ::javax::swing::JMenuItem)))) state;
  JArray< ::java::lang::Object * > * selectedObjects;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JCheckBoxMenuItem__
