
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_xml_namespace_QName__
#define __javax_xml_namespace_QName__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
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
}

class javax::xml::namespace$::QName : public ::java::lang::Object
{

public:
  QName(::java::lang::String *, ::java::lang::String *);
  QName(::java::lang::String *, ::java::lang::String *, ::java::lang::String *);
  QName(::java::lang::String *);
  virtual ::java::lang::String * getNamespaceURI();
  virtual ::java::lang::String * getLocalPart();
  virtual ::java::lang::String * getPrefix();
  virtual jboolean equals(::java::lang::Object *);
  virtual jint hashCode();
  virtual ::java::lang::String * toString();
  static ::javax::xml::namespace$::QName * valueOf(::java::lang::String *);
private:
  static const jlong serialVersionUID = 4418622981026545151LL;
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::lang::Object)))) namespaceURI;
  ::java::lang::String * localPart;
  ::java::lang::String * prefix;
  ::java::lang::String * qName;
public: // actually package-private
  jint hashCode__;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_xml_namespace_QName__
