
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_zip_Deflater__
#define __java_util_zip_Deflater__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace gcj
    {
        class RawData;
    }
  }
}

class java::util::zip::Deflater : public ::java::lang::Object
{

public:
  Deflater();
  Deflater(jint);
  Deflater(jint, jboolean);
private:
  void init(jint, jboolean);
  void update();
public:
  virtual void reset();
  virtual void end();
  virtual jint getAdler();
  virtual jint getTotalIn();
  virtual jint getTotalOut();
public: // actually protected
  virtual void finalize();
public:
  virtual void finish();
  virtual jboolean finished();
  virtual jboolean needsInput();
  virtual void setInput(JArray< jbyte > *);
  virtual void setInput(JArray< jbyte > *, jint, jint);
  virtual void setLevel(jint);
  virtual void setStrategy(jint);
  virtual jint deflate(JArray< jbyte > *);
  virtual jint deflate(JArray< jbyte > *, jint, jint);
  virtual void setDictionary(JArray< jbyte > *);
  virtual void setDictionary(JArray< jbyte > *, jint, jint);
public: // actually package-private
  virtual void flush();
public:
  static const jint BEST_COMPRESSION = 9;
  static const jint BEST_SPEED = 1;
  static const jint DEFAULT_COMPRESSION = -1;
  static const jint NO_COMPRESSION = 0;
  static const jint DEFAULT_STRATEGY = 0;
  static const jint FILTERED = 1;
  static const jint HUFFMAN_ONLY = 2;
  static const jint DEFLATED = 8;
private:
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) level;
  jint strategy;
  ::gnu::gcj::RawData * zstream;
  jboolean is_finished;
  jint flush_flag;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_zip_Deflater__
