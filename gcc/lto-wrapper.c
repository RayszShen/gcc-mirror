/* Wrapper to call lto.  Used by collect2 and the linker plugin.
   Copyright (C) 2009-2014 Free Software Foundation, Inc.

   Factored out of collect2 by Rafael Espindola <espindola@google.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


/* This program is passed a gcc, a list of gcc arguments and a list of
   object files containing IL. It scans the argument list to check if
   we are in whopr mode or not modifies the arguments and needed and
   prints a list of output files on stdout.

   Example:

   $ lto-wrapper gcc/xgcc -B gcc a.o b.o -o test -flto

   The above will print something like
   /tmp/ccwbQ8B2.lto.o

   If WHOPR is used instead, more than one file might be produced
   ./ccXj2DTk.lto.ltrans.o
   ./ccCJuXGv.lto.ltrans.o
*/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "intl.h"
#include "diagnostic.h"
#include "obstack.h"
#include "opts.h"
#include "options.h"
#include "simple-object.h"
#include "lto-section-names.h"
#include "collect-utils.h"

#define OFFLOAD_TARGET_NAMES_ENV	"OFFLOAD_TARGET_NAMES"

enum lto_mode_d {
  LTO_MODE_NONE,			/* Not doing LTO.  */
  LTO_MODE_LTO,				/* Normal LTO.  */
  LTO_MODE_WHOPR			/* WHOPR.  */
};

/* Current LTO mode.  */
static enum lto_mode_d lto_mode = LTO_MODE_NONE;

static char *ltrans_output_file;
static char *flto_out;
static unsigned int nr;
static char **input_names;
static char **output_names;
static char **offload_names;
static const char *ompbegin, *ompend;
static char *makefile;

const char tool_name[] = "lto-wrapper";

/* Delete tempfiles.  Called from utils_cleanup.  */

void
tool_cleanup (bool)
{
  unsigned int i;

  if (ltrans_output_file)
    maybe_unlink (ltrans_output_file);
  if (flto_out)
    maybe_unlink (flto_out);
  if (makefile)
    maybe_unlink (makefile);
  for (i = 0; i < nr; ++i)
    {
      maybe_unlink (input_names[i]);
      if (output_names[i])
	maybe_unlink (output_names[i]);
    }
}

static void
lto_wrapper_cleanup (void)
{
  utils_cleanup (false);
}

/* Unlink a temporary LTRANS file unless requested otherwise.  */

void
maybe_unlink (const char *file)
{
  if (! debug)
    {
      if (unlink_if_ordinary (file)
	  && errno != ENOENT)
	fatal_error ("deleting LTRANS file %s: %m", file);
    }
  else if (verbose)
    fprintf (stderr, "[Leaving LTRANS %s]\n", file);
}

/* Template of LTRANS dumpbase suffix.  */
#define DUMPBASE_SUFFIX ".ltrans18446744073709551615"

/* Create decoded options from the COLLECT_GCC and COLLECT_GCC_OPTIONS
   environment according to LANG_MASK.  */

static void
get_options_from_collect_gcc_options (const char *collect_gcc,
				      const char *collect_gcc_options,
				      unsigned int lang_mask,
				      struct cl_decoded_option **decoded_options,
				      unsigned int *decoded_options_count)
{
  struct obstack argv_obstack;
  char *argv_storage;
  const char **argv;
  int j, k, argc;

  argv_storage = xstrdup (collect_gcc_options);
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, collect_gcc);

  for (j = 0, k = 0; argv_storage[j] != '\0'; ++j)
    {
      if (argv_storage[j] == '\'')
	{
	  obstack_ptr_grow (&argv_obstack, &argv_storage[k]);
	  ++j;
	  do
	    {
	      if (argv_storage[j] == '\0')
		fatal_error ("malformed COLLECT_GCC_OPTIONS");
	      else if (strncmp (&argv_storage[j], "'\\''", 4) == 0)
		{
		  argv_storage[k++] = '\'';
		  j += 4;
		}
	      else if (argv_storage[j] == '\'')
		break;
	      else
		argv_storage[k++] = argv_storage[j++];
	    }
	  while (1);
	  argv_storage[k++] = '\0';
	}
    }

  obstack_ptr_grow (&argv_obstack, NULL);
  argc = obstack_object_size (&argv_obstack) / sizeof (void *) - 1;
  argv = XOBFINISH (&argv_obstack, const char **);

  decode_cmdline_options_to_array (argc, (const char **)argv,
				   lang_mask,
				   decoded_options, decoded_options_count);
  obstack_free (&argv_obstack, NULL);
}

