/* Routines for reading PPH data.
   Copyright (C) 2011 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@google.com>.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "pph.h"
#include "tree.h"
#include "langhooks.h"
#include "tree-iterator.h"
#include "tree-pretty-print.h"
#include "lto-streamer.h"
#include "pph-streamer.h"
#include "tree-pass.h"
#include "version.h"
#include "cppbuiltin.h"
#include "toplev.h"
#include "parser.h"
#include "pointer-set.h"

typedef char *char_p;
DEF_VEC_P(char_p);
DEF_VEC_ALLOC_P(char_p,heap);

/* String tables for all input streams.  These are allocated separately
  from streams because they cannot be deallocated after the streams
  have been read (string streaming works by pointing into these
  tables).

  Each stream will create a new entry in this table of tables.  The
  memory will remain allocated until the end of compilation.  */
static VEC(char_p,heap) *string_tables = NULL;

/* Wrapper for memory allocation calls that should have their results
   registered in the PPH streamer cache.  DATA is the pointer returned
   by the memory allocation call in ALLOC_EXPR.  IX is the cache slot 
   in CACHE where the newly allocated DATA should be registered at.  */
#define ALLOC_AND_REGISTER(CACHE, IX, TAG, DATA, ALLOC_EXPR)	\
    do {							\
      (DATA) = (ALLOC_EXPR);					\
      pph_cache_insert_at (CACHE, DATA, IX, TAG);		\
    } while (0)

/* Set in pph_in_and_merge_line_table. Represents the source_location offset
   which every streamed in token must add to it's serialized source_location.

   FIXME pph: Ideally this would be in pph_stream.encoder.r, but for that we
   first need to get rid of the dependency to the streamer_hook for locations.
   */
static int pph_loc_offset;

/* Forward declarations to avoid circularity.  */
static tree pph_in_merge_key_tree (pph_stream *, tree *, hashval_t);

/***************************************************** stream initialization */


/* Read into memory the contents of the file in STREAM.  Initialize
   internal tables and data structures needed to re-construct the
   ASTs in the file.  */

void
pph_init_read (pph_stream *stream)
{
  struct stat st;
  size_t i, bytes_read, strtab_size, body_size;
  int retcode;
  pph_file_header *header;
  const char *strtab, *body;
  char *new_strtab;

  /* Read STREAM->NAME into the memory buffer stream->encoder.r.file_data.  */
  retcode = fstat (fileno (stream->file), &st);
  gcc_assert (retcode == 0);
  stream->encoder.r.file_size = (size_t) st.st_size;
  stream->encoder.r.file_data = XCNEWVEC (char, stream->encoder.r.file_size);
  bytes_read = fread (stream->encoder.r.file_data, 1,
		      stream->encoder.r.file_size, stream->file);
  gcc_assert (bytes_read == stream->encoder.r.file_size);

  /* Set up the file sections, header, body and string table for the
     low-level streaming routines.  */
  stream->encoder.r.pph_sections = XCNEWVEC (struct lto_file_decl_data *,
					     PPH_NUM_SECTIONS);
  for (i = 0; i < PPH_NUM_SECTIONS; i++)
    stream->encoder.r.pph_sections[i] = XCNEW (struct lto_file_decl_data);

  header = (pph_file_header *) stream->encoder.r.file_data;
  strtab = (const char *) header + sizeof (pph_file_header);
  strtab_size = header->strtab_size;
  body = strtab + strtab_size;
  gcc_assert (stream->encoder.r.file_size
	      >= strtab_size + sizeof (pph_file_header));
  body_size = stream->encoder.r.file_size
	      - strtab_size - sizeof (pph_file_header);

  /* Create a new string table for STREAM.  This table is not part of
     STREAM because it needs to remain around until the end of
     compilation (all the string streaming routines work by pointing
     into the string table, so we cannot deallocate it after reading
     STREAM).  */
  new_strtab = XNEWVEC (char, strtab_size);
  memcpy (new_strtab, strtab, strtab_size);
  VEC_safe_push (char_p, heap, string_tables, new_strtab);

  /* Create an input block structure pointing right after the string
     table.  */
  stream->encoder.r.ib = XCNEW (struct lto_input_block);
  LTO_INIT_INPUT_BLOCK_PTR (stream->encoder.r.ib, body, 0, body_size);
  stream->encoder.r.data_in
      = lto_data_in_create (stream->encoder.r.pph_sections[0],
			    new_strtab, strtab_size, NULL);

  /* Associate STREAM with STREAM->ENCODER.R.DATA_IN so we can recover
     it from the streamer hooks.  */
  stream->encoder.r.data_in->sdata = (void *) stream;
}


/********************************************************** primitive values */


/* Read an unsigned char VALUE to STREAM.  */

static unsigned char
pph_in_uchar (pph_stream *stream)
{
  unsigned char n = streamer_read_uchar (stream->encoder.r.ib);
  return n;
}


/* Read a HOST_WIDE_INT from STREAM.  */

static inline HOST_WIDE_INT
pph_in_hwi (pph_stream *stream)
{
  return streamer_read_hwi (stream->encoder.r.ib);
}


/* Read an unsigned HOST_WIDE_INT from STREAM.  */

static inline unsigned HOST_WIDE_INT
pph_in_uhwi (pph_stream *stream)
{
  return streamer_read_uhwi (stream->encoder.r.ib);
}


/* Read an unsigned integer from STREAM.  */

unsigned int
pph_in_uint (pph_stream *stream)
{
  HOST_WIDE_INT unsigned n = streamer_read_uhwi (stream->encoder.r.ib);
  gcc_assert (n == (unsigned) n);
  return (unsigned) n;
}


/* Read N bytes from STREAM into P.  The caller is responsible for
   allocating a sufficiently large buffer.  */

static void
pph_in_bytes (pph_stream *stream, void *p, size_t n)
{
  lto_input_data_block (stream->encoder.r.ib, p, n);
}


/* Read and return a string from STREAM.  */

static const char *
pph_in_string (pph_stream *stream)
{
  const char *s = streamer_read_string (stream->encoder.r.data_in,
                                        stream->encoder.r.ib);
  return s;
}


/* Read a bitpack from STREAM.  */

static struct bitpack_d
pph_in_bitpack (pph_stream *stream)
{
  struct bitpack_d bp = streamer_read_bitpack (stream->encoder.r.ib);
  return bp;
}


/******************************************************** source information */


/* Read a linenum_type from STREAM.  */

static inline linenum_type
pph_in_linenum_type (pph_stream *stream)
{
  return (linenum_type) pph_in_uint (stream);
}


/* Read a source_location from STREAM.  */

static inline source_location
pph_in_source_location (pph_stream *stream)
{
  return (source_location) pph_in_uint (stream);
}


/* Read a line table marker from STREAM.  */

static inline enum pph_linetable_marker
pph_in_linetable_marker (pph_stream *stream)
{
  enum pph_linetable_marker m =
    (enum pph_linetable_marker) pph_in_uchar (stream);
  gcc_assert (m == PPH_LINETABLE_ENTRY
	      || m == PPH_LINETABLE_REFERENCE
	      || m == PPH_LINETABLE_END);
  return m;
}


/* Read a line_map from STREAM into LM.  */

static void
pph_in_line_map (pph_stream *stream, struct line_map *lm)
{
  struct bitpack_d bp;

  lm->to_file = pph_in_string (stream);
  lm->to_line = pph_in_linenum_type (stream);
  lm->start_location = pph_in_source_location (stream);
  lm->included_from = (int) pph_in_uint (stream);

  bp = pph_in_bitpack (stream);
  lm->reason = (enum lc_reason) bp_unpack_value (&bp, LC_REASON_BIT);
  gcc_assert (lm->reason == LC_ENTER
              || lm->reason == LC_LEAVE
              || lm->reason == LC_RENAME
              || lm->reason == LC_RENAME_VERBATIM);
  lm->sysp = (unsigned char) bp_unpack_value (&bp, CHAR_BIT);
  lm->column_bits = bp_unpack_value (&bp, COLUMN_BITS_BIT);
}


/* Read in from STREAM and merge a referenced include into the current parsing
   context.  */

static void
pph_in_include (pph_stream *stream)
{
  int old_loc_offset;
  const char *include_name;
  pph_stream *include_stream;
  source_location prev_start_loc = pph_in_source_location (stream);

  /* Simulate highest_location to be as it would be at this point in a non-pph
     compilation.  */
  line_table->highest_location = (prev_start_loc - 1) + pph_loc_offset;

  /* FIXME pph: If we move pph_loc_offset to pph_stream.encoder.r, we could
     have an independent offset for each stream and not have to save and
     restore the state of a global pph_loc_offset as we are doing here.  */
  old_loc_offset = pph_loc_offset;

  include_name = pph_in_string (stream);
  include_stream = pph_read_file (include_name);

  /* Add INCLUDE_STREAM, and the images included by it, to the list
     of included images for STREAM.  */
  pph_add_include (stream, include_stream);

  pph_loc_offset = old_loc_offset;
}


/* Read the line_table from STREAM and merge it in the current line_table.  At
   the same time load includes in the order they were originally included by
   loading them at the point they were referenced in the line_table.

   Returns the source_location of line 1 / col 0 for this include.

   FIXME pph: The line_table is now identical to the non-pph line_table, the
   only problem is that we load line_table entries twice for headers that are
   re-included and are #ifdef guarded; thus shouldn't be replayed.  This is
   a known current issue, so I didn't bother working around it here for now.  */

