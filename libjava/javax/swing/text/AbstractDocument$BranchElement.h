
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_AbstractDocument$BranchElement__
#define __javax_swing_text_AbstractDocument$BranchElement__

#pragma interface

#include <javax/swing/text/AbstractDocument$AbstractElement.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace text
      {
          class AbstractDocument;
          class AbstractDocument$BranchElement;
          class AttributeSet;
          class Element;
      }
    }
  }
}

class javax::swing::text::AbstractDocument$BranchElement : public ::javax::swing::text::AbstractDocument$AbstractElement
{

public:
  AbstractDocument$BranchElement(::javax::swing::text::AbstractDocument *, ::javax::swing::text::Element *, ::javax::swing::text::AttributeSet *);
  virtual ::java::util::Enumeration * children();
  virtual jboolean getAllowsChildren();
  virtual ::javax::swing::text::Element * getElement(jint);
  virtual jint getElementCount();
  virtual jint getElementIndex(jint);
  virtual jint getEndOffset();
  virtual ::java::lang::String * getName();
  virtual jint getStartOffset();
  virtual jboolean isLeaf();
  virtual ::javax::swing::text::Element * positionToElement(jint);
  virtual void replace(jint, jint, JArray< ::javax::swing::text::Element * > *);
  virtual ::java::lang::String * toString();
private:
  static const jlong serialVersionUID = -6037216547466333183LL;
  JArray< ::javax::swing::text::Element * > * __attribute__((aligned(__alignof__( ::javax::swing::text::AbstractDocument$AbstractElement)))) children__;
  jint numChildren;
public: // actually package-private
  ::javax::swing::text::AbstractDocument * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_AbstractDocument$BranchElement__
