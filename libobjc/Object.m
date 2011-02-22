/* The implementation of class Object for Objective-C.
   Copyright (C) 1993, 1994, 1995, 1997, 2002, 2009, 2010
   Free Software Foundation, Inc.

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

#include "objc-private/common.h"
#include <stdarg.h>
#include <errno.h>
#include "objc/Object.h"
#include "objc/Protocol.h"
#include "objc/objc-api.h"

@implementation Object

- (Class)class
{
  return object_get_class (self);
}

- (BOOL)isEqual: (id)anObject
{
  return self == anObject;
}

@end

/* The following methods were deprecated in GCC 4.6.0 and will be
   removed in the next GCC release.  */
@implementation Object (Deprecated)

+ initialize
{
  return self;
}

- init
{
  return self;
}

+ new
{
  return [[self alloc] init];
}

+ alloc
{
  return class_create_instance(self);
}

- free
{
  return object_dispose(self);
}

- copy
{
  return [[self shallowCopy] deepen];
}

- shallowCopy
{
  return object_copy(self);
}

- deepen
{
  return self;
}

- deepCopy
{
  return [self copy];
}

- (Class)superClass
{
  return object_get_super_class(self);
}

- (MetaClass)metaClass
{
  return object_get_meta_class(self);
}

- (const char *)name
{
  return object_get_class_name(self);
}

- self
{
  return self;
}

- (unsigned int)hash
{
  return (size_t)self;
}

- (int)compare:(id)anotherObject;
{
  if ([self isEqual:anotherObject])
    return 0;
  // Ordering objects by their address is pretty useless, 
  // so subclasses should override this is some useful way.
  else if ((id)self > anotherObject)
    return 1;
  else 
    return -1;
}

- (BOOL)isMetaClass
{
  return NO;
}

- (BOOL)isClass
{
  return object_is_class(self);
}

- (BOOL)isInstance
{
  return object_is_instance(self);
}

- (BOOL)isKindOf:(Class)aClassObject
{
  Class class;

  for (class = self->isa; class!=Nil; class = class_get_super_class(class))
    if (class==aClassObject)
      return YES;
  return NO;
}

- (BOOL)isMemberOf:(Class)aClassObject
{
  return self->isa==aClassObject;
}

- (BOOL)isKindOfClassNamed:(const char *)aClassName
{
  Class class;

  if (aClassName!=NULL)
    for (class = self->isa; class!=Nil; class = class_get_super_class(class))
      if (!strcmp(class_get_class_name(class), aClassName))
        return YES;
  return NO;
}

- (BOOL)isMemberOfClassNamed:(const char *)aClassName
{
  return ((aClassName!=NULL)
          &&!strcmp(class_get_class_name(self->isa), aClassName));
}

+ (BOOL)instancesRespondTo:(SEL)aSel
{
  return class_get_instance_method(self, aSel) != (Method_t)0;
}

- (BOOL)respondsTo:(SEL)aSel
{
  return ((object_is_instance(self)
           ?class_get_instance_method(self->isa, aSel)
           :class_get_class_method(self->isa, aSel)) != (Method_t)0);
}

+ (IMP)instanceMethodFor:(SEL)aSel
{
  return method_get_imp(class_get_instance_method(self, aSel));
}

// Indicates if the receiving class or instance conforms to the given protocol
// not usually overridden by subclasses
//
// Modified 9/5/94 to always search the class object's protocol list, rather
// than the meta class.

+ (BOOL) conformsTo: (Protocol*)aProtocol
{
  size_t i;
  struct objc_protocol_list* proto_list;
  id parent;

  for (proto_list = ((Class)self)->protocols;
       proto_list; proto_list = proto_list->next)
    {
      for (i=0; i < proto_list->count; i++)
      {
        if ([proto_list->list[i] conformsTo: aProtocol])
          return YES;
      }
    }

  if ((parent = [self superClass]))
    return [parent conformsTo: aProtocol];
  else
    return NO;
}

- (BOOL) conformsTo: (Protocol*)aProtocol
{
  return [[self class] conformsTo:aProtocol];
}

- (IMP)methodFor:(SEL)aSel
{
  return (method_get_imp(object_is_instance(self)
                         ?class_get_instance_method(self->isa, aSel)
                         :class_get_class_method(self->isa, aSel)));
}

