/* Set up combined include path chain for the preprocessor.
   Copyright (C) 1986, 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   Broken out of cppinit.c and cppfiles.c and rewritten Mar 2003.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "machmode.h"
#include "target.h"
#include "tm.h"
#include "cpplib.h"
#include "prefix.h"
#include "intl.h"
#include "c-incpath.h"
#include "cppdefault.h"

/* Windows does not natively support inodes, and neither does MSDOS.
   Cygwin's emulation can generate non-unique inodes, so don't use it.
   VMS has non-numeric inodes.  */
#ifdef VMS
# define INO_T_EQ(A, B) (!memcmp (&(A), &(B), sizeof (A)))
# define INO_T_COPY(DEST, SRC) memcpy(&(DEST), &(SRC), sizeof (SRC))
#else
# if (defined _WIN32 && ! defined (_UWIN)) || defined __MSDOS__
#  define INO_T_EQ(A, B) 0
# else
#  define INO_T_EQ(A, B) ((A) == (B))
# endif
# define INO_T_COPY(DEST, SRC) (DEST) = (SRC)
#endif

static void add_env_var_paths (const char *, int);
static void add_standard_paths (const char *, const char *, int);
static void free_path (struct cpp_dir *, int);
static void merge_include_chains (cpp_reader *, int);
static struct cpp_dir *remove_duplicates (cpp_reader *, struct cpp_dir *,
					   struct cpp_dir *,
					   struct cpp_dir *, int);

/* Include chains heads and tails.  */
static struct cpp_dir *heads[4];
static struct cpp_dir *tails[4];
static bool quote_ignores_source_dir;
enum { REASON_QUIET = 0, REASON_NOENT, REASON_DUP, REASON_DUP_SYS };

/* Free an element of the include chain, possibly giving a reason.  */
static void
free_path (struct cpp_dir *path, int reason)
{
  switch (reason)
    {
    case REASON_DUP:
    case REASON_DUP_SYS:
      fprintf (stderr, _("ignoring duplicate directory \"%s\"\n"), path->name);
      if (reason == REASON_DUP_SYS)
	fprintf (stderr,
 _("  as it is a non-system directory that duplicates a system directory\n"));
      break;

    case REASON_NOENT:
      fprintf (stderr, _("ignoring nonexistent directory \"%s\"\n"),
	       path->name);
      break;

    case REASON_QUIET:
    default:
      break;
    }

  free (path->name);
  free (path);
}

/* Read ENV_VAR for a PATH_SEPARATOR-separated list of file names; and
   append all the names to the search path CHAIN.  */
static void
add_env_var_paths (const char *env_var, int chain)
{
  char *p, *q, *path;

  GET_ENVIRONMENT (q, env_var);

  if (!q)
    return;

  for (p = q; *q; p = q + 1)
    {
      q = p;
      while (*q != 0 && *q != PATH_SEPARATOR)
	q++;

      if (p == q)
	path = xstrdup (".");
      else
	{
	  path = xmalloc (q - p + 1);
	  memcpy (path, p, q - p);
	  path[q - p] = '\0';
	}

      add_path (path, chain, chain == SYSTEM);
    }
}

/* Append the standard include chain defined in cppdefault.c.  */
static void
add_standard_paths (const char *sysroot, const char *iprefix, int cxx_stdinc)
{
  const struct default_include *p;
  size_t len;

  if (iprefix && (len = cpp_GCC_INCLUDE_DIR_len) != 0)
    {
      /* Look for directories that start with the standard prefix.
	 "Translate" them, ie. replace /usr/local/lib/gcc... with
	 IPREFIX and search them first.  */
      for (p = cpp_include_defaults; p->fname; p++)
	{
	  if (!p->cplusplus || cxx_stdinc)
	    {
	      /* Should we be translating sysrooted dirs too?  Assume
		 that iprefix and sysroot are mutually exclusive, for
		 now.  */
	      if (sysroot && p->add_sysroot)
		continue;
	      if (!strncmp (p->fname, cpp_GCC_INCLUDE_DIR, len))
		{
		  char *str = concat (iprefix, p->fname + len, NULL);
		  add_path (str, SYSTEM, p->cxx_aware);
		}
	    }
	}
    }

  for (p = cpp_include_defaults; p->fname; p++)
    {
      if (!p->cplusplus || cxx_stdinc)
	{
	  char *str;

	  /* Should this directory start with the sysroot?  */
	  if (sysroot && p->add_sysroot)
	    str = concat (sysroot, p->fname, NULL);
	  else
	    str = update_path (p->fname, p->component);

	  add_path (str, SYSTEM, p->cxx_aware);
	}
    }
}

