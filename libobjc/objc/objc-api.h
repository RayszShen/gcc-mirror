/* GNU Objective-C Runtime API - Traditional API
   Copyright (C) 1993, 1995, 1996, 1997, 2001, 2002, 2003, 2004, 2005,
   2007, 2009, 2010 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

GCC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef __objc_api_INCLUDE_GNU
#define __objc_api_INCLUDE_GNU

/*
  This file declares the "traditional" GNU Objective-C Runtime API.
  It is the API supported by older versions of the GNU Objective-C
  Runtime.  Include this file to use it.

  This API is being replaced by the "modern" GNU Objective-C Runtime
  API, which is declared in objc/runtime.h.  The "modern" API is very
  similar to the API used by the modern Apple/NeXT runtime.

  Because the two APIs have some conflicting definitions (in
  particular, Method and Category are defined differently) you should
  include either objc/objc-api.h (to use the traditional GNU
  Objective-C Runtime API) or objc/runtime.h (to use the modern GNU
  Objective-C Runtime API), but not both.
*/
#ifdef __objc_runtime_INCLUDE_GNU
# error You can not include both objc/objc-api.h and objc/runtime.h.  Include objc/objc-api.h for the traditional GNU Objective-C Runtime API and objc/runtime.h for the modern one.
#endif

#include "objc.h"
#ifndef GNU_LIBOBJC_COMPILING_LIBOBJC_ITSELF
# include "deprecated/hash.h"
#endif
#include "thr.h"
#include "objc-decls.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "deprecated/METHOD_NULL.h"

/* Method descriptor returned by introspective Object methods.
   This is really just the first part of the more complete objc_method
   structure defined below and used internally by the runtime.  */
struct objc_method_description
{
    SEL name;			/* this is a selector, not a string */
    char *types;		/* type encoding */
};

/* The following are used in encode strings to describe the type of
   Ivars and Methods.  */
#define _C_ID       '@'
#define _C_CLASS    '#'
#define _C_SEL      ':'
#define _C_CHR      'c'
#define _C_UCHR     'C'
#define _C_SHT      's'
#define _C_USHT     'S'
#define _C_INT      'i'
#define _C_UINT     'I'
#define _C_LNG      'l'
#define _C_ULNG     'L'
#define _C_LNG_LNG  'q'
#define _C_ULNG_LNG 'Q'
#define _C_FLT      'f'
#define _C_DBL      'd'
#define _C_LNG_DBL  'D'
#define _C_BFLD     'b'
#define _C_BOOL     'B'
#define _C_VOID     'v'
#define _C_UNDEF    '?'
#define _C_PTR      '^'
#define _C_CHARPTR  '*'
#define _C_ARY_B    '['
#define _C_ARY_E    ']'
#define _C_UNION_B  '('
#define _C_UNION_E  ')'
#define _C_STRUCT_B '{'
#define _C_STRUCT_E '}'
#define _C_VECTOR   '!'
#define _C_COMPLEX  'j'

/* _C_ATOM is never generated by the compiler.  You can treat it as
   equivalent to "*".  */
#define _C_ATOM     '%'

#include "deprecated/objc_error.h"

#include "deprecated/struct_objc_static_instances.h"
#include "deprecated/struct_objc_symtab.h"
#include "deprecated/struct_objc_module.h"
#include "deprecated/struct_objc_ivar.h"
#include "deprecated/struct_objc_ivar_list.h"
#include "deprecated/struct_objc_method.h"
typedef struct objc_method Method, *Method_t;

#include "deprecated/struct_objc_method_list.h"
#include "deprecated/struct_objc_protocol_list.h"

/*
** This is used to assure consistent access to the info field of 
** classes
*/
#ifndef HOST_BITS_PER_LONG
#define HOST_BITS_PER_LONG  (sizeof(long)*8)
#endif 

#define __CLS_INFO(cls) ((cls)->info)
#define __CLS_ISINFO(cls, mask) ((__CLS_INFO(cls)&mask)==mask)
#define __CLS_SETINFO(cls, mask) (__CLS_INFO(cls) |= mask)

