
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_validation_datatype_ShortType__
#define __gnu_xml_validation_datatype_ShortType__

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
            class ShortType;
        }
      }
    }
  }
  namespace org
  {
    namespace relaxng
    {
      namespace datatype
      {
          class ValidationContext;
      }
    }
  }
}

class gnu::xml::validation::datatype::ShortType : public ::gnu::xml::validation::datatype::AtomicSimpleType
{

public: // actually package-private
  ShortType();
public:
  JArray< jint > * getConstrainingFacets();
  void checkValid(::java::lang::String *, ::org::relaxng::datatype::ValidationContext *);
  ::java::lang::Object * createValue(::java::lang::String *, ::org::relaxng::datatype::ValidationContext *);
public: // actually package-private
  static JArray< jint > * CONSTRAINING_FACETS;
  static ::java::lang::String * MAX_VALUE;
  static ::java::lang::String * MIN_VALUE;
  static jint LENGTH;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_validation_datatype_ShortType__
