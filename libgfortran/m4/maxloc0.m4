`/* Implementation of the MAXLOC intrinsic
   Copyright 2002 Free Software Foundation, Inc.
   Contributed by Paul Brook <paul@nowt.org>

This file is part of the GNU Fortran 95 runtime library (libgfor).

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
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <limits.h>
#include "libgfortran.h"'

include(iparm.m4)dnl
include(iforeach.m4)dnl

FOREACH_FUNCTION(
`  atype_name maxval;

  maxval = atype_min;'
,
`  if (*base > maxval)
    {
      maxval = *base;
      for (n = 0; n < rank; n++)
        dest[n * dstride] = count[n] + 1;
    }')

MASKED_FOREACH_FUNCTION(
`  atype_name maxval;

  maxval = atype_min;'
,
`  if (*mbase && *base > maxval)
    {
      maxval = *base;
      for (n = 0; n < rank; n++)
        dest[n * dstride] = count[n] + 1;
    }')
