
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_print_ipp_attribute_supported_PrinterUriSupported__
#define __gnu_javax_print_ipp_attribute_supported_PrinterUriSupported__

#pragma interface

#include <javax/print/attribute/URISyntax.h>
extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace print
      {
        namespace ipp
        {
          namespace attribute
          {
            namespace supported
            {
                class PrinterUriSupported;
            }
          }
        }
      }
    }
  }
  namespace java
  {
    namespace net
    {
        class URI;
    }
  }
}

class gnu::javax::print::ipp::attribute::supported::PrinterUriSupported : public ::javax::print::attribute::URISyntax
{

public:
  PrinterUriSupported(::java::net::URI *);
  ::java::lang::Class * getCategory();
  ::java::lang::String * getName();
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_print_ipp_attribute_supported_PrinterUriSupported__
