/* Read and write coverage files, and associated functionality.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1996, 1997, 1998, 1999,
   2000, 2001, 2003, 2004, 2005, 2007, 2008, 2009, 2010
   Free Software Foundation, Inc.
   Contributed by James E. Wilson, UC Berkeley/Cygnus Support;
   based on some ideas from Dain Samples of UC Berkeley.
   Further mangling by Bob Manson, Cygnus Support.
   Further mangled by Nathan Sidwell, CodeSourcery

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


#define GCOV_LINKAGE

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "output.h"
#include "regs.h"
#include "expr.h"
#include "function.h"
#include "basic-block.h"
#include "toplev.h"
#include "tm_p.h"
#include "ggc.h"
#include "coverage.h"
#include "langhooks.h"
#include "hashtab.h"
#include "tree-iterator.h"
#include "cgraph.h"
#include "tree-pass.h"
#include "diagnostic-core.h"
#include "intl.h"
#include "filenames.h"

#include "gcov-io.h"
#include "gcov-io.c"

struct function_list
{
  struct function_list *next;	 /* next function */
  unsigned ident;		 /* function ident */
  unsigned lineno_checksum;	 /* function lineno checksum */
  unsigned cfg_checksum;	 /* function cfg checksum */
  unsigned n_ctrs[GCOV_COUNTERS];/* number of counters.  */
};

/* Counts information for a function.  */
typedef struct counts_entry
{
  /* We hash by  */
  unsigned ident;
  unsigned ctr;

  /* Store  */
  unsigned lineno_checksum;
  unsigned cfg_checksum;
  gcov_type *counts;
  struct gcov_ctr_summary summary;

  /* Workspace */
  struct counts_entry *chain;

} counts_entry_t;

static struct function_list *functions_head = 0;
static struct function_list **functions_tail = &functions_head;
static unsigned no_coverage = 0;

/* Cumulative counter information for whole program.  */
static unsigned prg_ctr_mask; /* Mask of counter types generated.  */
static unsigned prg_n_ctrs[GCOV_COUNTERS]; /* Total counters allocated.  */

/* Counter information for current function.  */
static unsigned fn_ctr_mask; /* Mask of counters used.  */
static unsigned fn_n_ctrs[GCOV_COUNTERS]; /* Counters allocated.  */
static unsigned fn_b_ctrs[GCOV_COUNTERS]; /* Allocation base.  */

/* Name of the output file for coverage output file.  */
static char *bbg_file_name;
static unsigned bbg_file_opened;
static int bbg_function_announced;

/* Name of the count data file.  */
static char *da_file_name;

/* Hash table of count data.  */
static htab_t counts_hash = NULL;

/* Trees representing the counter table arrays.  */
static GTY(()) tree tree_ctr_tables[GCOV_COUNTERS];

/* The names of merge functions for counters.  */
static const char *const ctr_merge_functions[GCOV_COUNTERS] = GCOV_MERGE_FUNCTIONS;
static const char *const ctr_names[GCOV_COUNTERS] = GCOV_COUNTER_NAMES;

/* Forward declarations.  */
static hashval_t htab_counts_entry_hash (const void *);
static int htab_counts_entry_eq (const void *, const void *);
static void htab_counts_entry_del (void *);
static void read_counts_file (void);
static tree build_fn_info_type (unsigned);
static tree build_fn_info_value (const struct function_list *, tree);
static tree build_ctr_info_type (void);
static tree build_ctr_info_value (unsigned, tree);
static tree build_gcov_info (void);
static void create_coverage (void);

/* Return the type node for gcov_type.  */

tree
get_gcov_type (void)
{
  return lang_hooks.types.type_for_size (GCOV_TYPE_SIZE, false);
}

/* Return the type node for gcov_unsigned_t.  */

static tree
get_gcov_unsigned_t (void)
{
  return lang_hooks.types.type_for_size (32, true);
}

static hashval_t
htab_counts_entry_hash (const void *of)
{
  const counts_entry_t *const entry = (const counts_entry_t *) of;

  return entry->ident * GCOV_COUNTERS + entry->ctr;
}

static int
htab_counts_entry_eq (const void *of1, const void *of2)
{
  const counts_entry_t *const entry1 = (const counts_entry_t *) of1;
  const counts_entry_t *const entry2 = (const counts_entry_t *) of2;

  return entry1->ident == entry2->ident && entry1->ctr == entry2->ctr;
}

