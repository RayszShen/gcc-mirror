
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_io_ObjectStreamClass__
#define __java_io_ObjectStreamClass__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace io
      {
          class NullOutputStream;
      }
    }
  }
}

class java::io::ObjectStreamClass : public ::java::lang::Object
{

public:
  static ::java::io::ObjectStreamClass * lookup(::java::lang::Class *);
public: // actually package-private
  static ::java::io::ObjectStreamClass * lookupForClassObject(::java::lang::Class *);
public:
  virtual ::java::lang::String * getName();
  virtual ::java::lang::Class * forClass();
  virtual jlong getSerialVersionUID();
  virtual JArray< ::java::io::ObjectStreamField * > * getFields();
  virtual ::java::io::ObjectStreamField * getField(::java::lang::String *);
  virtual ::java::lang::String * toString();
public: // actually package-private
  virtual jboolean hasWriteMethod();
  virtual jboolean isSerializable();
  virtual jboolean isExternalizable();
  virtual jboolean isEnum();
  virtual ::java::io::ObjectStreamClass * getSuper();
  virtual JArray< ::java::io::ObjectStreamClass * > * hierarchy();
  virtual jint getFlags();
  ObjectStreamClass(::java::lang::String *, jlong, jbyte, JArray< ::java::io::ObjectStreamField * > *);
  virtual void setClass(::java::lang::Class *, ::java::io::ObjectStreamClass *);
  virtual void setSuperclass(::java::io::ObjectStreamClass *);
  virtual void calculateOffsets();
private:
  ::java::lang::reflect::Method * findMethod(JArray< ::java::lang::reflect::Method * > *, ::java::lang::String *, JArray< ::java::lang::Class * > *, ::java::lang::Class *, jboolean);
  static jboolean inSamePackage(::java::lang::Class *, ::java::lang::Class *);
  static ::java::lang::reflect::Method * findAccessibleMethod(::java::lang::String *, ::java::lang::Class *);
  static jboolean loadedByBootOrApplicationClassLoader(::java::lang::Class *);
  void cacheMethods();
  ObjectStreamClass(::java::lang::Class *);
  void setFlags(::java::lang::Class *);
  void setFields(::java::lang::Class *);
  jlong getClassUID(::java::lang::Class *);
public: // actually package-private
  virtual jlong getClassUIDFromField(::java::lang::Class *);
  virtual jlong calculateClassUID(::java::lang::Class *);
private:
  JArray< ::java::io::ObjectStreamField * > * getSerialPersistentFields(::java::lang::Class *);
public: // actually package-private
  virtual ::java::io::Externalizable * newInstance();
  static JArray< ::java::io::ObjectStreamField * > * INVALID_FIELDS;
private:
  JArray< ::java::io::ObjectStreamClass * > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) hierarchy__;
public: // actually package-private
  static JArray< ::java::lang::Class * > * noArgs;
  static ::java::util::Hashtable * methodCache;
  static JArray< ::java::lang::Class * > * readObjectSignature;
  static JArray< ::java::lang::Class * > * writeObjectSignature;
  static ::java::util::Hashtable * uidCache;
public:
  static JArray< ::java::io::ObjectStreamField * > * NO_FIELDS;
private:
  static ::java::util::Hashtable * classLookupTable;
  static ::gnu::java::io::NullOutputStream * nullOutputStream;
  static ::java::util::Comparator * interfaceComparator;
  static ::java::util::Comparator * memberComparator;
  static JArray< ::java::lang::Class * > * writeMethodArgTypes;
  ::java::io::ObjectStreamClass * superClass;
  ::java::lang::Class * clazz;
  ::java::lang::String * name;
  jlong uid;
  jbyte flags;
public: // actually package-private
  JArray< ::java::io::ObjectStreamField * > * fields;
  jint primFieldSize;
  jint objectFieldCount;
  ::java::lang::reflect::Method * readObjectMethod;
  ::java::lang::reflect::Method * readResolveMethod;
  ::java::lang::reflect::Method * writeReplaceMethod;
  ::java::lang::reflect::Method * writeObjectMethod;
  jboolean realClassIsSerializable;
  jboolean realClassIsExternalizable;
  JArray< ::java::io::ObjectStreamField * > * fieldMapping;
  ::java::lang::reflect::Constructor * firstNonSerializableParentConstructor;
private:
  ::java::lang::reflect::Constructor * constructor;
public: // actually package-private
  jboolean isProxyClass;
private:
  static const jlong serialVersionUID = -6120832682080437368LL;
public:
  static ::java::lang::Class class$;
};

#endif // __java_io_ObjectStreamClass__