/* Append OPTION to the options array DECODED_OPTIONS with size
   DECODED_OPTIONS_COUNT.  */

static void
append_option (struct cl_decoded_option **decoded_options,
	       unsigned int *decoded_options_count,
	       struct cl_decoded_option *option)
{
  ++*decoded_options_count;
  *decoded_options
    = (struct cl_decoded_option *)
	xrealloc (*decoded_options,
		  (*decoded_options_count
		   * sizeof (struct cl_decoded_option)));
  memcpy (&(*decoded_options)[*decoded_options_count - 1], option,
	  sizeof (struct cl_decoded_option));
}

/* Try to merge and complain about options FDECODED_OPTIONS when applied
   ontop of DECODED_OPTIONS.  */

static void
merge_and_complain (struct cl_decoded_option **decoded_options,
		    unsigned int *decoded_options_count,
		    struct cl_decoded_option *fdecoded_options,
		    unsigned int fdecoded_options_count)
{
  unsigned int i, j;

  /* ???  Merge options from files.  Most cases can be
     handled by either unioning or intersecting
     (for example -fwrapv is a case for unioning,
     -ffast-math is for intersection).  Most complaints
     about real conflicts between different options can
     be deferred to the compiler proper.  Options that
     we can neither safely handle by intersection nor
     unioning would need to be complained about here.
     Ideally we'd have a flag in the opt files that
     tells whether to union or intersect or reject.
     In absence of that it's unclear what a good default is.
     It's also difficult to get positional handling correct.  */

  /* The following does what the old LTO option code did,
     union all target and a selected set of common options.  */
  for (i = 0; i < fdecoded_options_count; ++i)
    {
      struct cl_decoded_option *foption = &fdecoded_options[i];
      switch (foption->opt_index)
	{
	case OPT_SPECIAL_unknown:
	case OPT_SPECIAL_ignore:
	case OPT_SPECIAL_program_name:
	case OPT_SPECIAL_input_file:
	  break;

	default:
	  if (!(cl_options[foption->opt_index].flags & CL_TARGET))
	    break;

	  /* Fallthru.  */
	case OPT_fPIC:
	case OPT_fpic:
	case OPT_fPIE:
	case OPT_fpie:
	case OPT_fcommon:
	case OPT_fexceptions:
	case OPT_fnon_call_exceptions:
	case OPT_fgnu_tm:
	  /* Do what the old LTO code did - collect exactly one option
	     setting per OPT code, we pick the first we encounter.
	     ???  This doesn't make too much sense, but when it doesn't
	     then we should complain.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  break;

	case OPT_ftrapv:
	case OPT_fstrict_overflow:
	case OPT_ffp_contract_:
	  /* For selected options we can merge conservatively.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  /* FP_CONTRACT_OFF < FP_CONTRACT_ON < FP_CONTRACT_FAST,
	     -fno-trapv < -ftrapv,
	     -fno-strict-overflow < -fstrict-overflow  */
	  else if (foption->value < (*decoded_options)[j].value)
	    (*decoded_options)[j] = *foption;
	  break;

	case OPT_fwrapv:
	  /* For selected options we can merge conservatively.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  /* -fwrapv > -fno-wrapv.  */
	  else if (foption->value > (*decoded_options)[j].value)
	    (*decoded_options)[j] = *foption;
	  break;

	case OPT_freg_struct_return:
	case OPT_fpcc_struct_return:
	case OPT_fshort_double:
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    fatal_error ("Option %s not used consistently in all LTO input"
			 " files", foption->orig_option_with_args_text);
	  break;

	case OPT_O:
	case OPT_Ofast:
	case OPT_Og:
	case OPT_Os:
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == OPT_O
		|| (*decoded_options)[j].opt_index == OPT_Ofast
		|| (*decoded_options)[j].opt_index == OPT_Og
		|| (*decoded_options)[j].opt_index == OPT_Os)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  else if ((*decoded_options)[j].opt_index == foption->opt_index
		   && foption->opt_index != OPT_O)
	    /* Exact same options get merged.  */
	    ;
	  else
	    {
	      /* For mismatched option kinds preserve the optimization
	         level only, thus merge it as -On.  This also handles
		 merging of same optimization level -On.  */
	      int level = 0;
	      switch (foption->opt_index)
		{
		case OPT_O:
		  if (foption->arg[0] == '\0')
		    level = MAX (level, 1);
		  else
		    level = MAX (level, atoi (foption->arg));
		  break;
		case OPT_Ofast:
		  level = MAX (level, 3);
		  break;
		case OPT_Og:
		  level = MAX (level, 1);
		  break;
		case OPT_Os:
		  level = MAX (level, 2);
		  break;
		default:
		  gcc_unreachable ();
		}
	      switch ((*decoded_options)[j].opt_index)
		{
		case OPT_O:
		  if ((*decoded_options)[j].arg[0] == '\0')
		    level = MAX (level, 1);
		  else
		    level = MAX (level, atoi ((*decoded_options)[j].arg));
		  break;
		case OPT_Ofast:
		  level = MAX (level, 3);
		  break;
		case OPT_Og:
		  level = MAX (level, 1);
		  break;
		case OPT_Os:
		  level = MAX (level, 2);
		  break;
		default:
		  gcc_unreachable ();
		}
	      (*decoded_options)[j].opt_index = OPT_O;
	      char *tem;
	      asprintf (&tem, "-O%d", level);
	      (*decoded_options)[j].arg = &tem[2];
	      (*decoded_options)[j].canonical_option[0] = tem;
	      (*decoded_options)[j].value = 1;
	    }
	  break;
	}
    }
}