static void
htab_counts_entry_del (void *of)
{
  counts_entry_t *const entry = (counts_entry_t *) of;

  free (entry->counts);
  free (entry);
}

/* Read in the counts file, if available.  */

static void
read_counts_file (void)
{
  gcov_unsigned_t fn_ident = 0;
  counts_entry_t *summaried = NULL;
  unsigned seen_summary = 0;
  gcov_unsigned_t tag;
  int is_error = 0;
  unsigned lineno_checksum = 0;
  unsigned cfg_checksum = 0;

  if (!gcov_open (da_file_name, 1))
    return;

  if (!gcov_magic (gcov_read_unsigned (), GCOV_DATA_MAGIC))
    {
      warning (0, "%qs is not a gcov data file", da_file_name);
      gcov_close ();
      return;
    }
  else if ((tag = gcov_read_unsigned ()) != GCOV_VERSION)
    {
      char v[4], e[4];

      GCOV_UNSIGNED2STRING (v, tag);
      GCOV_UNSIGNED2STRING (e, GCOV_VERSION);

      warning (0, "%qs is version %q.*s, expected version %q.*s",
 	       da_file_name, 4, v, 4, e);
      gcov_close ();
      return;
    }

  /* Read and discard the stamp.  */
  gcov_read_unsigned ();

  counts_hash = htab_create (10,
			     htab_counts_entry_hash, htab_counts_entry_eq,
			     htab_counts_entry_del);
  while ((tag = gcov_read_unsigned ()))
    {
      gcov_unsigned_t length;
      gcov_position_t offset;

      length = gcov_read_unsigned ();
      offset = gcov_position ();
      if (tag == GCOV_TAG_FUNCTION)
	{
	  fn_ident = gcov_read_unsigned ();
	  lineno_checksum = gcov_read_unsigned ();
	  cfg_checksum = gcov_read_unsigned ();
	  if (seen_summary)
	    {
	      /* We have already seen a summary, this means that this
		 new function begins a new set of program runs. We
		 must unlink the summaried chain.  */
	      counts_entry_t *entry, *chain;

	      for (entry = summaried; entry; entry = chain)
		{
		  chain = entry->chain;
		  entry->chain = NULL;
		}
	      summaried = NULL;
	      seen_summary = 0;
	    }
	}
      else if (tag == GCOV_TAG_PROGRAM_SUMMARY)
	{
	  counts_entry_t *entry;
	  struct gcov_summary summary;

	  gcov_read_summary (&summary);
	  seen_summary = 1;
	  for (entry = summaried; entry; entry = entry->chain)
	    {
	      struct gcov_ctr_summary *csum = &summary.ctrs[entry->ctr];

	      entry->summary.runs += csum->runs;
	      entry->summary.sum_all += csum->sum_all;
	      if (entry->summary.run_max < csum->run_max)
		entry->summary.run_max = csum->run_max;
	      entry->summary.sum_max += csum->sum_max;
	    }
	}
      else if (GCOV_TAG_IS_COUNTER (tag) && fn_ident)
	{
	  counts_entry_t **slot, *entry, elt;
	  unsigned n_counts = GCOV_TAG_COUNTER_NUM (length);
	  unsigned ix;

	  elt.ident = fn_ident;
	  elt.ctr = GCOV_COUNTER_FOR_TAG (tag);

	  slot = (counts_entry_t **) htab_find_slot
	    (counts_hash, &elt, INSERT);
	  entry = *slot;
	  if (!entry)
	    {
	      *slot = entry = XCNEW (counts_entry_t);
	      entry->ident = fn_ident;
	      entry->ctr = elt.ctr;
	      entry->lineno_checksum = lineno_checksum;
	      entry->cfg_checksum = cfg_checksum;
	      entry->summary.num = n_counts;
	      entry->counts = XCNEWVEC (gcov_type, n_counts);
	    }
	  else if (entry->lineno_checksum != lineno_checksum
		   || entry->cfg_checksum != cfg_checksum)
	    {
	      error ("Profile data for function %u is corrupted", fn_ident);
	      error ("checksum is (%x,%x) instead of (%x,%x)",
		     entry->lineno_checksum, entry->cfg_checksum,
		     lineno_checksum, cfg_checksum);
	      htab_delete (counts_hash);
	      break;
	    }
	  else if (entry->summary.num != n_counts)
	    {
	      error ("Profile data for function %u is corrupted", fn_ident);
	      error ("number of counters is %d instead of %d", entry->summary.num, n_counts);
	      htab_delete (counts_hash);
	      break;
	    }
	  else if (elt.ctr >= GCOV_COUNTERS_SUMMABLE)
	    {
	      error ("cannot merge separate %s counters for function %u",
		     ctr_names[elt.ctr], fn_ident);
	      goto skip_merge;
	    }

	  if (elt.ctr < GCOV_COUNTERS_SUMMABLE
	      /* This should always be true for a just allocated entry,
		 and always false for an existing one. Check this way, in
		 case the gcov file is corrupt.  */
	      && (!entry->chain || summaried != entry))
	    {
	      entry->chain = summaried;
	      summaried = entry;
	    }
	  for (ix = 0; ix != n_counts; ix++)
	    entry->counts[ix] += gcov_read_counter ();
	skip_merge:;
	}
      gcov_sync (offset, length);
      if ((is_error = gcov_is_error ()))
	{
	  error (is_error < 0 ? "%qs has overflowed" : "%qs is corrupted",
		 da_file_name);
	  htab_delete (counts_hash);
	  break;
	}
    }

  gcov_close ();
}

