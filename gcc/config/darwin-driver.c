/* Darwin driver program that handles -arch commands and invokes
   appropriate compiler driver. 
   Copyright (C) 2004 Free Software Foundation, Inc.

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mach-o/arch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "libiberty.h"
#include "filenames.h"

/* Hack!.
   Pay the price for including darwin.h.  */
typedef int tree;  

/* Include darwin.h for SWITCH_TAKES_ARG and
   WORD_SWIATCH_TAKES_ARG. */

#include "darwin.h"

/* Include gcc.h for DEFAULT_SWITCH_TAKES_ARG and
   DEFAULT_WORD_SWIATCH_TAKES_ARG. */

#include "gcc.h"

/* This program name.  */
const char *progname;

/* driver prefix.  */
const char *driver_exec_prefix;

/* driver prefix length.  */
int prefix_len;

/* current working directory.  */
char *curr_dir;

/* Use if -o flag is absent.  */
const char *final_output = "a.out";

/* Variabless to track presence and/or absence of important command 
   line options.  */
int compile_only_request = 0;
int asm_output_request = 0;
int preprocessed_output_request = 0;
int ima_is_used = 0;
int dash_dynamiclib_seen = 0;

/* Support at the max 10 arch. at a time. This is historical limit.  */
#define MAX_ARCHES 10

/* Name of user supplied architectures.  */
const char *arches[MAX_ARCHES];

/* -arch counter.  */
static int num_arches;

/* Input filenames.  */
struct input_filename
{
  const char *name;
  int index;
  struct input_filename *next;
};
struct input_filename *in_files;
struct input_filename *last_infile;

static int num_infiles;

/* User specified output file name.  */
const char *output_filename = NULL;

/* Output file names used for arch specific driver invocation. These
   are input file names for 'lipo'.  */
const char **out_files;
static int num_outfiles;

/* Architecture names used by config.guess does not match the names
   used by NXGet... Use this hand coded mapping to connect them.  */
struct arch_config_guess_map
{
  const char *arch_name;
  const char *config_string;
};

struct arch_config_guess_map arch_config_map [] =
{
  {"i386", "i686"},
  {"ppc", "powerpc"},
  {NULL, NULL}
};

/* List of interpreted command line flags. Supply this to gcc driver.  */
const char **new_argv;
int new_argc;

/* Argument list for 'lipo'.  */
const char **lipo_argv;

/* Info about the sub process. Need one subprocess for each arch plus 
   additional one for 'lipo'.  */
struct command
{
  const char *prog;
  const char **argv;
  int pid;
} commands[MAX_ARCHES+1];

/* total number of argc.  */
static int total_argc;

static int greatest_status = 0;
static int signal_count = 0;

#ifndef SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR) DEFAULT_SWITCH_TAKES_ARG(CHAR)
#endif

#ifndef WORD_SWITCH_TAKES_ARG
#define WORD_SWITCH_TAKES_ARG(STR) DEFAULT_WORD_SWITCH_TAKES_ARG (STR)
#endif


/* Local function prototypes.  */
static const char * get_arch_name (const char *);
static char * get_driver_name (const char *);
static void delete_out_files (void);
static char * strip_path_and_suffix (const char *, const char *);
static void initialize (void);
static void final_cleanup (void);
static int do_wait (int, const char *);
static int do_lipo (int, const char *);
static int do_compile (const char **, int);
static int do_compile_separately (void);
static int do_lipo_separately (void);
static int add_arch_options (int, const char **, int);
static int remove_arch_options (const char**, int);

/* Find arch name for the given input string. If input name is NULL then local 
   arch name is used.  */

static const char *
get_arch_name (const char *name)
{
  const NXArchInfo * a_info;
  const NXArchInfo * all_info;
  cpu_type_t cputype;

  /* Find cputype associated with the given name.  */
  if (!name)
    a_info = NXGetLocalArchInfo ();
  else
    a_info = NXGetArchInfoFromName (name);

  if (!a_info)
    abort ();

  cputype = a_info->cputype;

  /* Now collect ALL supported arch info.  */
  all_info = NXGetAllArchInfos ();

  if (!all_info)
    abort ();

  /* Find first arch. that matches cputype.  */
  while (all_info->name)
    {
      if (all_info->cputype == cputype)
	break;
      else
	all_info++;
    }
  
  return all_info->name;
}

