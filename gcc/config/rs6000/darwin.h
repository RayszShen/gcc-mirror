/* Target definitions for PowerPC running Darwin (Mac OS X).
   Copyright (C) 1997, 2000, 2001, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Apple Computer Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the
   Free Software Foundation, 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */

#undef  TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (Darwin/PowerPC)");

/* The "Darwin ABI" is mostly like AIX, but with some key differences.  */

#define DEFAULT_ABI ABI_DARWIN

/* The object file format is Mach-O.  */

#define TARGET_OBJECT_FORMAT OBJECT_MACHO

/* We're not ever going to do TOCs.  */

#define TARGET_TOC 0
#define TARGET_NO_TOC 1

/* Darwin switches.  */
/* Use dynamic-no-pic codegen (no picbase reg; not suitable for shlibs.)  */
#define MASK_MACHO_DYNAMIC_NO_PIC 0x00800000

#define TARGET_DYNAMIC_NO_PIC	(target_flags & MASK_MACHO_DYNAMIC_NO_PIC)

/* Handle #pragma weak and #pragma pack.  */
#define HANDLE_SYSV_PRAGMA 1

#define TARGET_OS_CPP_BUILTINS()                \
  do                                            \
    {                                           \
      builtin_define ("__ppc__");               \
      builtin_define ("__POWERPC__");           \
      builtin_define ("__NATURAL_ALIGNMENT__"); \
      /* APPLE LOCAL constant cfstrings */	\
      SUBTARGET_OS_CPP_BUILTINS ();		\
    }                                           \
  while (0)


/*  */
#undef	SUBTARGET_SWITCHES
#define SUBTARGET_SWITCHES						\
  {"dynamic-no-pic",	MASK_MACHO_DYNAMIC_NO_PIC,			\
      N_("Generate code suitable for executables (NOT shared libs)")},	\
  {"no-dynamic-no-pic",	-MASK_MACHO_DYNAMIC_NO_PIC, ""},


/* The Darwin ABI always includes AltiVec, can't be (validly) turned
   off.  */

#define SUBTARGET_OVERRIDE_OPTIONS				  	\
do {									\
  rs6000_altivec_abi = 1;						\
  rs6000_altivec_vrsave = 1;						\
  if (DEFAULT_ABI == ABI_DARWIN)					\
  {									\
    if (MACHO_DYNAMIC_NO_PIC_P)						\
      {									\
        if (flag_pic)							\
            warning ("-mdynamic-no-pic overrides -fpic or -fPIC");	\
        flag_pic = 0;							\
      }									\
    else if (flag_pic == 1)						\
      {									\
        /* Darwin doesn't support -fpic.  */				\
        warning ("-fpic is not supported; -fPIC assumed");		\
        flag_pic = 2;							\
      }									\
    /* APPLE LOCAL long double default size --mrs */			\
    if (rs6000_long_double_size_string == 0)				\
      rs6000_long_double_type_size = 128;				\
  }									\
}while(0)

/* We want -fPIC by default, unless we're using -static to compile for
   the kernel or some such.  */

/* APPLE LOCAL begin gfull gused */
#define CC1_SPEC "\
%{gused: -g -feliminate-unused-debug-symbols %<gused }\
%{gfull: -g -fno-eliminate-unused-debug-symbols %<gfull }\
%{g: %{!gfull: -feliminate-unused-debug-symbols %<gfull }}\
%{static: %{Zdynamic: %e conflicting code gen style switches are used}}\
%{!static:%{!fast:%{!fastf:%{!fastcp:%{!mdynamic-no-pic:-fPIC}}}}}"
/* APPLE LOCAL end gfull gused */

/* APPLE LOCAL begin 3492132 */

/* It's virtually impossible to predict all the possible combinations
   of -mcpu and -maltivec and whatnot, so just supply
   -force_cpusubtype_ALL if any are seen.  Radar 3492132 against the
   assembler is asking for a .machine directive so we could get this
   really right.  */
#define ASM_SPEC " %(darwin_arch_asm_spec)\
  %{Zforce_cpusubtype_ALL:-force_cpusubtype_ALL} \
  %{!Zforce_cpusubtype_ALL:%{maltivec|faltivec:-force_cpusubtype_ALL}}"