/* For each duplicate path in chain HEAD, keep just the first one.
   Remove each path in chain HEAD that also exists in chain SYSTEM.
   Set the NEXT pointer of the last path in the resulting chain to
   JOIN, unless it duplicates JOIN in which case the last path is
   removed.  Return the head of the resulting chain.  Any of HEAD,
   JOIN and SYSTEM can be NULL.  */
static struct cpp_dir *
remove_duplicates (cpp_reader *pfile, struct cpp_dir *head,
		   struct cpp_dir *system, struct cpp_dir *join,
		   int verbose)
{
  struct cpp_dir **pcur, *tmp, *cur;
  struct stat st;

  for (pcur = &head; *pcur; )
    {
      int reason = REASON_QUIET;

      cur = *pcur;

      if (stat (cur->name, &st))
	{
	  /* Dirs that don't exist are silently ignored, unless verbose.  */
	  if (errno != ENOENT)
	    cpp_errno (pfile, DL_ERROR, cur->name);
	  else
	    reason = REASON_NOENT;
	}
      else if (!S_ISDIR (st.st_mode))
	cpp_error_with_line (pfile, DL_ERROR, 0, 0,
			     "%s: not a directory", cur->name);
      else
	{
	  INO_T_COPY (cur->ino, st.st_ino);
	  cur->dev  = st.st_dev;

	  /* Remove this one if it is in the system chain.  */
	  reason = REASON_DUP_SYS;
	  for (tmp = system; tmp; tmp = tmp->next)
	    if (INO_T_EQ (tmp->ino, cur->ino) && tmp->dev == cur->dev)
	      break;

	  if (!tmp)
	    {
	      /* Duplicate of something earlier in the same chain?  */
	      reason = REASON_DUP;
	      for (tmp = head; tmp != cur; tmp = tmp->next)
		if (INO_T_EQ (cur->ino, tmp->ino) && cur->dev == tmp->dev)
		  break;

	      if (tmp == cur
		  /* Last in the chain and duplicate of JOIN?  */
		  && !(cur->next == NULL && join
		       && INO_T_EQ (cur->ino, join->ino)
		       && cur->dev == join->dev))
		{
		  /* Unique, so keep this directory.  */
		  pcur = &cur->next;
		  continue;
		}
	    }
	}

      /* Remove this entry from the chain.  */
      *pcur = cur->next;
      free_path (cur, verbose ? reason: REASON_QUIET);
    }

  *pcur = join;
  return head;
}

/* Merge the four include chains together in the order quote, bracket,
   system, after.  Remove duplicate dirs (as determined by
   INO_T_EQ()).

   We can't just merge the lists and then uniquify them because then
   we may lose directories from the <> search path that should be
   there; consider -Ifoo -Ibar -I- -Ifoo -Iquux.  It is however safe
   to treat -Ibar -Ifoo -I- -Ifoo -Iquux as if written -Ibar -I- -Ifoo
   -Iquux.  */