static source_location
pph_in_line_table_and_includes (pph_stream *stream)
{
  unsigned int old_depth;
  bool first;
  int includer_ix = -1;
  unsigned int used_before = line_table->used;
  int entries_offset = line_table->used - PPH_NUM_IGNORED_LINE_TABLE_ENTRIES;
  enum pph_linetable_marker next_lt_marker = pph_in_linetable_marker (stream);

  for (first = true; next_lt_marker != PPH_LINETABLE_END;
       next_lt_marker = pph_in_linetable_marker (stream))
    {
      if (next_lt_marker == PPH_LINETABLE_REFERENCE)
	{
	  gcc_assert (!first);
	  pph_in_include (stream);
	}
      else
	{
	  struct line_map *lm;

	  linemap_ensure_extra_space_available (line_table);

	  lm = &line_table->maps[line_table->used];

	  pph_in_line_map (stream, lm);

	  if (first)
	    {
	      first = false;

	      pph_loc_offset = (line_table->highest_location + 1)
		               - lm->start_location;

	      includer_ix = line_table->used - 1;

	      gcc_assert (lm->included_from == -1);
	    }

	  gcc_assert (includer_ix != -1);

	  /* When parsing the pph: the header itself wasn't included by
	    anything, now it's included by the file just before it in
	    the current include tree.  */
	  if (lm->included_from == -1)
	    lm->included_from = includer_ix;
	  /* For the other entries in the pph's line_table which were included
	     from another entry, reflect their included_from to the new position
	     of the entry which they were included from.  */
	  else
	    lm->included_from += entries_offset;

	  lm->start_location += pph_loc_offset;

	  line_table->used++;
	}
    }

  /* We used to expect exactly the same number of entries, but files
     included from this PPH file may sometimes not be needed.  For
     example,

	#include "2.pph"
	#include "foo.pph"
	  +-->	#include "1.pph"
		#include "2.pph"
		#include "3.pph"

     When foo.pph was originally created, the line table was built
     with inclusions of 1.pph, 2.pph and 3.pph.  But when compiling
     the main translation unit, we include 2.pph before foo.pph, so
     the inclusion of 2.pph from foo.pph does nothing.  Leaving the
     line table in a different shape than the original compilation.

     Instead of insisting on getting EXPECTED_IN entries, we expect at
     most EXPECTED_IN entries.  */
  {
    unsigned int expected_in = pph_in_uint (stream);
    gcc_assert (line_table->used - used_before <= expected_in);
  }

  line_table->highest_location = pph_loc_offset + pph_in_uint (stream);
  line_table->highest_line = pph_loc_offset + pph_in_uint (stream);

  /* The MAX_COLUMN_HINT can be directly overwritten.  */
  line_table->max_column_hint = pph_in_uint (stream);

  /* The line_table doesn't store the last LC_LEAVE in any given compilation;
     thus we need to replay the LC_LEAVE for the header now.  For that same
     reason, the line_table should currently be in a state representing a depth
     one include deeper then the depth at which this pph was included.  The
     LC_LEAVE replay will then bring the depth back to what it was before
     calling this function.  */
  old_depth = line_table->depth++;
  linemap_add (line_table, LC_LEAVE, 0, NULL, 0);
  gcc_assert (line_table->depth == old_depth);

  return line_table->maps[used_before].start_location;
}


/*********************************************************** record handling */


/* Read and return a record marker from STREAM.  On return, *TAG_P will
   contain the tag for the data type stored in this record.  */
enum pph_record_marker
pph_in_record_marker (pph_stream *stream, enum pph_tag *tag_p)
{
  enum pph_record_marker m = (enum pph_record_marker) pph_in_uchar (stream);
  gcc_assert (m == PPH_RECORD_START
              || m == PPH_RECORD_START_NO_CACHE
              || m == PPH_RECORD_START_MUTATED
	      || m == PPH_RECORD_START_MERGE_KEY
	      || m == PPH_RECORD_START_MERGE_BODY
              || m == PPH_RECORD_END
              || m == PPH_RECORD_IREF
              || m == PPH_RECORD_XREF
              || m == PPH_RECORD_PREF);

  *tag_p = (enum pph_tag) pph_in_uint (stream);
  gcc_assert ((unsigned) *tag_p < (unsigned) PPH_NUM_TAGS);

  return m;
}


/* Read and return a record header from STREAM.  EXPECTED_TAG indicates
   the data type that should be stored in this record.  When a
   PPH_RECORD_START marker is read, the next word read is an index
   into the streamer cache where the rematerialized data structure
   should be stored. When the writer stored this data structure for
   the first time, it added it to its own streamer cache at slot
   number *CACHE_IX_P.

   This way, if the same data structure was written a second time to
   the stream, instead of writing the whole structure again, only the
   index *CACHE_IX_P is written as a PPH_RECORD_IREF record.

   Therefore, when reading a PPH_RECORD_START marker, *CACHE_IX_P will
   contain the slot number where the materialized data should be
   cached at.  When reading a PPH_RECORD_IREF marker, *CACHE_IX_P will
   contain the slot number the reader can find the previously
   materialized structure.

   If the record starts with PPH_RECORD_XREF, this means that the data
   we are about to read is located in the pickle cache of one of
   STREAM's included images.  In this case, the record consists of two
   indices: the first one (*INCLUDE_IX_P) indicates which included
   image contains the data (it is an index into STREAM->INCLUDES), the
   second one indicates which slot in that image's pickle cache we can
   find the data.  */

static inline enum pph_record_marker
pph_in_start_record (pph_stream *stream, unsigned *include_ix_p,
		     unsigned *cache_ix_p, enum pph_tag expected_tag)
{
  enum pph_tag read_tag;
  enum pph_record_marker marker = pph_in_record_marker (stream, &read_tag);

  /* If the caller expects any tree, make sure we get a valid tree code.  */
  if (expected_tag == PPH_any_tree)
    gcc_assert (read_tag < PPH_any_tree);
  else
    gcc_assert (read_tag == expected_tag);

  *include_ix_p = (unsigned) -1;
  *cache_ix_p = (unsigned) -1;

  /* For PPH_RECORD_START and PPH_RECORD_IREF markers, read the
     streamer cache slot where we should store or find the
     rematerialized data structure (see description above).
     Also read the preloaded cache slot in IX for PPH_RECORD_PREF.  */
  if (marker == PPH_RECORD_START
      || marker == PPH_RECORD_IREF
      || marker == PPH_RECORD_PREF
      || marker == PPH_RECORD_START_MERGE_BODY
      || marker == PPH_RECORD_START_MERGE_KEY)
    *cache_ix_p = pph_in_uint (stream);
  else if (marker == PPH_RECORD_XREF
           || marker == PPH_RECORD_START_MUTATED)
    {
      *include_ix_p = pph_in_uint (stream);
      *cache_ix_p = pph_in_uint (stream);
    }
  else if (marker == PPH_RECORD_END || marker == PPH_RECORD_START_NO_CACHE)
    ; /* Nothing to do.  This record will not need cache updates.  */
  else
    gcc_unreachable ();

  return marker;
}


/********************************************************** lexical elements */


/* Read and return a location_t from STREAM.  */

location_t
pph_in_location (pph_stream *stream)
{
  struct bitpack_d bp;
  bool is_builtin;
  unsigned HOST_WIDE_INT n;
  location_t old_loc;

  bp = pph_in_bitpack (stream);
  is_builtin = bp_unpack_value (&bp, 1);

  n = pph_in_uhwi (stream);
  old_loc = (location_t) n;
  gcc_assert (old_loc == n);

  return is_builtin ? old_loc : old_loc + pph_loc_offset;
}


/* Load the tree value associated with TOKEN from STREAM.  */

static void
pph_in_token_value (pph_stream *stream, cp_token *token)
{
  switch (token->type)
    {
      case CPP_TEMPLATE_ID:
      case CPP_NESTED_NAME_SPECIFIER:
	/* FIXME pph - Need to handle struct tree_check.  */
	break;

      case CPP_KEYWORD:
	token->u.value = ridpointers[token->keyword];
	break;

      case CPP_NAME:
      case CPP_CHAR:
      case CPP_WCHAR:
      case CPP_CHAR16:
      case CPP_CHAR32:
      case CPP_NUMBER:
      case CPP_STRING:
      case CPP_WSTRING:
      case CPP_STRING16:
      case CPP_STRING32:
	token->u.value = pph_in_tree (stream);
	break;

      case CPP_PRAGMA:
	/* Nothing to do.  Field pragma_kind has already been loaded.  */
	break;

      default:
	pph_in_bytes (stream, &token->u.value, sizeof (token->u.value));
	gcc_assert (token->u.value == NULL);
    }
}


/* Read and return a token from STREAM.  */

static cp_token *
pph_in_token (pph_stream *stream)
{
  cp_token *token = ggc_alloc_cleared_cp_token ();

  /* Do not read the whole structure, the token value has
     dynamic size as it contains swizzled pointers.
     FIXME pph, restructure to allow bulk reads of the whole
     section.  */
  pph_in_bytes (stream, token, sizeof (cp_token) - sizeof (void *));

  /* FIXME pph.  Use an arbitrary (but valid) location to avoid
     confusing the rest of the compiler for now.  */
  token->location = input_location;

  /* FIXME pph: verify that pph_in_token_value works with no tokens.  */
  pph_in_token_value (stream, token);

  return token;
}


/* Read and return a cp_token_cache instance from STREAM.  */

static cp_token_cache *
pph_in_token_cache (pph_stream *stream)
{
  unsigned i, num;
  cp_token *first, *last;

  num = pph_in_uint (stream);
  for (last = first = NULL, i = 0; i < num; i++)
    {
      last = pph_in_token (stream);
      if (first == NULL)
	first = last;
    }

  return cp_token_cache_new (first, last);
}


/******************************************************************* vectors */


/* Read and return a gc VEC of trees from STREAM.  */

static VEC(tree,gc) *
pph_in_tree_vec (pph_stream *stream)
{
  HOST_WIDE_INT i, num;
  VEC(tree,gc) *v;

  num = pph_in_hwi (stream);
  v = NULL;
  for (i = 0; i < num; i++)
    {
      tree t = pph_in_tree (stream);
      VEC_safe_push (tree, gc, v, t);
    }

  return v;
}


/* Read and return a gc VEC of qualified_typedef_usage_t from STREAM.  */

static VEC(qualified_typedef_usage_t,gc) *
pph_in_qual_use_vec (pph_stream *stream)
{
  unsigned i, num;
  VEC(qualified_typedef_usage_t,gc) *v;

  num = pph_in_uint (stream);
  v = NULL;
  for (i = 0; i < num; i++)
    {
      qualified_typedef_usage_t q;
      q.typedef_decl = pph_in_tree (stream);
      q.context = pph_in_tree (stream);
      q.locus = pph_in_location (stream);
      VEC_safe_push (qualified_typedef_usage_t, gc, v, &q);
    }

  return v;
}


/* Read the vector V of tree_pair_s instances from STREAM.  */

static VEC(tree_pair_s,gc) *
pph_in_tree_pair_vec (pph_stream *stream)
{
  unsigned i, num;
  VEC(tree_pair_s,gc) *v;

  num = pph_in_uint (stream);
  for (i = 0, v = NULL; i < num; i++)
    {
      tree_pair_s p;
      p.purpose = pph_in_tree (stream);
      p.value = pph_in_tree (stream);
      VEC_safe_push (tree_pair_s, gc, v, &p);
    }

  return v;
}


/******************************************************************** chains */


/* Read a chain of ASTs from STREAM.  */

static tree
pph_in_chain (pph_stream *stream)
{
  return streamer_read_chain (stream->encoder.r.ib, stream->encoder.r.data_in);
}


