
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_beans_decoder_PersistenceParser$ByteHandlerCreator__
#define __gnu_java_beans_decoder_PersistenceParser$ByteHandlerCreator__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace beans
      {
        namespace decoder
        {
            class AbstractElementHandler;
            class ElementHandler;
            class PersistenceParser;
            class PersistenceParser$ByteHandlerCreator;
        }
      }
    }
  }
}

class gnu::java::beans::decoder::PersistenceParser$ByteHandlerCreator : public ::java::lang::Object
{

public: // actually package-private
  PersistenceParser$ByteHandlerCreator(::gnu::java::beans::decoder::PersistenceParser *);
public:
  virtual ::gnu::java::beans::decoder::AbstractElementHandler * createHandler(::gnu::java::beans::decoder::ElementHandler *);
public: // actually package-private
  ::gnu::java::beans::decoder::PersistenceParser * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_beans_decoder_PersistenceParser$ByteHandlerCreator__
