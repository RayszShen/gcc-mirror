
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_management_MBeanNotificationInfo__
#define __javax_management_MBeanNotificationInfo__

#pragma interface

#include <javax/management/MBeanFeatureInfo.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace management
    {
        class MBeanNotificationInfo;
    }
  }
}

class javax::management::MBeanNotificationInfo : public ::javax::management::MBeanFeatureInfo
{

public:
  MBeanNotificationInfo(JArray< ::java::lang::String * > *, ::java::lang::String *, ::java::lang::String *);
  virtual ::java::lang::Object * clone();
  virtual jboolean equals(::java::lang::Object *);
  virtual JArray< ::java::lang::String * > * getNotifTypes();
  virtual jint hashCode();
  virtual ::java::lang::String * toString();
private:
  static const jlong serialVersionUID = -3888371564530107064LL;
  JArray< ::java::lang::String * > * __attribute__((aligned(__alignof__( ::javax::management::MBeanFeatureInfo)))) types;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_management_MBeanNotificationInfo__