/* Read a chain of AST merge keys from STREAM.  Merge each tree
   into *CHAIN.  IPATH_HASH is the hash value of the include path
   from STREAM to the root of the include tree.  */

static void
pph_in_merge_key_chain (pph_stream *stream, tree *chain, hashval_t ipath_hash)
{
  unsigned i;
  HOST_WIDE_INT count;

  count = pph_in_hwi (stream);
  for (i = 0; i < count; i++)
    pph_in_merge_key_tree (stream, chain, ipath_hash);
}


/* Read a chain of AST merge bodies from STREAM.  */

static void
pph_in_merge_body_chain (pph_stream *stream)
{
  unsigned i;
  HOST_WIDE_INT count;

  count = pph_in_hwi (stream);
  for (i = 0; i < count; i++)
    pph_in_tree (stream);
}


/* Merge table of contents.  This TOC is used to decide whether a
   symbol has already been merged into a given compilation context.
   Compilation contexts are usually tree chains (e.g.,
   scope_chain->bindings->names), but they can be any stable memory
   address.

   This TOC is indexed by two values: the merge key read by
   pph_in_merge_key_tree and the context in which we are doing this
   merge.  */
static htab_t merge_toc = NULL;

/* Table of contents entry.  */
typedef struct {
  /* Tree being matched.  */
  tree expr;

  /* Context where this tree should be inserted into.  */
  tree *context;

  /* Name of the tree (from pph_merge_name).  */
  const char *name;

  /* Hash value representing the include path starting at the image
     where EXPR resides up to the root of the include tree.  Objects
     found in any of these PPH images do not need to be merged.  They
     were already emitted as external references and merged when
     the PPH images were being generated.  */
  hashval_t ipath_hash;
} merge_toc_entry;


/* Hash and equivalence functions for the merge TOC.  */

static hashval_t
htab_merge_key_hash (const void *p)
{
  const merge_toc_entry *key = (const merge_toc_entry *) p;
  hashval_t context_val = htab_hash_pointer (key->context);
  hashval_t name_val = htab_hash_string (key->name);
  return iterative_hash_hashval_t (context_val, name_val);
}

static int
htab_merge_key_eq (const void *p1, const void *p2)
{
  const merge_toc_entry *key1 = (const merge_toc_entry *) p1;
  const merge_toc_entry *key2 = (const merge_toc_entry *) p2;

  /* Matches inside the same include path are not interesting.  These
     symbols have already been merged.  */
  if (key1->ipath_hash == key2->ipath_hash)
    return false;

  if (key1->context != key2->context)
    return false;

  if (TREE_CODE (key1->expr) != TREE_CODE (key2->expr))
    return false;

  if (key1->name == NULL || key2->name == NULL)
    return false;

  return strcmp (key1->name, key2->name) == 0;
}


/* Look in TOC for an existing tree matching KEY.  */

static tree
pph_toc_lookup (htab_t toc, merge_toc_entry *key)
{
  void *slot = htab_find (toc, key);
  tree expr = NULL;

  if (slot)
    {
      merge_toc_entry *e = (merge_toc_entry *) slot;
      expr = e->expr;
    }

  return expr;
}


/* Insert KEY into TOC.  */

static void
pph_toc_add (htab_t toc, merge_toc_entry *key)
{
  void **slot;
  merge_toc_entry *entry;

  slot = htab_find_slot (toc, key, INSERT);
  gcc_assert (*slot == NULL);

  entry = XCNEW (merge_toc_entry);
  memcpy (entry, key, sizeof (*key));
  *slot = (void *) entry;
}


/* Prepend a tree EXPR to a CHAIN.  */

static tree
pph_prepend_to_chain (tree expr, tree *chain)
{
  DECL_CHAIN (expr) = *chain;
  *chain = expr;
  return expr;
}


/* Merge the just-read header for tree EXPR with NAME onto the CHAIN.
   IPATH_HASH is the hash value of the include path from STREAM to the
   root of the include tree.  */

static tree
pph_merge_into_chain (tree expr, const char *name, tree *chain,
		      hashval_t ipath_hash)
{
  merge_toc_entry key = { expr, chain, name, ipath_hash };
  tree found = pph_toc_lookup (merge_toc, &key);
  if (!found)
    {
      pph_toc_add (merge_toc, &key);

      if (flag_pph_debug >= 3)
        fprintf (pph_logfile, "PPH: %s NOT found on chain\n", name);

      return pph_prepend_to_chain (expr, chain);
    }

  if (flag_pph_debug >= 3)
    fprintf (pph_logfile, "PPH: %s FOUND on chain\n", name);

  gcc_assert (TREE_CODE (found) == TREE_CODE (expr));

  return found;
}


/****************************************************************** bindings */


/* Forward declaration to break cyclic dependencies.  */
static cp_binding_level *pph_in_binding_level (pph_stream *);

/* Helper for pph_in_cxx_binding.  Read and return a cxx_binding
   instance from STREAM.  */

static cxx_binding *
pph_in_cxx_binding_1 (pph_stream *stream)
{
  struct bitpack_d bp;
  cxx_binding *cb;
  tree value, type;
  enum pph_record_marker marker;
  unsigned ix, image_ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cxx_binding);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (cxx_binding *) pph_cache_find (stream, marker, image_ix, ix,
					   PPH_cxx_binding);

  value = pph_in_tree (stream);
  type = pph_in_tree (stream);
  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_cxx_binding, cb,
                      cxx_binding_make (value, type));
  cb->scope = pph_in_binding_level (stream);
  bp = pph_in_bitpack (stream);
  cb->value_is_inherited = bp_unpack_value (&bp, 1);
  cb->is_local = bp_unpack_value (&bp, 1);

  return cb;
}


/* Read and return an instance of cxx_binding from STREAM.  */

static cxx_binding *
pph_in_cxx_binding (pph_stream *stream)
{
  cxx_binding *curr, *prev, *cb;

  /* Read the current binding first.  */
  cb = pph_in_cxx_binding_1 (stream);

  /* Read the list of previous bindings.  */
  for (curr = cb; curr; curr = prev)
    {
      prev = pph_in_cxx_binding_1 (stream);
      curr->previous = prev;
    }

  return cb;
}


/* Read all the fields of cp_class_binding instance CB to STREAM.  */

static cp_class_binding *
pph_in_class_binding (pph_stream *stream)
{
  cp_class_binding *cb;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cp_class_binding);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (cp_class_binding *) pph_cache_find (stream, marker, image_ix, ix,
						PPH_cp_class_binding);

  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_cp_class_binding, cb,
                      ggc_alloc_cleared_cp_class_binding ());
  cb->base = pph_in_cxx_binding (stream);
  cb->identifier = pph_in_tree (stream);

  return cb;
}


/* Read and return an instance of cp_label_binding from STREAM.  */

static cp_label_binding *
pph_in_label_binding (pph_stream *stream)
{
  cp_label_binding *lb;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cp_label_binding);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (cp_label_binding *) pph_cache_find (stream, marker, image_ix, ix,
						PPH_cp_label_binding);

  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_cp_label_binding, lb,
                      ggc_alloc_cleared_cp_label_binding ());
  lb->label = pph_in_tree (stream);
  lb->prev_value = pph_in_tree (stream);

  return lb;
}


/* Read the contents of binding level BL from STREAM.  */

static void
pph_in_binding_level_1 (pph_stream *stream, cp_binding_level *bl)
{
  unsigned i, num;
  struct bitpack_d bp;

  bl->this_entity = pph_in_tree (stream);
  bl->static_decls = pph_in_tree_vec (stream);

  num = pph_in_uint (stream);
  bl->class_shadowed = NULL;
  for (i = 0; i < num; i++)
    {
      cp_class_binding *cb = pph_in_class_binding (stream);
      VEC_safe_push (cp_class_binding, gc, bl->class_shadowed, cb);
    }

  bl->type_shadowed = pph_in_tree (stream);

  num = pph_in_uint (stream);
  bl->shadowed_labels = NULL;
  for (i = 0; i < num; i++)
    {
      cp_label_binding *sl = pph_in_label_binding (stream);
      VEC_safe_push (cp_label_binding, gc, bl->shadowed_labels, sl);
    }

  bl->blocks = pph_in_tree (stream);
  bl->level_chain = pph_in_binding_level (stream);
  bl->dead_vars_from_for = pph_in_tree_vec (stream);
  bl->statement_list = pph_in_chain (stream);
  bl->binding_depth = pph_in_uint (stream);

  bp = pph_in_bitpack (stream);
  bl->kind = (enum scope_kind) bp_unpack_value (&bp, 4);
  bl->keep = bp_unpack_value (&bp, 1);
  bl->more_cleanups_ok = bp_unpack_value (&bp, 1);
  bl->have_cleanups = bp_unpack_value (&bp, 1);
}


/* Read and return an instance of cp_binding_level from STREAM.
   This function is for use when we will not be merging the binding level.  */

static cp_binding_level *
pph_in_binding_level (pph_stream *stream)
{
  unsigned image_ix, ix;
  cp_binding_level *bl;
  enum pph_record_marker marker;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cp_binding_level);
  if (marker == PPH_RECORD_END)
    {
      return NULL;
    }
  else if (pph_is_reference_marker (marker))
    {
      return (cp_binding_level *)
          pph_cache_find (stream, marker, image_ix, ix, PPH_cp_binding_level);
    }

  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_cp_binding_level, bl,
		      ggc_alloc_cleared_cp_binding_level ());

  bl->names = pph_in_chain (stream);
  bl->namespaces = pph_in_chain (stream);
  bl->usings = pph_in_chain (stream);
  bl->using_directives = pph_in_chain (stream);
  pph_in_binding_level_1 (stream, bl);

  return bl;
}


/********************************************************** tree aux classes */


/* Read and return an instance of struct language_function from STREAM.  */

static struct language_function *
pph_in_language_function (pph_stream *stream)
{
  struct bitpack_d bp;
  struct language_function *lf;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_language_function);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (struct language_function *) pph_cache_find (stream, marker,
							image_ix, ix,
							PPH_language_function);

  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_language_function, lf,
                      ggc_alloc_cleared_language_function ());
  lf->base.x_stmt_tree.x_cur_stmt_list = pph_in_tree_vec (stream);
  lf->base.x_stmt_tree.stmts_are_full_exprs_p = pph_in_uint (stream);
  lf->x_cdtor_label = pph_in_tree (stream);
  lf->x_current_class_ptr = pph_in_tree (stream);
  lf->x_current_class_ref = pph_in_tree (stream);
  lf->x_eh_spec_block = pph_in_tree (stream);
  lf->x_in_charge_parm = pph_in_tree (stream);
  lf->x_vtt_parm = pph_in_tree (stream);
  lf->x_return_value = pph_in_tree (stream);
  bp = pph_in_bitpack (stream);
  lf->returns_value = bp_unpack_value (&bp, 1);
  lf->returns_null = bp_unpack_value (&bp, 1);
  lf->returns_abnormally = bp_unpack_value (&bp, 1);
  lf->x_in_function_try_handler = bp_unpack_value (&bp, 1);
  lf->x_in_base_initializer = bp_unpack_value (&bp, 1);
  lf->can_throw = bp_unpack_value (&bp, 1);

  /* FIXME pph.  We are not reading lf->x_named_labels.  */

  lf->bindings = pph_in_binding_level (stream);
  lf->x_local_names = pph_in_tree_vec (stream);

  /* FIXME pph.  We are not reading lf->extern_decl_map.  */

  return lf;
}


