/* Definitions for Intel 386 running Linux-based GNU systems with ELF format.
   Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2001, 2002, 2004, 2005,
   2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
   Contributed by Eric Youngdale.
   Modified for stabs-in-ELF by H.J. Lu.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#define GNU_USER_LINK_EMULATION "elf_i386"
#define GLIBC_DYNAMIC_LINKER "/lib/ld-linux.so.2"

#define MD_UNWIND_SUPPORT "config/i386/linux-unwind.h"
