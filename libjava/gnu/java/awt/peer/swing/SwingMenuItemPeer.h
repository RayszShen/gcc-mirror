
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_swing_SwingMenuItemPeer__
#define __gnu_java_awt_peer_swing_SwingMenuItemPeer__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace peer
        {
          namespace swing
          {
              class SwingMenuItemPeer;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Font;
        class MenuItem;
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JMenuItem;
    }
  }
}

class gnu::java::awt::peer::swing::SwingMenuItemPeer : public ::java::lang::Object
{

public:
  SwingMenuItemPeer(::java::awt::MenuItem *);
  virtual void disable();
  virtual void enable();
  virtual void setEnabled(jboolean);
  virtual void setLabel(::java::lang::String *);
  virtual void dispose();
  virtual void setFont(::java::awt::Font *);
public: // actually package-private
  ::java::awt::MenuItem * __attribute__((aligned(__alignof__( ::java::lang::Object)))) awtMenuItem;
  ::javax::swing::JMenuItem * menuItem;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_swing_SwingMenuItemPeer__