/* Read applicable fields of struct function from STREAM.  Associate
   the read structure to DECL.  */

static void
pph_in_struct_function (pph_stream *stream, tree decl)
{
  size_t count, i;
  unsigned image_ix, ix;
  enum pph_record_marker marker;
  struct function *fn;
  tree t;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_function);
  if (marker == PPH_RECORD_END)
    return;
  else if (pph_is_reference_marker (marker))
    {
      fn = (struct function *) pph_cache_find (stream, marker, image_ix, ix,
					       PPH_function);
      gcc_assert (DECL_STRUCT_FUNCTION (decl) == fn);
      return;
    }

  /* Possibly allocate a new DECL_STRUCT_FUNCTION for DECL.  */
  t = pph_in_tree (stream);
  gcc_assert (t == decl);
  fn = DECL_STRUCT_FUNCTION (decl);
  if (!fn)
    {
      /* The DECL_STRUCT_FUNCTION does not already already exists,
         which implies that we are reading an entirely new function
         and not merging into an existing function.  */
      /* We would normally use ALLOC_AND_REGISTER,
         but allocate_struct_function does not return a pointer.  */
      allocate_struct_function (decl, false);
      fn = DECL_STRUCT_FUNCTION (decl);
    }

  /* Now register it.  */
  pph_cache_insert_at (&stream->cache, fn, ix, PPH_function);

  /* FIXME pph: For now, accept the new version of the fields when merging.  */

  input_struct_function_base (fn, stream->encoder.r.data_in,
			      stream->encoder.r.ib);

  /* struct eh_status *eh;					-- zero init */
  /* struct control_flow_graph *cfg;				-- zero init */
  /* struct gimple_seq_d *gimple_body;				-- zero init */
  /* struct gimple_df *gimple_df;				-- zero init */
  /* struct loops *x_current_loops;				-- zero init */
  /* struct stack_usage *su;					-- zero init */
  /* htab_t value_histograms;					-- zero init */
  /* tree decl;							-- zero init */
  /* tree static_chain_decl;					-- in base */
  /* tree nonlocal_goto_save_area;				-- in base */
  /* tree local_decls;						-- in base */
  /* struct machine_function * machine;				-- zero init */

  fn->language = pph_in_language_function (stream);

  count = pph_in_uint (stream);
  if ( count > 0 )
    {
      fn->used_types_hash = htab_create_ggc (37, htab_hash_pointer,
					     htab_eq_pointer, NULL);
      for (i = 0; i < count;  i++)
	{
	  void **slot;
	  tree type = pph_in_tree (stream);
	  slot = htab_find_slot (fn->used_types_hash, type, INSERT);
	  if (*slot == NULL)
	    *slot = type;
	}
    }
  /* else zero initialized */

  /* int last_stmt_uid;						-- zero init */
  /* int funcdef_no;						-- zero init */
  /* location_t function_start_locus;				-- in base */
  /* location_t function_end_locus;				-- in base */
  /* unsigned int curr_properties;				-- in base */
  /* unsigned int last_verified;				-- zero init */
  /* const char *cannot_be_copied_reason;			-- zero init */

  /* unsigned int va_list_gpr_size : 8;				-- in base */
  /* unsigned int va_list_fpr_size : 8;				-- in base */
  /* unsigned int calls_setjmp : 1;				-- in base */
  /* unsigned int calls_alloca : 1;				-- in base */
  /* unsigned int has_nonlocal_label : 1;			-- in base */
  /* unsigned int cannot_be_copied_set : 1;			-- zero init */
  /* unsigned int stdarg : 1;					-- in base */
  /* unsigned int after_inlining : 1;				-- in base */
  /* unsigned int always_inline_functions_inlined : 1;		-- in base */
  /* unsigned int can_throw_non_call_exceptions : 1;		-- in base */
  /* unsigned int returns_struct : 1;				-- in base */
  /* unsigned int returns_pcc_struct : 1;			-- in base */
  /* unsigned int after_tree_profile : 1;			-- in base */
  /* unsigned int has_local_explicit_reg_vars : 1;		-- in base */
  /* unsigned int is_thunk : 1;					-- in base */
}


/* Read all fields in lang_decl_base instance LDB from STREAM.  */

static void
pph_in_ld_base (pph_stream *stream, struct lang_decl_base *ldb)
{
  struct bitpack_d bp;

  bp = pph_in_bitpack (stream);
  ldb->selector = bp_unpack_value (&bp, 16);
  ldb->language = (enum languages) bp_unpack_value (&bp, 4);
  ldb->use_template = bp_unpack_value (&bp, 2);
  ldb->not_really_extern = bp_unpack_value (&bp, 1);
  ldb->initialized_in_class = bp_unpack_value (&bp, 1);
  ldb->repo_available_p = bp_unpack_value (&bp, 1);
  ldb->threadprivate_or_deleted_p = bp_unpack_value (&bp, 1);
  ldb->anticipated_p = bp_unpack_value (&bp, 1);
  ldb->friend_attr = bp_unpack_value (&bp, 1);
  ldb->template_conv_p = bp_unpack_value (&bp, 1);
  ldb->odr_used = bp_unpack_value (&bp, 1);
  ldb->u2sel = bp_unpack_value (&bp, 1);
}


/* Read all the fields in lang_decl_min instance LDM from STREAM.  */

static void
pph_in_ld_min (pph_stream *stream, struct lang_decl_min *ldm)
{
  ldm->template_info = pph_in_tree (stream);
  if (ldm->base.u2sel == 0)
    ldm->u2.access = pph_in_tree (stream);
  else if (ldm->base.u2sel == 1)
    ldm->u2.discriminator = pph_in_uint (stream);
  else
    gcc_unreachable ();
}


/* Read all the fields of lang_decl_fn instance LDF from STREAM.  */

static void
pph_in_ld_fn (pph_stream *stream, struct lang_decl_fn *ldf)
{
  struct bitpack_d bp;

  /* Read all the fields in lang_decl_min.  */
  pph_in_ld_min (stream, &ldf->min);

  bp = pph_in_bitpack (stream);
  ldf->operator_code = (enum tree_code) bp_unpack_value (&bp, 16);
  ldf->global_ctor_p = bp_unpack_value (&bp, 1);
  ldf->global_dtor_p = bp_unpack_value (&bp, 1);
  ldf->constructor_attr = bp_unpack_value (&bp, 1);
  ldf->destructor_attr = bp_unpack_value (&bp, 1);
  ldf->assignment_operator_p = bp_unpack_value (&bp, 1);
  ldf->static_function = bp_unpack_value (&bp, 1);
  ldf->pure_virtual = bp_unpack_value (&bp, 1);
  ldf->defaulted_p = bp_unpack_value (&bp, 1);
  ldf->has_in_charge_parm_p = bp_unpack_value (&bp, 1);
  ldf->has_vtt_parm_p = bp_unpack_value (&bp, 1);
  ldf->pending_inline_p = bp_unpack_value (&bp, 1);
  ldf->nonconverting = bp_unpack_value (&bp, 1);
  ldf->thunk_p = bp_unpack_value (&bp, 1);
  ldf->this_thunk_p = bp_unpack_value (&bp, 1);
  ldf->hidden_friend_p = bp_unpack_value (&bp, 1);

  ldf->befriending_classes = pph_in_tree (stream);
  ldf->context = pph_in_tree (stream);

  if (ldf->thunk_p == 0)
    ldf->u5.cloned_function = pph_in_tree (stream);
  else if (ldf->thunk_p == 1)
    ldf->u5.fixed_offset = pph_in_uint (stream);
  else
    gcc_unreachable ();

  if (ldf->pending_inline_p == 1)
    ldf->u.pending_inline_info = pph_in_token_cache (stream);
  else if (ldf->pending_inline_p == 0)
    ldf->u.saved_language_function = pph_in_language_function (stream);
}


/* Read all the fields of lang_decl_ns instance LDNS from STREAM.  */

static void
pph_in_ld_ns (pph_stream *stream, struct lang_decl_ns *ldns)
{
  ldns->level = pph_in_binding_level (stream);
}


/* Read all the fields of lang_decl_parm instance LDP from STREAM.  */

static void
pph_in_ld_parm (pph_stream *stream, struct lang_decl_parm *ldp)
{
  ldp->level = pph_in_uint (stream);
  ldp->index = pph_in_uint (stream);
}


/* Read language specific data in DECL from STREAM.  */

static void
pph_in_lang_specific (pph_stream *stream, tree decl)
{
  struct lang_decl *ld;
  struct lang_decl_base *ldb;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_lang_decl);
  if (marker == PPH_RECORD_END)
    return;
  else if (pph_is_reference_marker (marker))
    {
      DECL_LANG_SPECIFIC (decl) =
	(struct lang_decl *) pph_cache_find (stream, marker, image_ix, ix,
					     PPH_lang_decl);
      return;
    }

  /* Allocate a lang_decl structure for DECL.  */
  retrofit_lang_decl (decl);
  ld = DECL_LANG_SPECIFIC (decl);

  /* Now register it.  We would normally use ALLOC_AND_REGISTER,
     but retrofit_lang_decl does not return a pointer.  */
  pph_cache_insert_at (&stream->cache, ld, ix, PPH_lang_decl);

  /* Read all the fields in lang_decl_base.  */
  ldb = &ld->u.base;
  pph_in_ld_base (stream, ldb);

  if (ldb->selector == 0)
    {
      /* Read all the fields in lang_decl_min.  */
      pph_in_ld_min (stream, &ld->u.min);
    }
  else if (ldb->selector == 1)
    {
      /* Read all the fields in lang_decl_fn.  */
      pph_in_ld_fn (stream, &ld->u.fn);
    }
  else if (ldb->selector == 2)
    {
      /* Read all the fields in lang_decl_ns.  */
      pph_in_ld_ns (stream, &ld->u.ns);
    }
  else if (ldb->selector == 3)
    {
      /* Read all the fields in lang_decl_parm.  */
      pph_in_ld_parm (stream, &ld->u.parm);
    }
  else
    gcc_unreachable ();
}


