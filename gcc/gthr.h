/* Threads compatibily routines for libgcc2.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1997 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

#ifndef __gthr_h
#define __gthr_h

/* If this file is compiled with threads support, it must
       #define __GTHREADS 1
   to indicate that threads support is present.  Also it has define
   function 
     int __gthread_active_p ()
   that returns 1 if thread system is active, 0 if not.

   The threads interface must define the following types:
     __gthread_key_t
     __gthread_once_t
     __gthread_mutex_t

   The threads interface must define the following macros:

     __GTHREAD_ONCE_INIT
     		to initialize __gthread_once_t
     __GTHREAD_MUTEX_INIT
     		to initialize __gthread_mutex_t to get a fast
		non-recursive mutex.

   The threads interface must define the following static functions:

     int __gthread_once (__gthread_once_t *once, void (*func) ())

     int __gthread_key_create (__gthread_key_t *keyp, void (*dtor) (void *))
     int __gthread_key_delete (__gthread_key_t key)

     int __gthread_key_dtor (__gthread_key_t key, void *ptr)

     void *__gthread_getspecific (__gthread_key_t key)
     int __gthread_setspecific (__gthread_key_t key, const void *ptr)

     int __gthread_mutex_lock (__gthread_mutex_t *mutex);
     int __gthread_mutex_trylock (__gthread_mutex_t *mutex);
     int __gthread_mutex_unlock (__gthread_mutex_t *mutex);

   All functions returning int should return 0 on success, -1 on error.

   Currently supported threads packages are
     POSIX threads with -D_PTHREADS
     DCE threads with -D_DCE_THREADS
     Solaris/UI threads with -D_SOLARIS_THREADS
*/

/* Check first for thread specific defines. */
#if _PTHREADS
#include "gthr-posix.h"
#elif _DCE_THREADS
#include "gthr-dce.h"
#elif _SOLARIS_THREADS
#include "gthr-solaris.h"

/* Include GTHREAD_FILE if one is defined. */
#elif SUPPORTS_WEAK && HAVE_GTHR_DEFAULT
#ifndef GTHREAD_USE_WEAK
#define GTHREAD_USE_WEAK 1
#endif
#include "gthr-default.h"

/* Fallback to single thread definitions. */
#else
#include "gthr-single.h"
#endif

#endif /* not __gthr_h */
