
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_AbstractNumberNode__
#define __gnu_xml_transform_AbstractNumberNode__

#pragma interface

#include <gnu/xml/transform/TemplateNode.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace transform
      {
          class AbstractNumberNode;
          class Stylesheet;
          class TemplateNode;
      }
    }
  }
  namespace javax
  {
    namespace xml
    {
      namespace namespace
      {
          class QName;
      }
    }
  }
  namespace org
  {
    namespace w3c
    {
      namespace dom
      {
          class Node;
      }
    }
  }
}

class gnu::xml::transform::AbstractNumberNode : public ::gnu::xml::transform::TemplateNode
{

public: // actually package-private
  AbstractNumberNode(::gnu::xml::transform::TemplateNode *, ::java::lang::String *, jint, ::java::lang::String *, jint);
  virtual void doApply(::gnu::xml::transform::Stylesheet *, ::javax::xml::namespace::QName *, ::org::w3c::dom::Node *, jint, jint, ::org::w3c::dom::Node *, ::org::w3c::dom::Node *);
  virtual ::java::lang::String * format(::java::lang::String *, JArray< jint > *);
  virtual void format(::java::lang::StringBuffer *, jint, ::java::lang::String *);
  static jboolean isAlphanumeric(jchar);
  static ::java::lang::String * alphabetic(jchar, jint);
  static ::java::lang::String * roman(jboolean, jint);
  virtual JArray< jint > * compute(::gnu::xml::transform::Stylesheet *, ::org::w3c::dom::Node *, jint, jint) = 0;
public:
  virtual jboolean references(::javax::xml::namespace::QName *);
  virtual ::java::lang::String * toString();
public: // actually package-private
  static const jint ALPHABETIC = 0;
  static const jint TRADITIONAL = 1;
  ::gnu::xml::transform::TemplateNode * __attribute__((aligned(__alignof__( ::gnu::xml::transform::TemplateNode)))) format__;
  ::java::lang::String * lang;
  jint letterValue;
  ::java::lang::String * groupingSeparator;
  jint groupingSize;
  static JArray< jint > * roman_numbers;
  static JArray< jchar > * roman_chars;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_AbstractNumberNode__
