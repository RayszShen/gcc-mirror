
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_print_ipp_attribute_supported_UriAuthenticationSupported__
#define __gnu_javax_print_ipp_attribute_supported_UriAuthenticationSupported__

#pragma interface

#include <javax/print/attribute/EnumSyntax.h>
#include <gcj/array.h>

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
                class UriAuthenticationSupported;
            }
          }
        }
      }
    }
  }
  namespace javax
  {
    namespace print
    {
      namespace attribute
      {
          class EnumSyntax;
      }
    }
  }
}

class gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported : public ::javax::print::attribute::EnumSyntax
{

public:
  UriAuthenticationSupported(jint);
  ::java::lang::Class * getCategory();
  ::java::lang::String * getName();
public: // actually protected
  JArray< ::java::lang::String * > * getStringTable();
  JArray< ::javax::print::attribute::EnumSyntax * > * getEnumValueTable();
public:
  static ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * NONE;
  static ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * REQUESTING_USER_NAME;
  static ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * BASIC;
  static ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * DIGEST;
  static ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * CERTIFICATE;
private:
  static JArray< ::java::lang::String * > * stringTable;
  static JArray< ::gnu::javax::print::ipp::attribute::supported::UriAuthenticationSupported * > * enumValueTable;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_print_ipp_attribute_supported_UriAuthenticationSupported__
