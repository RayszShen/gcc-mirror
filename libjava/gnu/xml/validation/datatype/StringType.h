
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_validation_datatype_StringType__
#define __gnu_xml_validation_datatype_StringType__

#pragma interface

#include <gnu/xml/validation/datatype/AtomicSimpleType.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace validation
      {
        namespace datatype
        {
            class StringType;
        }
      }
    }
  }
}

class gnu::xml::validation::datatype::StringType : public ::gnu::xml::validation::datatype::AtomicSimpleType
{

public: // actually package-private
  StringType();
public:
  JArray< jint > * getConstrainingFacets();
public: // actually package-private
  static JArray< jint > * CONSTRAINING_FACETS;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_validation_datatype_StringType__