/* Find driver name based on input arch name.  */

static char *
get_driver_name (const char *arch_name)
{
  char *driver_name;
  const char *config_name;
  int len;
  int index;
  struct arch_config_guess_map *map;

  /* find config name based on arch name.  */
  config_name = NULL;
  map = arch_config_map;
  while (map->arch_name)
    {
      if (!strcmp (map->arch_name, arch_name))
	{
	  config_name = map->config_string;
	  break;
	}
      else map++;
    }

  if (!config_name)
    abort ();

  len = strlen (config_name) + strlen (PDN) + prefix_len + 1;
  driver_name = (char *) malloc (sizeof (char) * len);

  if (driver_exec_prefix)
    strcpy (driver_name, driver_exec_prefix);
  strcat (driver_name, config_name);
  strcat (driver_name, PDN);

  return driver_name;
}

/* Delete out_files.  */

static void
delete_out_files (void)
{
  const char *temp;
  struct stat st;
  int i = 0;

  for (i = 0, temp = out_files[i]; 
       temp && i < total_argc * MAX_ARCHES; 
       temp = out_files[++i])
    if (stat (temp, &st) >= 0 && S_ISREG (st.st_mode))
      unlink (temp);

}

/* Put fatal error message on stderr and exit.  */

void
fatal (const char *msgid, ...)
{
  va_list ap;

  va_start (ap, msgid);

  fprintf (stderr, "%s: ", progname);
  vfprintf (stderr, msgid, ap);
  va_end (ap);
  fprintf (stderr, "\n");
  delete_out_files ();
  exit (1);
}

/* Print error message and exit.  */

static void
pfatal_pexecute (const char *errmsg_fmt, const char *errmsg_arg)
{
  if (errmsg_arg)
    {
      int save_errno = errno;

      /* Space for trailing '\0' is in %s.  */
      char *msg = (char *) malloc (strlen (errmsg_fmt) + strlen (errmsg_arg));
      sprintf (msg, errmsg_fmt, errmsg_arg);
      errmsg_fmt = msg;

      errno = save_errno;
    }

  fprintf (stderr,"%s: %s: %s", progname, errmsg_fmt, xstrerror (errno));
  delete_out_files ();
  exit (1);
}

#ifdef DEBUG
static void
debug_command_line (const char **debug_argv, int debug_argc)
{
  int i;

  fprintf (stderr,"%s: debug_command_line\n", progname);
  fprintf (stderr,"%s: arg count = %d\n", progname, debug_argc);

  for (i = 0; debug_argv[i]; i++)
    fprintf (stderr,"%s: arg [%d] %s\n", progname, i, debug_argv[i]);    
}
#endif

/* Strip directory name from the input file name and replace file name
   suffix with new.  */

static char *
strip_path_and_suffix (const char *full_name, const char *new_suffix)
{
  char *name;
  char *p;

  if (!full_name || !new_suffix)
    return NULL;

  /* Strip path name.  */
  p = (char *)full_name + strlen (full_name);
  while (p != full_name && !IS_DIR_SEPARATOR (p[-1]))
    --p;

  /* Now 'p' is a file name with suffix.  */
  name = (char *) malloc (strlen (p) + 1 + strlen (new_suffix));

  name = p;

  p = name + strlen (name);
  while (p != name && *p != '.')
    --p;

  /* If did not reach at the beginning of name then '.' is found.
     Replace '.' with NULL.  */
  if (p != name)
    *p = '\0';

  strcat (name, new_suffix);
  return name;
}

/* Initialization */

