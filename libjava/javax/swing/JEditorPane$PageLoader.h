
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JEditorPane$PageLoader__
#define __javax_swing_JEditorPane$PageLoader__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace java
  {
    namespace net
    {
        class URL;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JEditorPane;
        class JEditorPane$PageLoader;
        class JEditorPane$PageStream;
      namespace text
      {
          class Document;
      }
    }
  }
}

class javax::swing::JEditorPane$PageLoader : public ::java::lang::Object
{

public: // actually package-private
  JEditorPane$PageLoader(::javax::swing::JEditorPane *, ::javax::swing::text::Document *, ::java::io::InputStream *, ::java::net::URL *, ::java::net::URL *);
public:
  virtual void run();
public: // actually package-private
  virtual void cancel();
  static ::java::net::URL * access$0(::javax::swing::JEditorPane$PageLoader *);
  static ::javax::swing::JEditorPane * access$1(::javax::swing::JEditorPane$PageLoader *);
private:
  ::javax::swing::text::Document * __attribute__((aligned(__alignof__( ::java::lang::Object)))) doc;
  ::javax::swing::JEditorPane$PageStream * in;
  ::java::net::URL * old;
public: // actually package-private
  ::java::net::URL * page;
  ::javax::swing::JEditorPane * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JEditorPane$PageLoader__
