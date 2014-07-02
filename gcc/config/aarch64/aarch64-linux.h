/* Machine description for AArch64 architecture.
   Copyright (C) 2009-2014 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_AARCH64_LINUX_H
#define GCC_AARCH64_LINUX_H

#ifndef RUNTIME_ROOT_PREFIX
#define RUNTIME_ROOT_PREFIX ""
#endif
#define GLIBC_DYNAMIC_LINKER RUNTIME_ROOT_PREFIX "/lib/ld-linux-aarch64%{mbig-endian:_be}.so.1"

#define CPP_SPEC "%{pthread:-D_REENTRANT}"


#define LINUX_TARGET_LINK_SPEC  "%{h*}		\
   %{static:-Bstatic}				\
   %{shared:-shared}				\
   %{symbolic:-Bsymbolic}			\
   %{rdynamic:-export-dynamic}			\
   -dynamic-linker " GNU_USER_DYNAMIC_LINKER "	\
   -X						\
   %{mbig-endian:-EB} %{mlittle-endian:-EL}     \
   -maarch64linux%{mbig-endian:b}"

/* Android specific */
#ifndef LINUX_OR_ANDROID_CC
#define ANDROID_TARGET_OS_CPP_BUILTINS()
#endif

#define TARGET_OS_CPP_BUILTINS()		\
  do						\
    {						\
	GNU_USER_TARGET_OS_CPP_BUILTINS();	\
	ANDROID_TARGET_OS_CPP_BUILTINS();	\
    }						\
  while (0)

#define LINK_SPEC LINUX_TARGET_LINK_SPEC

/* Android specific macros. Should probably be 
   moved into android specific header file aarch64-linux-android.h
   and include it in tm.h (see gcc/config.gcc).  */

#ifdef LINUX_OR_ANDROID_CC

#undef  LINK_SPEC
#define LINK_SPEC							\
  LINUX_OR_ANDROID_LD (LINUX_TARGET_LINK_SPEC,				\
		       LINUX_TARGET_LINK_SPEC " " ANDROID_LINK_SPEC)

#undef  CC1_SPEC
#define CC1_SPEC							\
  LINUX_OR_ANDROID_CC (GNU_USER_TARGET_CC1_SPEC,			\
		      GNU_USER_TARGET_CC1_SPEC " " ANDROID_CC1_SPEC("-fpic"))

#define CC1PLUS_SPEC \
  LINUX_OR_ANDROID_CC ("", ANDROID_CC1PLUS_SPEC)

#undef  LIB_SPEC
#define LIB_SPEC							\
  LINUX_OR_ANDROID_LD (GNU_USER_TARGET_LIB_SPEC,			\
		    GNU_USER_TARGET_NO_PTHREADS_LIB_SPEC " " ANDROID_LIB_SPEC)

#undef	STARTFILE_SPEC
#define STARTFILE_SPEC \
  LINUX_OR_ANDROID_LD (GNU_USER_TARGET_STARTFILE_SPEC, ANDROID_STARTFILE_SPEC)

#undef	ENDFILE_SPEC
#define ENDFILE_SPEC \
  LINUX_OR_ANDROID_LD (GNU_USER_TARGET_ENDFILE_SPEC, ANDROID_ENDFILE_SPEC)

#endif

#endif  /* GCC_AARCH64_LINUX_H */