static void
initialize (void)
{

  int i;

  /* Let's count, how many additional arguments driver driver will supply
     to compiler driver:

     Each "-arch" "<blah>" is replaced by approriate "-mcpu=<blah>".
     That leaves one additional arg space available. 

     Note that only one -m* is supplied to each compiler driver. Which
     means, extra "-arch" "<blah>" are removed from the original command
     line. But lets not count how many additional slots are available.  

     Driver driver may need to specify temp. output file name, say
     "-o" "foobar". That needs two extra argments.

     Sometimes linker wants one additional "-Wl,-arch_multiple".

     Sometimes linker wants to see "-final_output" "outputname". 

     In the end, We need FOUR extra argument.  */

  new_argv = (const char **) malloc ((total_argc + 4) * sizeof (const char *));
  if (!new_argv)
    abort ();

  /* First slot, new_argv[0] is reserved for the driver name.  */
  new_argc = 1;

  /* For each -arch, three arguments are needed.
     For example, "-arch" "ppc" "file".  Additional slots are for
     "lipo" "-create" "-o" and "outputfilename". */
  lipo_argv = (const char **) malloc ((total_argc * 3 + 5) * sizeof (const char *));
  if (!lipo_argv)
    abort ();

  /* Need separate out_files for each arch, max is MAX_ARCHES. 
     Need separate out_files for each input file.  */

  out_files = (const char **) malloc ((total_argc * MAX_ARCHES) * sizeof (const char *));
  if (!out_files)
    abort ();

  num_arches = 0;
  num_infiles = 0;

  in_files = NULL;
  last_infile = NULL;

  for (i = 0; i < (MAX_ARCHES + 1); i++)
    {
      commands[i].prog = NULL;
      commands[i].argv = NULL;
      commands[i].pid = 0;
    }
}

/* Cleanup.  */

static void
final_cleanup (void)
{
  int i;
  struct input_filename *next;
  delete_out_files ();
  free (new_argv);
  free (lipo_argv);
  free (out_files);

  for (i = 0, next = in_files; 
       i < num_infiles && next; 
       i++)
    {
      next = in_files->next;
      free (in_files);
      in_files = next;
    }
}

/* Wait for the process pid and return appropriate code.  */

static int
do_wait (int pid, const char *prog)
{
  int status = 0;
  int ret = 0;

  pid = pwait (pid, &status, 0);

  if (WIFSIGNALED (status))
    {
      if (!signal_count &&
	  WEXITSTATUS (status) > greatest_status)
	greatest_status = WEXITSTATUS (status);
      ret = -1;
    }
  else if (WIFEXITED (status)
	   && WEXITSTATUS (status) >= 1)
    {
      if (WEXITSTATUS (status) > greatest_status)
	greatest_status = WEXITSTATUS (status);
      signal_count++;
      ret = -1;
    }
  return ret;
}

/* Invoke 'lipo' and combine and all output files.  */

static int
do_lipo (int start_outfile_index, const char *out_file)
{
  int i, j, pid;
  char *errmsg_fmt, *errmsg_arg; 

  /* Populate lipo arguments.  */
  lipo_argv[0] = "lipo";
  lipo_argv[1] = "-create";
  lipo_argv[2] = "-o";
  lipo_argv[3] = out_file;

  /* Already 4 lipo arguments are set.  Now add all lipo inputs.  */
  j = 4;
  for (i = 0; i < num_arches; i++)
    {
      lipo_argv[j++] = "-arch";
      lipo_argv[j++] = arches[i];
      lipo_argv[j++] = out_files[start_outfile_index + i];
    }

#ifdef DEBUG
  debug_command_line (lipo_argv, j);
#endif
  
  pid = pexecute (lipo_argv[0], (char *const *)lipo_argv, progname, NULL, &errmsg_fmt,
		  &errmsg_arg, PEXECUTE_SEARCH | PEXECUTE_LAST); 
  
  if (pid == -1)
    pfatal_pexecute (errmsg_fmt, errmsg_arg);
  
  return do_wait (pid, lipo_argv[0]);
}

/* Invoke compiler for all architectures.  */