/* Auxiliary function that frees elements of PTR and PTR itself.
   N is number of elements to be freed.
   If PTR is NULL, nothing is freed.  If an element is NULL, subsequent elements
   are not freed.  */
static void**
free_array_of_ptrs (void **ptr, unsigned n)
{
  unsigned i;
  if (!ptr)
    return NULL;
  for (i = 0; i < n; i++)
    {
      if (!ptr[i])
	break;
      free (ptr[i]);
    }
  free (ptr);
  return NULL;
}

/* Parse STR, saving found tokens into PVALUES and return their number.
   Tokens are assumed to be delimited by ':'.  If APPEND is non-null,
   append it to every token we find.  */

static unsigned
parse_env_var (const char *str, char ***pvalues, const char *append)
{
  const char *curval, *nextval;
  char **values;
  unsigned num = 1, i;

  curval = strchr (str, ':');
  while (curval)
    {
      num++;
      curval = strchr (curval + 1, ':');
    }

  values = (char**) xmalloc (num * sizeof (char*));
  curval = str;
  nextval = strchrnul (curval, ':');

  int append_len = append ? strlen (append) : 0;
  for (i = 0; i < num; i++)
    {
      int l = nextval - curval;
      values[i] = (char*) xmalloc (l + 1 + append_len);
      memcpy (values[i], curval, l);
      values[i][l] = 0;
      if (append)
	strcat (values[i], append);
      curval = nextval + 1;
      nextval = strchrnul (curval, ':');
    }
  *pvalues = values;
  return num;
}

/* Check whether NAME can be accessed in MODE.  This is like access,
   except that it never considers directories to be executable.  */

static int
access_check (const char *name, int mode)
{
  if (mode == X_OK)
    {
      struct stat st;

      if (stat (name, &st) < 0
	  || S_ISDIR (st.st_mode))
	return -1;
    }

  return access (name, mode);
}

/* Prepare target image for target NAME.
   Firstly, we execute COMPILER, passing all input files to it to produce DSO.
   When target DSO is ready, we pass it to objcopy to place its image into a
   special data section.  After that we rename target image's symbols to values,
   expected by the host side, and return the name of the resultant file.  */

static char*
prepare_target_image (const char *target, const char *compiler_path,
		      unsigned in_argc, char *in_argv[])
{
  const char **argv;
  struct obstack argv_obstack;
  unsigned i;
  char *filename = NULL;
  char *suffix = XALLOCAVEC (char, strlen ("/accel//mkoffload") + 1 + strlen (target));
  const char *compiler = NULL;

  strcpy (suffix, "/accel/");
  strcat (suffix, target);
  strcat (suffix, "/mkoffload");

  char **paths;
  int n_paths = parse_env_var (compiler_path, &paths, suffix);

  for (int i = 0; i < n_paths; i++)
    if (access_check (paths[i], X_OK) == 0)
      {
	compiler = paths[i];
	break;
      }

  if (compiler == NULL)
    goto out;

  /* Generate temp file name.  */
  filename = make_temp_file (".target.o");

  /* --------------------------------------  */
  /* Run gcc for target.  */
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, compiler);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, filename);

  for (i = 1; i < in_argc; ++i)
    if (strncmp (in_argv[i], "-fresolution=", sizeof ("-fresolution=") - 1))
      obstack_ptr_grow (&argv_obstack, in_argv[i]);
  obstack_ptr_grow (&argv_obstack, NULL);

  argv = XOBFINISH (&argv_obstack, const char **);
  fork_execute (argv[0], CONST_CAST (char **, argv), true);
  obstack_free (&argv_obstack, NULL);

 out:
  free_array_of_ptrs ((void**) paths, n_paths);
  return filename;
}


