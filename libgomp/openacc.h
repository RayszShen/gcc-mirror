/* OpenACC Runtime Library Declarations

   Copyright (C) 2013 Free Software Foundation, Inc.

   Contributed by Thomas Schwinge <thomas@codesourcery.com>.

   This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _OPENACC_H
#define _OPENACC_H 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
# define __GOACC_NOTHROW throw ()
#else
# define __GOACC_NOTHROW __attribute__ ((__nothrow__))
#endif

typedef enum acc_device_t
  {
    acc_device_none = 0,
    acc_device_default, /* This has to be a distinct value, as no
			   return value can match it.  */
    acc_device_host = 2,
    acc_device_not_host = 3
  } acc_device_t;

int acc_on_device (acc_device_t __dev) __GOACC_NOTHROW;

#ifdef __cplusplus
}
#endif

#endif /* _OPENACC_H */
