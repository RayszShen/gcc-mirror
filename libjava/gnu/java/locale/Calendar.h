
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_locale_Calendar__
#define __gnu_java_locale_Calendar__

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
          class Calendar;
      }
    }
  }
}

class gnu::java::locale::Calendar : public ::java::util::ListResourceBundle
{

public:
  Calendar();
  virtual JArray< JArray< ::java::lang::Object * > * > * getContents();
private:
  static JArray< ::java::util::Locale * > * availableLocales;
  static ::java::lang::String * calendarClass;
  static ::java::lang::Integer * firstDayOfWeek;
  static ::java::lang::Integer * minimalDaysInFirstWeek;
  static ::java::util::Date * gregorianCutOver;
  static JArray< JArray< ::java::lang::Object * > * > * contents;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_locale_Calendar__