/* The main routine dealing with openmp offloading.
   The routine builds a target image for each offloading target.
   IN_ARGC and IN_ARGV specify input files.  As all of them could contain
   omp-sections, we pass them all to target compilers.
   Env-variable OFFLOAD_TARGET_NAMES_ENV describes for which targets we should
   build images.
   This function stores the names of the object files in the OFFLOAD_NAMES
   array.  */

static void
compile_images_for_openmp_targets (unsigned in_argc, char *in_argv[])
{
  char *target_names;
  char **names;
  unsigned num_targets;

  /* Obtain names of offload targets and corresponding compilers.  */
  target_names = getenv (OFFLOAD_TARGET_NAMES_ENV);
  if (!target_names)
    return;

  num_targets = parse_env_var (target_names, &names, NULL);

  const char *compiler_path = getenv ("COMPILER_PATH");
  if (compiler_path == NULL)
    goto out;

  /* Prepare an image for each target.  The array is terminated by a NULL
     entry.  */
  offload_names = XCNEWVEC (char *, num_targets + 1);
  for (unsigned i = 0; i < num_targets; i++)
    {
      offload_names[i] = prepare_target_image (names[i], compiler_path,
					       in_argc, in_argv);
      if (!offload_names[i])
	fatal_error ("problem with building target image for %s: %m",
		     names[i]);
    }

 out:
  free_array_of_ptrs ((void**) names, num_targets);
}

/* Copy a file from SRC to DEST.  */
static void
copy_file (const char *dest, const char *src)
{
  FILE *d = fopen (dest, "wb");
  FILE *s = fopen (src, "rb");
  char buffer[512];
  while (!feof (s))
    {
      size_t len = fread (buffer, 1, 512, s);
      if (ferror (s) != 0)
	fatal_error ("reading input file");
      if (len > 0)
	{
	  fwrite (buffer, 1, len, d);
	  if (ferror (d) != 0)
	    fatal_error ("writing output file");
	}
    }
}

/* Find the omp_begin.o and omp_end.o files in LIBRARY_PATH, make copies
   and store the names of the copies in ompbegin and ompend.  */

static void
find_ompbeginend (void)
{
  char **paths;
  const char *library_path = getenv ("LIBRARY_PATH");
  if (library_path == NULL)
    return;
  int n_paths = parse_env_var (library_path, &paths, "/crtompbegin.o");

  int i;
  for (i = 0; i < n_paths; i++)
    if (access_check (paths[i], R_OK) == 0)
      {
	size_t len = strlen (paths[i]);
	char *tmp = xstrdup (paths[i]);
	strcpy (paths[i] + len - 7, "end.o");
	if (access_check (paths[i], R_OK) != 0)
	  fatal_error ("installation error, can't find crtompend.o");
	/* The linker will delete the filenames we give it, so make
	   copies.  */
	const char *omptmp1 = make_temp_file (".o");
	const char *omptmp2 = make_temp_file (".o");
	copy_file (omptmp1, tmp);
	ompbegin = omptmp1;
	copy_file (omptmp2, paths[i]);
	ompend = omptmp2;
	free (tmp);
	break;
      }
  if (i == n_paths)
    fatal_error ("installation error, can't find crtompbegin.o");

  free_array_of_ptrs ((void**) paths, n_paths);
}

/* Execute gcc. ARGC is the number of arguments. ARGV contains the arguments. */

