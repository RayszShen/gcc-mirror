
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_classpath_jdwp_VMMethod__
#define __gnu_classpath_jdwp_VMMethod__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace classpath
    {
      namespace jdwp
      {
          class VMMethod;
        namespace util
        {
            class LineTable;
            class VariableTable;
        }
      }
    }
  }
  namespace java
  {
    namespace nio
    {
        class ByteBuffer;
    }
  }
}

class gnu::classpath::jdwp::VMMethod : public ::java::lang::Object
{

public: // actually protected
  VMMethod(::java::lang::Class *, jlong);
public:
  virtual jlong getId();
  virtual ::java::lang::Class * getDeclaringClass();
  virtual ::java::lang::String * getName();
  virtual ::java::lang::String * getSignature();
  virtual jint getModifiers();
  virtual ::gnu::classpath::jdwp::util::LineTable * getLineTable();
  virtual ::gnu::classpath::jdwp::util::VariableTable * getVariableTable();
  virtual ::java::lang::String * toString();
  virtual void writeId(::java::io::DataOutputStream *);
  static ::gnu::classpath::jdwp::VMMethod * readId(::java::lang::Class *, ::java::nio::ByteBuffer *);
  static const jint SIZE = 8;
private:
  ::java::lang::Class * __attribute__((aligned(__alignof__( ::java::lang::Object)))) _class;
  jlong _methodId;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_classpath_jdwp_VMMethod__
