
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_CORBA_ServiceDetail__
#define __org_omg_CORBA_ServiceDetail__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class ServiceDetail;
      }
    }
  }
}

class org::omg::CORBA::ServiceDetail : public ::java::lang::Object
{

public:
  ServiceDetail();
  ServiceDetail(jint, JArray< jbyte > *);
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) service_detail_type;
  JArray< jbyte > * service_detail;
  static ::java::lang::Class class$;
};

#endif // __org_omg_CORBA_ServiceDetail__
