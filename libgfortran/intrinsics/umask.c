/* Implementation of the UMASK intrinsic.
   Copyright (C) 2004 Free Software Foundation, Inc.
   Contributed by Steven G. Kargl <kargls@comcast.net>.

This file is part of the GNU Fortran 95 runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libgfor; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include "config.h"
#include "libgfortran.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/* SUBROUTINE UMASK(MASK, OLD)
   INTEGER, INTENT(IN) :: MASK
   INTEGER, INTENT(OUT), OPTIONAL :: OLD  */

extern void umask_i4_sub (GFC_INTEGER_4 *, GFC_INTEGER_4 *);
iexport_proto(umask_i4_sub);

void
umask_i4_sub (GFC_INTEGER_4 *mask, GFC_INTEGER_4 *old)
{
  mode_t val = umask((mode_t) *mask);
  if (old != NULL)
    *old = (GFC_INTEGER_4) val;
}
iexport(umask_i4_sub);

extern void umask_i8_sub (GFC_INTEGER_8 *, GFC_INTEGER_8 *);
iexport_proto(umask_i8_sub);

void
umask_i8_sub (GFC_INTEGER_8 *mask, GFC_INTEGER_8 *old)
{
  mode_t val = umask((mode_t) *mask);
  if (old != NULL)
    *old = (GFC_INTEGER_8) val;
}
iexport(umask_i8_sub);

/* INTEGER FUNCTION UMASK(MASK)
   INTEGER, INTENT(IN) :: MASK  */

extern GFC_INTEGER_4 umask_i4 (GFC_INTEGER_4 *);
export_proto(umask_i4);

GFC_INTEGER_4
umask_i4 (GFC_INTEGER_4 *mask)
{
  GFC_INTEGER_4 old;
  umask_i4_sub (mask, &old);
  return old;
}

extern GFC_INTEGER_8 umask_i8 (GFC_INTEGER_8 *);
export_proto(umask_i8);

GFC_INTEGER_8
umask_i8 (GFC_INTEGER_8 *mask)
{
  GFC_INTEGER_8 old;
  umask_i8_sub (mask, &old);
  return old;
}