static int
do_compile (const char **current_argv, int current_argc)
{
  char *errmsg_fmt, *errmsg_arg; 
  int index = 0;
  int ret = 0;
  
  int dash_o_index = current_argc;
  int of_index = current_argc + 1;
  int argc_count = current_argc + 2;

  while (index < num_arches)
    {
      int additional_arch_options = 0;

      current_argv[0] = get_driver_name (get_arch_name (arches[index]));
      
      /* setup output file.  */
      out_files[num_outfiles] = make_temp_file (".out");
      current_argv[dash_o_index] = "-o";
      current_argv[of_index] = out_files [num_outfiles];
      num_outfiles++;

      /* Add arch option as the last option. Do not add any other option
	 before removing this option.  */
      additional_arch_options = add_arch_options (index, current_argv, argc_count);

      commands[index].prog = current_argv[0];
      commands[index].argv = current_argv;

#ifdef DEBUG
      debug_command_line (current_argv, of_index);
#endif
      commands[index].pid = pexecute (current_argv[0], 
				      (char *const *)current_argv, 
				      progname, NULL, 
				      &errmsg_fmt,
				      &errmsg_arg, 
				      PEXECUTE_SEARCH | PEXECUTE_LAST);
      
      if (commands[index].pid == -1)
	pfatal_pexecute (errmsg_fmt, errmsg_arg);
      
      /* Remove the last arch option added in the current_argv list.  */
      if (additional_arch_options)
	remove_arch_options (current_argv, argc_count);
      index++;
    }

  index = 0;
  while (index < num_arches)
    {
      ret = do_wait (commands[index].pid, commands[index].prog);
      fflush (stdout);
      index++;
    }
  return ret;
}

/* Invoke compiler for each input file separately.
   Construct command line for each invocation with one input file.  */

static int
do_compile_separately (void)
{
  const char **new_new_argv;
  int i, new_new_argc;
  struct input_filename *current_ifn;

  if (num_infiles == 1 || ima_is_used)
    abort ();

  /* Total number of arguments in separate compiler invocation is : 
     total number of original arguments - total no input files + one input 
     file + "-o" + output file .  */
  new_new_argv = (const char **) malloc ((new_argc - num_infiles + 4) * sizeof (const char *));
  if (!new_new_argv)
    abort ();

  for (current_ifn = in_files; current_ifn && current_ifn->name; 
       current_ifn = current_ifn->next)
    {
      struct input_filename *ifn = in_files;
      int go_back = 0;
      new_new_argc = 1;

      for (i = 1; i < new_argc; i++)
	{

	  if (ifn && ifn->name && !strcmp (new_argv[i], ifn->name))
	    {
	      /* This argument is one of the input file.  */

 	      if (!strcmp (new_argv[i], current_ifn->name))
		{
		  /* If it is current input file name then add it in the new 
		     list.  */
		  new_new_argv[new_new_argc++] = new_argv[i];
		}
	      /* This input file can  not appear in 
		 again on the command line so next time look for next input
		 file.  */
	      ifn = ifn->next;
	    }
	  else
	    {
	      /* This argument is not a input file name. Add it into new 
		 list.  */
	      new_new_argv[new_new_argc++] = new_argv[i];
	    }
	}

      /* OK now we have only one input file and all other arguments.  */
      do_compile (new_new_argv, new_new_argc);
    }
}

/* Invoke 'lipo' on set of output files and create multile FAT binaries.  */

static int
do_lipo_separately (void)
{
  int ifn_index;
  struct input_filename *ifn;
  for (ifn_index = 0, ifn = in_files; 
       ifn_index < num_infiles && ifn && ifn->name; 
       ifn_index++, ifn = ifn->next)
    do_lipo (ifn_index * num_arches,
	     strip_path_and_suffix (ifn->name, ".o"));
}

/* Replace -arch <blah> options with appropriate "-mcpu=<blah>" OR
   "-march=<blah>".  INDEX is the index in arches[] table. */

