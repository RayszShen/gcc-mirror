/* Functions related to the Boehm garbage collector.
   Copyright (C) 2000, 2003, 2004 Free Software Foundation, Inc.

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
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

Java and all Java-based marks are trademarks or registered trademarks
of Sun Microsystems, Inc. in the United States and other countries.
The Free Software Foundation is independent of Sun Microsystems, Inc.  */

/* Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "java-tree.h"
#include "parse.h"
#include "toplev.h"

static void mark_reference_fields (tree, tree, unsigned HOST_WIDE_INT *,
				   unsigned HOST_WIDE_INT *, unsigned int,
				   int *, int *, int *, HOST_WIDE_INT *);
static void set_bit (unsigned HOST_WIDE_INT *, unsigned HOST_WIDE_INT *,
		     unsigned int);

/* Treat two HOST_WIDE_INT's as a contiguous bitmap, with bit 0 being
   the least significant.  This function sets bit N in the bitmap.  */
static void
set_bit (unsigned HOST_WIDE_INT *low, unsigned HOST_WIDE_INT *high,
	 unsigned int n)
{
  unsigned HOST_WIDE_INT *which;

  if (n >= HOST_BITS_PER_WIDE_INT)
    {
      n -= HOST_BITS_PER_WIDE_INT;
      which = high;
    }
  else
    which = low;

  *which |= (unsigned HOST_WIDE_INT) 1 << n;
}

/* Return true if this field, from Class, should be marked.  Return
   false otherwise.  */
static bool
mark_class_field_p (tree field)
{
  char *name;

  assert (DECL_CONTEXT (field) == type_class);

  /* Only pointer and record fields are marked.  */
  if (TREE_CODE (TREE_TYPE (field)) != POINTER_TYPE
      && TREE_CODE (TREE_TYPE (field)) != RECORD_TYPE)
    return false;

  /* A few fields need not be visible to the collector.  */
  name = IDENTIFIER_POINTER (DECL_NAME (field));
  if (! strcmp (name, "next")
      || ! strcmp (name, "thread")
      || ! strcmp (name, "chain"))
    return false;

  return true;  
}

/* Recursively mark reference fields.  */
static void
mark_reference_fields (tree klass,
		       tree field,
		       unsigned HOST_WIDE_INT *low,
		       unsigned HOST_WIDE_INT *high,
		       unsigned int ubit,
		       int *pointer_after_end,
		       int *all_bits_set,
		       int *last_set_index,
		       HOST_WIDE_INT *last_view_index)
{
  /* See if we have fields from our superclass.  */
  if (DECL_NAME (field) == NULL_TREE)
    {
      mark_reference_fields (TREE_TYPE (field),
			     TYPE_FIELDS (TREE_TYPE (field)),
			     low, high, ubit,
			     pointer_after_end, all_bits_set,
			     last_set_index, last_view_index);
      field = TREE_CHAIN (field);
    }

  for (; field != NULL_TREE; field = TREE_CHAIN (field))
    {
      HOST_WIDE_INT offset;
      HOST_WIDE_INT size_bytes;

      if (FIELD_STATIC (field))
	continue;

      offset = int_byte_position (field);
      size_bytes = int_size_in_bytes (TREE_TYPE (field));
      if ((type == type_class && mark_class_field_p (field))
	  || (type != type_class
	      && JREFERENCE_TYPE_P (TREE_TYPE (field))
	      /* An `object' of type gnu.gcj.RawData is actually
		 non-Java data.  */
	      && TREE_TYPE (field) != rawdata_ptr_type_node))
	{
	  unsigned int count;
	  unsigned int size_words;
	  unsigned int i;

	  /* If this reference slot appears to overlay a slot we think
	     we already covered, then we are doomed.  */
	  assert (offset > *last_view_index);

	  /* When handling Class we might run into a record type.  We
	     don't try to be fully general here but instead just
	     handle it in an ad hoc way.  */
	  if (TREE_CODE (TREE_TYPE (field)) == RECORD_TYPE)
	    {
	      tree subfield;
	      HOST_WIDE_INT saved_offset = offset;
	      assert (type == type_class);
	      for (subfield = TYPE_FIELDS (TREE_TYPE (field));
		   subfield != NULL_TREE;
		   subfield = TREE_CHAIN (subfield))
		{
		  if (TREE_CODE (TREE_TYPE (subfield)) == POINTER_TYPE)
		    {
		      offset = saved_offset + int_byte_position (subfield);
		      count = offset * BITS_PER_UNIT / POINTER_SIZE;
		      size_bytes = int_size_in_bytes (TREE_TYPE (subfield));
		      size_words = size_bytes * BITS_PER_UNIT / POINTER_SIZE;

		      *last_set_index = count;
		      for (i = 0; i < size_words; ++i)
			set_bit (low, high, ubit - count - i - 1);

		      if (count >= ubit - 2)
			*pointer_after_end = 1;

		      /* If we saw a non-reference field earlier, then
			 we can't use the count representation.  We
			 keep track of that in *ALL_BITS_SET.  */
		      if (! *all_bits_set)
			*all_bits_set = -1;
		    }
		}
	    }
	  else
	    {
	      count = offset * BITS_PER_UNIT / POINTER_SIZE;
	      size_words = size_bytes * BITS_PER_UNIT / POINTER_SIZE;

	      *last_set_index = count;
	     
	      /* First word in object corresponds to most significant byte of 
		 bitmap.

		 In the case of a multiple-word record, we set pointer
		 bits for all words in the record. This is
		 conservative, but the size_words != 1 case is
		 impossible in regular java code. */
	      for (i = 0; i < size_words; ++i)
		set_bit (low, high, ubit - count - i - 1);

	      if (count >= ubit - 2)
		*pointer_after_end = 1;

	      /* If we saw a non-reference field earlier, then we
		 can't use the count representation.  We keep track of
		 that in *ALL_BITS_SET.  */
	      if (! *all_bits_set)
		*all_bits_set = -1;
	    }
	}
      else if (*all_bits_set > 0)
	*all_bits_set = 0;

      *last_view_index = offset;
    }
}

