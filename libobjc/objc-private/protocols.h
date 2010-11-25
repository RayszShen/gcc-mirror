/* GNU Objective C Runtime protocols - Private functions
   Copyright (C) 2010 Free Software Foundation, Inc.
   Contributed by Nicola Pero <nicola.pero@meta-innovation.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 3, or (at your option) any later version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef __objc_private_protocols_INCLUDE_GNU
#define __objc_private_protocols_INCLUDE_GNU

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This function needs to be called at startup by init.c.  */
void
__objc_protocols_init (void);

/* This function adds a protocol to the internal hashtable of
   protocols by name, which allows objc_getProtocol(name) to be
   implemented efficiently.  */
void
__objc_protocols_add_protocol (const char *name, Protocol *object);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* not __objc_private_protocols_INCLUDE_GNU */
