
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_UnparsedEntityUriFunction__
#define __gnu_xml_transform_UnparsedEntityUriFunction__

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
          class UnparsedEntityUriFunction;
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

class gnu::xml::transform::UnparsedEntityUriFunction : public ::gnu::xml::xpath::Expr
{

public: // actually package-private
  UnparsedEntityUriFunction();
public:
  ::java::lang::Object * evaluate(::java::util::List *);
  void setArguments(::java::util::List *);
  ::java::lang::Object * evaluate(::org::w3c::dom::Node *, jint, jint);
  ::gnu::xml::xpath::Expr * clone(::java::lang::Object *);
  jboolean references(::javax::xml::namespace$::QName *);
public: // actually package-private
  ::java::util::List * __attribute__((aligned(__alignof__( ::gnu::xml::xpath::Expr)))) args;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_UnparsedEntityUriFunction__
