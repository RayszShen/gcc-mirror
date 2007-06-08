
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_lang_Short__
#define __java_lang_Short__

#pragma interface

#include <java/lang/Number.h>
#include <gcj/array.h>


class java::lang::Short : public ::java::lang::Number
{

public:
  Short(jshort);
  Short(::java::lang::String *);
  static ::java::lang::String * toString(jshort);
  static jshort parseShort(::java::lang::String *);
  static jshort parseShort(::java::lang::String *, jint);
  static ::java::lang::Short * valueOf(::java::lang::String *, jint);
  static ::java::lang::Short * valueOf(::java::lang::String *);
  static ::java::lang::Short * valueOf(jshort);
  static ::java::lang::Short * decode(::java::lang::String *);
  jbyte byteValue();
  jshort shortValue();
  jint intValue();
  jlong longValue();
  jfloat floatValue();
  jdouble doubleValue();
  ::java::lang::String * toString();
  jint hashCode();
  jboolean equals(::java::lang::Object *);
  jint Short$compareTo(::java::lang::Short *);
  static jshort reverseBytes(jshort);
  jint compareTo(::java::lang::Object *);
private:
  static const jlong serialVersionUID = 7515723908773894738LL;
public:
  static const jshort MIN_VALUE = -32768;
  static const jshort MAX_VALUE = 32767;
  static ::java::lang::Class * TYPE;
  static const jint SIZE = 16;
private:
  static const jint MIN_CACHE = -128;
  static const jint MAX_CACHE = 127;
  static JArray< ::java::lang::Short * > * shortCache;
  jshort __attribute__((aligned(__alignof__( ::java::lang::Number)))) value;
public:
  static ::java::lang::Class class$;
};

#endif // __java_lang_Short__
