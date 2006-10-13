
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_util_prefs_gconf_GConfNativePeer__
#define __gnu_java_util_prefs_gconf_GConfNativePeer__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace util
      {
        namespace prefs
        {
          namespace gconf
          {
              class GConfNativePeer;
          }
        }
      }
    }
  }
}

class gnu::java::util::prefs::gconf::GConfNativePeer : public ::java::lang::Object
{

public:
  GConfNativePeer();
  jboolean nodeExist(::java::lang::String *);
  void startWatchingNode(::java::lang::String *);
  void stopWatchingNode(::java::lang::String *);
  jboolean setString(::java::lang::String *, ::java::lang::String *);
  jboolean unset(::java::lang::String *);
  ::java::lang::String * getKey(::java::lang::String *);
  ::java::util::List * getKeys(::java::lang::String *);
  ::java::util::List * getChildrenNodes(::java::lang::String *);
  void suggestSync();
public: // actually protected
  void finalize();
private:
  static void init_id_cache();
  static void init_class();
  static void finalize_class();
public: // actually protected
  static jboolean gconf_client_dir_exists(::java::lang::String *);
  static void gconf_client_add_dir(::java::lang::String *);
  static void gconf_client_remove_dir(::java::lang::String *);
  static jboolean gconf_client_set_string(::java::lang::String *, ::java::lang::String *);
  static ::java::lang::String * gconf_client_get_string(::java::lang::String *);
  static jboolean gconf_client_unset(::java::lang::String *);
  static void gconf_client_suggest_sync();
  static ::java::util::List * gconf_client_gconf_client_all_nodes(::java::lang::String *);
  static ::java::util::List * gconf_client_gconf_client_all_keys(::java::lang::String *);
private:
  static JArray< ::java::lang::Object * > * semaphore;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_util_prefs_gconf_GConfNativePeer__