static int
add_arch_options (int index, const char **current_argv, int arch_index)
{

  int count;

  /* We are adding 1 argument for selected arches.  */
  count = 1;

#ifdef DEBUG
  fprintf (stderr, "%s: add_arch_options\n", progname);
#endif

  if (!strcmp (arches[index], "ppc601"))
    current_argv[arch_index] = "-mcpu=601";
  else if (!strcmp (arches[index], "ppc603"))
    current_argv[arch_index] = "-mcpu=603";
  else if (!strcmp (arches[index], "ppc604"))
    current_argv[arch_index] = "-mcpu=604";
  else if (!strcmp (arches[index], "ppc604e"))
    current_argv[arch_index] = "-mcpu=604e";
  else if (!strcmp (arches[index], "ppc750"))
    current_argv[arch_index] = "-mcpu=750";
  else if (!strcmp (arches[index], "ppc7400"))
    current_argv[arch_index] = "-mcpu=7400";
  else if (!strcmp (arches[index], "ppc7450"))
    current_argv[arch_index] = "-mcpu=7450";
  else if (!strcmp (arches[index], "ppc970"))
    current_argv[arch_index] = "-mcpu=970";
  else if (!strcmp (arches[index], "i386"))
    current_argv[arch_index] = "-march=i386";
  else if (!strcmp (arches[index], "i486"))
    current_argv[arch_index] = "-march=i486";
  else if (!strcmp (arches[index], "i586"))
    current_argv[arch_index] = "-march=i586";
  else if (!strcmp (arches[index], "i686"))
    current_argv[arch_index] = "-march=i686";
  else if (!strcmp (arches[index], "pentium"))
    current_argv[arch_index] = "-march=pentium";
  else if (!strcmp (arches[index], "pentpro"))
    current_argv[arch_index] = "-march=pentiumpro";
  else if (!strcmp (arches[index], "pentIIm3"))
    current_argv[arch_index] = "-march=pentium3";
  else
    count = 0;

  return count;
}

/* Remove the last option, which is arch option, added by
   add_arch_options.  Return how count of arguments removed.  */
static int
remove_arch_options (const char **current_argv, int arch_index)
{
#ifdef DEBUG
  fprintf (stderr, "%s: Removing argument no %d\n", progname, arch_index);
#endif

  current_argv[arch_index] = '\0';

#ifdef DEBUG
      debug_command_line (current_argv, arch_index);
#endif
  
  return 1;
}

/* Main entry point. This is gcc driver driver!
   Interpret -arch flag from the list of input arguments. Invoke appropriate
   compiler driver. 'lipo' the results if more than one -arch is supplied.  */