static void
run_gcc (unsigned argc, char *argv[])
{
  unsigned i, j;
  const char **new_argv;
  const char **argv_ptr;
  char *list_option_full = NULL;
  const char *linker_output = NULL;
  const char *collect_gcc, *collect_gcc_options;
  int parallel = 0;
  int jobserver = 0;
  bool no_partition = false;
  struct cl_decoded_option *fdecoded_options = NULL;
  unsigned int fdecoded_options_count = 0;
  struct cl_decoded_option *decoded_options;
  unsigned int decoded_options_count;
  struct obstack argv_obstack;
  int new_head_argc;
  bool have_offload = false;

  /* Get the driver and options.  */
  collect_gcc = getenv ("COLLECT_GCC");
  if (!collect_gcc)
    fatal_error ("environment variable COLLECT_GCC must be set");
  collect_gcc_options = getenv ("COLLECT_GCC_OPTIONS");
  if (!collect_gcc_options)
    fatal_error ("environment variable COLLECT_GCC_OPTIONS must be set");
  get_options_from_collect_gcc_options (collect_gcc, collect_gcc_options,
					CL_LANG_ALL,
					&decoded_options,
					&decoded_options_count);

  /* Look at saved options in the IL files.  */
  for (i = 1; i < argc; ++i)
    {
      char *data, *p;
      char *fopts;
      int fd;
      const char *errmsg;
      int err;
      off_t file_offset = 0, offset, length;
      long loffset;
      simple_object_read *sobj;
      int consumed;
      struct cl_decoded_option *f2decoded_options;
      unsigned int f2decoded_options_count;
      char *filename = argv[i];
      if ((p = strrchr (argv[i], '@'))
	  && p != argv[i] 
	  && sscanf (p, "@%li%n", &loffset, &consumed) >= 1
	  && strlen (p) == (unsigned int) consumed)
	{
	  filename = XNEWVEC (char, p - argv[i] + 1);
	  memcpy (filename, argv[i], p - argv[i]);
	  filename[p - argv[i]] = '\0';
	  file_offset = (off_t) loffset;
	}
      fd = open (argv[i], O_RDONLY);
      if (fd == -1)
	continue;
      sobj = simple_object_start_read (fd, file_offset, "__GNU_LTO", 
	  			       &errmsg, &err);
      if (!sobj)
	{
	  close (fd);
	  continue;
	}
      if (!simple_object_find_section (sobj, LTO_SECTION_NAME_PREFIX "." "opts",
				       &offset, &length, &errmsg, &err))
	{
	  simple_object_release_read (sobj);
	  close (fd);
	  continue;
	}
      /* We may choose not to write out this .opts section in the future.  In
	 that case we'll have to use something else to look for.  */
      if (simple_object_find_section (sobj, OMP_SECTION_NAME_PREFIX "." "opts",
				      &offset, &length, &errmsg, &err))
	have_offload = true;
      lseek (fd, file_offset + offset, SEEK_SET);
      data = (char *)xmalloc (length);
      read (fd, data, length);
      fopts = data;
      do
	{
	  get_options_from_collect_gcc_options (collect_gcc,
						fopts, CL_LANG_ALL,
						&f2decoded_options,
						&f2decoded_options_count);
	  if (!fdecoded_options)
	    {
	      fdecoded_options = f2decoded_options;
	      fdecoded_options_count = f2decoded_options_count;
	    }
	  else
	    merge_and_complain (&fdecoded_options,
				&fdecoded_options_count,
				f2decoded_options, f2decoded_options_count);

	  fopts += strlen (fopts) + 1;
	}
      while (fopts - data < length);

      free (data);
      simple_object_release_read (sobj);
      close (fd);
    }

  /* Initalize the common arguments for the driver.  */
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, collect_gcc);
  obstack_ptr_grow (&argv_obstack, "-xlto");
  obstack_ptr_grow (&argv_obstack, "-c");

  /* Append compiler driver arguments as far as they were merged.  */
  for (j = 1; j < fdecoded_options_count; ++j)
    {
      struct cl_decoded_option *option = &fdecoded_options[j];

      /* File options have been properly filtered by lto-opts.c.  */
      switch (option->opt_index)
	{
	  /* Drop arguments that we want to take from the link line.  */
	  case OPT_flto_:
	  case OPT_flto:
	  case OPT_flto_partition_:
	      continue;

	  default:
	      break;
	}

      /* For now do what the original LTO option code was doing - pass
	 on any CL_TARGET flag and a few selected others.  */
      switch (option->opt_index)
	{
	case OPT_fPIC:
	case OPT_fpic:
	case OPT_fPIE:
	case OPT_fpie:
	case OPT_fcommon:
	case OPT_fexceptions:
	case OPT_fnon_call_exceptions:
	case OPT_fgnu_tm:
	case OPT_freg_struct_return:
	case OPT_fpcc_struct_return:
	case OPT_fshort_double:
	case OPT_ffp_contract_:
	case OPT_fwrapv:
	case OPT_ftrapv:
	case OPT_fstrict_overflow:
	case OPT_O:
	case OPT_Ofast:
	case OPT_Og:
	case OPT_Os:
	  break;

	default:
	  if (!(cl_options[option->opt_index].flags & CL_TARGET))
	    continue;
	}

      /* Pass the option on.  */
      for (i = 0; i < option->canonical_option_num_elements; ++i)
	obstack_ptr_grow (&argv_obstack, option->canonical_option[i]);
    }

  /* Append linker driver arguments.  Compiler options from the linker
     driver arguments will override / merge with those from the compiler.  */
  for (j = 1; j < decoded_options_count; ++j)
    {
      struct cl_decoded_option *option = &decoded_options[j];

      /* Do not pass on frontend specific flags not suitable for lto.  */
      if (!(cl_options[option->opt_index].flags
	    & (CL_COMMON|CL_TARGET|CL_DRIVER|CL_LTO)))
	continue;

      switch (option->opt_index)
	{
	case OPT_o:
	  linker_output = option->arg;
	  /* We generate new intermediate output, drop this arg.  */
	  continue;

	case OPT_save_temps:
	  debug = 1;
	  break;

	case OPT_v:
	  verbose = 1;
	  break;

	case OPT_flto_partition_:
	  if (strcmp (option->arg, "none") == 0)
	    no_partition = true;
	  break;

	case OPT_flto_:
	  if (strcmp (option->arg, "jobserver") == 0)
	    {
	      jobserver = 1;
	      parallel = 1;
	    }
	  else
	    {
	      parallel = atoi (option->arg);
	      if (parallel <= 1)
		parallel = 0;
	    }
	  /* Fallthru.  */

	case OPT_flto:
	  lto_mode = LTO_MODE_WHOPR;
	  /* We've handled these LTO options, do not pass them on.  */
	  continue;

	case OPT_freg_struct_return:
	case OPT_fpcc_struct_return:
	case OPT_fshort_double:
	  /* Ignore these, they are determined by the input files.
	     ???  We fail to diagnose a possible mismatch here.  */
	  continue;

	default:
	  break;
	}

      /* Pass the option on.  */
      for (i = 0; i < option->canonical_option_num_elements; ++i)
	obstack_ptr_grow (&argv_obstack, option->canonical_option[i]);
    }

  if (no_partition)
    {
      lto_mode = LTO_MODE_LTO;
      jobserver = 0;
      parallel = 0;
    }

  if (linker_output)
    {
      char *output_dir, *base, *name;
      bool bit_bucket = strcmp (linker_output, HOST_BIT_BUCKET) == 0;

      output_dir = xstrdup (linker_output);
      base = output_dir;
      for (name = base; *name; name++)
	if (IS_DIR_SEPARATOR (*name))
	  base = name + 1;
      *base = '\0';

      linker_output = &linker_output[base - output_dir];
      if (*output_dir == '\0')
	{
	  static char current_dir[] = { '.', DIR_SEPARATOR, '\0' };
	  output_dir = current_dir;
	}
      if (!bit_bucket)
	{
	  obstack_ptr_grow (&argv_obstack, "-dumpdir");
	  obstack_ptr_grow (&argv_obstack, output_dir);
	}

      obstack_ptr_grow (&argv_obstack, "-dumpbase");
    }

  /* Remember at which point we can scrub args to re-use the commons.  */
  new_head_argc = obstack_object_size (&argv_obstack) / sizeof (void *);

  if (lto_mode == LTO_MODE_LTO)
    {
      flto_out = make_temp_file (".lto.o");
      if (linker_output)
	obstack_ptr_grow (&argv_obstack, linker_output);
      obstack_ptr_grow (&argv_obstack, "-o");
      obstack_ptr_grow (&argv_obstack, flto_out);
    }
  else 
    {
      const char *list_option = "-fltrans-output-list=";
      size_t list_option_len = strlen (list_option);
      char *tmp;

      if (linker_output)
	{
	  char *dumpbase = (char *) xmalloc (strlen (linker_output)
					     + sizeof (".wpa") + 1);
	  strcpy (dumpbase, linker_output);
	  strcat (dumpbase, ".wpa");
	  obstack_ptr_grow (&argv_obstack, dumpbase);
	}

      if (linker_output && debug)
	{
	  ltrans_output_file = (char *) xmalloc (strlen (linker_output)
						 + sizeof (".ltrans.out") + 1);
	  strcpy (ltrans_output_file, linker_output);
	  strcat (ltrans_output_file, ".ltrans.out");
	}
      else
	ltrans_output_file = make_temp_file (".ltrans.out");
      list_option_full = (char *) xmalloc (sizeof (char) *
		         (strlen (ltrans_output_file) + list_option_len + 1));
      tmp = list_option_full;

      obstack_ptr_grow (&argv_obstack, tmp);
      strcpy (tmp, list_option);
      tmp += list_option_len;
      strcpy (tmp, ltrans_output_file);

      if (jobserver)
	obstack_ptr_grow (&argv_obstack, xstrdup ("-fwpa=jobserver"));
      else if (parallel > 1)
	{
	  char buf[256];
	  sprintf (buf, "-fwpa=%i", parallel);
	  obstack_ptr_grow (&argv_obstack, xstrdup (buf));
	}
      else
        obstack_ptr_grow (&argv_obstack, "-fwpa");
    }

  /* Append the input objects and possible preceding arguments.  */
  for (i = 1; i < argc; ++i)
    obstack_ptr_grow (&argv_obstack, argv[i]);
  obstack_ptr_grow (&argv_obstack, NULL);

  new_argv = XOBFINISH (&argv_obstack, const char **);
  argv_ptr = &new_argv[new_head_argc];
  fork_execute (new_argv[0], CONST_CAST (char **, new_argv), true);

  if (lto_mode == LTO_MODE_LTO)
    {
      printf ("%s\n", flto_out);
      free (flto_out);
      flto_out = NULL;
    }
  else
    {
      FILE *stream = fopen (ltrans_output_file, "r");
      FILE *mstream = NULL;
      struct obstack env_obstack;

      if (!stream)
	fatal_error ("fopen: %s: %m", ltrans_output_file);

      /* Parse the list of LTRANS inputs from the WPA stage.  */
      obstack_init (&env_obstack);
      nr = 0;
      for (;;)
	{
	  const unsigned piece = 32;
	  char *output_name = NULL;
	  char *buf, *input_name = (char *)xmalloc (piece);
	  size_t len;

	  buf = input_name;
cont:
	  if (!fgets (buf, piece, stream))
	    break;
	  len = strlen (input_name);
	  if (input_name[len - 1] != '\n')
	    {
	      input_name = (char *)xrealloc (input_name, len + piece);
	      buf = input_name + len;
	      goto cont;
	    }
	  input_name[len - 1] = '\0';

	  if (input_name[0] == '*')
	    output_name = &input_name[1];

	  nr++;
	  input_names = (char **)xrealloc (input_names, nr * sizeof (char *));
	  output_names = (char **)xrealloc (output_names, nr * sizeof (char *));
	  input_names[nr-1] = input_name;
	  output_names[nr-1] = output_name;
	}
      fclose (stream);
      maybe_unlink (ltrans_output_file);
      ltrans_output_file = NULL;

      if (parallel)
	{
	  makefile = make_temp_file (".mk");
	  mstream = fopen (makefile, "w");
	}

      /* Execute the LTRANS stage for each input file (or prepare a
	 makefile to invoke this in parallel).  */
      for (i = 0; i < nr; ++i)
	{
	  char *output_name;
	  char *input_name = input_names[i];
	  /* If it's a pass-through file do nothing.  */
	  if (output_names[i])
	    continue;

	  /* Replace the .o suffix with a .ltrans.o suffix and write
	     the resulting name to the LTRANS output list.  */
	  obstack_grow (&env_obstack, input_name, strlen (input_name) - 2);
	  obstack_grow (&env_obstack, ".ltrans.o", sizeof (".ltrans.o"));
	  output_name = XOBFINISH (&env_obstack, char *);

	  /* Adjust the dumpbase if the linker output file was seen.  */
	  if (linker_output)
	    {
	      char *dumpbase
		  = (char *) xmalloc (strlen (linker_output)
				      + sizeof (DUMPBASE_SUFFIX) + 1);
	      snprintf (dumpbase,
			strlen (linker_output) + sizeof (DUMPBASE_SUFFIX),
			"%s.ltrans%u", linker_output, i);
	      argv_ptr[0] = dumpbase;
	    }

	  argv_ptr[1] = "-fltrans";
	  argv_ptr[2] = "-o";
	  argv_ptr[3] = output_name;
	  argv_ptr[4] = input_name;
	  argv_ptr[5] = NULL;
	  if (parallel)
	    {
	      fprintf (mstream, "%s:\n\t@%s ", output_name, new_argv[0]);
	      for (j = 1; new_argv[j] != NULL; ++j)
		fprintf (mstream, " '%s'", new_argv[j]);
	      fprintf (mstream, "\n");
	      /* If we are not preserving the ltrans input files then
	         truncate them as soon as we have processed it.  This
		 reduces temporary disk-space usage.  */
	      if (! debug)
		fprintf (mstream, "\t@-touch -r %s %s.tem > /dev/null 2>&1 "
			 "&& mv %s.tem %s\n",
			 input_name, input_name, input_name, input_name); 
	    }
	  else
	    {
	      fork_execute (new_argv[0], CONST_CAST (char **, new_argv),
			    true);
	      maybe_unlink (input_name);
	    }

	  output_names[i] = output_name;
	}
      if (parallel)
	{
	  struct pex_obj *pex;
	  char jobs[32];

	  fprintf (mstream, "all:");
	  for (i = 0; i < nr; ++i)
	    fprintf (mstream, " \\\n\t%s", output_names[i]);
	  fprintf (mstream, "\n");
	  fclose (mstream);
	  if (!jobserver)
	    {
	      /* Avoid passing --jobserver-fd= and similar flags 
		 unless jobserver mode is explicitly enabled.  */
	      putenv (xstrdup ("MAKEFLAGS="));
	      putenv (xstrdup ("MFLAGS="));
	    }
	  new_argv[0] = getenv ("MAKE");
	  if (!new_argv[0])
	    new_argv[0] = "make";
	  new_argv[1] = "-f";
	  new_argv[2] = makefile;
	  i = 3;
	  if (!jobserver)
	    {
	      snprintf (jobs, 31, "-j%d", parallel);
	      new_argv[i++] = jobs;
	    }
	  new_argv[i++] = "all";
	  new_argv[i++] = NULL;
	  pex = collect_execute (new_argv[0], CONST_CAST (char **, new_argv),
				 NULL, NULL, PEX_SEARCH, false);
	  do_wait (new_argv[0], pex);
	  maybe_unlink (makefile);
	  makefile = NULL;
	  for (i = 0; i < nr; ++i)
	    maybe_unlink (input_names[i]);
	}
      if (have_offload)
	{
	  compile_images_for_openmp_targets (argc, argv);
	  if (offload_names)
	    {
	      find_ompbeginend ();
	      for (i = 0; offload_names[i]; i++)
		{
		  fputs (offload_names[i], stdout);
		  putc ('\n', stdout);
		}
	      free_array_of_ptrs ((void **)offload_names, i);
	    }
	}
      if (ompbegin)
	{
	  fputs (ompbegin, stdout);
	  putc ('\n', stdout);
	}

      for (i = 0; i < nr; ++i)
	{
	  fputs (output_names[i], stdout);
	  putc ('\n', stdout);
	  free (input_names[i]);
	}
      if (ompend)
	{
	  fputs (ompend, stdout);
	  putc ('\n', stdout);
	}
      nr = 0;
      free (output_names);
      free (input_names);
      free (list_option_full);
      obstack_free (&env_obstack, NULL);
    }

  obstack_free (&argv_obstack, NULL);
}