/* Returns the counters for a particular tag.  */

gcov_type *
get_coverage_counts (unsigned counter, unsigned expected,
                     unsigned cfg_checksum, unsigned lineno_checksum,
		     const struct gcov_ctr_summary **summary)
{
  counts_entry_t *entry, elt;

  /* No hash table, no counts.  */
  if (!counts_hash)
    {
      static int warned = 0;

      if (!warned++)
	inform (input_location, (flag_guess_branch_prob
		 ? "file %s not found, execution counts estimated"
		 : "file %s not found, execution counts assumed to be zero"),
		da_file_name);
      return NULL;
    }

  elt.ident = current_function_funcdef_no + 1;
  elt.ctr = counter;
  entry = (counts_entry_t *) htab_find (counts_hash, &elt);
  if (!entry)
    {
      warning (0, "no coverage for function %qE found",
	       DECL_ASSEMBLER_NAME (current_function_decl));
      return NULL;
    }

  if (entry->cfg_checksum != cfg_checksum
      || entry->summary.num != expected)
    {
      static int warned = 0;
      bool warning_printed = false;
      tree id = DECL_ASSEMBLER_NAME (current_function_decl);

      warning_printed =
	warning_at (input_location, OPT_Wcoverage_mismatch,
		    "The control flow of function %qE does not match "
		    "its profile data (counter %qs)", id, ctr_names[counter]);
      if (warning_printed)
	{
	 inform (input_location, "Use -Wno-error=coverage-mismatch to tolerate "
	 	 "the mismatch but performance may drop if the function is hot");
	  
	  if (!seen_error ()
	      && !warned++)
	    {
	      inform (input_location, "coverage mismatch ignored");
	      inform (input_location, flag_guess_branch_prob
		      ? G_("execution counts estimated")
		      : G_("execution counts assumed to be zero"));
	      if (!flag_guess_branch_prob)
		inform (input_location,
			"this can result in poorly optimized code");
	    }
	}

      return NULL;
    }
    else if (entry->lineno_checksum != lineno_checksum)
      {
        warning (0, "Source location for function %qE have changed,"
                 " the profile data may be out of date",
                 DECL_ASSEMBLER_NAME (current_function_decl));
      }

  if (summary)
    *summary = &entry->summary;

  return entry->counts;
}

/* Allocate NUM counters of type COUNTER. Returns nonzero if the
   allocation succeeded.  */

