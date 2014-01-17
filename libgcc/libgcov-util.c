/* Utility functions for reading gcda files into in-memory
   gcov_info structures and offline profile processing. */
/* Copyright (C) 2014 Free Software Foundation, Inc.
   Contributed by Rong Xu <xur@google.com>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


#define IN_GCOV_TOOL 1
#define L_gcov 1
#define L_gcov_merge_add 1
#define L_gcov_merge_single 1
#define L_gcov_merge_delta 1
#define L_gcov_merge_icall_topn 1
#define L_gcov_merge_dc 1
#define L_gcov_merge_ior 1
#define L_gcov_merge_reusedist 1

#include "libgcov.h"
#include "intl.h"
#include "diagnostic.h"
#include "version.h"
#include "demangle.h"

extern gcov_type gcov_read_counter_mem();
extern unsigned gcov_get_merge_weight();

/* We need the dumping and merge part of code in libgcov.  */
#include "libgcov-driver.c"
#include "libgcov-merge.c"

/* Verbose mode for debug.  */
static int verbose;

/* Set verbose flag.  */
void gcov_set_verbose (void)
{
  verbose = 1;
}

/* The following part is to read Gcda and reconstruct GCOV_INFO.  */

#include "obstack.h"
#include <unistd.h>
#include <ftw.h>

static void tag_function (unsigned, unsigned);
static void tag_blocks (unsigned, unsigned);
static void tag_arcs (unsigned, unsigned);
static void tag_lines (unsigned, unsigned);
static void tag_counters (unsigned, unsigned);
static void tag_summary (unsigned, unsigned);
static void tag_module_info (unsigned, unsigned);

/* The gcov_info for the first module.  */
static struct gcov_info *curr_gcov_info;
/* The gcov_info being processed.  */
static struct gcov_info *gcov_info_head;
/* This variable points to the module being processed.  */
static struct gcov_module_info *curr_module_info;
/* This variable contains all the functions in current module.  */
static struct obstack fn_info;
/* The function being processed.  */
static struct gcov_fn_info *curr_fn_info;
/* The number of functions seen so far.  */
static unsigned num_fn_info;
/* This variable contains all the counters for current module.  */
static int k_ctrs_mask[GCOV_COUNTERS];
/* The kind of counters that have been seen.  */
static struct gcov_ctr_info k_ctrs[GCOV_COUNTERS];
/* Number of kind of counters that have been seen.  */
static int k_ctrs_types;
/* The longest length of all the filenames.  */
static int max_filename_len;

/* Merge functions for counters.  */
static gcov_merge_fn ctr_merge_functions[GCOV_COUNTERS] = {
    __gcov_merge_add,
    __gcov_merge_add,
    __gcov_merge_add,
    __gcov_merge_single,
    __gcov_merge_delta,
    __gcov_merge_single,
    __gcov_merge_add,
    __gcov_merge_ior,
    __gcov_merge_icall_topn,
    __gcov_merge_dc,
};

/* Set the ctrs field in gcvo_fn_info object FN_INFO.  */

static void
set_fn_ctrs (struct gcov_fn_info *fn_info)
{
  int j = 0, i;

  for (i = 0; i < GCOV_COUNTERS; i++)
    {
      if (k_ctrs_mask[i] == 0)
        continue;
      fn_info->ctrs[j].num = k_ctrs[i].num;
      fn_info->ctrs[j].values = k_ctrs[i].values;
      j++;
    }
  if (k_ctrs_types == 0)
    k_ctrs_types = j;
  else
    gcc_assert (j == k_ctrs_types);
}

typedef struct tag_format
{
    unsigned tag;
    char const *name;
    void (*proc) (unsigned, unsigned);
} tag_format_t;

