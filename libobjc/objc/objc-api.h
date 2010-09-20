/* GNU Objective-C Runtime API.
   Copyright (C) 1993, 1995, 1996, 1997, 2001, 2002, 2003, 2004, 2005,
   2007, 2009 Free Software Foundation, Inc.

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

/* For functions which return Method_t */
#define METHOD_NULL	(Method_t)0
                                                /* Boolean typedefs */
/* Method descriptor returned by introspective Object methods.
   This is really just the first part of the more complete objc_method
   structure defined below and used internally by the runtime.  */
struct objc_method_description
{
    SEL name;			/* this is a selector, not a string */
    char *types;		/* type encoding */
};

/* Filer types used to describe Ivars and Methods.  */
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
#define _C_BFLD     'b'
#define _C_BOOL	    'B'
#define _C_VOID     'v'
#define _C_UNDEF    '?'
#define _C_PTR      '^'
#define _C_CHARPTR  '*'
#define _C_ATOM     '%'
#define _C_ARY_B    '['
#define _C_ARY_E    ']'
#define _C_UNION_B  '('
#define _C_UNION_E  ')'
#define _C_STRUCT_B '{'
#define _C_STRUCT_E '}'
#define _C_VECTOR   '!'
#define _C_COMPLEX   'j'

#include "deprecated/objc_error.h"

/* For every class which happens to have statically allocated instances in
   this module, one OBJC_STATIC_INSTANCES is allocated by the compiler.
   INSTANCES is NULL terminated and points to all statically allocated
   instances of this class.  */
struct objc_static_instances
{
  char *class_name;
#ifdef __cplusplus
  id instances[1];
#else
  id instances[0];
#endif
};

/* Whereas a Module (defined further down) is the root (typically) of a file,
   a Symtab is the root of the class and category definitions within the
   module.  
   
   A Symtab contains a variable length array of pointers to classes and
   categories  defined in the module.   */
typedef struct objc_symtab {
  unsigned long sel_ref_cnt;                     /* Unknown. */
  SEL        refs;                              /* Unknown. */
  unsigned short cls_def_cnt;                   /* Number of classes compiled
                                                  (defined) in the module. */
  unsigned short cat_def_cnt;                   /* Number of categories 
                                                  compiled (defined) in the 
                                                  module. */

  void      *defs[1];                           /* Variable array of pointers.
                                                  cls_def_cnt of type Class 
                                                  followed by cat_def_cnt of
                                                  type Category_t, followed
						  by a NULL terminated array
						  of objc_static_instances. */
} Symtab,   *Symtab_t;


/*
** The compiler generates one of these structures for each module that
** composes the executable (eg main.m).  
** 
** This data structure is the root of the definition tree for the module.  
** 
** A collect program runs between ld stages and creates a ObjC ctor array. 
** That array holds a pointer to each module structure of the executable. 
*/
typedef struct objc_module {
  unsigned long version; /* Version of the Module data structure.  */
  unsigned long size;    /* sizeof(Module) according to the compiler -
			    only used to sanity check that it matches
			    sizeof(Module) according to the
			    runtime.  */
  const char* name;      /* Name of the file used to compile the
			    module - not set by modern compilers for
			    security reasons.  */
  Symtab_t    symtab;    /* Pointer to the Symtab of the module.  The
			    Symtab holds an array of pointers to the
			    classes and categories defined in the
			    module. */
} Module, *Module_t;


/*
** The compiler generates one of these structures for a class that has
** instance variables defined in its specification. 
*/
typedef struct objc_ivar {
    const char* ivar_name;                      /* Name of the instance
                                                  variable as entered in the
                                                  class definition. */
    const char* ivar_type;                      /* Description of the Ivar's
                                                  type.  Useful for 
                                                  debuggers. */
    int        ivar_offset;                    /* Byte offset from the base 
                                                  address of the instance 
                                                  structure to the variable. */
} *Ivar_t;

typedef struct objc_ivar_list {
  int   ivar_count;                             /* Number of structures (Ivar) 
                                                  contained in the list.  One
                                                  structure per instance 
                                                  variable defined in the
                                                  class. */
  struct objc_ivar ivar_list[1];               /* Variable length 
                                                  structure. */
} IvarList, *IvarList_t;


/*
** The compiler generates one (or more) of these structures for a class that
** has methods defined in its specification. 
** 
** The implementation of a class can be broken into separate pieces in a file
** and categories can break them across modules. To handle this problem is a
** singly linked list of methods. 
*/
typedef struct objc_method {
  SEL         method_name;                  /* This variable is the method's 
                                               name.  It is a char*. 
                                               The unique integer passed to 
                                               objc_msg_send is a char* too.  
                                               It is compared against 
                                               method_name using strcmp. */
  const char* method_types;                 /* Description of the method's
                                               parameter list.  Useful for
                                               debuggers. */
  IMP         method_imp;                   /* Address of the method in the 
                                               executable. */
} Method, *Method_t;

