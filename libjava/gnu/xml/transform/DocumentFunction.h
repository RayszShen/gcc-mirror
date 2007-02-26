
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_DocumentFunction__
#define __gnu_xml_transform_DocumentFunction__

#pragma interface

#include <gnu/xml/xpath/Expr.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace transform
      {
          class DocumentFunction;
          class Stylesheet;
      }
      namespace xpath
      {
          class Expr;
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

class gnu::xml::transform::DocumentFunction : public ::gnu::xml::xpath::Expr
{

public: // actually package-private
  DocumentFunction(::gnu::xml::transform::Stylesheet *, ::org::w3c::dom::Node *);
public:
  ::java::lang::Object * evaluate(::java::util::List *);
  void setArguments(::java::util::List *);
  ::java::lang::Object * evaluate(::org::w3c::dom::Node *, jint, jint);
public: // actually package-private
  ::java::util::Collection * document(::java::lang::String *, ::java::lang::String *);
public:
  ::gnu::xml::xpath::Expr * clone(::java::lang::Object *);
  jboolean references(::javax::xml::namespace::QName *);
public: // actually package-private
  ::gnu::xml::transform::Stylesheet * __attribute__((aligned(__alignof__( ::gnu::xml::xpath::Expr)))) stylesheet;
  ::org::w3c::dom::Node * base;
  ::java::util::List * args;
  ::java::util::List * values;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_DocumentFunction__
