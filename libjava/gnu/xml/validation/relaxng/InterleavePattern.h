
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_validation_relaxng_InterleavePattern__
#define __gnu_xml_validation_relaxng_InterleavePattern__

#pragma interface

#include <gnu/xml/validation/relaxng/Pattern.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace validation
      {
        namespace relaxng
        {
            class InterleavePattern;
            class Pattern;
        }
      }
    }
  }
}

class gnu::xml::validation::relaxng::InterleavePattern : public ::gnu::xml::validation::relaxng::Pattern
{

public: // actually package-private
  InterleavePattern();
  ::gnu::xml::validation::relaxng::Pattern * __attribute__((aligned(__alignof__( ::gnu::xml::validation::relaxng::Pattern)))) pattern1;
  ::gnu::xml::validation::relaxng::Pattern * pattern2;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_validation_relaxng_InterleavePattern__