/* Return the marking bitmap for the class TYPE.  For now this is a
   single word describing the type.  */
tree
get_boehm_type_descriptor (tree type)
{
  unsigned int count, log2_size, ubit;
  int bit;
  int all_bits_set = 1;
  int last_set_index = 0;
  HOST_WIDE_INT last_view_index = -1;
  int pointer_after_end = 0;
  unsigned HOST_WIDE_INT low = 0, high = 0;
  tree field, value, value_type;

  /* If the GC wasn't requested, just use a null pointer.  */
  if (! flag_use_boehm_gc)
    return null_pointer_node;

  value_type = java_type_for_mode (ptr_mode, 1);
  /* If we have a type of unknown size, use a proc.  */
  if (int_size_in_bytes (type) == -1)
    goto procedure_object_descriptor;

  bit = POINTER_SIZE / BITS_PER_UNIT;
  /* The size of this node has to be known.  And, we only support 32
     and 64 bit targets, so we need to know that the log2 is one of
     our values.  */
  log2_size = exact_log2 (bit);
  assert (bit != -1);
  assert (log2_size == 2 || log2_size == 3);
  bit *= BITS_PER_UNIT;

  /* Warning avoidance.  */
  ubit = (unsigned int) bit;

  field = TYPE_FIELDS (type);
  mark_reference_fields (type, field, &low, &high, ubit,
			 &pointer_after_end, &all_bits_set,
			 &last_set_index, &last_view_index);

  /* If the object is all pointers, or if the part with pointers fits
     in our bitmap, then we are ok.  Otherwise we have to allocate it
     a different way.  */
  if (all_bits_set != -1)
    {
      /* In this case the initial part of the object is all reference
	 fields, and the end of the object is all non-reference
	 fields.  We represent the mark as a count of the fields,
	 shifted.  In the GC the computation looks something like
	 this:
	 value = DS_LENGTH | WORDS_TO_BYTES (last_set_index + 1);
	 DS_LENGTH is 0.
	 WORDS_TO_BYTES shifts by log2(bytes-per-pointer).  */
      count = 0;
      low = 0;
      high = 0;
      ++last_set_index;
      while (last_set_index)
	{
	  if ((last_set_index & 1))
	    set_bit (&low, &high, log2_size + count);
	  last_set_index >>= 1;
	  ++count;
	}
      value = build_int_cst_wide (value_type, low, high);
    }
  else if (! pointer_after_end)
    {
      /* Bottom two bits for bitmap mark type are 01.  */
      set_bit (&low, &high, 0);
      value = build_int_cst_wide (value_type, low, high);
    }
  else
    {
      /* Compute a procedure-based object descriptor.  We know that our
	 `kind' is 0, and `env' is likewise 0, so we have a simple
	 computation.  From the GC sources:
	    (((((env) << LOG_MAX_MARK_PROCS) | (proc_index)) << DS_TAG_BITS) \
	    | DS_PROC)
	 Here DS_PROC == 2.  */
    procedure_object_descriptor:
      assert (type != type_class);
      value = build_int_cst (value_type, 2);
    }

  return value;
}