static const tag_format_t tag_table[] =
{
  {0, "NOP", NULL},
  {0, "UNKNOWN", NULL},
  {0, "COUNTERS", tag_counters},
  {GCOV_TAG_FUNCTION, "FUNCTION", tag_function},
  {GCOV_TAG_BLOCKS, "BLOCKS", tag_blocks},
  {GCOV_TAG_ARCS, "ARCS", tag_arcs},
  {GCOV_TAG_LINES, "LINES", tag_lines},
  {GCOV_TAG_OBJECT_SUMMARY, "OBJECT_SUMMARY", tag_summary},
  {GCOV_TAG_PROGRAM_SUMMARY, "PROGRAM_SUMMARY", tag_summary},
  {GCOV_TAG_MODULE_INFO, "MODULE INFO", tag_module_info},
  {0, NULL, NULL}
};

/* Handler for reading function tag.  */

static void
tag_function (unsigned tag ATTRIBUTE_UNUSED, unsigned length ATTRIBUTE_UNUSED)
{
  int i;

  /* write out previous fn_info.  */
  if (num_fn_info)
    {
      set_fn_ctrs (curr_fn_info);
      obstack_ptr_grow (&fn_info, curr_fn_info);
    }

  /* Here we over allocate a bit, using GCOV_COUNTERS instead of the actual active
     counter types.  */
  curr_fn_info = (struct gcov_fn_info *) xcalloc (sizeof (struct gcov_fn_info)
                   + GCOV_COUNTERS * sizeof (struct gcov_ctr_info), 1);

  for (i = 0; i < GCOV_COUNTERS; i++)
     k_ctrs[i].num = 0;
  k_ctrs_types = 0;

  curr_fn_info->key = curr_gcov_info;
  curr_fn_info->ident = gcov_read_unsigned ();
  curr_fn_info->lineno_checksum = gcov_read_unsigned ();
  curr_fn_info->cfg_checksum = gcov_read_unsigned ();
  num_fn_info++;

  if (verbose)
    fprintf (stdout, "tag one function id=%d\n", curr_fn_info->ident);
}

/* Handler for reading block tag.  */

static void
tag_blocks (unsigned tag ATTRIBUTE_UNUSED, unsigned length ATTRIBUTE_UNUSED)
{
  gcc_assert (0);
}

/* Handler for reading flow arc tag.  */

static void
tag_arcs (unsigned tag ATTRIBUTE_UNUSED, unsigned length ATTRIBUTE_UNUSED)
{
  gcc_assert (0);
}

/* Handler for reading line tag.  */

static void
tag_lines (unsigned tag ATTRIBUTE_UNUSED, unsigned length ATTRIBUTE_UNUSED)
{
  gcc_assert (0);
}

/* Handler for reading counters array tag with value as TAG and length of LENGTH.  */

static void
tag_counters (unsigned tag, unsigned length)
{
  unsigned n_counts = GCOV_TAG_COUNTER_NUM (length);
  gcov_type *values;
  unsigned ix;
  unsigned tag_ix;

  tag_ix = GCOV_COUNTER_FOR_TAG (tag);
  gcc_assert (tag_ix < GCOV_COUNTERS);
  k_ctrs_mask [tag_ix] = 1;
  gcc_assert (k_ctrs[tag_ix].num == 0);
  k_ctrs[tag_ix].num = n_counts;

  k_ctrs[tag_ix].values = values = (gcov_type *) xmalloc (n_counts * sizeof (gcov_type));
  gcc_assert (values);

  for (ix = 0; ix != n_counts; ix++)
    values[ix] = gcov_read_counter ();
}

/* Handler for reading summary tag.  */

static void
tag_summary (unsigned tag ATTRIBUTE_UNUSED, unsigned length ATTRIBUTE_UNUSED)
{
  struct gcov_summary summary;

  gcov_read_summary (&summary);
}

/* This function is called at the end of reading a gcda file.
   It flushes the contents in curr_fn_info to gcov_info object OBJ_INFO.  */

static void
read_gcda_finalize (struct gcov_info *obj_info)
{
  int i;

  set_fn_ctrs (curr_fn_info);
  obstack_ptr_grow (&fn_info, curr_fn_info);

  /* We set the following fields: merge, n_functions, and functions.  */
  obj_info->n_functions = num_fn_info;
  obj_info->functions = (const struct gcov_fn_info**) obstack_finish (&fn_info);

  /* wrap all the counter array.  */
  for (i=0; i< GCOV_COUNTERS; i++)
    {
      if (k_ctrs_mask[i])
        obj_info->merge[i] = ctr_merge_functions[i];
    }

  obj_info->mod_info = curr_module_info;
}

