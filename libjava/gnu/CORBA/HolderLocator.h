
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_CORBA_HolderLocator__
#define __gnu_CORBA_HolderLocator__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace CORBA
    {
        class HolderLocator;
    }
  }
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class TypeCode;
        namespace portable
        {
            class Streamable;
        }
      }
    }
  }
}

class gnu::CORBA::HolderLocator : public ::java::lang::Object
{

public:
  HolderLocator();
  static ::org::omg::CORBA::portable::Streamable * createHolder(::org::omg::CORBA::TypeCode *);
private:
  static JArray< ::java::lang::Class * > * holders;
  static JArray< ::java::lang::Class * > * seqHolders;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_CORBA_HolderLocator__
