/* Command line option handling.
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef GCC_OPTS_H
#define GCC_OPTS_H

extern int handle_option (int argc, char **argv, int lang_mask);

struct cl_option
{
  const char *opt_text;
  unsigned char opt_len;
  unsigned int flags;
};

extern const struct cl_option cl_options[];
extern const unsigned int cl_options_count;

#define CL_JOINED		(1 << 24) /* If takes joined argument.  */
#define CL_SEPARATE		(1 << 25) /* If takes a separate argument.  */
#define CL_REJECT_NEGATIVE	(1 << 26) /* Reject no- form.  */
#define CL_COMMON		(1 << 27) /* Language-independent.  */

#endif