int
coverage_counter_alloc (unsigned counter, unsigned num)
{
  if (no_coverage)
    return 0;

  if (!num)
    return 1;

  if (!tree_ctr_tables[counter])
    {
      /* Generate and save a copy of this so it can be shared.  Leave
	 the index type unspecified for now; it will be set after all
	 functions have been compiled.  */
      char buf[20];
      tree gcov_type_node = get_gcov_type ();
      tree gcov_type_array_type
        = build_array_type (gcov_type_node, NULL_TREE);
      tree_ctr_tables[counter]
        = build_decl (BUILTINS_LOCATION,
		      VAR_DECL, NULL_TREE, gcov_type_array_type);
      TREE_STATIC (tree_ctr_tables[counter]) = 1;
      ASM_GENERATE_INTERNAL_LABEL (buf, "LPBX", counter + 1);
      DECL_NAME (tree_ctr_tables[counter]) = get_identifier (buf);
      DECL_ALIGN (tree_ctr_tables[counter]) = TYPE_ALIGN (gcov_type_node);

      if (dump_file)
	fprintf (dump_file, "Using data file %s\n", da_file_name);
    }
  fn_b_ctrs[counter] = fn_n_ctrs[counter];
  fn_n_ctrs[counter] += num;
  fn_ctr_mask |= 1 << counter;
  return 1;
}

/* Generate a tree to access COUNTER NO.  */

tree
tree_coverage_counter_ref (unsigned counter, unsigned no)
{
  tree gcov_type_node = get_gcov_type ();

  gcc_assert (no < fn_n_ctrs[counter] - fn_b_ctrs[counter]);
  no += prg_n_ctrs[counter] + fn_b_ctrs[counter];

  /* "no" here is an array index, scaled to bytes later.  */
  return build4 (ARRAY_REF, gcov_type_node, tree_ctr_tables[counter],
		 build_int_cst (integer_type_node, no), NULL, NULL);
}

/* Generate a tree to access the address of COUNTER NO.  */

tree
tree_coverage_counter_addr (unsigned counter, unsigned no)
{
  tree gcov_type_node = get_gcov_type ();

  gcc_assert (no < fn_n_ctrs[counter] - fn_b_ctrs[counter]);
  no += prg_n_ctrs[counter] + fn_b_ctrs[counter];

  TREE_ADDRESSABLE (tree_ctr_tables[counter]) = 1;

  /* "no" here is an array index, scaled to bytes later.  */
  return build_fold_addr_expr (build4 (ARRAY_REF, gcov_type_node,
				       tree_ctr_tables[counter],
				       build_int_cst (integer_type_node, no),
				       NULL, NULL));
}


/* Generate a checksum for a string.  CHKSUM is the current
   checksum.  */

static unsigned
coverage_checksum_string (unsigned chksum, const char *string)
{
  int i;
  char *dup = NULL;

  /* Look for everything that looks if it were produced by
     get_file_function_name and zero out the second part
     that may result from flag_random_seed.  This is not critical
     as the checksums are used only for sanity checking.  */
  for (i = 0; string[i]; i++)
    {
      int offset = 0;
      if (!strncmp (string + i, "_GLOBAL__N_", 11))
      offset = 11;
      if (!strncmp (string + i, "_GLOBAL__", 9))
      offset = 9;

      /* C++ namespaces do have scheme:
         _GLOBAL__N_<filename>_<wrongmagicnumber>_<magicnumber>functionname
       since filename might contain extra underscores there seems
       to be no better chance then walk all possible offsets looking
       for magicnumber.  */
      if (offset)
	{
	  for (i = i + offset; string[i]; i++)
	    if (string[i]=='_')
	      {
		int y;

		for (y = 1; y < 9; y++)
		  if (!(string[i + y] >= '0' && string[i + y] <= '9')
		      && !(string[i + y] >= 'A' && string[i + y] <= 'F'))
		    break;
		if (y != 9 || string[i + 9] != '_')
		  continue;
		for (y = 10; y < 18; y++)
		  if (!(string[i + y] >= '0' && string[i + y] <= '9')
		      && !(string[i + y] >= 'A' && string[i + y] <= 'F'))
		    break;
		if (y != 18)
		  continue;
		if (!dup)
		  string = dup = xstrdup (string);
		for (y = 10; y < 18; y++)
		  dup[i + y] = '0';
	      }
	  break;
	}
    }

  chksum = crc32_string (chksum, string);
  free (dup);

  return chksum;
}

/* Compute checksum for the current function.  We generate a CRC32.  */

unsigned
coverage_compute_lineno_checksum (void)
{
  expanded_location xloc
    = expand_location (DECL_SOURCE_LOCATION (current_function_decl));
  unsigned chksum = xloc.line;

  chksum = coverage_checksum_string (chksum, xloc.file);
  chksum = coverage_checksum_string
    (chksum, IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (current_function_decl)));

  return chksum;
}