int
main (int argc, const char **argv)
{
  size_t i;
  int l, pid, ret, argv_0_len, prog_len;
  char *errmsg_fmt, *errmsg_arg; 

  total_argc = argc;
  argv_0_len = strlen (argv[0]);
  prog_len = 0;

  /* Get the progname, required by pexecute () and program location.  */
  progname = argv[0] + argv_0_len;
  while (progname != argv[0] &&  !IS_DIR_SEPARATOR (progname[-1]))
    {
      prog_len++;
      --progname;
    }

  /* Setup driver prefix.  */
  prefix_len = argv_0_len - prog_len;
  curr_dir = (char *) malloc (sizeof (char) * (prefix_len + 1));
  strncpy (curr_dir, argv[0], prefix_len);
  curr_dir[prefix_len] = '\0';
  driver_exec_prefix = (argv[0], "/usr/bin", curr_dir);

#ifdef DEBUG
  fprintf (stderr,"%s: full progname = %s\n", progname, argv[0]);
  fprintf (stderr,"%s: progname = %s\n", progname, progname);
  fprintf (stderr,"%s: driver_exec_prefix = %s\n", progname, driver_exec_prefix);
#endif

  initialize ();

  /* Process arguments. Take appropriate actions when
     -arch, -c, -S, -E, -o is encountered. Find input file name.  */
  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-arch"))
	{
	  if (i + 1 >= argc)
	    abort ();

	  /*	  arches[num_arches] = get_arch_name (argv[i+1]);*/
	  arches[num_arches] = argv[i+1];

	  num_arches++;
	  i++;
	}
      else if (!strcmp (argv[i], "-c"))
	{
	  new_argv[new_argc++] = argv[i];
	  compile_only_request = 1;
	}
      else if (!strcmp (argv[i], "-S"))
	{
	  new_argv[new_argc++] = argv[i];
	  asm_output_request = 1;
	}
      else if (!strcmp (argv[i], "-E"))
	{
	  new_argv[new_argc++] = argv[i];
	  preprocessed_output_request = 1;
	}
      else if (!strcmp (argv[i], "-dynamiclib"))
	{
	  new_argv[new_argc++] = argv[i];
	  dash_dynamiclib_seen = 1;
	}

      else if (!strcmp (argv[i], "-o"))
	{
	  if (i + 1 >= argc)
	    abort ();

	  output_filename = argv[i+1];
	  i++;
	}
      else if ((! strcmp (argv[i], "-pass-exit-codes"))
	       || (! strcmp (argv[i], "-print-search-dirs"))
	       || (! strcmp (argv[i], "-print-libgcc-file-name"))
	       || (! strncmp (argv[i], "-print-file-name=", 17))
	       || (! strncmp (argv[i], "-print-prog-name=", 17))
	       || (! strcmp (argv[i], "-print-multi-lib"))
	       || (! strcmp (argv[i], "-print-multi-directory"))
	       || (! strcmp (argv[i], "-print-multi-os-directory"))
	       || (! strcmp (argv[i], "-ftarget-help"))
	       || (! strcmp (argv[i], "-fhelp"))
	       || (! strcmp (argv[i], "+e"))
	       || (! strncmp (argv[i], "-Wa,",4))
	       || (! strncmp (argv[i], "-Wp,",4))
	       || (! strncmp (argv[i], "-Wl,",4))
	       || (! strncmp (argv[i], "-l", 2))
	       || (! strncmp (argv[i], "-weak-l", 7))
	       || (! strncmp (argv[i], "-specs=", 7))
	       || (! strcmp (argv[i], "-ObjC"))
	       || (! strcmp (argv[i], "-fobjC"))
	       || (! strcmp (argv[i], "-ObjC++"))
	       || (! strcmp (argv[i], "-time"))
	       || (! strcmp (argv[i], "-###"))
	       || (! strcmp (argv[i], "-fconstant-cfstrings"))
	       || (! strcmp (argv[i], "-fno-constant-cfstrings"))
	       || (! strcmp (argv[i], "-save-temps"))
	       || (! strcmp (argv[i], "-static-libgcc"))
	       || (! strcmp (argv[i], "-shared-libgcc"))
	       || (! strcmp (argv[i], "-pipe"))
	       )
	{
	  new_argv[new_argc++] = argv[i];
	}
      else if ((! strcmp (argv[i], "-Xlinker"))
	       || (! strcmp (argv[i], "-Xassembler"))
	       || (! strcmp (argv[i], "-Xpreprocessor"))
	       || (! strcmp (argv[i], "-l"))
	       || (! strcmp (argv[i], "-weak_library"))
	       || (! strcmp (argv[i], "-weak_framework"))
	       || (! strcmp (argv[i], "-specs"))
	       || (! strcmp (argv[i], "-framework"))
	       )
	{
	  new_argv[new_argc++] = argv[i];
	  i++;
	  new_argv[new_argc++] = argv[i];
	}
      else if (argv[i][0] == '-' && argv[i][1] != 0)
	{
	  const char *p = &argv[i][1];
	  int c = *p;

	  /* First copy this flag itself.  */
	  new_argv[new_argc++] = argv[i];

	  /* Now copy this flag's arguments, if any, appropriately.  */
	  if (c == 'x')
	    {
	      if (p[1] == 0 && i + 1 == argc)
		fatal ("argument to `-x` is missing");

	      if (p[1] == 0)
		{
		  i++;
		  new_argv[new_argc++] = argv[i];
		}
	    }

	  if ((SWITCH_TAKES_ARG (c) > (p[1] != 0)) 
	      || WORD_SWITCH_TAKES_ARG (p))
	    {
	      int j = 0;
	      int n_args = WORD_SWITCH_TAKES_ARG (p);
	      if (n_args == 0)
		{
		  /* Count only the option arguments in separate argv elements.  */
		  n_args = SWITCH_TAKES_ARG (c) - (p[1] != 0);
		}
	      if (i + n_args >= argc)
		fatal ("argument to `-%s' is missing", p);


	      while ( j < n_args)
		{
		  i++;
		  new_argv[new_argc++] = argv[i];
		  j++;
		}
	    }
	    
	}
      else
	{
	  struct input_filename *ifn;
	  new_argv[new_argc++] = argv[i];
	  ifn = (struct input_filename *) malloc (sizeof (struct input_filename));
	  ifn->name = argv[i];
	  ifn->index = i;
	  num_infiles++;

	  if (last_infile)
	      last_infile->next = ifn;
	  else
	    in_files = ifn;

	  last_infile = ifn;
	}
    }

