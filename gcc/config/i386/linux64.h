/* Definitions for AMD x86-64 running Linux-based GNU systems with ELF format.
   Copyright (C) 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.
   Contributed by Jan Hubicka <jh@suse.cz>, based on linux.h.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#define GNU_USER_LINK_EMULATION32 "elf_i386"
#define GNU_USER_LINK_EMULATION64 "elf_x86_64"

#ifndef RUNTIME_ROOT_PREFIX
#define RUNTIME_ROOT_PREFIX ""
#endif
#define GLIBC_DYNAMIC_LINKER32 RUNTIME_ROOT_PREFIX "/lib/ld-linux.so.2"
#define GLIBC_DYNAMIC_LINKER64 RUNTIME_ROOT_PREFIX "/lib64/ld-linux-x86-64.so.2"

/* These may be provided by config/linux-grtev2.h.  */
#ifndef LINUX_GRTE_EXTRA_SPECS
#define LINUX_GRTE_EXTRA_SPECS
#endif

#undef  SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS \
  LINUX_GRTE_EXTRA_SPECS

