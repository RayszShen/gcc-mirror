
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_security_auth_callback_SwingCallbackHandler$3__
#define __gnu_javax_security_auth_callback_SwingCallbackHandler$3__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace security
      {
        namespace auth
        {
          namespace callback
          {
              class SwingCallbackHandler;
              class SwingCallbackHandler$3;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
      namespace event
      {
          class ActionEvent;
      }
    }
  }
  namespace javax
  {
    namespace security
    {
      namespace auth
      {
        namespace callback
        {
            class ConfirmationCallback;
        }
      }
    }
    namespace swing
    {
        class JDialog;
    }
  }
}

class gnu::javax::security::auth::callback::SwingCallbackHandler$3 : public ::java::lang::Object
{

public: // actually package-private
  SwingCallbackHandler$3(::gnu::javax::security::auth::callback::SwingCallbackHandler *, JArray< ::java::lang::String * > *, ::javax::security::auth::callback::ConfirmationCallback *, ::javax::swing::JDialog *);
public:
  virtual void actionPerformed(::java::awt::event::ActionEvent *);
public: // actually package-private
  ::gnu::javax::security::auth::callback::SwingCallbackHandler * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$0;
private:
  JArray< ::java::lang::String * > * val$options;
  ::javax::security::auth::callback::ConfirmationCallback * val$callback;
  ::javax::swing::JDialog * val$dialog;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_security_auth_callback_SwingCallbackHandler$3__