extern void gcov_read_module_info (struct gcov_module_info *mod_info,
    gcov_unsigned_t len);

/* This function reads module_info from a gcda file.  */

static void
tag_module_info (unsigned tag ATTRIBUTE_UNUSED, unsigned length)
{
  struct gcov_module_info* mod_info;

  mod_info = (struct gcov_module_info *)
      xmalloc ((length + 2) * sizeof (gcov_unsigned_t));

  gcov_read_module_info (mod_info, length);

  if (mod_info->is_primary)
    curr_module_info = mod_info;
  else
    free (mod_info);
}

/* Read the content of a gcda file FILENAME, and return a gcov_info data structure.
   Program level summary CURRENT_SUMMARY will also be updated.  */

static struct gcov_info *
read_gcda_file (const char *filename)
{
  unsigned tags[4];
  unsigned depth = 0;
  unsigned magic, version;
  struct gcov_info *obj_info;
  int i;

  for (i=0; i< GCOV_COUNTERS; i++)
    k_ctrs_mask[i] = 0;
  k_ctrs_types = 0;

  if (!gcov_open (filename))
    {
      fprintf (stderr, "%s:cannot open\n", filename);
      return NULL;
    }

  /* Read magic.  */
  magic = gcov_read_unsigned ();
  if (magic != GCOV_DATA_MAGIC)
    {
      fprintf (stderr, "%s:not a gcov data file\n", filename);
      gcov_close ();
      return NULL;
    }

  /* Read version.  */
  version = gcov_read_unsigned ();
  if (version != GCOV_VERSION)
    {
      fprintf (stderr, "%s:incorrect gcov version %d vs %d \n", filename, version, GCOV_VERSION);
      gcov_close ();
      return NULL;
    }

  /* Instantiate a gcov_info object.  */
  curr_gcov_info = obj_info = (struct gcov_info *) xcalloc (sizeof (struct gcov_info) +
             sizeof (struct gcov_ctr_info) * GCOV_COUNTERS, 1);

  obj_info->version = version;
  obstack_init (&fn_info);
  num_fn_info = 0;
  curr_fn_info = 0;
  curr_module_info = 0;

  {
    char *str_dup = (char*) xmalloc (strlen (filename) + 1);
    int len;

    strcpy (str_dup, filename);
    obj_info->filename = str_dup;
    if ((len = strlen (filename)) > max_filename_len)
      max_filename_len = len;
  }

  /* Read stamp.  */
  obj_info->stamp = gcov_read_unsigned ();

  while (1)
    {
      gcov_position_t base;
      unsigned tag, length;
      tag_format_t const *format;
      unsigned tag_depth;
      int error;
      unsigned mask;

      tag = gcov_read_unsigned ();
      if (!tag)
	break;
      length = gcov_read_unsigned ();
      base = gcov_position ();
      mask = GCOV_TAG_MASK (tag) >> 1;
      for (tag_depth = 4; mask; mask >>= 8)
	{
	  if (((mask & 0xff) != 0xff))
	    {
	      fprintf (stderr, "warning: %s:tag `%08x' is invalid\n", filename, tag);
	      break;
	    }
	  tag_depth--;
	}
      for (format = tag_table; format->name; format++)
	if (format->tag == tag)
	  goto found;
      format = &tag_table[GCOV_TAG_IS_COUNTER (tag) ? 2 : 1];
    found:;
      if (tag)
	{
	  if (depth && depth < tag_depth)
	    {
	      if (!GCOV_TAG_IS_SUBTAG (tags[depth - 1], tag))
		fprintf (stderr, "warning: %s:tag `%08x' is incorrectly nested\n",
			filename, tag);
	    }
	  depth = tag_depth;
	  tags[depth - 1] = tag;
	}

      if (format->proc)
        {
	  unsigned long actual_length;

	  (*format->proc) (tag, length);

	  actual_length = gcov_position () - base;
	  if (actual_length > length)
	    fprintf (stderr,"warning: %s:record size mismatch %lu bytes overread\n",
		    filename, actual_length - length);
	  else if (length > actual_length)
	    fprintf (stderr,"warning: %s:record size mismatch %lu bytes unread\n",
		    filename, length - actual_length);
	}

      gcov_sync (base, length);
      if ((error = gcov_is_error ()))
	{
	  fprintf (stderr,error < 0 ? "warning:%s:counter overflow at %lu\n" :
		  "Warning:%s:read error at %lu\n", filename,
		  (long unsigned) gcov_position ());
	  break;
	}
    }

  read_gcda_finalize (obj_info);
  gcov_close ();

  return obj_info;
}

