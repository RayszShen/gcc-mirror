
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_ListResourceBundle__
#define __java_util_ListResourceBundle__

#pragma interface

#include <java/util/ResourceBundle.h>
#include <gcj/array.h>


class java::util::ListResourceBundle : public ::java::util::ResourceBundle
{

public:
  ListResourceBundle();
  virtual ::java::lang::Object * handleGetObject(::java::lang::String *);
  virtual ::java::util::Enumeration * getKeys();
public: // actually protected
  virtual JArray< JArray< ::java::lang::Object * > * > * getContents() = 0;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_ListResourceBundle__
