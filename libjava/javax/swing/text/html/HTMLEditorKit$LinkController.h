
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_text_html_HTMLEditorKit$LinkController__
#define __javax_swing_text_html_HTMLEditorKit$LinkController__

#pragma interface

#include <java/awt/event/MouseAdapter.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class MouseEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JEditorPane;
      namespace text
      {
        namespace html
        {
            class HTMLEditorKit$LinkController;
        }
      }
    }
  }
}

class javax::swing::text::html::HTMLEditorKit$LinkController : public ::java::awt::event::MouseAdapter
{

public:
  HTMLEditorKit$LinkController();
  virtual void mouseClicked(::java::awt::event::MouseEvent *);
  virtual void mouseDragged(::java::awt::event::MouseEvent *);
  virtual void mouseMoved(::java::awt::event::MouseEvent *);
public: // actually protected
  virtual void activateLink(jint, ::javax::swing::JEditorPane *);
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_text_html_HTMLEditorKit$LinkController__
