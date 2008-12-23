
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_util_regex_RETokenAny__
#define __gnu_java_util_regex_RETokenAny__

#pragma interface

#include <gnu/java/util/regex/REToken.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace lang
      {
          class CPStringBuilder;
      }
      namespace util
      {
        namespace regex
        {
            class CharIndexed;
            class REMatch;
            class RETokenAny;
        }
      }
    }
  }
}

class gnu::java::util::regex::RETokenAny : public ::gnu::java::util::regex::REToken
{

public: // actually package-private
  RETokenAny(jint, jboolean, jboolean);
  jint getMinimumLength();
  jint getMaximumLength();
  ::gnu::java::util::regex::REMatch * matchThis(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *);
  jboolean matchOneChar(jchar);
  jboolean returnsFixedLengthMatches();
  jint findFixedLengthMatches(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *, jint);
  void dump(::gnu::java::lang::CPStringBuilder *);
private:
  jboolean __attribute__((aligned(__alignof__( ::gnu::java::util::regex::REToken)))) newline;
  jboolean matchNull;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_util_regex_RETokenAny__