/* Entry point.  */

int
main (int argc, char *argv[])
{
  const char *p;

  gcc_obstack_init (&opts_obstack);

  p = argv[0] + strlen (argv[0]);
  while (p != argv[0] && !IS_DIR_SEPARATOR (p[-1]))
    --p;
  progname = p;

  xmalloc_set_program_name (progname);

  if (atexit (lto_wrapper_cleanup) != 0)
    fatal_error ("atexit failed");

  gcc_init_libintl ();

  diagnostic_initialize (global_dc, 0);

  if (signal (SIGINT, SIG_IGN) != SIG_IGN)
    signal (SIGINT, fatal_signal);
#ifdef SIGHUP
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, fatal_signal);
#endif
  if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
    signal (SIGTERM, fatal_signal);
#ifdef SIGPIPE
  if (signal (SIGPIPE, SIG_IGN) != SIG_IGN)
    signal (SIGPIPE, fatal_signal);
#endif
#ifdef SIGCHLD
  /* We *MUST* set SIGCHLD to SIG_DFL so that the wait4() call will
     receive the signal.  A different setting is inheritable */
  signal (SIGCHLD, SIG_DFL);
#endif

  /* We may be called with all the arguments stored in some file and
     passed with @file.  Expand them into argv before processing.  */
  expandargv (&argc, &argv);

  run_gcc (argc, argv);

  return 0;
}