#if 0
  if (num_infiles == 0)
    fatal ("no input files");
#endif

  if (preprocessed_output_request && asm_output_request && num_infiles > 1)
    fatal ("-E and -S are not allowed with multiple -arch flags");

  /* If -arch is not present OR Only one -arch <blah> is specified.  
     Invoke appropriate compiler driver.  FAT build is not required in this
     case.  */ 

  if (num_arches == 0 || num_arches == 1)
    {

      /* If no -arch is specified than use host compiler driver.  */
      if (num_arches == 0)
	new_argv[0] = get_driver_name (get_arch_name (NULL));
      else if (num_arches == 1)
	{
	  /* Find compiler driver based on -arch <foo> and add approriate
	     -m* argument.  */
	  new_argv[0] = get_driver_name (get_arch_name (arches[0]));
	  new_argc = new_argc + add_arch_options (0, new_argv, new_argc);
	}


#ifdef DEBUG
      printf ("%s: invoking single driver name = %s\n", progname, new_argv[0]);
#endif

      /* Re insert output file name.  */
      if (output_filename)
	{
	  new_argv[new_argc++] = "-o";
	  new_argv[new_argc++] = output_filename;
	}

#ifdef DEBUG
      debug_command_line (new_argv, new_argc);
#endif
  
      pid = pexecute (new_argv[0], (char *const *)new_argv, progname, NULL, 
		      &errmsg_fmt, &errmsg_arg, PEXECUTE_SEARCH | PEXECUTE_LAST); 
      
      if (pid == -1)
	pfatal_pexecute (errmsg_fmt, errmsg_arg);

      ret = do_wait (pid, new_argv[0]);
    }
  else
    {
      /* Handle multiple -arch <blah>.  */

      /* If more than one input files are supplied but only one output filename
	 is pressent then IMA will be used.  */
      if (num_infiles > 1 && output_filename)
	ima_is_used = 1;

      /* Linker wants to know this in case of multiple -arch.  */
      if (!compile_only_request && !dash_dynamiclib_seen)
	new_argv[new_argc++] = "-Wl,-arch_multiple";


      /* If only one input file is specified OR IMA is used then expected output
	 is one FAT binary.  */
      if (num_infiles == 1 || ima_is_used)
	{
	  const char *out_file;

	     /* Create output file name based on 
	     input filename, if required.  */
	  if (compile_only_request && !output_filename && num_infiles == 1)
	    out_file = strip_path_and_suffix (in_files->name, ".o");
	  else
	    out_file = (output_filename ? output_filename : final_output);


	  /* Linker wants to know name of output file using one extra arg.  */
	  if (!compile_only_request)
	    {
	      char *oname = (char *)(output_filename ? output_filename : final_output);
	      char *n =  malloc (sizeof (char) * (strlen (oname) + 5));
	      strcpy (n, "-Wl,");
	      strcat (n, oname);
	      new_argv[new_argc++] = "-Wl,-final_output";
	      new_argv[new_argc++] = n;
	    }

	  /* Compile file(s) for each arch and lipo 'em together.  */
	  ret = do_compile (new_argv, new_argc);

	  /* Make FAT binary by combining individual output files for each
	     architecture, using 'lipo'.  */
	  ret = do_lipo (0, out_file);
	}
      else
	{
	  /* Multiple input files are present and IMA is not used.
	     Which means need to generate multiple FAT files.  */
	  ret = do_compile_separately ();
	  ret = do_lipo_separately ();
	}
    }

  final_cleanup ();
  free (curr_dir);
  return greatest_status;
}
