
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_basic_BasicListUI$ListSelectionHandler__
#define __javax_swing_plaf_basic_BasicListUI$ListSelectionHandler__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
      namespace event
      {
          class ListSelectionEvent;
      }
      namespace plaf
      {
        namespace basic
        {
            class BasicListUI;
            class BasicListUI$ListSelectionHandler;
        }
      }
    }
  }
}

class javax::swing::plaf::basic::BasicListUI$ListSelectionHandler : public ::java::lang::Object
{

public:
  BasicListUI$ListSelectionHandler(::javax::swing::plaf::basic::BasicListUI *);
  virtual void valueChanged(::javax::swing::event::ListSelectionEvent *);
public: // actually package-private
  ::javax::swing::plaf::basic::BasicListUI * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_basic_BasicListUI$ListSelectionHandler__
