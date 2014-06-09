/* Definitions for option handling for TILE-Gx.
   Copyright (C) 2012-2013 Free Software Foundation, Inc.
   Contributed by Walter Lee (walt@tilera.com)

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef TILEGX_OPTS_H
#define TILEGX_OPTS_H

enum cmodel {
  CM_SMALL,	/* Makes various assumpation about sizes of code and
		   data fits.  */
  CM_LARGE,	/* No assumptions.  */
  CM_SMALL_PIC,	/* Makes various assumpation about sizes of code and
		   data fits.  */
  CM_LARGE_PIC	/* No assumptions.  */
};

#endif
