
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_locale_LocaleInformation_zh_SG__
#define __gnu_java_locale_LocaleInformation_zh_SG__

#pragma interface

#include <java/util/ListResourceBundle.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace locale
      {
          class LocaleInformation_zh_SG;
      }
    }
  }
}

class gnu::java::locale::LocaleInformation_zh_SG : public ::java::util::ListResourceBundle
{

public:
  LocaleInformation_zh_SG();
  virtual JArray< JArray< ::java::lang::Object * > * > * getContents();
public: // actually package-private
  static ::java::lang::String * decimalSeparator;
  static ::java::lang::String * groupingSeparator;
  static ::java::lang::String * numberFormat;
  static ::java::lang::String * percentFormat;
  static JArray< ::java::lang::String * > * weekdays;
  static JArray< ::java::lang::String * > * shortWeekdays;
  static JArray< ::java::lang::String * > * shortMonths;
  static JArray< ::java::lang::String * > * months;
  static JArray< ::java::lang::String * > * ampms;
  static ::java::lang::String * shortDateFormat;
  static ::java::lang::String * defaultTimeFormat;
  static ::java::lang::String * currencySymbol;
  static ::java::lang::String * intlCurrencySymbol;
  static ::java::lang::String * currencyFormat;
private:
  static JArray< JArray< ::java::lang::Object * > * > * contents;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_locale_LocaleInformation_zh_SG__