/* Compute cfg checksum for the current function.
   The checksum is calculated carefully so that
   source code changes that doesn't affect the control flow graph
   won't change the checksum.
   This is to make the profile data useable across source code change.
   The downside of this is that the compiler may use potentially
   wrong profile data - that the source code change has non-trivial impact
   on the validity of profile data (e.g. the reversed condition)
   but the compiler won't detect the change and use the wrong profile data.  */

unsigned
coverage_compute_cfg_checksum (void)
{
  basic_block bb;
  unsigned chksum = n_basic_blocks;

  FOR_EACH_BB (bb)
    {
      edge e;
      edge_iterator ei;
      chksum = crc32_byte (chksum, bb->index);
      FOR_EACH_EDGE (e, ei, bb->succs)
        {
          chksum = crc32_byte (chksum, e->dest->index);
        }
    }

  return chksum;
}

/* Begin output to the graph file for the current function.
   Opens the output file, if not already done. Writes the
   function header, if not already done. Returns nonzero if data
   should be output.  */

int
coverage_begin_output (unsigned lineno_checksum, unsigned cfg_checksum)
{
  /* We don't need to output .gcno file unless we're under -ftest-coverage
     (e.g. -fprofile-arcs/generate/use don't need .gcno to work). */
  if (no_coverage || !flag_test_coverage || flag_compare_debug)
    return 0;

  if (!bbg_function_announced)
    {
      expanded_location xloc
	= expand_location (DECL_SOURCE_LOCATION (current_function_decl));
      unsigned long offset;

      if (!bbg_file_opened)
	{
	  if (!gcov_open (bbg_file_name, -1))
	    error ("cannot open %s", bbg_file_name);
	  else
	    {
	      gcov_write_unsigned (GCOV_NOTE_MAGIC);
	      gcov_write_unsigned (GCOV_VERSION);
	      gcov_write_unsigned (local_tick);
	    }
	  bbg_file_opened = 1;
	}


      /* Announce function */
      offset = gcov_write_tag (GCOV_TAG_FUNCTION);
      gcov_write_unsigned (current_function_funcdef_no + 1);
      gcov_write_unsigned (lineno_checksum);
      gcov_write_unsigned (cfg_checksum);
      gcov_write_string (IDENTIFIER_POINTER
                         (DECL_ASSEMBLER_NAME (current_function_decl)));
      gcov_write_string (xloc.file);
      gcov_write_unsigned (xloc.line);
      gcov_write_length (offset);

      bbg_function_announced = 1;
    }
  return !gcov_is_error ();
}

/* Finish coverage data for the current function. Verify no output
   error has occurred.  Save function coverage counts.  */

void
coverage_end_function (unsigned lineno_checksum, unsigned cfg_checksum)
{
  unsigned i;

  if (bbg_file_opened > 1 && gcov_is_error ())
    {
      warning (0, "error writing %qs", bbg_file_name);
      bbg_file_opened = -1;
    }

  if (fn_ctr_mask)
    {
      struct function_list *item;

      item = XCNEW (struct function_list);

      *functions_tail = item;
      functions_tail = &item->next;


      item->next = 0;
      item->ident = current_function_funcdef_no + 1;
      item->lineno_checksum = lineno_checksum;
      item->cfg_checksum = cfg_checksum;
      for (i = 0; i != GCOV_COUNTERS; i++)
	{
	  item->n_ctrs[i] = fn_n_ctrs[i];
	  prg_n_ctrs[i] += fn_n_ctrs[i];
	  fn_n_ctrs[i] = fn_b_ctrs[i] = 0;
	}
      prg_ctr_mask |= fn_ctr_mask;
      fn_ctr_mask = 0;
    }
  bbg_function_announced = 0;
}

/* Creates the gcov_fn_info RECORD_TYPE.  */

