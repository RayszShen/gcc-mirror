
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_security_auth_Subject$SecureSet__
#define __javax_security_auth_Subject$SecureSet__

#pragma interface

#include <java/util/AbstractSet.h>
extern "Java"
{
  namespace javax
  {
    namespace security
    {
      namespace auth
      {
          class Subject;
          class Subject$SecureSet;
      }
    }
  }
}

class javax::security::auth::Subject$SecureSet : public ::java::util::AbstractSet
{

public: // actually package-private
  Subject$SecureSet(::javax::security::auth::Subject *, jint, ::java::util::Collection *);
  Subject$SecureSet(::javax::security::auth::Subject *, jint);
public:
  virtual jint size();
  virtual ::java::util::Iterator * iterator();
  virtual jboolean add(::java::lang::Object *);
  virtual jboolean remove(::java::lang::Object *);
  virtual jboolean contains(::java::lang::Object *);
  virtual jboolean removeAll(::java::util::Collection *);
  virtual jboolean retainAll(::java::util::Collection *);
  virtual void clear();
private:
  void writeObject(::java::io::ObjectOutputStream *);
  void readObject(::java::io::ObjectInputStream *);
  static const jlong serialVersionUID = 7911754171111800359LL;
public: // actually package-private
  static const jint PRINCIPALS = 0;
  static const jint PUBLIC_CREDENTIALS = 1;
  static const jint PRIVATE_CREDENTIALS = 2;
private:
  ::javax::security::auth::Subject * __attribute__((aligned(__alignof__( ::java::util::AbstractSet)))) subject;
  ::java::util::LinkedList * elements;
  jint type;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_security_auth_Subject$SecureSet__