/* This will be called by ftw(). It opens and read a gcda file FILENAME.
   Return a non-zero value to stop the tree walk.  */

static int
ftw_read_file (const char *filename,
               const struct stat *status ATTRIBUTE_UNUSED,
               int type)
{
  int filename_len;
  int suffix_len;
  struct gcov_info *obj_info;

  /* Only read regular files.  */
  if (type != FTW_F)
    return 0;

  filename_len = strlen (filename);
  suffix_len = strlen (GCOV_DATA_SUFFIX);

  if (filename_len <= suffix_len)
    return 0;

  if (strcmp(filename + filename_len - suffix_len, GCOV_DATA_SUFFIX))
    return 0;

   if (verbose)
    fprintf (stderr, "reading file: %s\n", filename);

  obj_info = read_gcda_file (filename);

  obj_info->next = gcov_info_head;
  gcov_info_head = obj_info;

  return 0;
}

/* Initializer for reading a profile dir.  */

static inline void
read_profile_dir_init (void)
{
  gcov_info_head = 0;
}

/* Driver for read a profile directory and convert into gcov_info list in memory.
   Return NULL on error,
   Return the head of gcov_info list on success.
   Note the file static variable GCOV_MAX_FILENAME is also set.  */

struct gcov_info *
gcov_read_profile_dir (const char* dir_name, int recompute_summary ATTRIBUTE_UNUSED)
{
  char *pwd;
  int ret;

  read_profile_dir_init ();

  if (access (dir_name, R_OK) != 0)
    {
      fprintf (stderr, "cannot access directory %s\n", dir_name);
      return NULL;
    }
  pwd = getcwd (NULL, 0);
  gcc_assert (pwd);
  ret = chdir (dir_name);
  if (ret !=0)
    {
      fprintf (stderr, "%s is not a directory\n", dir_name);
      return NULL;
    }
  ftw (".", ftw_read_file, 50);
  ret = chdir (pwd);
  free (pwd);


  /* gcov_max_filename is defined in libgcov.c that records the
     max filename len. We need to set it here to allocate the
     array for dumping.  */
  gcov_max_filename = max_filename_len;

  return gcov_info_head;;
}

/* This part of the code is to merge profile counters.  */

static gcov_type *gcov_value_buf;
static gcov_unsigned_t gcov_value_buf_size;
static gcov_unsigned_t gcov_value_buf_pos;
static unsigned gcov_merge_weight;

/* Read a counter value from gcov_value_buf array.  */

gcov_type
gcov_read_counter_mem (void)
{
  gcov_type ret;
  gcc_assert (gcov_value_buf_pos < gcov_value_buf_size);
  ret = *(gcov_value_buf + gcov_value_buf_pos);
  ++gcov_value_buf_pos;
  return ret;
}

/* Return the recorded merge weight.  */

unsigned
gcov_get_merge_weight (void)
{
  return gcov_merge_weight;
}

/* A wrapper function for merge functions. It sets up the
   value buffer and weights and then calls the merge function.  */

static void
merge_wrapper (gcov_merge_fn f, gcov_type *v1, gcov_unsigned_t n,
               gcov_type *v2, unsigned w)
{
  gcov_value_buf = v2;
  gcov_value_buf_pos = 0;
  gcov_value_buf_size = n;
  gcov_merge_weight = w;
  (*f) (v1, n);
}

