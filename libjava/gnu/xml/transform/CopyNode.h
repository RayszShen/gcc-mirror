
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_CopyNode__
#define __gnu_xml_transform_CopyNode__

#pragma interface

#include <gnu/xml/transform/TemplateNode.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace transform
      {
          class CopyNode;
          class Stylesheet;
          class TemplateNode;
      }
    }
  }
  namespace javax
  {
    namespace xml
    {
      namespace namespace$
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

class gnu::xml::transform::CopyNode : public ::gnu::xml::transform::TemplateNode
{

public: // actually package-private
  CopyNode(::java::lang::String *);
  ::gnu::xml::transform::TemplateNode * clone(::gnu::xml::transform::Stylesheet *);
  void doApply(::gnu::xml::transform::Stylesheet *, ::javax::xml::namespace$::QName *, ::org::w3c::dom::Node *, jint, jint, ::org::w3c::dom::Node *, ::org::w3c::dom::Node *);
  void addAttributeSet(::gnu::xml::transform::Stylesheet *, ::javax::xml::namespace$::QName *, ::org::w3c::dom::Node *, jint, jint, ::org::w3c::dom::Node *, ::org::w3c::dom::Node *, ::java::lang::String *);
public:
  ::java::lang::String * toString();
public: // actually package-private
  ::java::lang::String * __attribute__((aligned(__alignof__( ::gnu::xml::transform::TemplateNode)))) uas;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_CopyNode__