#define DARWIN_ARCH_LD_SPEC                                        \
"%{mcpu=601: %{!Zdynamiclib:-arch ppc601} %{Zdynamiclib:-arch_only ppc601}}    \
 %{mcpu=603: %{!Zdynamiclib:-arch ppc603} %{Zdynamiclib:-arch_only ppc603}}    \
 %{mcpu=604: %{!Zdynamiclib:-arch ppc604} %{Zdynamiclib:-arch_only ppc604}}    \
 %{mcpu=604e: %{!Zdynamiclib:-arch ppc604e} %{Zdynamiclib:-arch_only ppc604}}  \
 %{mcpu=750: %{!Zdynamiclib:-arch ppc750} %{Zdynamiclib:-arch_only ppc750}}    \
 %{mcpu=7400: %{!Zdynamiclib:-arch ppc7400} %{Zdynamiclib:-arch_only ppc7400}} \
 %{mcpu=7450: %{!Zdynamiclib:-arch ppc7450} %{Zdynamiclib:-arch_only ppc7450}} \
 %{mcpu=970: %{!Zdynamiclib:-arch ppc970} %{Zdynamiclib:-arch_only ppc970}}    \
 %{mcpu=G5: %{!Zdynamiclib:-arch ppc970} %{Zdynamiclib:-arch_only ppc970}}    \
 %{!mcpu*:%{!march*:%{!Zdynamiclib:-arch ppc} %{Zdynamiclib:-arch_only ppc}}}  "

#define DARWIN_ARCH_ASM_SPEC                                        \
"%{mcpu=601: -arch ppc601}   \
 %{mcpu=603: -arch ppc603}   \
 %{mcpu=604: -arch ppc604}   \
 %{mcpu=604e: -arch ppc604e} \
 %{mcpu=750: -arch ppc750}   \
 %{mcpu=7400: -arch ppc7400} \
 %{mcpu=7450: -arch ppc7450} \
 %{mcpu=970: -arch ppc970}   \
 %{mcpu=G5: -arch ppc970}   \
 %{!mcpu*:%{!march*: -arch ppc}} "

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS			\
  { "darwin_arch_asm_spec",	DARWIN_ARCH_ASM_SPEC },     \
  { "darwin_arch_ld_spec",	DARWIN_ARCH_LD_SPEC },     \
  { "darwin_arch", "ppc" },

/* APPLE LOCAL end 3492132 */

/* The "-faltivec" option should have been called "-maltivec" all along.  */
#define SUBTARGET_OPTION_TRANSLATE_TABLE				\
  { "-faltivec", "-maltivec -include altivec.h" },	\
  { "-fno-altivec", "-mno-altivec" },	\
  { "-Waltivec-long-deprecated",	"-mwarn-altivec-long" }, \
  { "-Wno-altivec-long-deprecated", "-mno-warn-altivec-long" }

/* Make both r2 and r3 available for allocation.  */
#define FIXED_R2 0
#define FIXED_R13 0

/* Base register for access to local variables of the function.  */

#undef  FRAME_POINTER_REGNUM
#define FRAME_POINTER_REGNUM 30

#undef  RS6000_PIC_OFFSET_TABLE_REGNUM
#define RS6000_PIC_OFFSET_TABLE_REGNUM 31

/* APPLE LOCAL begin -pg fix */
/* -pg has a problem which is normally concealed by -fPIC;
   either -mdynamic-no-pic or -static exposes the -pg problem, causing the
   crash.  FSF gcc for Darwin also has this bug.  The problem is that -pg
   causes several int registers to be saved and restored although they may
   not actually be used (config/rs6000/rs6000.c:first_reg_to_save()).  In the
   rare case where none of them is actually used, a consistency check fails
   (correctly).  This cannot happen with -fPIC because the PIC register (R31)
   is always "used" in the sense checked by the consistency check.  The
   easy fix, here, is therefore to mark R31 always "used" whenever -pg is on.
   A better, but harder, fix would be to improve -pg's register-use
   logic along the lines suggested by comments in the function listed above. */
#undef PIC_OFFSET_TABLE_REGNUM
#define PIC_OFFSET_TABLE_REGNUM ((flag_pic || profile_flag) \
    ? RS6000_PIC_OFFSET_TABLE_REGNUM \
    : INVALID_REGNUM)
/* APPLE LOCAL end -pg fix */

/* Pad the outgoing args area to 16 bytes instead of the usual 8.  */

#undef STARTING_FRAME_OFFSET
#define STARTING_FRAME_OFFSET						\
  (RS6000_ALIGN (current_function_outgoing_args_size, 16)		\
   + RS6000_VARARGS_AREA						\
   + RS6000_SAVE_AREA)

#undef STACK_DYNAMIC_OFFSET
#define STACK_DYNAMIC_OFFSET(FUNDECL)					\
  (RS6000_ALIGN (current_function_outgoing_args_size, 16)		\
   + (STACK_POINTER_OFFSET))

/* These are used by -fbranch-probabilities */
#define HOT_TEXT_SECTION_NAME "__TEXT,__text,regular,pure_instructions"
#define NORMAL_TEXT_SECTION_NAME "__TEXT,__text,regular,pure_instructions"
#define UNLIKELY_EXECUTED_TEXT_SECTION_NAME \
                              "__TEXT,__unlikely,regular,pure_instructions"