/********************************************************* tree base classes */


/* Read in the tree_common fields.  */

static void
pph_in_tree_common (pph_stream *stream, tree t)
{
  /* The 'struct tree_typed typed' base class is handled in LTO.  */
  TREE_CHAIN (t) = pph_in_tree (stream);
}


/* Read all the fields in lang_type_header instance LTH from STREAM.  */

static void
pph_in_lang_type_header (pph_stream *stream, struct lang_type_header *lth)
{
  struct bitpack_d bp;

  bp = pph_in_bitpack (stream);
  lth->is_lang_type_class = bp_unpack_value (&bp, 1);
  lth->has_type_conversion = bp_unpack_value (&bp, 1);
  lth->has_copy_ctor = bp_unpack_value (&bp, 1);
  lth->has_default_ctor = bp_unpack_value (&bp, 1);
  lth->const_needs_init = bp_unpack_value (&bp, 1);
  lth->ref_needs_init = bp_unpack_value (&bp, 1);
  lth->has_const_copy_assign = bp_unpack_value (&bp, 1);
}


/* Read a struct sorted_fields_type instance SFT to STREAM.  */

static struct sorted_fields_type *
pph_in_sorted_fields_type (pph_stream *stream)
{
  unsigned i, num_fields;
  struct sorted_fields_type *v;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_sorted_fields_type);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (struct sorted_fields_type *)
	  pph_cache_find (stream, marker, image_ix, ix, PPH_sorted_fields_type);

  num_fields = pph_in_uint (stream);
  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_sorted_fields_type, v,
                      sorted_fields_type_new (num_fields));
  for (i = 0; i < num_fields; i++)
    v->elts[i] = pph_in_tree (stream);

  return v;
}


/* Read all the fields in lang_type_class instance LTC to STREAM.  */

static void
pph_in_lang_type_class (pph_stream *stream, struct lang_type_class *ltc)
{
  struct bitpack_d bp;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  ltc->align = pph_in_uchar (stream);

  bp = pph_in_bitpack (stream);
  ltc->has_mutable = bp_unpack_value (&bp, 1);
  ltc->com_interface = bp_unpack_value (&bp, 1);
  ltc->non_pod_class = bp_unpack_value (&bp, 1);
  ltc->nearly_empty_p = bp_unpack_value (&bp, 1);
  ltc->user_align = bp_unpack_value (&bp, 1);
  ltc->has_copy_assign = bp_unpack_value (&bp, 1);
  ltc->has_new = bp_unpack_value (&bp, 1);
  ltc->has_array_new = bp_unpack_value (&bp, 1);
  ltc->gets_delete = bp_unpack_value (&bp, 2);
  ltc->interface_only = bp_unpack_value (&bp, 1);
  ltc->interface_unknown = bp_unpack_value (&bp, 1);
  ltc->contains_empty_class_p = bp_unpack_value (&bp, 1);
  ltc->anon_aggr = bp_unpack_value (&bp, 1);
  ltc->non_zero_init = bp_unpack_value (&bp, 1);
  ltc->empty_p = bp_unpack_value (&bp, 1);
  ltc->vec_new_uses_cookie = bp_unpack_value (&bp, 1);
  ltc->declared_class = bp_unpack_value (&bp, 1);
  ltc->diamond_shaped = bp_unpack_value (&bp, 1);
  ltc->repeated_base = bp_unpack_value (&bp, 1);
  ltc->being_defined = bp_unpack_value (&bp, 1);
  ltc->java_interface = bp_unpack_value (&bp, 1);
  ltc->debug_requested = bp_unpack_value (&bp, 1);
  ltc->fields_readonly = bp_unpack_value (&bp, 1);
  ltc->use_template = bp_unpack_value (&bp, 2);
  ltc->ptrmemfunc_flag = bp_unpack_value (&bp, 1);
  ltc->was_anonymous = bp_unpack_value (&bp, 1);
  ltc->lazy_default_ctor = bp_unpack_value (&bp, 1);
  ltc->lazy_copy_ctor = bp_unpack_value (&bp, 1);
  ltc->lazy_copy_assign = bp_unpack_value (&bp, 1);
  ltc->lazy_destructor = bp_unpack_value (&bp, 1);
  ltc->has_const_copy_ctor = bp_unpack_value (&bp, 1);
  ltc->has_complex_copy_ctor = bp_unpack_value (&bp, 1);
  ltc->has_complex_copy_assign = bp_unpack_value (&bp, 1);
  ltc->non_aggregate = bp_unpack_value (&bp, 1);
  ltc->has_complex_dflt = bp_unpack_value (&bp, 1);
  ltc->has_list_ctor = bp_unpack_value (&bp, 1);
  ltc->non_std_layout = bp_unpack_value (&bp, 1);
  ltc->is_literal = bp_unpack_value (&bp, 1);
  ltc->lazy_move_ctor = bp_unpack_value (&bp, 1);
  ltc->lazy_move_assign = bp_unpack_value (&bp, 1);
  ltc->has_complex_move_ctor = bp_unpack_value (&bp, 1);
  ltc->has_complex_move_assign = bp_unpack_value (&bp, 1);
  ltc->has_constexpr_ctor = bp_unpack_value (&bp, 1);

  ltc->primary_base = pph_in_tree (stream);
  ltc->vcall_indices = pph_in_tree_pair_vec (stream);
  ltc->vtables = pph_in_tree (stream);
  ltc->typeinfo_var = pph_in_tree (stream);
  ltc->vbases = pph_in_tree_vec (stream);

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_binding_table);
  if (marker == PPH_RECORD_START)
    {
      ltc->nested_udts = pph_in_binding_table (stream);
      pph_cache_insert_at (&stream->cache, ltc->nested_udts, ix,
                           PPH_binding_table);
    }
  else if (pph_is_reference_marker (marker))
    ltc->nested_udts = (binding_table) pph_cache_find (stream, marker,
						       image_ix, ix,
						       PPH_binding_table);

  ltc->as_base = pph_in_tree (stream);
  ltc->pure_virtuals = pph_in_tree_vec (stream);
  ltc->friend_classes = pph_in_tree (stream);
  ltc->methods = pph_in_tree_vec (stream);
  ltc->key_method = pph_in_tree (stream);
  ltc->decl_list = pph_in_tree (stream);
  ltc->template_info = pph_in_tree (stream);
  ltc->befriending_classes = pph_in_tree (stream);
  ltc->objc_info = pph_in_tree (stream);
  ltc->sorted_fields = pph_in_sorted_fields_type (stream);
  ltc->lambda_expr = pph_in_tree (stream);
}


/* Read all fields of struct lang_type_ptrmem instance LTP from STREAM.  */

static void
pph_in_lang_type_ptrmem (pph_stream *stream,
				  struct lang_type_ptrmem *ltp)
{
  ltp->record = pph_in_tree (stream);
}


/* Read all the fields in struct lang_type from STREAM.  */

static struct lang_type *
pph_in_lang_type (pph_stream *stream)
{
  struct lang_type *lt;
  enum pph_record_marker marker;
  unsigned image_ix, ix;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_lang_type);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (struct lang_type *) pph_cache_find (stream, marker, image_ix, ix,
						PPH_lang_type);

  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_lang_type, lt,
                      ggc_alloc_cleared_lang_type (sizeof (struct lang_type)));

  pph_in_lang_type_header (stream, &lt->u.h);
  if (lt->u.h.is_lang_type_class)
    pph_in_lang_type_class (stream, &lt->u.c);
  else
    pph_in_lang_type_ptrmem (stream, &lt->u.ptrmem);

  return lt;
}


/* Read from STREAM the body of tcc_type node TYPE.  */

static void
pph_in_tcc_type (pph_stream *stream, tree type)
{
  TYPE_LANG_SPECIFIC (type) = pph_in_lang_type (stream);
  TYPE_POINTER_TO (type) = pph_in_tree (stream);
  TYPE_REFERENCE_TO (type) = pph_in_tree (stream);
  TYPE_NEXT_VARIANT (type) = pph_in_tree (stream);
  /* FIXME pph - Streaming TYPE_CANONICAL generates many type comparison
     failures.  Why?  */
  /* FIXME pph: apparently redundant.  */
  TREE_CHAIN (type) = pph_in_tree (stream);

  /* The type values cache is built as constants are instantiated,
     so we only stream it on the nodes that use it for
     other purposes.  */
  switch (TREE_CODE (type))
    {
    case BOUND_TEMPLATE_TEMPLATE_PARM:
    case DECLTYPE_TYPE:
    case TEMPLATE_TEMPLATE_PARM:
    case TEMPLATE_TYPE_PARM:
    case TYPENAME_TYPE:
    case TYPEOF_TYPE:
      TYPE_VALUES_RAW (type) = pph_in_tree (stream);
      break;

    default:
      break;
    }

  /* If TYPE has a METHOD_VEC, we need to resort it.  Name lookup in
     classes relies on the specific ordering of the class method
     pointers.  Since we generally instantiate them in a different
     order than the original compile, the pointer values will be
     different.  This will cause name lookups to fail, unless we
     resort the vector.  */
  if (CLASS_TYPE_P (type) && CLASSTYPE_METHOD_VEC (type))
    finish_struct_methods (type);
}


/* Read from STREAM the body of tcc_declaration tree DECL.  */

static void
pph_in_tcc_declaration (pph_stream *stream, tree decl)
{
  pph_in_lang_specific (stream, decl);
  DECL_INITIAL (decl) = pph_in_tree (stream);

  /* The tree streamer only writes DECL_CHAIN for PARM_DECL nodes.
     We need to read DECL_CHAIN for variables and functions because
     they are sometimes chained together in places other than regular
     tree chains.  For example in BINFO_VTABLEs, the decls are chained
     together).  */
  if (TREE_CODE (decl) == VAR_DECL
      || TREE_CODE (decl) == FUNCTION_DECL)
    DECL_CHAIN (decl) = pph_in_tree (stream);

  /* Handle some individual decl nodes.  */
  switch (TREE_CODE (decl))
    {
    case FUNCTION_DECL:
      DECL_SAVED_TREE (decl) = pph_in_tree (stream);
      break;

    case TYPE_DECL:
      DECL_ORIGINAL_TYPE (decl) = pph_in_tree (stream);
      break;

    case TEMPLATE_DECL:
      DECL_TEMPLATE_RESULT (decl) = pph_in_tree (stream);
      DECL_TEMPLATE_PARMS (decl) = pph_in_tree (stream);
      break;

    default:
      break;
    }

  /* When the declaration was compiled originally, the parser marks
     it DECL_EXTERNAL, to avoid emitting it more than once.  It also
     remembers the true external state in DECL_NOT_REALLY_EXTERN.  So,
     if both bits are set, the declaration should not be considered
     external.  */
  if (TREE_CODE (decl) == FUNCTION_DECL
      && DECL_LANG_SPECIFIC (decl)
      && DECL_NOT_REALLY_EXTERN (decl)
      && DECL_EXTERNAL (decl))
    {
      DECL_EXTERNAL (decl) = 0;
      DECL_NOT_REALLY_EXTERN (decl) = 0;
    }
}


