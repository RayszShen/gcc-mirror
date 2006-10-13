
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_w3c_dom_html2_HTMLAnchorElement__
#define __org_w3c_dom_html2_HTMLAnchorElement__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace org
  {
    namespace w3c
    {
      namespace dom
      {
          class Attr;
          class Document;
          class NamedNodeMap;
          class Node;
          class NodeList;
          class TypeInfo;
          class UserDataHandler;
        namespace html2
        {
            class HTMLAnchorElement;
        }
      }
    }
  }
}

class org::w3c::dom::html2::HTMLAnchorElement : public ::java::lang::Object
{

public:
  virtual ::java::lang::String * getAccessKey() = 0;
  virtual void setAccessKey(::java::lang::String *) = 0;
  virtual ::java::lang::String * getCharset() = 0;
  virtual void setCharset(::java::lang::String *) = 0;
  virtual ::java::lang::String * getCoords() = 0;
  virtual void setCoords(::java::lang::String *) = 0;
  virtual ::java::lang::String * getHref() = 0;
  virtual void setHref(::java::lang::String *) = 0;
  virtual ::java::lang::String * getHreflang() = 0;
  virtual void setHreflang(::java::lang::String *) = 0;
  virtual ::java::lang::String * getName() = 0;
  virtual void setName(::java::lang::String *) = 0;
  virtual ::java::lang::String * getRel() = 0;
  virtual void setRel(::java::lang::String *) = 0;
  virtual ::java::lang::String * getRev() = 0;
  virtual void setRev(::java::lang::String *) = 0;
  virtual ::java::lang::String * getShape() = 0;
  virtual void setShape(::java::lang::String *) = 0;
  virtual jint getTabIndex() = 0;
  virtual void setTabIndex(jint) = 0;
  virtual ::java::lang::String * getTarget() = 0;
  virtual void setTarget(::java::lang::String *) = 0;
  virtual ::java::lang::String * getType() = 0;
  virtual void setType(::java::lang::String *) = 0;
  virtual void blur() = 0;
  virtual void focus() = 0;
  virtual ::java::lang::String * getId() = 0;
  virtual void setId(::java::lang::String *) = 0;
  virtual ::java::lang::String * getTitle() = 0;
  virtual void setTitle(::java::lang::String *) = 0;
  virtual ::java::lang::String * getLang() = 0;
  virtual void setLang(::java::lang::String *) = 0;
  virtual ::java::lang::String * getDir() = 0;
  virtual void setDir(::java::lang::String *) = 0;
  virtual ::java::lang::String * getClassName() = 0;
  virtual void setClassName(::java::lang::String *) = 0;
  virtual ::java::lang::String * getTagName() = 0;
  virtual ::java::lang::String * getAttribute(::java::lang::String *) = 0;
  virtual void setAttribute(::java::lang::String *, ::java::lang::String *) = 0;
  virtual void removeAttribute(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * getAttributeNode(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * setAttributeNode(::org::w3c::dom::Attr *) = 0;
  virtual ::org::w3c::dom::Attr * removeAttributeNode(::org::w3c::dom::Attr *) = 0;
  virtual ::org::w3c::dom::NodeList * getElementsByTagName(::java::lang::String *) = 0;
  virtual ::java::lang::String * getAttributeNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual void setAttributeNS(::java::lang::String *, ::java::lang::String *, ::java::lang::String *) = 0;
  virtual void removeAttributeNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * getAttributeNodeNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * setAttributeNodeNS(::org::w3c::dom::Attr *) = 0;
  virtual ::org::w3c::dom::NodeList * getElementsByTagNameNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual jboolean hasAttribute(::java::lang::String *) = 0;
  virtual jboolean hasAttributeNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::TypeInfo * getSchemaTypeInfo() = 0;
  virtual void setIdAttribute(::java::lang::String *, jboolean) = 0;
  virtual void setIdAttributeNS(::java::lang::String *, ::java::lang::String *, jboolean) = 0;
  virtual void setIdAttributeNode(::org::w3c::dom::Attr *, jboolean) = 0;
  virtual ::java::lang::String * getNodeName() = 0;
  virtual ::java::lang::String * getNodeValue() = 0;
  virtual void setNodeValue(::java::lang::String *) = 0;
  virtual jshort getNodeType() = 0;
  virtual ::org::w3c::dom::Node * getParentNode() = 0;
  virtual ::org::w3c::dom::NodeList * getChildNodes() = 0;
  virtual ::org::w3c::dom::Node * getFirstChild() = 0;
  virtual ::org::w3c::dom::Node * getLastChild() = 0;
  virtual ::org::w3c::dom::Node * getPreviousSibling() = 0;
  virtual ::org::w3c::dom::Node * getNextSibling() = 0;
  virtual ::org::w3c::dom::NamedNodeMap * getAttributes() = 0;
  virtual ::org::w3c::dom::Document * getOwnerDocument() = 0;
  virtual ::org::w3c::dom::Node * insertBefore(::org::w3c::dom::Node *, ::org::w3c::dom::Node *) = 0;
  virtual ::org::w3c::dom::Node * replaceChild(::org::w3c::dom::Node *, ::org::w3c::dom::Node *) = 0;
  virtual ::org::w3c::dom::Node * removeChild(::org::w3c::dom::Node *) = 0;
  virtual ::org::w3c::dom::Node * appendChild(::org::w3c::dom::Node *) = 0;
  virtual jboolean hasChildNodes() = 0;
  virtual ::org::w3c::dom::Node * cloneNode(jboolean) = 0;
  virtual void normalize() = 0;
  virtual jboolean isSupported(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::java::lang::String * getNamespaceURI() = 0;
  virtual ::java::lang::String * getPrefix() = 0;
  virtual void setPrefix(::java::lang::String *) = 0;
  virtual ::java::lang::String * getLocalName() = 0;
  virtual jboolean hasAttributes() = 0;
  virtual ::java::lang::String * getBaseURI() = 0;
  virtual jshort compareDocumentPosition(::org::w3c::dom::Node *) = 0;
  virtual ::java::lang::String * getTextContent() = 0;
  virtual void setTextContent(::java::lang::String *) = 0;
  virtual jboolean isSameNode(::org::w3c::dom::Node *) = 0;
  virtual ::java::lang::String * lookupPrefix(::java::lang::String *) = 0;
  virtual jboolean isDefaultNamespace(::java::lang::String *) = 0;
  virtual ::java::lang::String * lookupNamespaceURI(::java::lang::String *) = 0;
  virtual jboolean isEqualNode(::org::w3c::dom::Node *) = 0;
  virtual ::java::lang::Object * getFeature(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::java::lang::Object * setUserData(::java::lang::String *, ::java::lang::Object *, ::org::w3c::dom::UserDataHandler *) = 0;
  virtual ::java::lang::Object * getUserData(::java::lang::String *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __org_w3c_dom_html2_HTMLAnchorElement__
