
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_management_Server$ServerNotificationListener__
#define __gnu_javax_management_Server$ServerNotificationListener__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace management
      {
          class Server;
          class Server$ServerNotificationListener;
      }
    }
  }
  namespace javax
  {
    namespace management
    {
        class Notification;
        class NotificationListener;
        class ObjectName;
    }
  }
}

class gnu::javax::management::Server$ServerNotificationListener : public ::java::lang::Object
{

public:
  Server$ServerNotificationListener(::gnu::javax::management::Server *, ::java::lang::Object *, ::javax::management::ObjectName *, ::javax::management::NotificationListener *);
  virtual void handleNotification(::javax::management::Notification *, ::java::lang::Object *);
public: // actually package-private
  ::java::lang::Object * __attribute__((aligned(__alignof__( ::java::lang::Object)))) bean;
  ::javax::management::ObjectName * name;
  ::javax::management::NotificationListener * listener;
  ::gnu::javax::management::Server * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_management_Server$ServerNotificationListener__
