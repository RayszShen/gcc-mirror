
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_AllPermission$AllPermissionCollection__
#define __java_security_AllPermission$AllPermissionCollection__

#pragma interface

#include <java/security/PermissionCollection.h>
extern "Java"
{
  namespace java
  {
    namespace security
    {
        class AllPermission$AllPermissionCollection;
        class Permission;
    }
  }
}

class java::security::AllPermission$AllPermissionCollection : public ::java::security::PermissionCollection
{

  AllPermission$AllPermissionCollection();
public:
  void add(::java::security::Permission *);
  jboolean implies(::java::security::Permission *);
  ::java::util::Enumeration * elements();
public: // actually package-private
  AllPermission$AllPermissionCollection(::java::security::AllPermission$AllPermissionCollection *);
private:
  static const jlong serialVersionUID = -4023755556366636806LL;
  jboolean __attribute__((aligned(__alignof__( ::java::security::PermissionCollection)))) all_allowed;
public:
  static ::java::lang::Class class$;
};

#endif // __java_security_AllPermission$AllPermissionCollection__