typedef struct objc_method_list {
  struct objc_method_list*  method_next;    /* This variable is used to link 
                                               a method list to another.  It 
                                               is a singly linked list. */
  int            method_count;              /* Number of methods defined in 
                                               this structure. */
  Method method_list[1];                    /* Variable length 
                                               structure. */
} MethodList, *MethodList_t;

struct objc_protocol_list {
  struct objc_protocol_list *next;
  size_t count;
  Protocol *list[1];
};

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

/*
** The compiler generates one of these structures for each category.  A class
** may have many categories and contain both instance and factory methods.  
*/
typedef struct objc_category {
  const char*   category_name;                /* Name of the category.  Name
                                                contained in the () of the
                                                category definition. */
  const char*   class_name;                   /* Name of the class to which
                                                the category belongs. */
  MethodList_t  instance_methods;             /* Linked list of instance
                                                methods defined in the 
                                                category. NULL indicates no
                                                instance methods defined. */
  MethodList_t  class_methods;                /* Linked list of factory 
                                                methods defined in the
                                                category.  NULL indicates no
                                                class methods defined. */
  struct objc_protocol_list *protocols;	      /* List of Protocols 
					         conformed to */
} Category, *Category_t;

/*
** Structure used when a message is send to a class's super class.  The
** compiler generates one of these structures and passes it to
** objc_msg_super.
*/
typedef struct objc_super {
  id      self;                           /* Id of the object sending
                                                the message. */
#ifdef __cplusplus
  Class super_class;
#else
  Class class;                              /* Object's super class. */
#endif
} Super, *Super_t;

IMP objc_msg_lookup_super(Super_t super, SEL sel);

retval_t objc_msg_sendv(id, SEL, arglist_t);



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

/*
** Hook functions for allocating, copying and disposing of instances
*/
objc_EXPORT id (*_objc_object_alloc)(Class _class);
objc_EXPORT id (*_objc_object_copy)(id object);
objc_EXPORT id (*_objc_object_dispose)(id object);

/*
  Standard functions for memory allocation and disposal.  Users should
  use these functions in their ObjC programs so that they work so that
  they work properly with garbage collectors.
*/
void *
objc_malloc(size_t size);

/* FIXME: Shouldn't the following be called objc_malloc_atomic ?  The
   GC function is GC_malloc_atomic() which makes sense.
 */
void *
objc_atomic_malloc(size_t size);

void *
objc_realloc(void *mem, size_t size);

void *
objc_calloc(size_t nelem, size_t size);

void
objc_free(void *mem);

#include "deprecated/objc_valloc.h"
#include "deprecated/objc_malloc.h"

/*
**  Hooks for method forwarding. This makes it easy to substitute a
**  library, such as ffcall, that implements closures, thereby avoiding
**  gcc's __builtin_apply problems.  __objc_msg_forward2's result will
**  be preferred over that of __objc_msg_forward if both are set and
**  return non-NULL.
*/
objc_EXPORT IMP (*__objc_msg_forward)(SEL);
objc_EXPORT IMP (*__objc_msg_forward2)(id, SEL);

#include "deprecated/objc_unexpected_exception.h"

Method_t class_get_class_method(MetaClass _class, SEL aSel);

Method_t class_get_instance_method(Class _class, SEL aSel);

Class class_pose_as(Class impostor, Class superclass);

Class objc_get_class(const char *name);

Class objc_lookup_class(const char *name);

Class objc_next_class(void **enum_state);

const char *sel_get_name(SEL selector);

const char *sel_get_type(SEL selector);

SEL sel_get_uid(const char *name);

SEL sel_get_any_uid(const char *name);

SEL sel_get_any_typed_uid(const char *name);

SEL sel_get_typed_uid(const char *name, const char*);

SEL sel_register_name(const char *name);

SEL sel_register_typed_name(const char *name, const char*type);


BOOL sel_is_mapped (SEL aSel);

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

static inline IMP
method_get_imp(Method_t method)
{
  return (method!=METHOD_NULL)?method->method_imp:(IMP)0;
}

IMP get_imp (Class _class, SEL sel);

id object_copy(id object);

id object_dispose(id object);

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

struct sarray* 
objc_get_uninstalled_dtable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* not __objc_api_INCLUDE_GNU */