/******************************************************** tree head and body */


/* Read the body fields of EXPR from STREAM.  */

static void
pph_in_tree_body (pph_stream *stream, tree expr)
{
  struct lto_input_block *ib = stream->encoder.r.ib;
  struct data_in *data_in = stream->encoder.r.data_in;
  bool handled_p;

  /* Read the language-independent parts of EXPR's body.  */
  streamer_read_tree_body (ib, data_in, expr);

  /* Handle common tree code classes first.  */
  handled_p = true;
  switch (TREE_CODE_CLASS (TREE_CODE (expr)))
    {
      case tcc_declaration:
	pph_in_tcc_declaration (stream, expr);
	break;

      case tcc_type:
	pph_in_tcc_type (stream, expr);
	break;

      case tcc_constant:
	if (TREE_CODE (expr) == PTRMEM_CST)
	  {
	    pph_in_tree_common (stream, expr);
	    PTRMEM_CST_MEMBER (expr) = pph_in_tree (stream);
	  }
	break;

      case tcc_expression:
      case tcc_unary:
      case tcc_binary:
      case tcc_vl_exp:
      case tcc_reference:
      case tcc_comparison:
      case tcc_statement:
	/* These tree classes are completely handled by the tree streamer.  */
	break;

      default:
	handled_p = false;
	break;
    }

  /* If we've already handled the tree, we are done.  */
  if (handled_p)
    return;

  /* Only tcc_exceptional tree codes are left to handle.  */
  gcc_assert (TREE_CODE_CLASS (TREE_CODE (expr)) == tcc_exceptional);

  switch (TREE_CODE (expr))
    {
    case STATEMENT_LIST:
      {
        HOST_WIDE_INT i, num_trees = pph_in_uint (stream);
        for (i = 0; i < num_trees; i++)
	  {
	    tree stmt = pph_in_tree (stream);
	    append_to_statement_list_force (stmt, &expr);
	  }
      }
      break;

    case OVERLOAD:
      pph_in_tree_common (stream, expr);
      OVL_FUNCTION (expr) = pph_in_tree (stream);
      break;

    case IDENTIFIER_NODE:
      IDENTIFIER_NAMESPACE_BINDINGS (expr) = pph_in_cxx_binding (stream);
      IDENTIFIER_BINDING (expr) = pph_in_cxx_binding (stream);
      IDENTIFIER_TEMPLATE (expr) = pph_in_tree (stream);
      IDENTIFIER_LABEL_VALUE (expr) = pph_in_tree (stream);
      REAL_IDENTIFIER_TYPE_VALUE (expr) = pph_in_tree (stream);
      break;

    case BASELINK:
      pph_in_tree_common (stream, expr);
      BASELINK_BINFO (expr) = pph_in_tree (stream);
      BASELINK_FUNCTIONS (expr) = pph_in_tree (stream);
      BASELINK_ACCESS_BINFO (expr) = pph_in_tree (stream);
      break;

    case TEMPLATE_INFO:
      pph_in_tree_common (stream, expr);
      TI_TYPEDEFS_NEEDING_ACCESS_CHECKING (expr)
          = pph_in_qual_use_vec (stream);
      break;

    case DEFAULT_ARG:
      pph_in_tree_common (stream, expr);
      DEFARG_TOKENS (expr) = pph_in_token_cache (stream);
      DEFARG_INSTANTIATIONS (expr) = pph_in_tree_vec (stream);
      break;

    case STATIC_ASSERT:
      pph_in_tree_common (stream, expr);
      STATIC_ASSERT_CONDITION (expr) = pph_in_tree (stream);
      STATIC_ASSERT_MESSAGE (expr) = pph_in_tree (stream);
      STATIC_ASSERT_SOURCE_LOCATION (expr) = pph_in_location (stream);
      break;

    case ARGUMENT_PACK_SELECT:
      pph_in_tree_common (stream, expr);
      ARGUMENT_PACK_SELECT_FROM_PACK (expr) = pph_in_tree (stream);
      ARGUMENT_PACK_SELECT_INDEX (expr) = pph_in_uint (stream);
      break;

    case TRAIT_EXPR:
      pph_in_tree_common (stream, expr);
      TRAIT_EXPR_TYPE1 (expr) = pph_in_tree (stream);
      TRAIT_EXPR_TYPE2 (expr) = pph_in_tree (stream);
      TRAIT_EXPR_KIND (expr) = (enum cp_trait_kind) pph_in_uint (stream);
      break;

    case LAMBDA_EXPR:
      pph_in_tree_common (stream, expr);
      LAMBDA_EXPR_LOCATION (expr) = pph_in_location (stream);
      LAMBDA_EXPR_CAPTURE_LIST (expr) = pph_in_tree (stream);
      LAMBDA_EXPR_THIS_CAPTURE (expr) = pph_in_tree (stream);
      LAMBDA_EXPR_RETURN_TYPE (expr) = pph_in_tree (stream);
      LAMBDA_EXPR_EXTRA_SCOPE (expr) = pph_in_tree (stream);
      LAMBDA_EXPR_DISCRIMINATOR (expr) = pph_in_uint (stream);
      break;

    case TREE_VEC:
      /* TREE_VECs hold template argument lists.  */
      NON_DEFAULT_TEMPLATE_ARGS_COUNT (expr) = pph_in_tree (stream);
      break;

    case PLACEHOLDER_EXPR:
      TREE_TYPE (expr) = pph_in_tree (stream);
      break;

    case TEMPLATE_PARM_INDEX:
      pph_in_tree_common (stream, expr);
      TEMPLATE_PARM_IDX (expr) = pph_in_uint (stream);
      TEMPLATE_PARM_LEVEL (expr) = pph_in_uint (stream);
      TEMPLATE_PARM_ORIG_LEVEL (expr) = pph_in_uint (stream);
      TEMPLATE_PARM_NUM_SIBLINGS (expr) = pph_in_uint (stream);
      TEMPLATE_PARM_DECL (expr) = pph_in_tree (stream);
      break;

    case DEFERRED_NOEXCEPT:
      DEFERRED_NOEXCEPT_PATTERN (expr) = pph_in_tree (stream);
      DEFERRED_NOEXCEPT_ARGS (expr) = pph_in_tree (stream);
      break;

    /* TREES ALREADY HANDLED */
    case ERROR_MARK:
    case TREE_LIST:
    case BLOCK:
    case CONSTRUCTOR:
    case SSA_NAME:
    case TREE_BINFO:
      break;

    /* TREES UNIMPLEMENTED */
    case OMP_CLAUSE:
    case OPTIMIZATION_NODE:
    case TARGET_OPTION_NODE:
      fatal_error ("PPH: unimplemented tree node '%s'",
		   pph_tree_code_text (TREE_CODE (expr)));
      break;

    /* TREES UNRECOGNIZED */
    default:
      fatal_error ("PPH: unrecognized tree node '%s'",
                   pph_tree_code_text (TREE_CODE (expr)));
    }
}


/* Unpack language-dependent bitfields from BP into EXPR.  */

static void
pph_unpack_value_fields (struct bitpack_d *bp, tree expr)
{
  if (TYPE_P (expr))
    {
      TYPE_LANG_FLAG_0 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_1 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_2 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_3 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_4 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_5 (expr) = bp_unpack_value (bp, 1);
      TYPE_LANG_FLAG_6 (expr) = bp_unpack_value (bp, 1);
    }
  else if (DECL_P (expr))
    {
      DECL_LANG_FLAG_0 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_1 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_2 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_3 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_4 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_5 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_6 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_7 (expr) = bp_unpack_value (bp, 1);
      DECL_LANG_FLAG_8 (expr) = bp_unpack_value (bp, 1);
    }

  TREE_LANG_FLAG_0 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_1 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_2 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_3 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_4 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_5 (expr) = bp_unpack_value (bp, 1);
  TREE_LANG_FLAG_6 (expr) = bp_unpack_value (bp, 1);
}


/* Read a tree header from STREAM and allocate a memory instance for it.
   Return the new tree.  */

static tree
pph_in_tree_header (pph_stream *stream, enum LTO_tags tag)
{
  struct lto_input_block *ib = stream->encoder.r.ib;
  struct data_in *data_in = stream->encoder.r.data_in;
  struct bitpack_d bp;
  tree expr;
  enum tree_code code;

  /* Allocate the tree.  Handle C++-specific codes first.  */
  code = lto_tag_to_tree_code (tag);
  if (code == AGGR_INIT_EXPR)
    {
      unsigned nargs = pph_in_uint (stream);
      expr = build_vl_exp (AGGR_INIT_EXPR, nargs + 3);
    }
  else
    expr = streamer_alloc_tree (ib, data_in, tag);

  /* Read the language-independent bitfields for EXPR.  */
  bp = streamer_read_tree_bitfields (ib, expr);

  /* Unpack all language-dependent bitfields.  */
  pph_unpack_value_fields (&bp, expr);

  return expr;
}


/* Read a merge key from STREAM.  If the merge key read from
   STREAM is not found in *CHAIN, the newly allocated tree is added to
   it.  IPATH_HASH is the hash value of the include path from STREAM to
   the root of the include tree.  */

static tree
pph_in_merge_key_tree (pph_stream *stream, tree *chain, hashval_t ipath_hash)
{
  struct lto_input_block *ib = stream->encoder.r.ib;
  enum pph_record_marker marker;
  unsigned image_ix, ix;
  tree read_expr, expr;
  enum LTO_tags tag;
  const char *name;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_any_tree);
  gcc_assert (marker == PPH_RECORD_START_MERGE_KEY);
  tag = streamer_read_record_start (ib);

  /* Materialize a new node from STREAM.  This will also read all the
     language-independent bitfields for the new tree.  */
  read_expr = pph_in_tree_header (stream, tag);
  name = pph_in_string (stream);

  /* Look for a match in CHAIN to READ_EXPR's header.  If we found a
     match, EXPR will be the existing tree that matches READ_EXPR.
     Otherwise, EXPR is the newly allocated READ_EXPR.  */
  expr = pph_merge_into_chain (read_expr, name, chain, ipath_hash);

  gcc_assert (expr != NULL);

  pph_cache_insert_at (&stream->cache, expr, ix, pph_tree_code_to_tag (expr));

  if (flag_pph_tracer)
    pph_trace_tree (expr, true, expr != read_expr);

  return expr;
}


