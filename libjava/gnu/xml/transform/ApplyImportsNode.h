
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_ApplyImportsNode__
#define __gnu_xml_transform_ApplyImportsNode__

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
          class ApplyImportsNode;
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

class gnu::xml::transform::ApplyImportsNode : public ::gnu::xml::transform::TemplateNode
{

public: // actually package-private
  ApplyImportsNode();
  ::gnu::xml::transform::TemplateNode * clone(::gnu::xml::transform::Stylesheet *);
  void doApply(::gnu::xml::transform::Stylesheet *, ::javax::xml::namespace$::QName *, ::org::w3c::dom::Node *, jint, jint, ::org::w3c::dom::Node *, ::org::w3c::dom::Node *);
public:
  ::java::lang::String * toString();
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_ApplyImportsNode__
