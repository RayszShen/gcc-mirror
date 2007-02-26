
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_dom_DomEvent$DomMutationEvent__
#define __gnu_xml_dom_DomEvent$DomMutationEvent__

#pragma interface

#include <gnu/xml/dom/DomEvent.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace dom
      {
          class DomEvent$DomMutationEvent;
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

class gnu::xml::dom::DomEvent$DomMutationEvent : public ::gnu::xml::dom::DomEvent
{

public:
  ::org::w3c::dom::Node * getRelatedNode();
  ::java::lang::String * getPrevValue();
  ::java::lang::String * getNewValue();
  ::java::lang::String * getAttrName();
  jshort getAttrChange();
  void initMutationEvent(::java::lang::String *, jboolean, jboolean, ::org::w3c::dom::Node *, ::java::lang::String *, ::java::lang::String *, ::java::lang::String *, jshort);
public: // actually package-private
  void clear();
public:
  DomEvent$DomMutationEvent(::java::lang::String *);
public: // actually package-private
  ::org::w3c::dom::Node * __attribute__((aligned(__alignof__( ::gnu::xml::dom::DomEvent)))) relatedNode;
private:
  ::java::lang::String * prevValue;
  ::java::lang::String * newValue;
  ::java::lang::String * attrName;
  jshort attrChange;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_dom_DomEvent$DomMutationEvent__