static tree
build_fn_info_type (unsigned int counters)
{
  tree type = lang_hooks.types.make_type (RECORD_TYPE);
  tree field, fields;
  tree array_type;

  /* ident */
  fields = build_decl (BUILTINS_LOCATION,
		       FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  /* lineno_checksum */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;

  /* cfg checksum */
  field = build_decl (BUILTINS_LOCATION,
                      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;

  array_type = build_index_type (size_int (counters - 1));
  array_type = build_array_type (get_gcov_unsigned_t (), array_type);

  /* counters */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, array_type);
  DECL_CHAIN (field) = fields;
  fields = field;

  finish_builtin_struct (type, "__gcov_fn_info", fields, NULL_TREE);

  return type;
}

/* Creates a CONSTRUCTOR for a gcov_fn_info. FUNCTION is
   the function being processed and TYPE is the gcov_fn_info
   RECORD_TYPE.  */

static tree
build_fn_info_value (const struct function_list *function, tree type)
{
  tree fields = TYPE_FIELDS (type);
  unsigned ix;
  VEC(constructor_elt,gc) *v1 = NULL;
  VEC(constructor_elt,gc) *v2 = NULL;

  /* ident */
  CONSTRUCTOR_APPEND_ELT (v1, fields,
			  build_int_cstu (get_gcov_unsigned_t (),
					  function->ident));
  fields = DECL_CHAIN (fields);

  /* lineno_checksum */
  CONSTRUCTOR_APPEND_ELT (v1, fields,
			  build_int_cstu (get_gcov_unsigned_t (),
					  function->lineno_checksum));
  fields = DECL_CHAIN (fields);

  /* cfg_checksum */
  CONSTRUCTOR_APPEND_ELT (v1, fields,
			  build_int_cstu (get_gcov_unsigned_t (),
					  function->cfg_checksum));
  fields = DECL_CHAIN (fields);

  /* counters */
  for (ix = 0; ix != GCOV_COUNTERS; ix++)
    if (prg_ctr_mask & (1 << ix))
      CONSTRUCTOR_APPEND_ELT (v2, NULL,
			      build_int_cstu (get_gcov_unsigned_t (),
					      function->n_ctrs[ix]));

  CONSTRUCTOR_APPEND_ELT (v1, fields,
			  build_constructor (TREE_TYPE (fields), v2));

  return build_constructor (type, v1);
}

/* Creates the gcov_ctr_info RECORD_TYPE.  */

static tree
build_ctr_info_type (void)
{
  tree type = lang_hooks.types.make_type (RECORD_TYPE);
  tree field, fields = NULL_TREE;
  tree gcov_ptr_type = build_pointer_type (get_gcov_type ());
  tree gcov_merge_fn_type;

  /* counters */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;

  /* values */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, gcov_ptr_type);
  DECL_CHAIN (field) = fields;
  fields = field;

  /* merge */
  gcov_merge_fn_type =
    build_function_type_list (void_type_node,
			      gcov_ptr_type, get_gcov_unsigned_t (),
			      NULL_TREE);
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE,
		      build_pointer_type (gcov_merge_fn_type));
  DECL_CHAIN (field) = fields;
  fields = field;

  finish_builtin_struct (type, "__gcov_ctr_info", fields, NULL_TREE);

  return type;
}

/* Creates a CONSTRUCTOR for a gcov_ctr_info. COUNTER is
   the counter being processed and TYPE is the gcov_ctr_info
   RECORD_TYPE.  */

static tree
build_ctr_info_value (unsigned int counter, tree type)
{
  tree fields = TYPE_FIELDS (type);
  tree fn;
  VEC(constructor_elt,gc) *v = NULL;

  /* counters */
  CONSTRUCTOR_APPEND_ELT (v, fields,
			  build_int_cstu (get_gcov_unsigned_t (),
					  prg_n_ctrs[counter]));
  fields = DECL_CHAIN (fields);

  if (prg_n_ctrs[counter])
    {
      tree array_type;

      array_type = build_int_cstu (get_gcov_unsigned_t (),
				   prg_n_ctrs[counter] - 1);
      array_type = build_index_type (array_type);
      array_type = build_array_type (TREE_TYPE (TREE_TYPE (fields)),
				     array_type);

      TREE_TYPE (tree_ctr_tables[counter]) = array_type;
      DECL_SIZE (tree_ctr_tables[counter]) = TYPE_SIZE (array_type);
      DECL_SIZE_UNIT (tree_ctr_tables[counter]) = TYPE_SIZE_UNIT (array_type);
      varpool_finalize_decl (tree_ctr_tables[counter]);

      CONSTRUCTOR_APPEND_ELT (v, fields,
			      build1 (ADDR_EXPR, TREE_TYPE (fields),
				      tree_ctr_tables[counter]));
    }
  else
    CONSTRUCTOR_APPEND_ELT (v, fields, null_pointer_node);
  fields = DECL_CHAIN (fields);

  fn = build_decl (BUILTINS_LOCATION,
		   FUNCTION_DECL,
		   get_identifier (ctr_merge_functions[counter]),
		   TREE_TYPE (TREE_TYPE (fields)));
  DECL_EXTERNAL (fn) = 1;
  TREE_PUBLIC (fn) = 1;
  DECL_ARTIFICIAL (fn) = 1;
  TREE_NOTHROW (fn) = 1;
  DECL_ASSEMBLER_NAME (fn);  /* Initialize assembler name so we can stream out. */
  CONSTRUCTOR_APPEND_ELT (v, fields, build1 (ADDR_EXPR, TREE_TYPE (fields), fn));

  return build_constructor (type, v);
}

