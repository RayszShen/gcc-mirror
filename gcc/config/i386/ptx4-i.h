/* Target definitions for GNU compiler for Intel 80386 running Dynix/ptx v4
   Copyright (C) 1996 Free Software Foundation, Inc.

   Modified from sysv4.h
   Originally written by Ron Guilmette (rfg@netcom.com).
   Modified by Tim Wright (timw@sequent.com).

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


#undef TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (i386 Sequent Dynix/ptx Version 4)");

/* The svr4 ABI for the i386 says that records and unions are returned
   in memory.  */

#undef RETURN_IN_MEMORY
#define RETURN_IN_MEMORY(TYPE) \
  (TYPE_MODE (TYPE) == BLKmode \
   || (VECTOR_MODE_P (TYPE_MODE (TYPE)) && int_size_in_bytes (TYPE) == 8))

/* Define which macros to predefine.  _SEQUENT_ is our extension.  */
/* This used to define X86, but james@bigtex.cactus.org says that
   is supposed to be defined optionally by user programs--not by default.  */
#define CPP_PREDEFINES \
  "-Dunix -D_SEQUENT_ -Asystem=unix -Asystem=ptx4"

/* This is how to output assembly code to define a `float' constant.
   We always have to use a .long pseudo-op to do this because the native
   SVR4 ELF assembler is buggy and it generates incorrect values when we
   try to use the .float pseudo-op instead.  */

#undef ASM_OUTPUT_FLOAT
#define ASM_OUTPUT_FLOAT(FILE,VALUE)					\
do { long value;							\
     REAL_VALUE_TO_TARGET_SINGLE ((VALUE), value);			\
     if (sizeof (int) == sizeof (long))					\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value);		\
     else								\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value);			\
   } while (0)

/* This is how to output assembly code to define a `double' constant.
   We always have to use a pair of .long pseudo-ops to do this because
   the native SVR4 ELF assembler is buggy and it generates incorrect
   values when we try to use the .double pseudo-op instead.  */

#undef ASM_OUTPUT_DOUBLE
#define ASM_OUTPUT_DOUBLE(FILE,VALUE)					\
do { long value[2];							\
     REAL_VALUE_TO_TARGET_DOUBLE ((VALUE), value);			\
     if (sizeof (int) == sizeof (long))					\
       {								\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value[0]);		\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value[1]);		\
       }								\
     else								\
       {								\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value[0]);		\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value[1]);		\
       }								\
   } while (0)


#undef ASM_OUTPUT_LONG_DOUBLE
#define ASM_OUTPUT_LONG_DOUBLE(FILE,VALUE)				\
do { long value[3];							\
     REAL_VALUE_TO_TARGET_LONG_DOUBLE ((VALUE), value);			\
     if (sizeof (int) == sizeof (long))					\
       {								\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value[0]);		\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value[1]);		\
         fprintf((FILE), "%s0x%x\n", ASM_LONG, (int) value[2]);		\
       }								\
     else								\
       {								\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value[0]);		\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value[1]);		\
         fprintf((FILE), "%s0x%lx\n", ASM_LONG, value[2]);		\
       }								\
   } while (0)

#undef DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(n)  svr4_dbx_register_map[n]

/* The routine used to output sequences of byte values.  We use a special
   version of this for most svr4 targets because doing so makes the
   generated assembly code more compact (and thus faster to assemble)
   as well as more readable.  Note that if we find subparts of the
   character sequence which end with NUL (and which are shorter than
   STRING_LIMIT) we output those using ASM_OUTPUT_LIMITED_STRING.  */

#undef ASM_OUTPUT_ASCII
#define ASM_OUTPUT_ASCII(FILE, STR, LENGTH)				\
  do									\
    {									\
      register const unsigned char *_ascii_bytes =			\
        (const unsigned char *) (STR);					\
      register const unsigned char *limit = _ascii_bytes + (LENGTH);	\
      register unsigned bytes_in_chunk = 0;				\
      for (; _ascii_bytes < limit; _ascii_bytes++)			\
        {								\
	  register const unsigned char *p;				\
	  if (bytes_in_chunk >= 64)					\
	    {								\
	      fputc ('\n', (FILE));					\
	      bytes_in_chunk = 0;					\
	    }								\
	  for (p = _ascii_bytes; p < limit && *p != '\0'; p++)		\
	    continue;							\
	  if (p < limit && (p - _ascii_bytes) <= (long) STRING_LIMIT)	\
	    {								\
	      if (bytes_in_chunk > 0)					\
		{							\
		  fputc ('\n', (FILE));					\
		  bytes_in_chunk = 0;					\
		}							\
	      ASM_OUTPUT_LIMITED_STRING ((FILE), _ascii_bytes);		\
	      _ascii_bytes = p;						\
	    }								\
	  else								\
	    {								\
	      if (bytes_in_chunk == 0)					\
		fprintf ((FILE), "\t.byte\t");				\
	      else							\
		fputc (',', (FILE));					\
	      fprintf ((FILE), "0x%02x", *_ascii_bytes);		\
	      bytes_in_chunk += 5;					\
	    }								\
	}								\
      if (bytes_in_chunk > 0)						\
        fprintf ((FILE), "\n");						\
    }									\
  while (0)