/* Offline tool to manipulate profile data.
   This tool targets on matched profiles. But it has some tolerance on
   unmatched profiles.
   When merging p1 to p2 (p2 is the dst),
   * m.gcda in p1 but not in p2: append m.gcda to p2 with specified weight;
     emit warning
   * m.gcda in p2 but not in p1: keep m.gcda in p2 and multiply by
     specified weight; emit warning.
   * m.gcda in both p1 and p2:
   ** p1->m.gcda->f checksum matches p2->m.gcda->f: simple merge.
   ** p1->m.gcda->f checksum does not matches p2->m.gcda->f: keep
      p2->m.gcda->f and
      drop p1->m.gcda->f. A warning is emitted.  */

/* Add INFO2's counter to INFO1, multiplying by weight W.  */

static int
gcov_merge (struct gcov_info *info1, struct gcov_info *info2, int w)
{
  unsigned f_ix;
  unsigned n_functions = info1->n_functions;
  int has_mismatch = 0;

  gcc_assert (info2->n_functions == n_functions);
  for (f_ix = 0; f_ix < n_functions; f_ix++)
    {
      unsigned t_ix;
      const struct gcov_fn_info *gfi_ptr1 = info1->functions[f_ix];
      const struct gcov_fn_info *gfi_ptr2 = info2->functions[f_ix];
      const struct gcov_ctr_info *ci_ptr1, *ci_ptr2;

      if (!gfi_ptr1 || gfi_ptr1->key != info1)
        continue;
      if (!gfi_ptr2 || gfi_ptr2->key != info2)
        continue;

      if (gfi_ptr1->cfg_checksum != gfi_ptr2->cfg_checksum)
        {
          fprintf (stderr, "in %s, cfg_checksum mismatch, skipping\n",
                  info1->filename);
          has_mismatch = 1;
          continue;
        }
      ci_ptr1 = gfi_ptr1->ctrs;
      ci_ptr2 = gfi_ptr2->ctrs;
      for (t_ix = 0; t_ix != GCOV_COUNTERS; t_ix++)
        {
          gcov_merge_fn merge1 = info1->merge[t_ix];
          gcov_merge_fn merge2 = info2->merge[t_ix];

          gcc_assert (merge1 == merge2);
          if (!merge1)
            continue;
          gcc_assert (ci_ptr1->num == ci_ptr2->num);
          merge_wrapper (merge1, ci_ptr1->values, ci_ptr1->num, ci_ptr2->values, w);
          ci_ptr1++;
          ci_ptr2++;
        }
    }

  return has_mismatch;
}

/* Find and return the match gcov_info object for INFO from ARRAY.
   SIZE is the length of ARRAY.
   Return NULL if there is no match.  */

static struct gcov_info *
find_match_gcov_info (struct gcov_info **array, int size, struct gcov_info *info)
{
  struct gcov_info *gi_ptr;
  struct gcov_info *ret = NULL;
  int i;

  for (i = 0; i < size; i++)
    {
      gi_ptr = array[i];
      if (gi_ptr == 0)
        continue;
      /* For LIPO, it's easy as we can just match the module_id.  */
      if (gi_ptr->mod_info && info->mod_info)
        {
          if (gi_ptr->mod_info->ident == info->mod_info->ident)
            {
              ret = gi_ptr;
              array[i] = 0;
              break;
            }
        }
      else /* For FDO, we have to match the name. This can be expensive.
              Maybe we should use hash here.  */
        if (!strcmp (gi_ptr->filename, info->filename))
          {
            ret = gi_ptr;
            array[i] = 0;
            break;
          }
    }

  if (ret && ret->n_functions != info->n_functions)
    {
      fprintf (stderr, "mismatched profiles in %s (%d functions"
                       " vs %d functions)\n",
                       ret->filename,
                       ret->n_functions,
                       info->n_functions);
      ret = NULL;
    }
  return ret;
}

/* Merge the list of gcov_info list from SRC_PROFILE to TGT_PROFILE.
   Return 0 on success: without mismatch.
   Reutrn 1 on error.  */

