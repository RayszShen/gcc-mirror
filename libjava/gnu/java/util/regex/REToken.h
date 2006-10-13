
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_util_regex_REToken__
#define __gnu_java_util_regex_REToken__

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
            class REToken;
        }
      }
    }
  }
}

class gnu::java::util::regex::REToken : public ::java::lang::Object
{

public:
  virtual ::java::lang::Object * clone();
public: // actually protected
  REToken(jint);
public: // actually package-private
  virtual jint getMinimumLength();
  virtual jint getMaximumLength();
  virtual void setUncle(::gnu::java::util::regex::REToken *);
  virtual jboolean match(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *);
  virtual ::gnu::java::util::regex::REMatch * matchThis(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *);
public: // actually protected
  virtual jboolean next(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *);
public: // actually package-private
  virtual ::gnu::java::util::regex::REToken * getNext();
  virtual ::gnu::java::util::regex::REMatch * findMatch(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *);
  virtual jboolean returnsFixedLengthMatches();
  virtual jint findFixedLengthMatches(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *, jint);
  virtual ::gnu::java::util::regex::REMatch * backtrack(::gnu::java::util::regex::CharIndexed *, ::gnu::java::util::regex::REMatch *, ::java::lang::Object *);
  virtual jboolean chain(::gnu::java::util::regex::REToken *);
  virtual void dump(::java::lang::StringBuffer *) = 0;
  virtual void dumpAll(::java::lang::StringBuffer *);
public:
  virtual ::java::lang::String * toString();
  static jchar toLowerCase(jchar, jboolean);
  static jchar toUpperCase(jchar, jboolean);
public: // actually protected
  ::gnu::java::util::regex::REToken * __attribute__((aligned(__alignof__( ::java::lang::Object)))) next__;
  ::gnu::java::util::regex::REToken * uncle;
  jint subIndex;
  jboolean unicodeAware;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_util_regex_REToken__
