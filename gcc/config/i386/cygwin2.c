/* Helper routines for cygwin-specific command-line parsing.
   Contributed by Christopher Faylor (cgf@redhat.com)
   Copyright 2002 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "cppdefault.h"
#include <ctype.h>
#include <string.h>

void remove_w32api PARAMS ((void));
void add_mingw PARAMS ((void));

extern char cygwin_tool_include_dir[];
extern char cygwin_gplusplus_tool_include_dir[];
extern char cygwin_cross_include_dir[];
extern char cygwin_standard_include_dir[];

void
add_mingw ()
{
  char **av;
  char *p;
  for (av = cvt_to_mingw; *av; av++)
    {
      int sawcygwin = 0;
      while ((p = strstr (*av, "-cygwin")))
	{
	  char *over = p + sizeof ("-cygwin") - 1;
	  memmove (over + 1, over, strlen (over));
	  memcpy (p, "-mingw32", sizeof("-mingw32") - 1);
	  p = ++over;
	  while (isalnum (*p))
	    p++;
	  strcpy (over, p);
	  sawcygwin = 1;
	}
      if (!sawcygwin && !strstr (*av, "mingw"))
	strcat (*av, "/mingw");
    }
}

static void set_mingw PARAMS((void)) __attribute__ ((constructor));

static void
set_mingw ()
{
  char *env = getenv ("GCC_CYGWIN_MINGW");
  if (env && *env == '1')
    add_mingw ();
}