int
gcov_profile_merge (struct gcov_info *tgt_profile, struct gcov_info *src_profile,
                    int w1, int w2)
{
  struct gcov_info *gi_ptr;
  struct gcov_info **tgt_infos;
  struct gcov_info *tgt_tail;
  struct gcov_info **in_src_not_tgt;
  unsigned tgt_cnt = 0, src_cnt = 0;
  unsigned unmatch_info_cnt = 0;
  unsigned int i;

  for (gi_ptr = tgt_profile; gi_ptr; gi_ptr = gi_ptr->next)
    tgt_cnt++;
  for (gi_ptr = src_profile; gi_ptr; gi_ptr = gi_ptr->next)
    src_cnt++;
  tgt_infos = (struct gcov_info **) xmalloc (sizeof (struct gcov_info *)
                 * tgt_cnt);
  gcc_assert (tgt_infos);
  in_src_not_tgt = (struct gcov_info **) xmalloc (sizeof (struct gcov_info *)
                     * src_cnt);
  gcc_assert (in_src_not_tgt);

  for (gi_ptr = tgt_profile, i = 0; gi_ptr; gi_ptr = gi_ptr->next, i++)
    tgt_infos[i] = gi_ptr;

  tgt_tail = tgt_infos[tgt_cnt - 1];

  /* First pass on tgt_profile, we multiply w1 to all counters.  */
  if (w1 > 1)
    {
       for (i = 0; i < tgt_cnt; i++)
         gcov_merge (tgt_infos[i], tgt_infos[i], w1-1);
    }

  /* Second pass, add src_profile to the tgt_profile.  */
  for (gi_ptr = src_profile; gi_ptr; gi_ptr = gi_ptr->next)
    {
      struct gcov_info *gi_ptr1;

      gi_ptr1 = find_match_gcov_info (tgt_infos, tgt_cnt, gi_ptr);
      if (gi_ptr1 == NULL)
        {
          in_src_not_tgt[unmatch_info_cnt++] = gi_ptr;
          continue;
        }
      gcov_merge (gi_ptr1, gi_ptr, w2);
    }

  /* For modules in src but not in tgt. We adjust the counter and append.  */
  for (i = 0; i < unmatch_info_cnt; i++)
    {
      gi_ptr = in_src_not_tgt[i];
      gcov_merge (gi_ptr, gi_ptr, w2 - 1);
      tgt_tail->next = gi_ptr;
      tgt_tail = gi_ptr;
    }

  return 0;
}

/* This part of code is to scale profile counters.  */

/* Type of function used to normalize counters.  */
typedef void (*gcov_scale_fn) (gcov_type *, gcov_unsigned_t, double);

/* Scale arc counters. N_COUNTERS of counter value in COUNTERS array are
   multiplied by a factor F.  */

static void
__gcov_scale_add (gcov_type *counters, unsigned n_counters, double f)
{
  for (; n_counters; counters++, n_counters--)
    {
      gcov_type val = *counters;
      *counters = val * f;
    }
}

/* Scale ior counters.  */

static void
__gcov_scale_ior (gcov_type *counters ATTRIBUTE_UNUSED,
                      unsigned n_counters ATTRIBUTE_UNUSED,
                      double f ATTRIBUTE_UNUSED)
{
  /* Do nothing.  */
}

/* Scale delta counters. Multiplied the counters in COUNTERS array
   by a factor of F.  */

static void
__gcov_scale_delta (gcov_type *counters, unsigned n_counters, double f)
{
  unsigned i, n_measures;

  gcc_assert (!(n_counters % 4));
  n_measures = n_counters / 4;
  for (i = 0; i < n_measures; i++, counters += 4)
    {
      counters[2] *= f;
      counters[3] *= f;
    }
}

/* Scale single counters. Multiplied the counters in COUNTERS array
   by a factor of F.  */

static void
__gcov_scale_single (gcov_type *counters, unsigned n_counters, double f)
{
  unsigned i, n_measures;

  gcc_assert (!(n_counters % 3));
  n_measures = n_counters / 3;
  for (i = 0; i < n_measures; i++, counters += 3)
    {
      counters[1] *= f;
      counters[2] *= f;
    }
}

/* Scale indirect-call profile counters. Multiplied the counters in COUNTERS
   array by a factor of F.  */