/* The following is used by hot/cold partitioning to determine whether to
   unconditional branches are "long enough" to span the distance between
   hot and cold sections  (otherwise we have to use indirect jumps).  It 
   is set based on the -mlongcall flag.
   If -mlongcall is set, we use the indirect jumps (the macro below gets '0');
   otherwise we use unconditional branches (the macro below gets '1').  */
#define HAS_LONG_UNCOND_BRANCH (TARGET_LONG_BRANCH ? 0 : 1)

#define SECTION_FORMAT_STRING ".section %s\n\t.align 2\n"

/* APPLE LOCAL begin long branch */
/* Define cutoff for using external functions to save floating point.
   For Darwin, use the function for more than a few registers.  */

/* APPLE LOCAL begin 3414605 */
#undef FP_SAVE_INLINE
#define FP_SAVE_INLINE(FIRST_REG) \
(optimize >= 3   \
|| ((FIRST_REG) > 60 && (FIRST_REG) < 64) \
|| TARGET_LONG_BRANCH)
/* APPLE LOCAL end 3414605 */

/* Define cutoff for using external functions to save vector registers.  */

#undef VECTOR_SAVE_INLINE
#define VECTOR_SAVE_INLINE(FIRST_REG) \
  (((FIRST_REG) >= LAST_ALTIVEC_REGNO - 1 && (FIRST_REG) <= LAST_ALTIVEC_REGNO) \
   || TARGET_LONG_BRANCH)
/* APPLE LOCAL end long branch */

/* The assembler wants the alternate register names, but without
   leading percent sign.  */
#undef REGISTER_NAMES
#define REGISTER_NAMES							\
{									\
     "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",		\
     "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",		\
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",		\
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",		\
     "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",		\
     "f8",  "f9", "f10", "f11", "f12", "f13", "f14", "f15",		\
    "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",		\
    "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",		\
     "mq",  "lr", "ctr",  "ap",						\
    "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",		\
    "xer",								\
     "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",             \
     "v8",  "v9", "v10", "v11", "v12", "v13", "v14", "v15",             \
    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",             \
    "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",             \
    "vrsave", "vscr",							\
    "spe_acc", "spefscr"                                                \
}

/* This outputs NAME to FILE.  */

#undef  RS6000_OUTPUT_BASENAME
#define RS6000_OUTPUT_BASENAME(FILE, NAME)	\
    assemble_name (FILE, NAME)

/* Globalizing directive for a label.  */
#undef GLOBAL_ASM_OP
#define GLOBAL_ASM_OP "\t.globl "
#undef TARGET_ASM_GLOBALIZE_LABEL

/* This is how to output an internal label prefix.  rs6000.c uses this
   when generating traceback tables.  */
/* Not really used for Darwin?  */

#undef ASM_OUTPUT_INTERNAL_LABEL_PREFIX
#define ASM_OUTPUT_INTERNAL_LABEL_PREFIX(FILE,PREFIX)	\
  fprintf (FILE, "%s", PREFIX)

/* This says how to output an assembler line to define a global common
   symbol.  */
/* ? */
#undef  ASM_OUTPUT_ALIGNED_COMMON
#define ASM_OUTPUT_COMMON(FILE, NAME, SIZE, ROUNDED)	\
  do { fputs (".comm ", (FILE));			\
       RS6000_OUTPUT_BASENAME ((FILE), (NAME));		\
       fprintf ((FILE), ","HOST_WIDE_INT_PRINT_UNSIGNED"\n",\
		(SIZE)); } while (0)

/* Override the standard rs6000 definition.  */

#undef ASM_COMMENT_START
#define ASM_COMMENT_START ";"

/* APPLE LOCAL don't define SAVE_FP_PREFIX and friends */

/* This is how to output an assembler line that says to advance
   the location counter to a multiple of 2**LOG bytes using the
   "nop" instruction as padding.  */

#define ASM_OUTPUT_ALIGN_WITH_NOP(FILE,LOG)                   \
  do                                                          \
    {                                                         \
      if ((LOG) < 3)                                          \
        {                                                     \
          ASM_OUTPUT_ALIGN (FILE,LOG);                        \
        }                                                     \
      else /* nop == ori r0,r0,0 */                           \
        fprintf (FILE, "\t.align32 %d,0x60000000\n", (LOG));  \
    } while (0)

/* Generate insns to call the profiler.  */

#define PROFILE_HOOK(LABEL)   output_profile_hook (LABEL)

/* Function name to call to do profiling.  */

#define RS6000_MCOUNT "*mcount"

/* Default processor: a G4.  */

#undef PROCESSOR_DEFAULT
#define PROCESSOR_DEFAULT  PROCESSOR_PPC7400

/* Default target flag settings.  Despite the fact that STMW/LMW
   serializes, it's still a big code size win to use them.  Use FSEL by
   default as well.  */

#undef  TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_POWERPC | MASK_MULTIPLE | MASK_NEW_MNEMONICS \
                      | MASK_PPC_GFXOPT)

