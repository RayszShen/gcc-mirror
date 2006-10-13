
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_ScrollPaneAdjustable__
#define __java_awt_ScrollPaneAdjustable__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class ScrollPane;
        class ScrollPaneAdjustable;
      namespace event
      {
          class AdjustmentListener;
      }
    }
  }
}

class java::awt::ScrollPaneAdjustable : public ::java::lang::Object
{

public: // actually package-private
  ScrollPaneAdjustable(::java::awt::ScrollPane *, jint);
  ScrollPaneAdjustable(::java::awt::ScrollPane *, jint, jint, jint, jint, jint, jint, jint);
public:
  virtual void addAdjustmentListener(::java::awt::event::AdjustmentListener *);
  virtual void removeAdjustmentListener(::java::awt::event::AdjustmentListener *);
  virtual JArray< ::java::awt::event::AdjustmentListener * > * getAdjustmentListeners();
  virtual jint getBlockIncrement();
  virtual jint getMaximum();
  virtual jint getMinimum();
  virtual jint getOrientation();
  virtual jint getUnitIncrement();
  virtual jint getValue();
  virtual jint getVisibleAmount();
  virtual void setBlockIncrement(jint);
  virtual void setMaximum(jint);
  virtual void setMinimum(jint);
  virtual void setUnitIncrement(jint);
  virtual void setValue(jint);
  virtual void setVisibleAmount(jint);
  virtual ::java::lang::String * paramString();
  virtual ::java::lang::String * toString();
  virtual jboolean getValueIsAdjusting();
  virtual void setValueIsAdjusting(jboolean);
private:
  static const jlong serialVersionUID = -3359745691033257079LL;
public: // actually package-private
  ::java::awt::ScrollPane * __attribute__((aligned(__alignof__( ::java::lang::Object)))) sp;
  jint orientation;
  jint value;
  jint minimum;
  jint maximum;
  jint visibleAmount;
  jint unitIncrement;
  jint blockIncrement;
  ::java::awt::event::AdjustmentListener * adjustmentListener;
private:
  jboolean valueIsAdjusting;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_ScrollPaneAdjustable__