+ (struct objc_method_description *)descriptionForInstanceMethod:(SEL)aSel
{
  return ((struct objc_method_description *)
           class_get_instance_method(self, aSel));
}

- (struct objc_method_description *)descriptionForMethod:(SEL)aSel
{
  return ((struct objc_method_description *)
           (object_is_instance(self)
            ?class_get_instance_method(self->isa, aSel)
            :class_get_class_method(self->isa, aSel)));
}

- perform:(SEL)aSel
{
  IMP msg = objc_msg_lookup(self, aSel);
  if (!msg)
    return [self error:"invalid selector passed to %s", sel_get_name(_cmd)];
  return (*msg)(self, aSel);
}

- perform:(SEL)aSel with:anObject
{
  IMP msg = objc_msg_lookup(self, aSel);
  if (!msg)
    return [self error:"invalid selector passed to %s", sel_get_name(_cmd)];
  return (*msg)(self, aSel, anObject);
}

- perform:(SEL)aSel with:anObject1 with:anObject2
{
  IMP msg = objc_msg_lookup(self, aSel);
  if (!msg)
    return [self error:"invalid selector passed to %s", sel_get_name(_cmd)];
  return (*msg)(self, aSel, anObject1, anObject2);
}

- (retval_t)forward:(SEL)aSel :(arglist_t)argFrame
{
  (void) argFrame; /* UNUSED */
  return (retval_t)[self doesNotRecognize: aSel];
}

- (retval_t)performv:(SEL)aSel :(arglist_t)argFrame
{
  return objc_msg_sendv(self, aSel, argFrame);
}

+ poseAs:(Class)aClassObject
{
  return class_pose_as(self, aClassObject);
}

- (Class)transmuteClassTo:(Class)aClassObject
{
  if (object_is_instance(self))
    if (class_is_class(aClassObject))
      if (class_get_instance_size(aClassObject)==class_get_instance_size(isa))
        if ([self isKindOf:aClassObject])
          {
            Class old_isa = isa;
            isa = aClassObject;
            return old_isa;
          }
  return nil;
}

- subclassResponsibility:(SEL)aSel
{
  return [self error:"subclass should override %s", sel_get_name(aSel)];
}

- notImplemented:(SEL)aSel
{
  return [self error:"method %s not implemented", sel_get_name(aSel)];
}

- shouldNotImplement:(SEL)aSel
{
  return [self error:"%s should not implement %s", 
	             object_get_class_name(self), sel_get_name(aSel)];
}

- doesNotRecognize:(SEL)aSel
{
  return [self error:"%s does not recognize %s",
                     object_get_class_name(self), sel_get_name(aSel)];
}

- error:(const char *)aString, ...
{
#define FMT "error: %s (%s)\n%s\n"
  char fmt[(strlen((char*)FMT)+strlen((char*)object_get_class_name(self))
            +((aString!=NULL)?strlen((char*)aString):0)+8)];
  va_list ap;

  sprintf(fmt, FMT, object_get_class_name(self),
                    object_is_instance(self)?"instance":"class",
                    (aString!=NULL)?aString:"");
  va_start(ap, aString);
  objc_verror(self, OBJC_ERR_UNKNOWN, fmt, ap);
  va_end(ap);
  return nil;
#undef FMT
}

+ (int)version
{
  return class_get_version(self);
}

+ setVersion:(int)aVersion
{
  class_set_version(self, aVersion);
  return self;
}

+ (int)streamVersion: (TypedStream*)aStream
{
  if (aStream->mode == OBJC_READONLY)
    return objc_get_stream_class_version (aStream, self);
  else
    return class_get_version (self);
}

// These are used to write or read the instance variables 
// declared in this particular part of the object.  Subclasses
// should extend these, by calling [super read/write: aStream]
// before doing their own archiving.  These methods are private, in
// the sense that they should only be called from subclasses.

- read: (TypedStream*)aStream
{
  (void) aStream; /* UNUSED */
  // [super read: aStream];  
  return self;
}

- write: (TypedStream*)aStream
{
  (void) aStream; /* UNUSED */
  // [super write: aStream];
  return self;
}

- awake
{
  // [super awake];
  return self;
}

@end