static void
__gcov_scale_icall_topn (gcov_type *counters, unsigned n_counters, double f)
{
  unsigned i;

  gcc_assert (!(n_counters % GCOV_ICALL_TOPN_NCOUNTS));
  for (i = 0; i < n_counters; i += GCOV_ICALL_TOPN_NCOUNTS)
    {
      unsigned j;
      gcov_type *value_array = &counters[i + 1];

      for (j = 0; j < GCOV_ICALL_TOPN_NCOUNTS - 1; j += 2)
        value_array[1] *= f;
    }
}

/* Scale direct-call profile counters. Multiplied the counters in COUNTERS
   by a factor of F.  */

static void
__gcov_scale_dc (gcov_type *counters, unsigned n_counters, double f)
{
  unsigned i;

  gcc_assert (!(n_counters % 2));
  for (i = 0; i < n_counters; i += 2)
    counters[1] *= f;
}

/* Scaling functions for counters.  */
static gcov_scale_fn ctr_scale_functions[GCOV_COUNTERS] = {
    __gcov_scale_add,
    __gcov_scale_add,
    __gcov_scale_add,
    __gcov_scale_single,
    __gcov_scale_delta,
    __gcov_scale_single,
    __gcov_scale_add,
    __gcov_scale_ior,
    __gcov_scale_icall_topn,
    __gcov_scale_dc,
};

/* Driver for scaling profile counters.  */

int
gcov_profile_scale (struct gcov_info *profile, float scale_factor)
{
  struct gcov_info *gi_ptr;
  unsigned f_ix;

  if (verbose)
    fprintf (stdout, "scale_factor is %f\n", scale_factor);

  /* Scaling the counters.  */
  for (gi_ptr = profile; gi_ptr; gi_ptr = gi_ptr->next)
    for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
      {
        unsigned t_ix;
        const struct gcov_fn_info *gfi_ptr = gi_ptr->functions[f_ix];
        const struct gcov_ctr_info *ci_ptr;

        if (!gfi_ptr || gfi_ptr->key != gi_ptr)
          continue;

        ci_ptr = gfi_ptr->ctrs;
        for (t_ix = 0; t_ix != GCOV_COUNTERS; t_ix++)
          {
            gcov_merge_fn merge = gi_ptr->merge[t_ix];

            if (!merge)
              continue;
            (*ctr_scale_functions[t_ix]) (ci_ptr->values, ci_ptr->num, scale_factor);
            ci_ptr++;
          }
      }

  return 0;
}

/* Driver for normalize profile counters.  */

int
gcov_profile_normalize (struct gcov_info *profile, gcov_type max_val)
{
  struct gcov_info *gi_ptr;
  gcov_type curr_max_val = 0;
  unsigned f_ix;
  unsigned int i;
  float scale_factor;

  /* Find the larest count value.  */
  for (gi_ptr = profile; gi_ptr; gi_ptr = gi_ptr->next)
    for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
      {
        unsigned t_ix;
        const struct gcov_fn_info *gfi_ptr = gi_ptr->functions[f_ix];
        const struct gcov_ctr_info *ci_ptr;

        if (!gfi_ptr || gfi_ptr->key != gi_ptr)
          continue;

        ci_ptr = gfi_ptr->ctrs;
        for (t_ix = 0; t_ix < 1; t_ix++)
          {
            for (i = 0; i < ci_ptr->num; i++)
              if (ci_ptr->values[i] > curr_max_val)
                curr_max_val = ci_ptr->values[i];
            ci_ptr++;
          }
      }

  scale_factor = (float)max_val / curr_max_val;
  if (verbose)
    fprintf (stdout, "max_val is %lld\n", (long long) curr_max_val);

  return gcov_profile_scale (profile, scale_factor);
}

/* Type of function used to normalize counters.  */
typedef void (*gcov_scale2_fn) (gcov_type *, gcov_unsigned_t, int, int);

/* Scale2 arc counters.  */

static void
__gcov_scale2_add (gcov_type *counters, unsigned n_counters, int n, int d)
{
  for (; n_counters; counters++, n_counters--)
    {
      gcov_type val = *counters;
      *counters = (val / d) * n;
    }
}