/* Read and return a tree from STREAM.  */

tree
pph_in_tree (pph_stream *stream)
{
  struct lto_input_block *ib = stream->encoder.r.ib;
  struct data_in *data_in = stream->encoder.r.data_in;
  tree expr = NULL;
  enum pph_record_marker marker;
  unsigned image_ix, ix;
  enum LTO_tags tag;
  tree saved_expr_chain = NULL;

  /* Read record start and test cache.  */
  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_any_tree);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (tree) pph_cache_find (stream, marker, image_ix, ix, PPH_any_tree);
  else if (marker == PPH_RECORD_START || marker == PPH_RECORD_START_NO_CACHE)
    {
      /* This is a new tree that we need to allocate.  Start by
         reading the header fields, so we know how to allocate it
         (different tree nodes need to be allocated in different
         ways).  */
      tag = streamer_read_record_start (ib);
      gcc_assert ((unsigned) tag < (unsigned) LTO_NUM_TAGS);
      if (tag == LTO_builtin_decl)
        {
          /* If we are going to read a built-in function, all we need is
             the code and class.  */
          gcc_assert (marker == PPH_RECORD_START_NO_CACHE);
          return streamer_get_builtin_tree (ib, data_in);
        }
      else if (tag == lto_tree_code_to_tag (INTEGER_CST))
        {
          /* For integer constants we only need the type and its hi/low
             words.  */
          gcc_assert (marker == PPH_RECORD_START_NO_CACHE);
          return streamer_read_integer_cst (ib, data_in);
        }

      /* Materialize a new node from STREAM.  This will also read all the
         language-independent bitfields for the new tree.  */
      expr = pph_in_tree_header (stream, tag);
    }

  gcc_assert (marker == PPH_RECORD_START
              || marker == PPH_RECORD_START_MUTATED
	      || marker == PPH_RECORD_START_MERGE_BODY);

  if (marker == PPH_RECORD_START_MUTATED)
    {
      /* When reading a mutated tree, we only need to re-read its
         body, the tree itself is already in the cache for another
         PPH image.  */
      expr = (tree) pph_cache_find (stream, PPH_RECORD_XREF, image_ix, ix,
				    PPH_any_tree);

      /* Read the internal cache slot where EXPR should be stored at.  */
      ix = pph_in_uint (stream);
    }
  else if (marker == PPH_RECORD_START_MERGE_BODY)
    {
      /* When reading a merge body, the tree has already been allocated
	 and added to STREAM's cache.  All we have to do now is read
	 its body.  FIXME pph, this read should be a merging read; we are
	 overwriting EXPR's fields now.  */
      expr = (tree) pph_cache_get (&stream->cache, ix);
    }

  /* Add the new tree to the cache and read its body.  The tree is
     added to the cache before we read its body to handle circular
     references and references from children nodes.  If we are reading
     a merge body, then the tree is already in the cache (it was added
     by pph_in_merge_key_tree).  */
  if (marker != PPH_RECORD_START_MERGE_BODY)
    pph_cache_insert_at (&stream->cache, expr, ix, pph_tree_code_to_tag (expr));

  /* If we are reading a merge body, it means that EXPR is already in
     some chain.  Given that EXPR may now be in a different location
     in the chain, we need to make sure we do not lose it.  */
  if (marker == PPH_RECORD_START_MERGE_BODY)
    saved_expr_chain = TREE_CHAIN (expr);

  pph_in_tree_body (stream, expr);

  /* Restore TREE_CHAIN if necessary.  FIXME pph, we should just not
     save TREE_CHAIN for merge bodies.  */
  if (marker == PPH_RECORD_START_MERGE_BODY)
    TREE_CHAIN (expr) = saved_expr_chain;

  if (flag_pph_tracer)
    pph_trace_tree (expr, false, false);

  /* If needed, sign the recently materialized tree to detect
     mutations.  Note that we only need to compute signatures
     if we are generating a PPH image.  That is the only time
     where we need to determine whether a tree read from PPH
     was updated while parsing the header file that we are
     currently generating.  */
  if (pph_writer_enabled_p () && tree_needs_signature (expr))
    {
      unsigned crc;
      size_t nbytes;
      crc = pph_get_signature (expr, &nbytes);
      pph_cache_sign (&stream->cache, ix, crc, nbytes);
    }

  return expr;
}


/************************************************************* file contents */


/* Read a symbol table marker from STREAM.  */

static inline enum pph_symtab_action
pph_in_symtab_action (pph_stream *stream)
{
  enum pph_symtab_action m = (enum pph_symtab_action) pph_in_uchar (stream);
  gcc_assert (m == PPH_SYMTAB_DECLARE || m == PPH_SYMTAB_EXPAND);
  return m;
}


/* Read and return a callgraph node from STREAM.  If this is the first
   time we read this node, add it to the callgraph.  */

static struct cgraph_node *
pph_in_cgraph_node (pph_stream *stream)
{
  enum pph_record_marker marker;
  unsigned image_ix, ix;
  struct cgraph_node *node;
  tree fndecl;
  struct bitpack_d bp;

  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cgraph_node);
  if (marker == PPH_RECORD_END)
    return NULL;
  else if (pph_is_reference_marker (marker))
    return (struct cgraph_node *) pph_cache_find (stream, marker, image_ix,
						  ix, PPH_cgraph_node);

  fndecl = pph_in_tree (stream);
  ALLOC_AND_REGISTER (&stream->cache, ix, PPH_cgraph_node, node,
                      cgraph_create_node (fndecl));

  node->origin = pph_in_cgraph_node (stream);
  node->nested = pph_in_cgraph_node (stream);
  node->next_nested = pph_in_cgraph_node (stream);
  node->next_needed = pph_in_cgraph_node (stream);
  node->next_sibling_clone = pph_in_cgraph_node (stream);
  node->prev_sibling_clone = pph_in_cgraph_node (stream);
  node->clones = pph_in_cgraph_node (stream);
  node->clone_of = pph_in_cgraph_node (stream);
  node->same_comdat_group = pph_in_cgraph_node (stream);
  gcc_assert (node->call_site_hash == NULL);
  node->former_clone_of = pph_in_tree (stream);
  gcc_assert (node->aux == NULL);
  gcc_assert (VEC_empty (ipa_opt_pass, node->ipa_transforms_to_apply));

  gcc_assert (VEC_empty (ipa_ref_t, node->ref_list.references));
  gcc_assert (VEC_empty (ipa_ref_ptr, node->ref_list.refering));

  gcc_assert (node->local.lto_file_data == NULL);
  bp = pph_in_bitpack (stream);
  node->local.local = bp_unpack_value (&bp, 1);
  node->local.externally_visible = bp_unpack_value (&bp, 1);
  node->local.finalized = bp_unpack_value (&bp, 1);
  node->local.can_change_signature = bp_unpack_value (&bp, 1);
  node->local.redefined_extern_inline = bp_unpack_value (&bp, 1);

  node->global.inlined_to = pph_in_cgraph_node (stream);

  node->rtl.preferred_incoming_stack_boundary = pph_in_uint (stream);

  gcc_assert (VEC_empty (ipa_replace_map_p, node->clone.tree_map));
  node->thunk.fixed_offset = pph_in_uhwi (stream);
  node->thunk.virtual_value = pph_in_uhwi (stream);
  node->thunk.alias = pph_in_tree (stream);
  bp = pph_in_bitpack (stream);
  node->thunk.this_adjusting = bp_unpack_value (&bp, 1);
  node->thunk.virtual_offset_p = bp_unpack_value (&bp, 1);
  node->thunk.thunk_p = bp_unpack_value (&bp, 1);

  node->count = pph_in_uhwi (stream);
  node->count_materialization_scale = pph_in_uint (stream);

  bp = pph_in_bitpack (stream);
  node->needed = bp_unpack_value (&bp, 1);
  node->address_taken = bp_unpack_value (&bp, 1);
  node->abstract_and_needed = bp_unpack_value (&bp, 1);
  node->reachable = bp_unpack_value (&bp, 1);
  node->reachable_from_other_partition = bp_unpack_value (&bp, 1);
  node->lowered = bp_unpack_value (&bp, 1);
  node->analyzed = bp_unpack_value (&bp, 1);
  node->in_other_partition = bp_unpack_value (&bp, 1);
  node->process = bp_unpack_value (&bp, 1);
  node->alias = bp_unpack_value (&bp, 1);
  node->same_body_alias = bp_unpack_value (&bp, 1);
  node->frequency = (enum node_frequency) bp_unpack_value (&bp, 2);
  node->only_called_at_startup = bp_unpack_value (&bp, 1);
  node->only_called_at_exit = bp_unpack_value (&bp, 1);

  return node;
}


/* Have we already emitted this DECL?  */

static bool
pph_decl_already_emitted (tree decl)
{
  static struct pointer_set_t *emitted_syms = NULL;
  gcc_assert (decl != NULL);
  if (!emitted_syms)
    emitted_syms = pointer_set_create ();
  return pointer_set_insert (emitted_syms, decl) != 0;
}


/* Read the symbol table from STREAM.  When this image is read into
   another translation unit, we want to guarantee that the IL
   instances taken from this image are instantiated in the same order
   that they were instantiated when we generated this image.

   With this, we can generate code in the same order out of the
   original header files and out of PPH images.  */

static void
pph_in_symtab (pph_stream *stream)
{
  unsigned i, num;

  /* Register all the symbols in STREAM in the same order of the
     original compilation for this header file.  */
  num = pph_in_uint (stream);
  for (i = 0; i < num; i++)
    {
      enum pph_symtab_action action;
      tree decl;
      bool top_level, at_end;

      action = pph_in_symtab_action (stream);
      decl = pph_in_tree (stream);
      if (action == PPH_SYMTAB_DECLARE)
	{
	  struct bitpack_d bp;
	  bp = pph_in_bitpack (stream);
	  top_level = bp_unpack_value (&bp, 1);
	  at_end = bp_unpack_value (&bp, 1);
          if (pph_decl_already_emitted (decl))
            continue;
	  cp_rest_of_decl_compilation (decl, top_level, at_end);
	}
      else if (action == PPH_SYMTAB_EXPAND)
	{
	  struct cgraph_node *node;

	  pph_in_struct_function (stream, decl);
	  node = pph_in_cgraph_node (stream);
	  if (node && node->local.finalized)
	    {
	      if (pph_decl_already_emitted (decl))
		continue;
	      /* Since the writer had finalized this cgraph node,
		 we have to re-play its actions.  To do that, we need
		 to clear the finalized and reachable bits in the
		 node, otherwise cgraph_finalize_function will toss
		 out this node.  */
	      node->local.finalized = false;
	      node->reachable = false;
	      cgraph_finalize_function (node->decl, true);
	    }
	}
      else
	gcc_unreachable ();
    }
}


