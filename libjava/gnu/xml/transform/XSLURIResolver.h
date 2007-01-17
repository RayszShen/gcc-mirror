
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_transform_XSLURIResolver__
#define __gnu_xml_transform_XSLURIResolver__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace transform
      {
          class XSLURIResolver;
      }
    }
  }
  namespace java
  {
    namespace net
    {
        class URL;
    }
  }
  namespace javax
  {
    namespace xml
    {
      namespace parsers
      {
          class DocumentBuilder;
      }
      namespace transform
      {
          class ErrorListener;
          class Source;
          class URIResolver;
        namespace dom
        {
            class DOMSource;
        }
      }
    }
  }
}

class gnu::xml::transform::XSLURIResolver : public ::java::lang::Object
{

public: // actually package-private
  XSLURIResolver();
  virtual void setUserResolver(::javax::xml::transform::URIResolver *);
  virtual void setUserListener(::javax::xml::transform::ErrorListener *);
  virtual void flush();
public:
  virtual ::javax::xml::transform::Source * resolve(::java::lang::String *, ::java::lang::String *);
public: // actually package-private
  virtual ::javax::xml::transform::dom::DOMSource * resolveDOM(::javax::xml::transform::Source *, ::java::lang::String *, ::java::lang::String *);
  virtual ::java::net::URL * resolveURL(::java::lang::String *, ::java::lang::String *, ::java::lang::String *);
  virtual ::javax::xml::parsers::DocumentBuilder * getDocumentBuilder();
  ::java::util::Map * __attribute__((aligned(__alignof__( ::java::lang::Object)))) lastModifiedCache;
  ::java::util::Map * nodeCache;
  ::javax::xml::parsers::DocumentBuilder * builder;
  ::javax::xml::transform::URIResolver * userResolver;
  ::javax::xml::transform::ErrorListener * userListener;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_transform_XSLURIResolver__
