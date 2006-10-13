
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_util_regex_CharIndexed__
#define __gnu_java_util_regex_CharIndexed__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace util
      {
        namespace regex
        {
            class CharIndexed;
            class REMatch;
        }
      }
    }
  }
}

class gnu::java::util::regex::CharIndexed : public ::java::lang::Object
{

public:
  virtual jchar charAt(jint) = 0;
  virtual jboolean move(jint) = 0;
  virtual jboolean isValid() = 0;
  virtual ::gnu::java::util::regex::CharIndexed * lookBehind(jint, jint) = 0;
  virtual jint length() = 0;
  virtual void setLastMatch(::gnu::java::util::regex::REMatch *) = 0;
  virtual ::gnu::java::util::regex::REMatch * getLastMatch() = 0;
  virtual jint getAnchor() = 0;
  virtual void setAnchor(jint) = 0;
  static const jchar OUT_OF_BOUNDS = 65535;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __gnu_java_util_regex_CharIndexed__