/* The structure is of type MetaClass */
#define _CLS_META 0x2L
#define CLS_ISMETA(cls) ((cls)&&__CLS_ISINFO(cls, _CLS_META))


/* The structure is of type Class */
#define _CLS_CLASS 0x1L
#define CLS_ISCLASS(cls) ((cls)&&__CLS_ISINFO(cls, _CLS_CLASS))

/*
** The class is initialized within the runtime.  This means that 
** it has had correct super and sublinks assigned
*/
#define _CLS_RESOLV 0x8L
#define CLS_ISRESOLV(cls) __CLS_ISINFO(cls, _CLS_RESOLV)
#define CLS_SETRESOLV(cls) __CLS_SETINFO(cls, _CLS_RESOLV)

/*
** The class has been send a +initialize message or a such is not 
** defined for this class
*/
#define _CLS_INITIALIZED 0x04L
#define CLS_ISINITIALIZED(cls) __CLS_ISINFO(cls, _CLS_INITIALIZED)
#define CLS_SETINITIALIZED(cls) __CLS_SETINFO(cls, _CLS_INITIALIZED)

/*
** The class number of this class.  This must be the same for both the 
** class and its meta class object
*/
#define CLS_GETNUMBER(cls) (__CLS_INFO(cls) >> (HOST_BITS_PER_LONG/2))
#define CLS_SETNUMBER(cls, num) \
  ({ (cls)->info <<= (HOST_BITS_PER_LONG/2); \
     (cls)->info >>= (HOST_BITS_PER_LONG/2); \
     __CLS_SETINFO(cls, (((unsigned long)num) << (HOST_BITS_PER_LONG/2))); })

#include "deprecated/struct_objc_category.h"

typedef struct objc_category Category, *Category_t;

/* We include message.h for compatibility with the old objc-api.h
   which included the declarations currently in message.h.  The
   Apple/NeXT runtime does not do this and only includes message.h in
   objc-runtime.h.  It does not matter that much since most of the
   definitions in message.h are runtime-specific.  */
#include "message.h"

/*
** This is a hook which is called by objc_lookup_class and
** objc_get_class if the runtime is not able to find the class.
** This may e.g. try to load in the class using dynamic loading.
** The function is guaranteed to be passed a non-NULL name string.
*/
objc_EXPORT Class (*_objc_lookup_class)(const char *name);

/*
** This is a hook which is called by __objc_exec_class every time a class
** or a category is loaded into the runtime.  This may e.g. help a
** dynamic loader determine the classes that have been loaded when
** an object file is dynamically linked in.
*/
objc_EXPORT void (*_objc_load_callback)(Class _class, Category* category);

#include "deprecated/objc_object_alloc.h"

/*
  Standard functions for memory allocation and disposal.  Users should
  use these functions in their ObjC programs so that they work so that
  they work properly with garbage collectors.
*/
objc_EXPORT void *
objc_malloc(size_t size);

/* FIXME: Shouldn't the following be called objc_malloc_atomic ?  The
   GC function is GC_malloc_atomic() which makes sense.
 */
objc_EXPORT void *
objc_atomic_malloc(size_t size);

objc_EXPORT void *
objc_realloc(void *mem, size_t size);

objc_EXPORT void *
objc_calloc(size_t nelem, size_t size);

objc_EXPORT void
objc_free(void *mem);

#include "deprecated/objc_valloc.h"
#include "deprecated/objc_malloc.h"

#include "deprecated/objc_unexpected_exception.h"

objc_EXPORT Method_t class_get_class_method(MetaClass _class, SEL aSel);

objc_EXPORT Method_t class_get_instance_method(Class _class, SEL aSel);

objc_EXPORT Class class_pose_as(Class impostor, Class superclass);

objc_EXPORT Class objc_get_class(const char *name);

objc_EXPORT Class objc_lookup_class(const char *name);

objc_EXPORT Class objc_next_class(void **enum_state);

objc_EXPORT const char *sel_get_name(SEL selector);

objc_EXPORT const char *sel_get_type(SEL selector);

objc_EXPORT SEL sel_get_uid(const char *name);

