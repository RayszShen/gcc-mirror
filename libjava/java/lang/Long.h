
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_lang_Long__
#define __java_lang_Long__

#pragma interface

#include <java/lang/Number.h>
#include <gcj/array.h>


class java::lang::Long : public ::java::lang::Number
{

public:
  Long(jlong);
  Long(::java::lang::String *);
private:
  static jint stringSize(jlong, jint);
public:
  static ::java::lang::String * toString(jlong, jint);
  static ::java::lang::String * toHexString(jlong);
  static ::java::lang::String * toOctalString(jlong);
  static ::java::lang::String * toBinaryString(jlong);
  static ::java::lang::String * toString(jlong);
  static jlong parseLong(::java::lang::String *, jint);
  static jlong parseLong(::java::lang::String *);
  static ::java::lang::Long * valueOf(::java::lang::String *, jint);
  static ::java::lang::Long * valueOf(::java::lang::String *);
  static ::java::lang::Long * valueOf(jlong);
  static ::java::lang::Long * decode(::java::lang::String *);
  jbyte byteValue();
  jshort shortValue();
  jint intValue();
  jlong longValue();
  jfloat floatValue();
  jdouble doubleValue();
  ::java::lang::String * toString();
  jint hashCode();
  jboolean equals(::java::lang::Object *);
  static ::java::lang::Long * getLong(::java::lang::String *);
  static ::java::lang::Long * getLong(::java::lang::String *, jlong);
  static ::java::lang::Long * getLong(::java::lang::String *, ::java::lang::Long *);
  jint Long$compareTo(::java::lang::Long *);
  static jint bitCount(jlong);
  static jlong rotateLeft(jlong, jint);
  static jlong rotateRight(jlong, jint);
  static jlong highestOneBit(jlong);
  static jint numberOfLeadingZeros(jlong);
  static jlong lowestOneBit(jlong);
  static jint numberOfTrailingZeros(jlong);
  static jint signum(jlong);
  static jlong reverseBytes(jlong);
  static jlong reverse(jlong);
private:
  static ::java::lang::String * toUnsignedString(jlong, jint);
  static jlong parseLong(::java::lang::String *, jint, jboolean);
public:
  jint compareTo(::java::lang::Object *);
private:
  static const jlong serialVersionUID = 4290774380558885855LL;
public:
  static const jlong MIN_VALUE = -9223372036854775807LL - 1;
  static const jlong MAX_VALUE = 9223372036854775807LL;
  static ::java::lang::Class * TYPE;
  static const jint SIZE = 64;
private:
  static const jint MIN_CACHE = -128;
  static const jint MAX_CACHE = 127;
  static JArray< ::java::lang::Long * > * longCache;
  jlong __attribute__((aligned(__alignof__( ::java::lang::Number)))) value;
public:
  static ::java::lang::Class class$;
};

#endif // __java_lang_Long__
