
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_xml_validation_Schema__
#define __javax_xml_validation_Schema__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace xml
    {
      namespace validation
      {
          class Schema;
          class Validator;
          class ValidatorHandler;
      }
    }
  }
}

class javax::xml::validation::Schema : public ::java::lang::Object
{

public: // actually protected
  Schema();
public:
  virtual ::javax::xml::validation::Validator * newValidator() = 0;
  virtual ::javax::xml::validation::ValidatorHandler * newValidatorHandler() = 0;
  static ::java::lang::Class class$;
};

#endif // __javax_xml_validation_Schema__