/* Since Darwin doesn't do TOCs, stub this out.  */

#define ASM_OUTPUT_SPECIAL_POOL_ENTRY_P(X, MODE)  0

/* Unlike most other PowerPC targets, chars are signed, for
   consistency with other Darwin architectures.  */

#undef DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR (1)

/* Given an rtx X being reloaded into a reg required to be      
   in class CLASS, return the class of reg to actually use.     
   In general this is just CLASS; but on some machines
   in some cases it is preferable to use a more restrictive class.
  
   On the RS/6000, we have to return NO_REGS when we want to reload a
   floating-point CONST_DOUBLE to force it to be copied to memory.

   Don't allow R0 when loading the address of, or otherwise furtling with,
   a SYMBOL_REF.  */

#undef PREFERRED_RELOAD_CLASS
#define PREFERRED_RELOAD_CLASS(X,CLASS)				\
  ((GET_CODE (X) == CONST_DOUBLE				\
    && GET_MODE_CLASS (GET_MODE (X)) == MODE_FLOAT)		\
   ? NO_REGS							\
   : ((GET_CODE (X) == SYMBOL_REF || GET_CODE (X) == HIGH)	\
      && reg_class_subset_p (BASE_REGS, (CLASS)))		\
   ? BASE_REGS							\
   : (GET_MODE_CLASS (GET_MODE (X)) == MODE_INT			\
      && (CLASS) == NON_SPECIAL_REGS)				\
   ? GENERAL_REGS						\
   : (CLASS))

/* APPLE LOCAL begin Macintosh alignment 2002-2-26 --ff */
/* This now supports the Macintosh power, mac68k, and natural 
   alignment modes.  It now has one more parameter than the standard 
   version of the ADJUST_FIELD_ALIGN macro.  
   
   The macro works as follows: We use the computed alignment of the 
   field if we are in the natural alignment mode or if the field is 
   a vector.  Otherwise, if we are in the mac68k alignment mode, we
   use the minimum of the computed alignment and 16 (pegging at
   2-byte alignment).  If we are in the power mode, we peg at 32
   (word alignment) unless it is the first field of the struct, in 
   which case we use the computed alignment.  */
#undef ADJUST_FIELD_ALIGN
#define ADJUST_FIELD_ALIGN(FIELD, COMPUTED, FIRST_FIELD_P)	\
  (TARGET_ALIGN_NATURAL ? (COMPUTED) :				\
   (((COMPUTED) == RS6000_VECTOR_ALIGNMENT)			\
    ? RS6000_VECTOR_ALIGNMENT					\
    : (MIN ((COMPUTED), 					\
    	    (TARGET_ALIGN_MAC68K ? 16 				\
    	    			 : ((FIRST_FIELD_P) ? (COMPUTED) \
    	    			 		    : 32))))))

#undef ROUND_TYPE_ALIGN
/* Macintosh alignment modes require more complicated handling
   of alignment, so we replace the macro with a call to a
   out-of-line function.  */
union tree_node;
extern unsigned round_type_align (union tree_node*, unsigned, unsigned); /* rs6000.c  */
#define ROUND_TYPE_ALIGN(STRUCT, COMPUTED, SPECIFIED)	\
  round_type_align(STRUCT, COMPUTED, SPECIFIED)
/* APPLE LOCAL end Macintosh alignment 2002-2-26 --ff */

/* APPLE LOCAL begin alignment */
/* Make sure local alignments come from the type node, not the mode;
   mode-based alignments are wrong for vectors.  */
#undef LOCAL_ALIGNMENT
#define LOCAL_ALIGNMENT(TYPE, ALIGN)	(MAX ((unsigned) ALIGN,	\
					      TYPE_ALIGN (TYPE)))
/* APPLE LOCAL end alignment */

/* XXX: Darwin supports neither .quad, or .llong, but it also doesn't
   support 64 bit PowerPC either, so this just keeps things happy.  */
#define DOUBLE_INT_ASM_OP "\t.quad\t"

/* APPLE LOCAL begin branch cost */
#undef BRANCH_COST
/* Better code is generated by saying conditional branches take 1 tick.  */
#define BRANCH_COST	1
/* APPLE LOCAL end branch cost */

/* APPLE LOCAL indirect calls in R12 */
/* Address of indirect call must be computed here */
#define MAGIC_INDIRECT_CALL_REG 12

/* For binary compatibility with 2.95; Darwin C APIs use bool from
   stdbool.h, which was an int-sized enum in 2.95.  */
#define BOOL_TYPE_SIZE INT_TYPE_SIZE

/* APPLE LOCAL OS pragma hook */
/* Register generic Darwin pragmas as "OS" pragmas.  */