objc_EXPORT SEL sel_get_any_uid(const char *name);

objc_EXPORT SEL sel_get_any_typed_uid(const char *name);

objc_EXPORT SEL sel_get_typed_uid(const char *name, const char*);

objc_EXPORT SEL sel_register_name(const char *name);

objc_EXPORT SEL sel_register_typed_name(const char *name, const char*type);


objc_EXPORT BOOL sel_is_mapped (SEL aSel);

extern id class_create_instance(Class _class);

static inline const char *
class_get_class_name(Class _class)
{
  return CLS_ISCLASS(_class)?_class->name:((_class==Nil)?"Nil":0);
}

static inline long
class_get_instance_size(Class _class)
{
  return CLS_ISCLASS(_class)?_class->instance_size:0;
}

static inline MetaClass
class_get_meta_class(Class _class)
{
  return CLS_ISCLASS(_class)?_class->class_pointer:Nil;
}

static inline Class
class_get_super_class(Class _class)
{
  return CLS_ISCLASS(_class)?_class->super_class:Nil;
}

static inline int
class_get_version(Class _class)
{
  return CLS_ISCLASS(_class)?_class->version:-1;
}

static inline BOOL
class_is_class(Class _class)
{
  return CLS_ISCLASS(_class);
}

static inline BOOL
class_is_meta_class(Class _class)
{
  return CLS_ISMETA(_class);
}


static inline void
class_set_version(Class _class, long version)
{
  if (CLS_ISCLASS(_class))
    _class->version = version;
}

static inline void *
class_get_gc_object_type (Class _class)
{
  return CLS_ISCLASS(_class) ? _class->gc_object_type : NULL;
}

/* Mark the instance variable as innaccessible to the garbage collector */
extern void class_ivar_set_gcinvisible (Class _class,
					const char* ivarname,
					BOOL gcInvisible);

objc_EXPORT IMP method_get_imp(Method_t method);

objc_EXPORT IMP get_imp (Class _class, SEL sel);

/* object_copy used to take a single argument in the traditional GNU
   Objective-C Runtime API (the one declared here), but takes 2 in the
   modern API (implemented in the actual runtime).  Define the old
   object_copy in terms of the new one.  */
objc_EXPORT id object_copy (id object, size_t size);
#define object_copy(X) (object_copy ((X), 0))

objc_EXPORT id object_dispose(id object);

static inline Class
object_get_class(id object)
{
  return ((object!=nil)
	  ? (CLS_ISCLASS(object->class_pointer)
	     ? object->class_pointer
	     : (CLS_ISMETA(object->class_pointer)
		? (Class)object
		: Nil))
	  : Nil);
}

static inline const char *
object_get_class_name(id object)
{
  return ((object!=nil)?(CLS_ISCLASS(object->class_pointer)
                         ?object->class_pointer->name
                         :((Class)object)->name)
                       :"Nil");
}

static inline MetaClass
object_get_meta_class(id object)
{
  return ((object!=nil)?(CLS_ISCLASS(object->class_pointer)
                         ?object->class_pointer->class_pointer
                         :(CLS_ISMETA(object->class_pointer)
                           ?object->class_pointer
                           :Nil))
                       :Nil);
}

static inline Class
object_get_super_class
(id object)
{
  return ((object!=nil)?(CLS_ISCLASS(object->class_pointer)
                         ?object->class_pointer->super_class
                         :(CLS_ISMETA(object->class_pointer)
                           ?((Class)object)->super_class
                           :Nil))
                       :Nil);
}

static inline BOOL
object_is_class (id object)
{
  return ((object != nil)  &&  CLS_ISMETA (object->class_pointer));
}
 
static inline BOOL
object_is_instance (id object)
{
  return ((object != nil)  &&  CLS_ISCLASS (object->class_pointer));
}

static inline BOOL
object_is_meta_class (id object)
{
  return ((object != nil)
	  &&  !object_is_instance (object)  
	  &&  !object_is_class (object));
}

#include "deprecated/objc_get_uninstalled_dtable.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* not __objc_api_INCLUDE_GNU */



