
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_xpath_XPathImpl__
#define __gnu_xml_xpath_XPathImpl__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace xpath
      {
          class XPathImpl;
          class XPathParser;
      }
    }
  }
  namespace javax
  {
    namespace xml
    {
      namespace namespace$
      {
          class NamespaceContext;
          class QName;
      }
      namespace xpath
      {
          class XPathExpression;
          class XPathFunctionResolver;
          class XPathVariableResolver;
      }
    }
  }
  namespace org
  {
    namespace xml
    {
      namespace sax
      {
          class InputSource;
      }
    }
  }
}

class gnu::xml::xpath::XPathImpl : public ::java::lang::Object
{

public: // actually package-private
  XPathImpl(::javax::xml::namespace$::NamespaceContext *, ::javax::xml::xpath::XPathVariableResolver *, ::javax::xml::xpath::XPathFunctionResolver *);
public:
  virtual void reset();
  virtual void setXPathVariableResolver(::javax::xml::xpath::XPathVariableResolver *);
  virtual ::javax::xml::xpath::XPathVariableResolver * getXPathVariableResolver();
  virtual void setXPathFunctionResolver(::javax::xml::xpath::XPathFunctionResolver *);
  virtual ::javax::xml::xpath::XPathFunctionResolver * getXPathFunctionResolver();
  virtual void setNamespaceContext(::javax::xml::namespace$::NamespaceContext *);
  virtual ::javax::xml::namespace$::NamespaceContext * getNamespaceContext();
  virtual ::javax::xml::xpath::XPathExpression * compile(::java::lang::String *);
  virtual ::java::lang::Object * evaluate(::java::lang::String *, ::java::lang::Object *, ::javax::xml::namespace$::QName *);
  virtual ::java::lang::String * evaluate(::java::lang::String *, ::java::lang::Object *);
  virtual ::java::lang::Object * evaluate(::java::lang::String *, ::org::xml::sax::InputSource *, ::javax::xml::namespace$::QName *);
  virtual ::java::lang::String * evaluate(::java::lang::String *, ::org::xml::sax::InputSource *);
public: // actually package-private
  ::gnu::xml::xpath::XPathParser * __attribute__((aligned(__alignof__( ::java::lang::Object)))) parser;
  ::javax::xml::namespace$::NamespaceContext * namespaceContext;
  ::javax::xml::xpath::XPathVariableResolver * variableResolver;
  ::javax::xml::xpath::XPathFunctionResolver * functionResolver;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_xpath_XPathImpl__
