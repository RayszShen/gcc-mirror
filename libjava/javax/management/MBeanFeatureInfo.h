
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_management_MBeanFeatureInfo__
#define __javax_management_MBeanFeatureInfo__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace management
    {
        class MBeanFeatureInfo;
    }
  }
}

class javax::management::MBeanFeatureInfo : public ::java::lang::Object
{

public:
  MBeanFeatureInfo(::java::lang::String *, ::java::lang::String *);
  virtual jboolean equals(::java::lang::Object *);
  virtual ::java::lang::String * getDescription();
  virtual ::java::lang::String * getName();
  virtual jint hashCode();
  virtual ::java::lang::String * toString();
private:
  static const jlong serialVersionUID = 3952882688968447265LL;
public: // actually protected
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::lang::Object)))) description;
  ::java::lang::String * name;
public: // actually package-private
  ::java::lang::String * string;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_management_MBeanFeatureInfo__
