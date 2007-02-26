
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_swing_text_html_parser_gnuDTD__
#define __gnu_javax_swing_text_html_parser_gnuDTD__

#pragma interface

#include <javax/swing/text/html/parser/DTD.h>
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
                class gnuDTD;
            }
          }
        }
      }
    }
  }
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
              class AttributeList;
              class ContentModel;
              class Element;
              class Entity;
          }
        }
      }
    }
  }
}

class gnu::javax::swing::text::html::parser::gnuDTD : public ::javax::swing::text::html::parser::DTD
{

public:
  gnuDTD(::java::lang::String *);
  virtual ::javax::swing::text::html::parser::AttributeList * defAttributeList(::java::lang::String *, jint, jint, ::java::lang::String *, ::java::lang::String *, ::javax::swing::text::html::parser::AttributeList *);
  virtual void defAttrsFor(::java::lang::String *, ::javax::swing::text::html::parser::AttributeList *);
  virtual ::javax::swing::text::html::parser::ContentModel * defContentModel(jint, ::java::lang::Object *, ::javax::swing::text::html::parser::ContentModel *);
  virtual ::javax::swing::text::html::parser::Element * defElement(::java::lang::String *, jint, jboolean, jboolean, ::javax::swing::text::html::parser::ContentModel *, JArray< ::java::lang::String * > *, JArray< ::java::lang::String * > *, ::javax::swing::text::html::parser::AttributeList *);
  virtual ::javax::swing::text::html::parser::Element * defElement(::java::lang::String *, jint, jboolean, jboolean, ::javax::swing::text::html::parser::ContentModel *, ::java::util::Collection *, ::java::util::Collection *, ::javax::swing::text::html::parser::AttributeList *);
  virtual ::javax::swing::text::html::parser::Element * defElement(::java::lang::String *, jint, jboolean, jboolean, ::javax::swing::text::html::parser::ContentModel *, JArray< ::java::lang::String * > *, JArray< ::java::lang::String * > *, JArray< ::javax::swing::text::html::parser::AttributeList * > *);
  virtual ::javax::swing::text::html::parser::Entity * defEntity(::java::lang::String *, jint, ::java::lang::String *);
  virtual void dump(::java::io::PrintStream *);
  virtual void dump(::java::util::BitSet *);
public: // actually protected
  virtual ::javax::swing::text::html::parser::AttributeList * attr(::java::lang::String *, ::java::lang::String *, JArray< ::java::lang::String * > *, jint, jint);
  virtual void defineEntity(::java::lang::String *, jint);
private:
  JArray< ::java::lang::String * > * toStringArray(::java::util::Collection *);
public:
  static const jint URI = 512;
  static const jint Length = 513;
  static const jint Char = 514;
  static const jint Color = 515;
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_swing_text_html_parser_gnuDTD__
