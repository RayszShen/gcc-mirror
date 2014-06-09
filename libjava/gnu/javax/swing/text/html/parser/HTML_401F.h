
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_swing_text_html_parser_HTML_401F__
#define __gnu_javax_swing_text_html_parser_HTML_401F__

#pragma interface

#include <gnu/javax/swing/text/html/parser/gnuDTD.h>
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
                class HTML_401F;
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
              class ContentModel;
              class DTD;
          }
        }
      }
    }
  }
}

class gnu::javax::swing::text::html::parser::HTML_401F : public ::gnu::javax::swing::text::html::parser::gnuDTD
{

public: // actually protected
  HTML_401F();
public:
  static ::javax::swing::text::html::parser::DTD * getInstance();
public: // actually protected
  virtual void defineElements();
private:
  void defineElements1();
  void defineElements2();
  void defineElements3();
  void defineElements4();
  void defineElements5();
  void defineElements6();
public: // actually protected
  virtual void defineEntities();
  virtual ::javax::swing::text::html::parser::ContentModel * model(::java::lang::String *);
private:
  ::javax::swing::text::html::parser::ContentModel * model(::java::lang::String *, jint);
public: // actually protected
  virtual ::javax::swing::text::html::parser::ContentModel * createHtmlContentModel();
  virtual ::javax::swing::text::html::parser::ContentModel * createTableContentModel();
  virtual ::javax::swing::text::html::parser::ContentModel * createDefListModel();
  virtual ::javax::swing::text::html::parser::ContentModel * createListModel();
  virtual JArray< ::java::lang::String * > * getBodyElements();
private:
  static const jlong serialVersionUID = 1LL;
public:
  static ::java::lang::String * DTD_NAME;
public: // actually package-private
  static const jint PIXELS = 12;
  static JArray< ::java::lang::String * > * NONE;
  static ::java::lang::String * PCDATA;
  static ::java::lang::String * A;
  static ::java::lang::String * ABBR;
  static ::java::lang::String * ACRONYM;
  static ::java::lang::String * ADDRESS;
  static ::java::lang::String * APPLET;
  static ::java::lang::String * AREA;
  static ::java::lang::String * B;
  static ::java::lang::String * BASE;
  static ::java::lang::String * BASEFONT;
  static ::java::lang::String * BDO;
  static ::java::lang::String * BIG;
  static ::java::lang::String * BLOCKQUOTE;
  static ::java::lang::String * BODY;
  static ::java::lang::String * BR;
  static ::java::lang::String * BUTTON;
  static ::java::lang::String * CAPTION;
  static ::java::lang::String * CENTER;
  static ::java::lang::String * CITE;
  static ::java::lang::String * CODE;
  static ::java::lang::String * COL;
  static ::java::lang::String * COLGROUP;
  static ::java::lang::String * DEFAULTS;
  static ::java::lang::String * DD;
  static ::java::lang::String * DEL;
  static ::java::lang::String * DFN;
  static ::java::lang::String * DIR;
  static ::java::lang::String * DIV;
  static ::java::lang::String * DL;
  static ::java::lang::String * DT;
  static ::java::lang::String * EM;
  static ::java::lang::String * FIELDSET;
  static ::java::lang::String * FONT;
  static ::java::lang::String * FORM;
  static ::java::lang::String * FRAME;
  static ::java::lang::String * FRAMESET;
  static ::java::lang::String * H1;
  static ::java::lang::String * H2;
  static ::java::lang::String * H3;
  static ::java::lang::String * H4;
  static ::java::lang::String * H5;
  static ::java::lang::String * H6;
  static ::java::lang::String * HEAD;
  static ::java::lang::String * HR;
  static ::java::lang::String * HTML;
  static ::java::lang::String * I;
  static ::java::lang::String * IFRAME;
  static ::java::lang::String * IMG;
  static ::java::lang::String * INPUT;
  static ::java::lang::String * INS;
  static ::java::lang::String * ISINDEX;
  static ::java::lang::String * KBD;
  static ::java::lang::String * LABEL;
  static ::java::lang::String * LEGEND;
  static ::java::lang::String * LI;
  static ::java::lang::String * LINK;
  static ::java::lang::String * MAP;
  static ::java::lang::String * MENU;
  static ::java::lang::String * META;
  static ::java::lang::String * NOFRAMES;
  static ::java::lang::String * NOSCRIPT;
  static ::java::lang::String * NONES;
  static ::java::lang::String * sNAME;
  static ::java::lang::String * OBJECT;
  static ::java::lang::String * OL;
  static ::java::lang::String * OPTGROUP;
  static ::java::lang::String * OPTION;
  static ::java::lang::String * P;
  static ::java::lang::String * PARAM;
  static ::java::lang::String * PRE;
  static ::java::lang::String * Q;
  static ::java::lang::String * S;
  static ::java::lang::String * SAMP;
  static ::java::lang::String * SCRIPT;
  static ::java::lang::String * SELECT;
  static ::java::lang::String * SMALL;
  static ::java::lang::String * SPAN;
  static ::java::lang::String * STRIKE;
  static ::java::lang::String * STRONG;
  static ::java::lang::String * STYLE;
  static ::java::lang::String * SUB;
  static ::java::lang::String * SUP;
  static ::java::lang::String * TABLE;
  static ::java::lang::String * TBODY;
  static ::java::lang::String * TD;
  static ::java::lang::String * TEXTAREA;
  static ::java::lang::String * TFOOT;
  static ::java::lang::String * TH;
  static ::java::lang::String * THEAD;
  static ::java::lang::String * TITLE;
  static ::java::lang::String * TR;
  static ::java::lang::String * TT;
  static ::java::lang::String * U;
  static ::java::lang::String * UL;
  static ::java::lang::String * VAR;
  static ::java::lang::String * C_0;
  static ::java::lang::String * C_1;
  static ::java::lang::String * CHECKBOX;
  static ::java::lang::String * DATA;
  static ::java::lang::String * FILE;
  static ::java::lang::String * GET;
  static ::java::lang::String * HIDDEN;
  static ::java::lang::String * IMAGE;
  static ::java::lang::String * PASSWORD;
  static ::java::lang::String * POST;
  static ::java::lang::String * RADIO;
  static ::java::lang::String * REF;
  static ::java::lang::String * RESET;
  static ::java::lang::String * SUBMIT;
  static ::java::lang::String * TEXT;
  static ::java::lang::String * ABOVE;
  static ::java::lang::String * ACCEPT;
  static ::java::lang::String * ACCEPTCHARSET;
  static ::java::lang::String * ACCESSKEY;
  static ::java::lang::String * ACTION;
  static ::java::lang::String * ALIGN;
  static ::java::lang::String * ALINK;
  static ::java::lang::String * ALL;
  static ::java::lang::String * ALT;
  static ::java::lang::String * APPLICATION_X_WWW_FORM_URLENCODED;
  static ::java::lang::String * ARCHIVE;
  static ::java::lang::String * AUTO;
  static ::java::lang::String * AXIS;
  static ::java::lang::String * BACKGROUND;
  static ::java::lang::String * BASELINE;
  static ::java::lang::String * BELOW;
  static ::java::lang::String * BGCOLOR;
  static ::java::lang::String * BORDER;
  static ::java::lang::String * BOTTOM;
  static ::java::lang::String * BOX;
  static ::java::lang::String * CELLPADDING;
  static ::java::lang::String * CELLSPACING;
  static ::java::lang::String * CHAR;
  static ::java::lang::String * CHAROFF;
  static ::java::lang::String * CHARSET;
  static ::java::lang::String * CHECKED;
  static ::java::lang::String * CIRCLE;
  static ::java::lang::String * CLASS;
  static ::java::lang::String * CLASSID;
  static ::java::lang::String * CLEAR;
  static ::java::lang::String * CODEBASE;
  static ::java::lang::String * CODETYPE;
  static ::java::lang::String * COLOR;
  static ::java::lang::String * COLS;
  static ::java::lang::String * COLSPAN;
  static ::java::lang::String * COMPACT;
  static ::java::lang::String * CONTENT;
  static ::java::lang::String * COORDS;
  static ::java::lang::String * DATAPAGESIZE;
  static ::java::lang::String * DATETIME;
  static ::java::lang::String * DECLARE;
  static ::java::lang::String * DEFER;
  static ::java::lang::String * DISABLED;
  static ::java::lang::String * DISC;
  static ::java::lang::String * ENCTYPE;
  static ::java::lang::String * EVENT;
  static ::java::lang::String * FACE;
  static ::java::lang::String * FOR;
  static ::java::lang::String * FRAMEBORDER;
  static ::java::lang::String * GROUPS;
  static ::java::lang::String * HEADERS;
  static ::java::lang::String * HEIGHT;
  static ::java::lang::String * HREF;
  static ::java::lang::String * HREFLANG;
  static ::java::lang::String * HSIDES;
  static ::java::lang::String * HSPACE;
  static ::java::lang::String * HTTPEQUIV;
  static ::java::lang::String * sID;
  static ::java::lang::String * ISMAP;
  static ::java::lang::String * JUSTIFY;
  static ::java::lang::String * LANG;
  static ::java::lang::String * LANGUAGE;
  static ::java::lang::String * LEFT;
  static ::java::lang::String * LHS;
  static ::java::lang::String * LONGDESC;
  static ::java::lang::String * LTR;
  static ::java::lang::String * MARGINHEIGHT;
  static ::java::lang::String * MARGINWIDTH;
  static ::java::lang::String * MAXLENGTH;
  static ::java::lang::String * MEDIA;
  static ::java::lang::String * METHOD;
  static ::java::lang::String * MIDDLE;
  static ::java::lang::String * MULTIPLE;
  static ::java::lang::String * NO;
  static ::java::lang::String * NOHREF;
  static ::java::lang::String * NORESIZE;
  static ::java::lang::String * NOSHADE;
  static ::java::lang::String * NOWRAP;
  static ::java::lang::String * ONBLUR;
  static ::java::lang::String * ONCHANGE;
  static ::java::lang::String * ONCLICK;
  static ::java::lang::String * ONDBLCLICK;
  static ::java::lang::String * ONFOCUS;
  static ::java::lang::String * ONKEYDOWN;
  static ::java::lang::String * ONKEYPRESS;
  static ::java::lang::String * ONKEYUP;
  static ::java::lang::String * ONLOAD;
  static ::java::lang::String * ONMOUSEDOWN;
  static ::java::lang::String * ONMOUSEMOVE;
  static ::java::lang::String * ONMOUSEOUT;
  static ::java::lang::String * ONMOUSEOVER;
  static ::java::lang::String * ONMOUSEUP;
  static ::java::lang::String * ONRESET;
  static ::java::lang::String * ONSELECT;
  static ::java::lang::String * ONSUBMIT;
  static ::java::lang::String * ONUNLOAD;
  static ::java::lang::String * POLY;
  static ::java::lang::String * PROFILE;
  static ::java::lang::String * PROMPT;
  static ::java::lang::String * READONLY;
  static ::java::lang::String * RECT;
  static ::java::lang::String * REL;
  static ::java::lang::String * REV;
  static ::java::lang::String * RHS;
  static ::java::lang::String * RIGHT;
  static ::java::lang::String * ROW;
  static ::java::lang::String * ROWGROUP;
  static ::java::lang::String * ROWS;
  static ::java::lang::String * ROWSPAN;
  static ::java::lang::String * RTL;
  static ::java::lang::String * RULES;
  static ::java::lang::String * SCHEME;
  static ::java::lang::String * SCOPE;
  static ::java::lang::String * SCROLLING;
  static ::java::lang::String * SELECTED;
  static ::java::lang::String * SHAPE;
  static ::java::lang::String * SIZE;
  static ::java::lang::String * SQUARE;
  static ::java::lang::String * SRC;
  static ::java::lang::String * STANDBY;
  static ::java::lang::String * START;
  static ::java::lang::String * SUMMARY;
  static ::java::lang::String * TABINDEX;
  static ::java::lang::String * TARGET;
  static ::java::lang::String * TOP;
  static ::java::lang::String * TYPE;
  static ::java::lang::String * USEMAP;
  static ::java::lang::String * VALIGN;
  static ::java::lang::String * VALUE;
  static ::java::lang::String * VALUETYPE;
  static ::java::lang::String * VERSION;
  static ::java::lang::String * VLINK;
  static ::java::lang::String * VOID;
  static ::java::lang::String * VSIDES;
  static ::java::lang::String * VSPACE;
  static ::java::lang::String * WIDTH;
  static ::java::lang::String * YES;
  static JArray< ::java::lang::String * > * BLOCK;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_swing_text_html_parser_HTML_401F__
