
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_management_StandardMBean__
#define __javax_management_StandardMBean__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace management
    {
        class Attribute;
        class AttributeList;
        class MBeanAttributeInfo;
        class MBeanConstructorInfo;
        class MBeanFeatureInfo;
        class MBeanInfo;
        class MBeanOperationInfo;
        class MBeanParameterInfo;
        class StandardMBean;
    }
  }
}

class javax::management::StandardMBean : public ::java::lang::Object
{

public: // actually protected
  StandardMBean(::java::lang::Class *);
public:
  StandardMBean(::java::lang::Object *, ::java::lang::Class *);
public: // actually protected
  virtual void cacheMBeanInfo(::javax::management::MBeanInfo *);
public:
  virtual ::java::lang::Object * getAttribute(::java::lang::String *);
  virtual ::javax::management::AttributeList * getAttributes(JArray< ::java::lang::String * > *);
public: // actually protected
  virtual ::javax::management::MBeanInfo * getCachedMBeanInfo();
  virtual ::java::lang::String * getClassName(::javax::management::MBeanInfo *);
  virtual JArray< ::javax::management::MBeanConstructorInfo * > * getConstructors(JArray< ::javax::management::MBeanConstructorInfo * > *, ::java::lang::Object *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanAttributeInfo *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanConstructorInfo *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanConstructorInfo *, ::javax::management::MBeanParameterInfo *, jint);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanFeatureInfo *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanInfo *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanOperationInfo *);
  virtual ::java::lang::String * getDescription(::javax::management::MBeanOperationInfo *, ::javax::management::MBeanParameterInfo *, jint);
  virtual jint getImpact(::javax::management::MBeanOperationInfo *);
public:
  virtual ::java::lang::Object * getImplementation();
  virtual ::java::lang::Class * getImplementationClass();
  virtual ::javax::management::MBeanInfo * getMBeanInfo();
  virtual ::java::lang::Class * getMBeanInterface();
public: // actually protected
  virtual ::java::lang::String * getParameterName(::javax::management::MBeanConstructorInfo *, ::javax::management::MBeanParameterInfo *, jint);
  virtual ::java::lang::String * getParameterName(::javax::management::MBeanOperationInfo *, ::javax::management::MBeanParameterInfo *, jint);
public:
  virtual ::java::lang::Object * invoke(::java::lang::String *, JArray< ::java::lang::Object * > *, JArray< ::java::lang::String * > *);
  virtual void setAttribute(::javax::management::Attribute *);
  virtual ::javax::management::AttributeList * setAttributes(::javax::management::AttributeList *);
  virtual void setImplementation(::java::lang::Object *);
private:
  ::java::lang::reflect::Method * getMutator(::java::lang::String *, ::java::lang::Class *);
  ::java::lang::Class * __attribute__((aligned(__alignof__( ::java::lang::Object)))) iface;
  ::java::lang::Object * impl;
  ::javax::management::MBeanInfo * info;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_management_StandardMBean__