/* Creates the gcov_info RECORD_TYPE and initializer for it. Returns a
   CONSTRUCTOR.  */

static tree
build_gcov_info (void)
{
  unsigned n_ctr_types, ix;
  tree type, const_type;
  tree fn_info_type, fn_info_value = NULL_TREE;
  tree fn_info_ptr_type;
  tree ctr_info_type, ctr_info_ary_type, ctr_info_value = NULL_TREE;
  tree field, fields = NULL_TREE;
  tree filename_string;
  int da_file_name_len;
  unsigned n_fns;
  const struct function_list *fn;
  tree string_type;
  VEC(constructor_elt,gc) *v1 = NULL;
  VEC(constructor_elt,gc) *v2 = NULL;

  /* Count the number of active counters.  */
  for (n_ctr_types = 0, ix = 0; ix != GCOV_COUNTERS; ix++)
    if (prg_ctr_mask & (1 << ix))
      n_ctr_types++;

  type = lang_hooks.types.make_type (RECORD_TYPE);
  const_type = build_qualified_type (type, TYPE_QUAL_CONST);

  /* Version ident */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field,
			  build_int_cstu (TREE_TYPE (field), GCOV_VERSION));

  /* next -- NULL */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, build_pointer_type (const_type));
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field, null_pointer_node);

  /* stamp */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field,
			  build_int_cstu (TREE_TYPE (field), local_tick));

  /* Filename */
  string_type = build_pointer_type (build_qualified_type (char_type_node,
						    TYPE_QUAL_CONST));
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, string_type);
  DECL_CHAIN (field) = fields;
  fields = field;
  da_file_name_len = strlen (da_file_name);
  filename_string = build_string (da_file_name_len + 1, da_file_name);
  TREE_TYPE (filename_string) = build_array_type
    (char_type_node, build_index_type (size_int (da_file_name_len)));
  CONSTRUCTOR_APPEND_ELT (v1, field,
			  build1 (ADDR_EXPR, string_type, filename_string));

  /* Build the fn_info type and initializer.  */
  fn_info_type = build_fn_info_type (n_ctr_types);
  fn_info_ptr_type = build_pointer_type (build_qualified_type
					 (fn_info_type, TYPE_QUAL_CONST));
  for (fn = functions_head, n_fns = 0; fn; fn = fn->next, n_fns++)
    CONSTRUCTOR_APPEND_ELT (v2, NULL_TREE,
			    build_fn_info_value (fn, fn_info_type));

  if (n_fns)
    {
      tree array_type;

      array_type = build_index_type (size_int (n_fns - 1));
      array_type = build_array_type (fn_info_type, array_type);

      fn_info_value = build_constructor (array_type, v2);
      fn_info_value = build1 (ADDR_EXPR, fn_info_ptr_type, fn_info_value);
    }
  else
    fn_info_value = null_pointer_node;

  /* number of functions */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field,
			  build_int_cstu (get_gcov_unsigned_t (), n_fns));

  /* fn_info table */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, fn_info_ptr_type);
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field, fn_info_value);

  /* counter_mask */
  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, get_gcov_unsigned_t ());
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field, 
			  build_int_cstu (get_gcov_unsigned_t (),
					  prg_ctr_mask));

  /* counters */
  ctr_info_type = build_ctr_info_type ();
  ctr_info_ary_type = build_index_type (size_int (n_ctr_types));
  ctr_info_ary_type = build_array_type (ctr_info_type, ctr_info_ary_type);
  v2 = NULL;
  for (ix = 0; ix != GCOV_COUNTERS; ix++)
    if (prg_ctr_mask & (1 << ix))
      CONSTRUCTOR_APPEND_ELT (v2, NULL_TREE,
			      build_ctr_info_value (ix, ctr_info_type));
  ctr_info_value = build_constructor (ctr_info_ary_type, v2);

  field = build_decl (BUILTINS_LOCATION,
		      FIELD_DECL, NULL_TREE, ctr_info_ary_type);
  DECL_CHAIN (field) = fields;
  fields = field;
  CONSTRUCTOR_APPEND_ELT (v1, field, ctr_info_value);

  finish_builtin_struct (type, "__gcov_info", fields, NULL_TREE);

  return build_constructor (type, v1);
}

