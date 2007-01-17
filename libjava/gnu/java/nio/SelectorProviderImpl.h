
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_nio_SelectorProviderImpl__
#define __gnu_java_nio_SelectorProviderImpl__

#pragma interface

#include <java/nio/channels/spi/SelectorProvider.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace nio
      {
          class SelectorProviderImpl;
      }
    }
  }
  namespace java
  {
    namespace nio
    {
      namespace channels
      {
          class DatagramChannel;
          class Pipe;
          class ServerSocketChannel;
          class SocketChannel;
        namespace spi
        {
            class AbstractSelector;
        }
      }
    }
  }
}

class gnu::java::nio::SelectorProviderImpl : public ::java::nio::channels::spi::SelectorProvider
{

public:
  SelectorProviderImpl();
  virtual ::java::nio::channels::DatagramChannel * openDatagramChannel();
  virtual ::java::nio::channels::Pipe * openPipe();
  virtual ::java::nio::channels::spi::AbstractSelector * openSelector();
  virtual ::java::nio::channels::ServerSocketChannel * openServerSocketChannel();
  virtual ::java::nio::channels::SocketChannel * openSocketChannel();
  static ::java::lang::Class class$;
};

#endif // __gnu_java_nio_SelectorProviderImpl__
