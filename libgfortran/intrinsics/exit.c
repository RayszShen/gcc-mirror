/* Implementation of the EXIT intrinsic.
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

/* SUBROUTINE EXIT(STATUS)
   INTEGER, INTENT(IN), OPTIONAL :: STATUS  */

void
prefix(exit_i4) (GFC_INTEGER_4 * status)
{

  if (status == NULL)
    exit(0);
  exit(*status);
}

void
prefix(exit_i8) (GFC_INTEGER_8 * status)
{

  if (status == NULL)
    exit(0);
  exit((int) *status);
}