/* Scale2 ior counters.  */

static void
__gcov_scale2_ior (gcov_type *counters ATTRIBUTE_UNUSED,
                      unsigned n_counters ATTRIBUTE_UNUSED,
                      int n ATTRIBUTE_UNUSED,
                      int d ATTRIBUTE_UNUSED)
{
  /* do nothing.  */
}

/* Scale2 delta counters.  */

static void
__gcov_scale2_delta (gcov_type *counters, unsigned n_counters, int n, int d)
{
  unsigned i, n_measures;

  gcc_assert (!(n_counters % 4));
  n_measures = n_counters / 4;
  for (i = 0; i < n_measures; i++, counters += 4)
    {
      counters[2] = (counters[2] / d) * n;
      counters[3] = (counters[3] / d) * n;
    }
}

/* Scale2 single counters.  */

static void
__gcov_scale2_single (gcov_type *counters, unsigned n_counters, int n, int d)
{
  unsigned i, n_measures;

  gcc_assert (!(n_counters % 3));
  n_measures = n_counters / 3;
  for (i = 0; i < n_measures; i++, counters += 3)
    {
      counters[1] = (counters[1] / d) * n;
      counters[2] = (counters[2] / d) * n;
    }
}

/* Scale2 indirect-call profile counters. Multiplied the counters in COUNTERS
   array by a factor of F.  */

static void
__gcov_scale2_icall_topn (gcov_type *counters, unsigned n_counters, int n, int d)
{
  unsigned i;

  gcc_assert (!(n_counters % GCOV_ICALL_TOPN_NCOUNTS));
  for (i = 0; i < n_counters; i += GCOV_ICALL_TOPN_NCOUNTS)
    {
      unsigned j;
      gcov_type *value_array = &counters[i+1];

      for (j = 0; j < GCOV_ICALL_TOPN_NCOUNTS - 1; j += 2)
        value_array[j+1] = (value_array[j+1] / d) * n;
    }
}

/* Scale2 direct-call profile counters. Multiplied the counters in COUNTERS
   by a factor of F.  */

static void
__gcov_scale2_dc (gcov_type *counters, unsigned n_counters, int n, int d)
{
  unsigned i;

  gcc_assert (!(n_counters % 2));
  for (i = 0; i < n_counters; i += 2)
    counters[i+1] = (counters[i+1] / d) * n;
}

/* Scale2 functions for counters.  */
static gcov_scale2_fn ctr_scale2_functions[GCOV_COUNTERS] = {
    __gcov_scale2_add,
    __gcov_scale2_add,
    __gcov_scale2_add,
    __gcov_scale2_single,
    __gcov_scale2_delta,
    __gcov_scale2_single,
    __gcov_scale2_add,
    __gcov_scale2_ior,
    __gcov_scale2_icall_topn,
    __gcov_scale2_dc,
};

/* Driver for scale2 profile counters.  */

int
gcov_profile_scale2 (struct gcov_info *profile, int n, int d)
{
  struct gcov_info *gi_ptr;
  unsigned f_ix;

  if (verbose)
    fprintf (stdout, "scale_factor is %d/%d\n", n, d);

  gcc_assert (n >= 0 && d > 0);

  /* Scale the counters.  */
  for (gi_ptr = profile; gi_ptr; gi_ptr = gi_ptr->next)
    for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
      {
        unsigned t_ix;
        const struct gcov_fn_info *gfi_ptr = gi_ptr->functions[f_ix];
        const struct gcov_ctr_info *ci_ptr;

        if (!gfi_ptr || gfi_ptr->key != gi_ptr)
          continue;

        ci_ptr = gfi_ptr->ctrs;
        for (t_ix = 0; t_ix != GCOV_COUNTERS; t_ix++)
          {
            gcov_merge_fn merge = gi_ptr->merge[t_ix];

            if (!merge)
              continue;
            (*ctr_scale2_functions[t_ix]) (ci_ptr->values, ci_ptr->num, n, d);
            ci_ptr++;
          }
      }

  return 0;
}

