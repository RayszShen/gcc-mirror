
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_w3c_dom_html2_HTMLDocument__
#define __org_w3c_dom_html2_HTMLDocument__

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
          class CDATASection;
          class Comment;
          class DOMConfiguration;
          class DOMImplementation;
          class Document;
          class DocumentFragment;
          class DocumentType;
          class Element;
          class EntityReference;
          class NamedNodeMap;
          class Node;
          class NodeList;
          class ProcessingInstruction;
          class Text;
          class UserDataHandler;
        namespace html2
        {
            class HTMLCollection;
            class HTMLDocument;
            class HTMLElement;
        }
      }
    }
  }
}

class org::w3c::dom::html2::HTMLDocument : public ::java::lang::Object
{

public:
  virtual ::java::lang::String * getTitle() = 0;
  virtual void setTitle(::java::lang::String *) = 0;
  virtual ::java::lang::String * getReferrer() = 0;
  virtual ::java::lang::String * getDomain() = 0;
  virtual ::java::lang::String * getURL() = 0;
  virtual ::org::w3c::dom::html2::HTMLElement * getBody() = 0;
  virtual void setBody(::org::w3c::dom::html2::HTMLElement *) = 0;
  virtual ::org::w3c::dom::html2::HTMLCollection * getImages() = 0;
  virtual ::org::w3c::dom::html2::HTMLCollection * getApplets() = 0;
  virtual ::org::w3c::dom::html2::HTMLCollection * getLinks() = 0;
  virtual ::org::w3c::dom::html2::HTMLCollection * getForms() = 0;
  virtual ::org::w3c::dom::html2::HTMLCollection * getAnchors() = 0;
  virtual ::java::lang::String * getCookie() = 0;
  virtual void setCookie(::java::lang::String *) = 0;
  virtual void open() = 0;
  virtual void close() = 0;
  virtual void write(::java::lang::String *) = 0;
  virtual void writeln(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::NodeList * getElementsByName(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::DocumentType * getDoctype() = 0;
  virtual ::org::w3c::dom::DOMImplementation * getImplementation() = 0;
  virtual ::org::w3c::dom::Element * getDocumentElement() = 0;
  virtual ::org::w3c::dom::Element * createElement(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::DocumentFragment * createDocumentFragment() = 0;
  virtual ::org::w3c::dom::Text * createTextNode(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Comment * createComment(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::CDATASection * createCDATASection(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::ProcessingInstruction * createProcessingInstruction(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * createAttribute(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::EntityReference * createEntityReference(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::NodeList * getElementsByTagName(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Node * importNode(::org::w3c::dom::Node *, jboolean) = 0;
  virtual ::org::w3c::dom::Element * createElementNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Attr * createAttributeNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::NodeList * getElementsByTagNameNS(::java::lang::String *, ::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Element * getElementById(::java::lang::String *) = 0;
  virtual ::java::lang::String * getInputEncoding() = 0;
  virtual ::java::lang::String * getXmlEncoding() = 0;
  virtual jboolean getXmlStandalone() = 0;
  virtual void setXmlStandalone(jboolean) = 0;
  virtual ::java::lang::String * getXmlVersion() = 0;
  virtual void setXmlVersion(::java::lang::String *) = 0;
  virtual jboolean getStrictErrorChecking() = 0;
  virtual void setStrictErrorChecking(jboolean) = 0;
  virtual ::java::lang::String * getDocumentURI() = 0;
  virtual void setDocumentURI(::java::lang::String *) = 0;
  virtual ::org::w3c::dom::Node * adoptNode(::org::w3c::dom::Node *) = 0;
  virtual ::org::w3c::dom::DOMConfiguration * getDomConfig() = 0;
  virtual void normalizeDocument() = 0;
  virtual ::org::w3c::dom::Node * renameNode(::org::w3c::dom::Node *, ::java::lang::String *, ::java::lang::String *) = 0;
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

#endif // __org_w3c_dom_html2_HTMLDocument__