/* Write out the structure which libgcov uses to locate all the
   counters.  The structures used here must match those defined in
   gcov-io.h.  Write out the constructor to call __gcov_init.  */

static void
create_coverage (void)
{
  tree gcov_info, gcov_init, body, t;
  char name_buf[32];

  no_coverage = 1; /* Disable any further coverage.  */

  if (!prg_ctr_mask)
    return;

  t = build_gcov_info ();

  gcov_info = build_decl (BUILTINS_LOCATION,
			  VAR_DECL, NULL_TREE, TREE_TYPE (t));
  TREE_STATIC (gcov_info) = 1;
  ASM_GENERATE_INTERNAL_LABEL (name_buf, "LPBX", 0);
  DECL_NAME (gcov_info) = get_identifier (name_buf);
  DECL_INITIAL (gcov_info) = t;

  /* Build structure.  */
  varpool_finalize_decl (gcov_info);

  /* Build a decl for __gcov_init.  */
  t = build_pointer_type (TREE_TYPE (gcov_info));
  t = build_function_type_list (void_type_node, t, NULL);
  t = build_decl (BUILTINS_LOCATION,
		  FUNCTION_DECL, get_identifier ("__gcov_init"), t);
  TREE_PUBLIC (t) = 1;
  DECL_EXTERNAL (t) = 1;
  DECL_ASSEMBLER_NAME (t);  /* Initialize assembler name so we can stream out. */
  gcov_init = t;

  /* Generate a call to __gcov_init(&gcov_info).  */
  body = NULL;
  t = build_fold_addr_expr (gcov_info);
  t = build_call_expr (gcov_init, 1, t);
  append_to_statement_list (t, &body);

  /* Generate a constructor to run it.  */
  cgraph_build_static_cdtor ('I', body, DEFAULT_INIT_PRIORITY);
}

/* Perform file-level initialization. Read in data file, generate name
   of graph file.  */

void
coverage_init (const char *filename)
{
  int len = strlen (filename);
  /* + 1 for extra '/', in case prefix doesn't end with /.  */
  int prefix_len;

  if (profile_data_prefix == 0 && !IS_ABSOLUTE_PATH(&filename[0]))
    profile_data_prefix = getpwd ();

  prefix_len = (profile_data_prefix) ? strlen (profile_data_prefix) + 1 : 0;

  /* Name of da file.  */
  da_file_name = XNEWVEC (char, len + strlen (GCOV_DATA_SUFFIX)
			  + prefix_len + 1);

  if (profile_data_prefix)
    {
      strcpy (da_file_name, profile_data_prefix);
      da_file_name[prefix_len - 1] = '/';
      da_file_name[prefix_len] = 0;
    }
  else
    da_file_name[0] = 0;
  strcat (da_file_name, filename);
  strcat (da_file_name, GCOV_DATA_SUFFIX);

  /* Name of bbg file.  */
  bbg_file_name = XNEWVEC (char, len + strlen (GCOV_NOTE_SUFFIX) + 1);
  strcpy (bbg_file_name, filename);
  strcat (bbg_file_name, GCOV_NOTE_SUFFIX);

  if (flag_profile_use)
    read_counts_file ();
}

/* Performs file-level cleanup.  Close graph file, generate coverage
   variables and constructor.  */

void
coverage_finish (void)
{
  create_coverage ();
  if (bbg_file_opened)
    {
      int error = gcov_close ();

      if (error)
	unlink (bbg_file_name);
      if (!local_tick)
	/* Only remove the da file, if we cannot stamp it. If we can
	   stamp it, libgcov will DTRT.  */
	unlink (da_file_name);
    }
}

#include "gt-coverage.h"