/* Wrap a macro DEFINITION for printing in an error.  */

static char *
wrap_macro_def (const char *definition)
{
  char *string;
  if (definition)
    {
      size_t length;
      length = strlen (definition);
      string = (char *) xmalloc (length+3);
      string[0] = '"';
      strcpy (string + 1, definition);
      string[length + 1] = '"';
      string[length + 2] = '\0';
    }
  else
    string = xstrdup ("undefined");
  return string;
}


/* Report a macro validation error in FILENAME for macro IDENT,
   which should have the value EXPECTED but actually had the value FOUND. */

static void
report_validation_error (const char *filename,
			 const char *ident, const char *found,
			 const char *before, const char *after)
{
  char* quote_found = wrap_macro_def (found);
  char* quote_before = wrap_macro_def (before);
  char* quote_after = wrap_macro_def (after);
  warning (0, "PPH file %s fails macro validation, "
           "%s is %s and should be %s or %s\n",
         filename, ident, quote_found, quote_before, quote_after);
  free (quote_found);
  free (quote_before);
  free (quote_after);
}


/* Load the IDENTIFIERS for a hunk from a STREAM.  */

static void
pph_in_identifiers (pph_stream *stream, cpp_idents_used *identifiers)
{
  unsigned int j;
  unsigned int max_ident_len, max_value_len, num_entries;
  unsigned int ident_len, before_len, after_len;

  max_ident_len = pph_in_uint (stream);
  identifiers->max_ident_len = max_ident_len;
  max_value_len = pph_in_uint (stream);
  identifiers->max_value_len = max_value_len;
  num_entries = pph_in_uint (stream);
  identifiers->num_entries = num_entries;
  identifiers->entries = XCNEWVEC (cpp_ident_use, num_entries);
  identifiers->strings = XCNEW (struct obstack);

  /* Strings need no alignment.  */
  _obstack_begin (identifiers->strings, 0, 0,
                  (void *(*) (long)) xmalloc,
                  (void (*) (void *)) free);
  obstack_alignment_mask (identifiers->strings) = 0;
  /* FIXME pph: We probably need to free all these things somewhere.  */

  /* Read the identifiers in HUNK. */
  for (j = 0; j < num_entries; ++j)
    {
      const char *s;
      identifiers->entries[j].used_by_directive = pph_in_uint (stream);
      identifiers->entries[j].expanded_to_text = pph_in_uint (stream);
      s = pph_in_string (stream);
      gcc_assert (s);
      ident_len = strlen (s);
      identifiers->entries[j].ident_len = ident_len;
      identifiers->entries[j].ident_str =
        (const char *) obstack_copy0 (identifiers->strings, s, ident_len);

      s = pph_in_string (stream);
      if (s)
	{
	  before_len = strlen (s);
	  identifiers->entries[j].before_len = before_len;
	  identifiers->entries[j].before_str = (const char *)
	      obstack_copy0 (identifiers->strings, s, before_len);
	}
      else
	{
	  /* The identifier table expects NULL entries to have
	     a length of -1U.  */
	  identifiers->entries[j].before_len = -1U;
	  identifiers->entries[j].before_str = NULL;
	}

      s = pph_in_string (stream);
      if (s)
	{
	  after_len = strlen (s);
	  identifiers->entries[j].after_len = after_len;
	  identifiers->entries[j].after_str = (const char *)
	      obstack_copy0 (identifiers->strings, s, after_len);
	}
      else
	{
	  /* The identifier table expects NULL entries to have
	     a length of -1U.  */
	  identifiers->entries[j].after_len = -1U;
	  identifiers->entries[j].after_str = NULL;
	}
    }
}


/* Compute a hash value for all the images starting at STREAM up to the
   root of the include hierarchy.  */

static hashval_t
pph_get_include_path_hash (pph_stream *stream)
{
  pph_stream *i;
  hashval_t val;

  for (val = 0, i = stream; i; i = i->parent)
    val = iterative_hash_hashval_t (htab_hash_pointer (i), val);

  return val;
}


/* Read all the merge keys from STREAM.  Merge into the corresponding
   contexts.  Return a VEC of all the merge keys read.  */

static void
pph_in_merge_keys (pph_stream *stream)
{
  cp_binding_level *bl = scope_chain->bindings;
  hashval_t include_path_hash = 0;

  /* Compute the signature for the include path from STREAM up to
     the root of the inclusion tree.  Symbols found in any image in
     the direct path from STREAM up to the root do not need to be
     merged.  They were already merged when the images were generated.  */
  include_path_hash = pph_get_include_path_hash (stream);

  /* First read all the merge keys and merge into the global bindings.  */
  pph_in_merge_key_chain (stream, &bl->names, include_path_hash);
  pph_in_merge_key_chain (stream, &bl->namespaces, include_path_hash);
  pph_in_merge_key_chain (stream, &bl->usings, include_path_hash);
  pph_in_merge_key_chain (stream, &bl->using_directives, include_path_hash);

  /* Now read the bodies of all the trees merged above.  */
  pph_in_merge_body_chain (stream);
  pph_in_merge_body_chain (stream);
  pph_in_merge_body_chain (stream);
  pph_in_merge_body_chain (stream);
}


/* Read global bindings from STREAM into scope_chain->bindings.  Note
   that this does not call pph_in_binding_level because that would
   overwrite the fields merged by pph_in_merge_keys.  */

static void
pph_in_global_binding (pph_stream *stream)
{
  unsigned image_ix, ix;
  enum pph_record_marker marker;
  cp_binding_level *bl;

  bl = scope_chain->bindings;
  marker = pph_in_start_record (stream, &image_ix, &ix, PPH_cp_binding_level);
  gcc_assert (marker != PPH_RECORD_END);

  if (pph_is_reference_marker (marker))
    {
      /* If we found a reference to BL, it should be the same.  This happens
	 when we pull BL from a nested PPH image.  */
      cp_binding_level *other_bl;
      other_bl = (cp_binding_level *) pph_cache_find (stream, marker, image_ix,
						      ix, PPH_cp_binding_level);
      gcc_assert (other_bl == bl);
    }
  else
    {
      /* Note that here we do not allocate a new binding level.  Since we
	 are reading into the global one, we register the current
	 scope_chain->bindings into the cache.  Otherwise, we would be
	 associating these symbols into the global binding level from
	 STREAM, instead of the current translation unit.  */
      pph_cache_insert_at (&stream->cache, bl, ix, PPH_cp_binding_level);
    }

  /* Read the merge keys and merge them into the current compilation
     context.  Since we have registered scope_chain->bindings in the
     same slot IX that the writer used, the trees read now will be
     bound to scope_chain->bindings.  */
  pph_in_merge_keys (stream);

  pph_in_binding_level_1 (stream, bl);
}


/* Helper for pph_read_file.  Read contents of PPH file in STREAM.  */

static void
pph_read_file_1 (pph_stream *stream)
{
  bool verified;
  cpp_ident_use *bad_use;
  const char *cur_def;
  cpp_idents_used idents_used;
  tree t, file_keyed_classes, file_static_aggregates;
  unsigned i;
  VEC(tree,gc) *file_unemitted_tinfo_decls;
  source_location cpp_token_replay_loc;

  /* If we have read STREAM before, we do not need to re-read the rest
     of its body.  We only needed to read its line table.  */
  if (stream->in_memory_p)
    return;

  if (flag_pph_tracer >= 1)
    fprintf (pph_logfile, "PPH: Reading %s\n", stream->name);

  /* Read in STREAM's line table and merge it in the current line table.
     At the same time, read in includes in the order they were originally
     read.  */
  cpp_token_replay_loc = pph_in_line_table_and_includes (stream);

  /* Read all the identifiers and pre-processor symbols in the global
     namespace.  */
  pph_in_identifiers (stream, &idents_used);

  /* FIXME pph: This validation is weak.  */
  verified = cpp_lt_verify_1 (parse_in, &idents_used, &bad_use, &cur_def, true);
  if (!verified)
    report_validation_error (stream->name, bad_use->ident_str, cur_def,
                             bad_use->before_str, bad_use->after_str);

  /* Re-instantiate all the pre-processor symbols defined by STREAM.  Force
     their source_location to line 1 / column 0 of the file they were included
     in.  This avoids shifting all of the line_table's locations as we would by
     adding locations which wouldn't be there in the non-pph compile; thus
     working towards an identical line_table in pph and non-pph.  */
  cpp_lt_replay (parse_in, &idents_used, &cpp_token_replay_loc);

  /* Read the bindings from STREAM.  */
  pph_in_global_binding (stream);

  /* Read and merge the other global state collected during parsing of
     the original header.  */
  file_keyed_classes = pph_in_tree (stream);
  keyed_classes = chainon (file_keyed_classes, keyed_classes);

  file_unemitted_tinfo_decls = pph_in_tree_vec (stream);
  FOR_EACH_VEC_ELT (tree, file_unemitted_tinfo_decls, i, t)
    VEC_safe_push (tree, gc, unemitted_tinfo_decls, t);

  pph_in_pending_templates_list (stream);
  pph_in_spec_entry_tables (stream);

  file_static_aggregates = pph_in_tree (stream);
  static_aggregates = chainon (file_static_aggregates, static_aggregates);

  /* Read and process the symbol table.  */
  pph_in_symtab (stream);

  /* Mark this file as read.  If other images need to access its contents,
     we will not need to actually read it again.  */
  pph_mark_stream_read (stream);

  if (flag_pph_dump_tree)
    pph_dump_namespace (pph_logfile, global_namespace);
}


/* Read PPH file FILENAME.  Return the in-memory pph_stream instance.  */

pph_stream *
pph_read_file (const char *filename)
{
  pph_stream *stream = pph_stream_open (filename, "rb");
  if (stream == NULL)
    fatal_error ("cannot open PPH file %s for reading: %m", filename);

  pph_read_file_1 (stream);

  return stream;
}


/* Initialize the reader.  */

void
pph_reader_init (void)
{
  merge_toc = htab_create (551, htab_merge_key_hash, htab_merge_key_eq, free);
}


/* Finalize the reader.  */

void
pph_reader_finish (void)
{
  htab_delete (merge_toc);
}