static void
merge_include_chains (cpp_reader *pfile, int verbose)
{
  /* Join the SYSTEM and AFTER chains.  Remove duplicates in the
     resulting SYSTEM chain.  */
  if (heads[SYSTEM])
    tails[SYSTEM]->next = heads[AFTER];
  else
    heads[SYSTEM] = heads[AFTER];
  heads[SYSTEM] = remove_duplicates (pfile, heads[SYSTEM], 0, 0, verbose);

  /* Remove duplicates from BRACKET that are in itself or SYSTEM, and
     join it to SYSTEM.  */
  heads[BRACKET] = remove_duplicates (pfile, heads[BRACKET], heads[SYSTEM],
				      heads[SYSTEM], verbose);

  /* Remove duplicates from QUOTE that are in itself or SYSTEM, and
     join it to BRACKET.  */
  heads[QUOTE] = remove_duplicates (pfile, heads[QUOTE], heads[SYSTEM],
				    heads[BRACKET], verbose);

  /* If verbose, print the list of dirs to search.  */
  if (verbose)
    {
      struct cpp_dir *p;

      fprintf (stderr, _("#include \"...\" search starts here:\n"));
      for (p = heads[QUOTE];; p = p->next)
	{
	  if (p == heads[BRACKET])
	    fprintf (stderr, _("#include <...> search starts here:\n"));
	  if (!p)
	    break;
	  fprintf (stderr, " %s\n", p->name);
	}
      fprintf (stderr, _("End of search list.\n"));
    }
}

/* Use given -I paths for #include "..." but not #include <...>, and
   don't search the directory of the present file for #include "...".
   (Note that -I. -I- is not the same as the default setup; -I. uses
   the compiler's working dir.)  */
void
split_quote_chain (void)
{
  heads[QUOTE] = heads[BRACKET];
  tails[QUOTE] = tails[BRACKET];
  heads[BRACKET] = NULL;
  tails[BRACKET] = NULL;
  /* This is NOT redundant.  */
  quote_ignores_source_dir = true;
}

/* Add PATH to the include chain CHAIN. PATH must be malloc-ed and
   NUL-terminated.  */
void
add_path (char *path, int chain, int cxx_aware)
{
  cpp_dir *p;

  p = xmalloc (sizeof (cpp_dir));
  p->next = NULL;
  p->name = path;
  if (chain == SYSTEM || chain == AFTER)
    p->sysp = 1 + !cxx_aware;
  else
    p->sysp = 0;
  p->construct = 0;

  if (tails[chain])
    tails[chain]->next = p;
  else
    heads[chain] = p;
  tails[chain] = p;
}

void
add_system_path (cpp_dir *p)
{
  if (tails[SYSTEM])
    tails[SYSTEM]->next = p;
  else
    heads[SYSTEM] = p;
  tails[SYSTEM] = p;
}

void
add_bracket_path (cpp_dir *p)
{
  if (tails[BRACKET])
    tails[BRACKET]->next = p;
  else
    heads[BRACKET] = p;
  tails[BRACKET] = p;
}

/* Exported function to handle include chain merging, duplicate
   removal, and registration with cpplib.  */
void
register_include_chains (cpp_reader *pfile, const char *sysroot,
			 const char *iprefix, int stdinc, int cxx_stdinc,
			 int verbose)
{
  static const char *const lang_env_vars[] =
    { "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH",
      "OBJC_INCLUDE_PATH", "OBJCPLUS_INCLUDE_PATH" };
  cpp_options *cpp_opts = cpp_get_options (pfile);
  size_t idx = (cpp_opts->objc ? 2: 0);

  if (cpp_opts->cplusplus)
    idx++;
  else
    cxx_stdinc = false;

  /* CPATH and language-dependent environment variables may add to the
     include chain.  */
  add_env_var_paths ("CPATH", BRACKET);
  add_env_var_paths (lang_env_vars[idx], SYSTEM);

  /* Finally chain on the standard directories.  */
  if (stdinc)
    add_standard_paths (sysroot, iprefix, cxx_stdinc);

  if (targetm.extra_includes)
    targetm.extra_includes (stdinc);

  merge_include_chains (pfile, verbose);

  cpp_set_include_chains (pfile, heads[QUOTE], heads[BRACKET],
			  quote_ignores_source_dir);
}
