/* Copyright (C) 1999, 2003, 2004 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

#undef  TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (MIPS, VxWorks syntax)");

/* Combination of mips.h and svr4.h.  */
#undef  SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR)          \
  (DEFAULT_SWITCH_TAKES_ARG (CHAR)      \
   || (CHAR) == 'G'                     \
   || (CHAR) == 'h'                     \
   || (CHAR) == 'x'                     \
   || (CHAR) == 'z')

#undef  ASM_SPEC
#define ASM_SPEC "\
%{!G:-G 0} %{G*} %(endian_spec) %{mips1} %{mips2} %{mips3} %{mips4} \
%{mips32} %{mips32r2} %{mips64} \
%{mips16:%{!mno-mips16:-mips16}} %{mno-mips16:-no-mips16} \
%(subtarget_asm_optimizing_spec) \
%(subtarget_asm_debugging_spec) \
%{mabi=*} %{!mabi*: %(asm_abi_default_spec)} \
%{mgp32} %{mgp64} %{march=*} %{mxgot:-xgot} \
%{mtune=*} %{v} \
%(subtarget_asm_spec)"

#undef LINK_SPEC
/* LINK_SPEC is clobbered in svr4.h. ugh!  */
#define LINK_SPEC "\
%(endian_spec) \
%{!G:-G 0} %{G*} %{mips1} %{mips2} %{mips3} %{mips4} %{mips32} %{mips64} \
%{bestGnum}" \
VXWORKS_LINK_SPEC

#undef  LIB_SPEC
#define LIB_SPEC VXWORKS_LIB_SPEC
#undef  STARTFILE_SPEC
#define STARTFILE_SPEC VXWORKS_STARTFILE_SPEC
#undef  ENDFILE_SPEC
#define ENDFILE_SPEC VXWORKS_ENDFILE_SPEC

#define TARGET_OS_CPP_BUILTINS()                        \
  do                                                    \
    {                                                   \
      if (TARGET_64BIT)					\
	builtin_define ("CPU=MIPS64");			\
      else						\
	builtin_define ("CPU=MIPS32");			\
      if (TARGET_BIG_ENDIAN)				\
	builtin_define ("MIPSEB");			\
      else						\
	builtin_define ("MIPSEL");			\
      if (TARGET_SOFT_FLOAT)				\
	builtin_define ("SOFT_FLOAT");			\
      VXWORKS_OS_CPP_BUILTINS ();			\
    }                                                   \
  while (0)

#undef SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC VXWORKS_ADDITIONAL_CPP_SPEC

/* No sdata.  */
#undef MIPS_DEFAULT_GVALUE
#define MIPS_DEFAULT_GVALUE 0

/* Other formats are already disabled in config/vxworks.h.  */
#undef MIPS_DEBUGGING_INFO

/* No _mcount profiling on VxWorks.  */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER VXWORKS_FUNCTION_PROFILER
