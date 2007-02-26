
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_swing_text_html_parser_support_low_ReaderTokenizer__
#define __gnu_javax_swing_text_html_parser_support_low_ReaderTokenizer__

#pragma interface

#include <gnu/javax/swing/text/html/parser/support/low/Constants.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace swing
      {
        namespace text
        {
          namespace html
          {
            namespace parser
            {
              namespace support
              {
                namespace low
                {
                    class Buffer;
                    class Queue;
                    class ReaderTokenizer;
                    class Token;
                }
              }
            }
          }
        }
      }
    }
  }
}

class gnu::javax::swing::text::html::parser::support::low::ReaderTokenizer : public ::gnu::javax::swing::text::html::parser::support::low::Constants
{

public:
  ReaderTokenizer();
  virtual ::java::lang::String * getEndOfLineSequence();
  virtual ::gnu::javax::swing::text::html::parser::support::low::Token * getNextToken();
  virtual ::gnu::javax::swing::text::html::parser::support::low::Token * getTokenAhead(jint);
  virtual ::gnu::javax::swing::text::html::parser::support::low::Token * getTokenAhead();
  virtual void error(::java::lang::String *, ::gnu::javax::swing::text::html::parser::support::low::Token *);
  virtual void mark(jboolean);
  virtual void reset(::java::io::Reader *);
  virtual void reset();
public: // actually package-private
  virtual void read(jint);
  virtual void readToken();
  virtual ::gnu::javax::swing::text::html::parser::support::low::Token * tokenMatches();
private:
  void consumeBuffer(::gnu::javax::swing::text::html::parser::support::low::Token *);
  ::gnu::javax::swing::text::html::parser::support::low::Token * eofToken();
public: // actually protected
  jboolean __attribute__((aligned(__alignof__( ::gnu::javax::swing::text::html::parser::support::low::Constants)))) advanced;
  jboolean backupMode;
public: // actually package-private
  ::gnu::javax::swing::text::html::parser::support::low::Buffer * buffer;
  ::gnu::javax::swing::text::html::parser::support::low::Queue * backup;
  ::gnu::javax::swing::text::html::parser::support::low::Queue * queue;
  ::java::io::Reader * reader;
  JArray< jchar > * charTokens;
  JArray< ::java::lang::String * > * stringTokens;
  jint readerPosition;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_swing_text_html_parser_support_low_ReaderTokenizer__
