/* Dataflow support routines.
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.
   Contributed by Michael P. Hayes (m.hayes@elec.canterbury.ac.nz,
                                    mhayes@redhat.com)

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
Boston, MA 02111-1307, USA.  


OVERVIEW:

This file provides some dataflow routines for computing reaching defs,
upward exposed uses, live variables, def-use chains, and use-def
chains.  The global dataflow is performed using simple iterative
methods with a worklist and could be sped up by ordering the blocks
with a depth first search order.

A `struct ref' data structure (ref) is allocated for every register
reference (def or use) and this records the insn and bb the ref is
found within.  The refs are linked together in chains of uses and defs
for each insn and for each register.  Each ref also has a chain field
that links all the use refs for a def or all the def refs for a use.
This is used to create use-def or def-use chains.


USAGE:

Here's an example of using the dataflow routines.

      struct df *df;

      df = df_init ();

      df_analyse (df, 0, DF_ALL);

      df_dump (df, DF_ALL, stderr);

      df_finish (df);


df_init simply creates a poor man's object (df) that needs to be
passed to all the dataflow routines.  df_finish destroys this
object and frees up any allocated memory.

df_analyse performs the following:

1. Records defs and uses by scanning the insns in each basic block
   or by scanning the insns queued by df_insn_modify.
2. Links defs and uses into insn-def and insn-use chains.
3. Links defs and uses into reg-def and reg-use chains.
4. Assigns LUIDs to each insn (for modified blocks).
5. Calculates local reaching definitions.
6. Calculates global reaching definitions.
7. Creates use-def chains.
8. Calculates local reaching uses (upwards exposed uses).
9. Calculates global reaching uses.
10. Creates def-use chains.
11. Calculates local live registers.
12. Calculates global live registers.
13. Calculates register lifetimes and determines local registers.


PHILOSOPHY:

Note that the dataflow information is not updated for every newly
deleted or created insn.  If the dataflow information requires
updating then all the changed, new, or deleted insns needs to be
marked with df_insn_modify (or df_insns_modify) either directly or
indirectly (say through calling df_insn_delete).  df_insn_modify
marks all the modified insns to get processed the next time df_analyse
 is called.

Beware that tinkering with insns may invalidate the dataflow information.
The philosophy behind these routines is that once the dataflow
information has been gathered, the user should store what they require 
before they tinker with any insn.  Once a reg is replaced, for example,
then the reg-def/reg-use chains will point to the wrong place.  Once a
whole lot of changes have been made, df_analyse can be called again
to update the dataflow information.  Currently, this is not very smart
with regard to propagating changes to the dataflow so it should not
be called very often.


DATA STRUCTURES:

The basic object is a REF (reference) and this may either be a DEF
(definition) or a USE of a register.

These are linked into a variety of lists; namely reg-def, reg-use,
  insn-def, insn-use, def-use, and use-def lists.  For example,
the reg-def lists contain all the refs that define a given register
while the insn-use lists contain all the refs used by an insn.

Note that the reg-def and reg-use chains are generally short (except for the
hard registers) and thus it is much faster to search these chains
rather than searching the def or use bitmaps.  

If the insns are in SSA form then the reg-def and use-def lists
should only contain the single defining ref.

TODO:

1) Incremental dataflow analysis.

Note that if a loop invariant insn is hoisted (or sunk), we do not
need to change the def-use or use-def chains.  All we have to do is to
change the bb field for all the associated defs and uses and to
renumber the LUIDs for the original and new basic blocks of the insn.

When shadowing loop mems we create new uses and defs for new pseudos
so we do not affect the existing dataflow information.

My current strategy is to queue up all modified, created, or deleted
insns so when df_analyse is called we can easily determine all the new
or deleted refs.  Currently the global dataflow information is
recomputed from scratch but this could be propagated more efficiently.

2) Improved global data flow computation using depth first search.

3) Reduced memory requirements.

We could operate a pool of ref structures.  When a ref is deleted it
gets returned to the pool (say by linking on to a chain of free refs).
This will require a pair of bitmaps for defs and uses so that we can
tell which ones have been changed.  Alternatively, we could
periodically squeeze the def and use tables and associated bitmaps and
renumber the def and use ids.

4) Ordering of reg-def and reg-use lists. 

Should the first entry in the def list be the first def (within a BB)?
Similarly, should the first entry in the use list be the last use
(within a BB)? 

5) Working with a sub-CFG.

Often the whole CFG does not need to be analysed, for example,
when optimising a loop, only certain registers are of interest.
Perhaps there should be a bitmap argument to df_analyse to specify
 which registers should be analysed?   */

#define HANDLE_SUBREG

#include "config.h"
#include "system.h"
#include "rtl.h" 
#include "insn-config.h" 
#include "recog.h" 
#include "function.h" 
#include "regs.h" 
#include "obstack.h" 
#include "hard-reg-set.h"
#include "basic-block.h"
#include "df.h"
#include "bitmap.h"


#define FOR_ALL_BBS(BB, CODE)					\
do {								\
  int node_;							\
  for (node_ = 0; node_ < n_basic_blocks; node_++)		\
    {(BB) = BASIC_BLOCK (node_); CODE;};} while (0)

#define FOR_EACH_BB_IN_BITMAP(BITMAP, MIN, BB, CODE)		\
do {								\
  unsigned int node_;						\
  EXECUTE_IF_SET_IN_BITMAP (BITMAP, MIN, node_, 		\
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define FOR_EACH_BB_IN_BITMAP_REV(BITMAP, MIN, BB, CODE)	\
do {								\
  unsigned int node_;						\
  EXECUTE_IF_SET_IN_BITMAP_REV (BITMAP, node_, 		\
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define FOR_EACH_BB_IN_SBITMAP(BITMAP, MIN, BB, CODE)           \
do {                                                            \
  unsigned int node_;                                           \
  EXECUTE_IF_SET_IN_SBITMAP (BITMAP, MIN, node_,                \
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

static struct obstack df_ref_obstack;
static struct df *ddf;

static void df_reg_table_realloc PARAMS((struct df *, int));
static void df_insn_table_realloc PARAMS((struct df *, int));
static void df_bitmaps_alloc PARAMS((struct df *, int));
static void df_bitmaps_free PARAMS((struct df *, int));
static void df_free PARAMS((struct df *));
static void df_alloc PARAMS((struct df *, int));

static rtx df_reg_clobber_gen PARAMS((unsigned int));
static rtx df_reg_use_gen PARAMS((unsigned int));

static inline struct df_link *df_link_create PARAMS((struct ref *, 
						     struct df_link *));
static struct df_link *df_ref_unlink PARAMS((struct df_link **, struct ref *));
static void df_def_unlink PARAMS((struct df *, struct ref *));
static void df_use_unlink PARAMS((struct df *, struct ref *));
static void df_insn_refs_unlink PARAMS ((struct df *, basic_block, rtx));
static void df_bb_refs_unlink PARAMS ((struct df *, basic_block));
static void df_refs_unlink PARAMS ((struct df *, bitmap));

static struct ref *df_ref_create PARAMS((struct df *, 
					 rtx, rtx *, basic_block, rtx,
					 enum df_ref_type));
static void df_ref_record_1 PARAMS((struct df *, rtx, rtx *, 
				    basic_block, rtx, enum df_ref_type));
static void df_ref_record PARAMS((struct df *, rtx, rtx *, 
				  basic_block bb, rtx, enum df_ref_type));
static void df_def_record_1 PARAMS((struct df *, rtx, basic_block, rtx));
static void df_defs_record PARAMS((struct df *, rtx, basic_block, rtx));
static void df_uses_record PARAMS((struct df *, rtx *,
				   enum df_ref_type, basic_block, rtx));
static void df_insn_refs_record PARAMS((struct df *, basic_block, rtx));
static void df_bb_refs_record PARAMS((struct df *, basic_block));
static void df_refs_record PARAMS((struct df *, bitmap));

static int df_visit_next PARAMS ((struct df *, sbitmap));
static void df_bb_reg_def_chain_create PARAMS((struct df *, basic_block));
static void df_reg_def_chain_create PARAMS((struct df *, bitmap));
static void df_bb_reg_use_chain_create PARAMS((struct df *, basic_block));
static void df_reg_use_chain_create PARAMS((struct df *, bitmap));
static void df_bb_du_chain_create PARAMS((struct df *, basic_block, bitmap));
static void df_du_chain_create PARAMS((struct df *, bitmap));
static void df_bb_ud_chain_create PARAMS((struct df *, basic_block));
static void df_ud_chain_create PARAMS((struct df *, bitmap));
static void df_rd_global_compute PARAMS((struct df *, bitmap));
static void df_ru_global_compute PARAMS((struct df *, bitmap));
static void df_lr_global_compute PARAMS((struct df *, bitmap));
static void df_bb_rd_local_compute PARAMS((struct df *, basic_block));
static void df_rd_local_compute PARAMS((struct df *, bitmap));
static void df_bb_ru_local_compute PARAMS((struct df *, basic_block));
static void df_ru_local_compute PARAMS((struct df *, bitmap));
static void df_bb_lr_local_compute PARAMS((struct df *, basic_block));
static void df_lr_local_compute PARAMS((struct df *, bitmap));
static void df_bb_reg_info_compute PARAMS((struct df *, basic_block, bitmap));
static void df_reg_info_compute PARAMS((struct df *, bitmap));

static int df_bb_luids_set PARAMS((struct df *df, basic_block));
static int df_luids_set PARAMS((struct df *df, bitmap));

static int df_modified_p PARAMS ((struct df *, bitmap));
static int df_refs_queue PARAMS ((struct df *));
static int df_refs_process PARAMS ((struct df *));
static int df_bb_refs_update PARAMS ((struct df *, basic_block));
static int df_refs_update PARAMS ((struct df *));
static void df_analyse_1 PARAMS((struct df *, bitmap, int, int));

static void df_insns_modify PARAMS((struct df *, basic_block,
				    rtx, rtx));
static int df_rtx_mem_replace PARAMS ((rtx *, void *));
static int df_rtx_reg_replace PARAMS ((rtx *, void *));
void df_refs_reg_replace PARAMS ((struct df *, bitmap,
					 struct df_link *, rtx, rtx));

static int df_def_dominates_all_uses_p PARAMS((struct df *, struct ref *def));
static int df_def_dominates_uses_p PARAMS((struct df *,
					   struct ref *def, bitmap));
static struct ref *df_bb_regno_last_use_find PARAMS((struct df *, basic_block,
						     unsigned int));
static struct ref *df_bb_regno_first_def_find PARAMS((struct df *, basic_block,
						      unsigned int));
static struct ref *df_bb_insn_regno_last_use_find PARAMS((struct df *,
							  basic_block,
							  rtx, unsigned int));
static struct ref *df_bb_insn_regno_first_def_find PARAMS((struct df *,
							   basic_block,
							   rtx, unsigned int));

static void df_chain_dump PARAMS((struct df_link *, FILE *file));
static void df_chain_dump_regno PARAMS((struct df_link *, FILE *file));
static void df_regno_debug PARAMS ((struct df *, unsigned int, FILE *));
static void df_ref_debug PARAMS ((struct df *, struct ref *, FILE *));


/* Local memory allocation/deallocation routines.  */


/* Increase the insn info table by SIZE more elements.  */
static void
df_insn_table_realloc (df, size)
     struct df *df;
     int size;
{
  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->insn_size / 4;

  size += df->insn_size;
  
  df->insns = (struct insn_info *)
    xrealloc (df->insns, size * sizeof (struct insn_info));
  
  memset (df->insns + df->insn_size, 0, 
	  (size - df->insn_size) * sizeof (struct insn_info));

  df->insn_size = size;

  if (! df->insns_modified)
    {
      df->insns_modified = BITMAP_XMALLOC ();
      bitmap_zero (df->insns_modified);
    }
}


/* Increase the reg info table by SIZE more elements.  */
static void
df_reg_table_realloc (df, size)
     struct df *df;
     int size;
{
  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->reg_size / 4;

  size += df->reg_size;

  df->regs = (struct reg_info *)
    xrealloc (df->regs, size * sizeof (struct reg_info));

  /* Zero the new entries.  */
  memset (df->regs + df->reg_size, 0, 
	  (size - df->reg_size) * sizeof (struct reg_info));

  df->reg_size = size;
}


void
df_def_table_realloc (df, size)
     struct df *df;
     int size;
{
  int i;
  struct ref *refs;

  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->def_size / 4;

  df->def_size += size;
  df->defs = xrealloc (df->defs, 
		       df->def_size * sizeof (*df->defs));

  /* Allocate a new block of memory and link into list of blocks
     that will need to be freed later.  */

  refs = xmalloc (size * sizeof (*refs));
  
  /* Link all the new refs together, overloading the chain field.  */
  for (i = 0; i < size - 1; i++)
      refs[i].chain = (struct df_link *)(refs + i + 1);
  refs[size - 1].chain = 0;
}



/* Allocate bitmaps for each basic block.  */
static void
df_bitmaps_alloc (df, flags)
     struct df *df;
     int flags;
{
  unsigned int i;
  int dflags = 0;

  /* Free the bitmaps if they need resizing.  */
  if ((flags & DF_LR) && df->n_regs < (unsigned int)max_reg_num ())
    dflags |= DF_LR | DF_RU;
  if ((flags & DF_RU) && df->n_uses < df->use_id)
    dflags |= DF_RU;
  if ((flags & DF_RD) && df->n_defs < df->def_id)
    dflags |= DF_RD;

  if (dflags)
    df_bitmaps_free (df, dflags);

  df->n_defs = df->def_id;
  df->n_uses = df->use_id;

  for (i = 0; i < df->n_bbs; i++)
    {
      basic_block bb = BASIC_BLOCK (i);
      struct bb_info *bb_info = DF_BB_INFO (df, bb);
      
      if (flags & DF_RD && ! bb_info->rd_in)
	{
	  /* Allocate bitmaps for reaching definitions.  */
	  bb_info->rd_kill = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->rd_kill);
	  bb_info->rd_gen = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->rd_gen);
	  bb_info->rd_in = BITMAP_XMALLOC ();
	  bb_info->rd_out = BITMAP_XMALLOC ();
	  bb_info->rd_valid = 0;
	}

      if (flags & DF_RU && ! bb_info->ru_in)
	{
	  /* Allocate bitmaps for upward exposed uses.  */
	  bb_info->ru_kill = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->ru_kill);
	  /* Note the lack of symmetry.  */
	  bb_info->ru_gen = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->ru_gen);
	  bb_info->ru_in = BITMAP_XMALLOC ();
	  bb_info->ru_out = BITMAP_XMALLOC ();
	  bb_info->ru_valid = 0;
	}

      if (flags & DF_LR && ! bb_info->lr_in)
	{
	  /* Allocate bitmaps for live variables.  */
	  bb_info->lr_def = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->lr_def);
	  bb_info->lr_use = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->lr_use);
	  bb_info->lr_in = BITMAP_XMALLOC ();
	  bb_info->lr_out = BITMAP_XMALLOC ();
	  bb_info->lr_valid = 0;
	}
    }
}


/* Free bitmaps for each basic block.  */
static void
df_bitmaps_free (df, flags)
     struct df *df ATTRIBUTE_UNUSED;
     int flags;
{
  unsigned int i;

  for (i = 0; i < df->n_bbs; i++)
    {
      basic_block bb = BASIC_BLOCK (i);
      struct bb_info *bb_info = DF_BB_INFO (df, bb);

      if (!bb_info)
	continue;

      if ((flags & DF_RD) && bb_info->rd_in)
	{
	  /* Free bitmaps for reaching definitions.  */
	  BITMAP_XFREE (bb_info->rd_kill);
	  bb_info->rd_kill = NULL;
	  BITMAP_XFREE (bb_info->rd_gen);
	  bb_info->rd_gen = NULL;
	  BITMAP_XFREE (bb_info->rd_in);
	  bb_info->rd_in = NULL;
	  BITMAP_XFREE (bb_info->rd_out);
	  bb_info->rd_out = NULL;
	}

      if ((flags & DF_RU) && bb_info->ru_in)
	{
	  /* Free bitmaps for upward exposed uses.  */
	  BITMAP_XFREE (bb_info->ru_kill);
	  bb_info->ru_kill = NULL;
	  BITMAP_XFREE (bb_info->ru_gen);
	  bb_info->ru_gen = NULL;
	  BITMAP_XFREE (bb_info->ru_in);
	  bb_info->ru_in = NULL;
	  BITMAP_XFREE (bb_info->ru_out);
	  bb_info->ru_out = NULL;
	}

      if ((flags & DF_LR) && bb_info->lr_in)
	{
	  /* Free bitmaps for live variables.  */
	  BITMAP_XFREE (bb_info->lr_def);
	  bb_info->lr_def = NULL;
	  BITMAP_XFREE (bb_info->lr_use);
	  bb_info->lr_use = NULL;
	  BITMAP_XFREE (bb_info->lr_in);
	  bb_info->lr_in = NULL;
	  BITMAP_XFREE (bb_info->lr_out);
	  bb_info->lr_out = NULL;
	}
    }
  df->flags &= ~(flags & (DF_RD | DF_RU | DF_LR));
}


/* Allocate and initialise dataflow memory.  */
static void
df_alloc (df, n_regs)
     struct df *df;
     int n_regs;
{
  int n_insns;
  int i;

  gcc_obstack_init (&df_ref_obstack);

  /* Perhaps we should use LUIDs to save memory for the insn_refs
     table.  This is only a small saving; a few pointers.  */
  n_insns = get_max_uid () + 1;

  df->def_id = 0;
  df->n_defs = 0;
  /* Approximate number of defs by number of insns.  */
  df->def_size = n_insns;
  df->defs = xmalloc (df->def_size * sizeof (*df->defs));

  df->use_id = 0;
  df->n_uses = 0;
  /* Approximate number of uses by twice number of insns.  */
  df->use_size = n_insns * 2;
  df->uses = xmalloc (df->use_size * sizeof (*df->uses));

  df->n_regs = n_regs;
  df->n_bbs = n_basic_blocks;

  /* Allocate temporary working array used during local dataflow analysis.  */
  df->reg_def_last = xmalloc (df->n_regs * sizeof (struct ref *));

  df_insn_table_realloc (df, n_insns);

  df_reg_table_realloc (df, df->n_regs);

  df->bbs_modified = BITMAP_XMALLOC ();
  bitmap_zero (df->bbs_modified);

  df->flags = 0;

  df->bbs = xcalloc (df->n_bbs, sizeof (struct bb_info));

  df->all_blocks = BITMAP_XMALLOC ();
  for (i = 0; i < n_basic_blocks; i++)
    bitmap_set_bit (df->all_blocks, i);
}


/* Free all the dataflow info.  */
static void
df_free (df)
     struct df *df;
{
  df_bitmaps_free (df, DF_ALL);

  if (df->bbs)
    free (df->bbs);
  df->bbs = 0;

  if (df->insns)
    free (df->insns);
  df->insns = 0;
  df->insn_size = 0;

  if (df->defs)
    free (df->defs);
  df->defs = 0;
  df->def_size = 0;
  df->def_id = 0;

  if (df->uses)
    free (df->uses);
  df->uses = 0;
  df->use_size = 0;
  df->use_id = 0;

  if (df->regs)
    free (df->regs);
  df->regs = 0;
  df->reg_size = 0;

  if (df->bbs_modified)
    BITMAP_XFREE (df->bbs_modified);
  df->bbs_modified = 0;

  if (df->insns_modified)
    BITMAP_XFREE (df->insns_modified);
  df->insns_modified = 0;

  BITMAP_XFREE (df->all_blocks);
  df->all_blocks = 0;

  obstack_free (&df_ref_obstack, NULL_PTR);
}

/* Local miscellaneous routines.  */

/* Return a USE for register REGNO.  */
static rtx df_reg_use_gen (regno)
     unsigned int regno;
{
  rtx reg;
  rtx use;

  reg = regno >= FIRST_PSEUDO_REGISTER
    ? regno_reg_rtx[regno] : gen_rtx_REG (reg_raw_mode[regno], regno);
 
  use = gen_rtx_USE (GET_MODE (reg), reg);
  return use;
}


/* Return a CLOBBER for register REGNO.  */
static rtx df_reg_clobber_gen (regno)
     unsigned int regno;
{
  rtx reg;
  rtx use;

  reg = regno >= FIRST_PSEUDO_REGISTER
    ? regno_reg_rtx[regno] : gen_rtx_REG (reg_raw_mode[regno], regno);

  use = gen_rtx_CLOBBER (GET_MODE (reg), reg);
  return use;
}

/* Local chain manipulation routines.  */

/* Create a link in a def-use or use-def chain.  */
static inline struct df_link *
df_link_create (ref, next)
     struct ref *ref;
     struct df_link *next;
{
  struct df_link *link;

  link = (struct df_link *) obstack_alloc (&df_ref_obstack, 
					   sizeof (*link));
  link->next = next;
  link->ref = ref;
  return link;
}


/* Add REF to chain head pointed to by PHEAD.  */
static struct df_link *
df_ref_unlink (phead, ref)
     struct df_link **phead;
     struct ref *ref;
{
  struct df_link *link = *phead;

  if (link)
    {
      if (! link->next)
	{
	  /* Only a single ref.  It must be the one we want.
	     If not, the def-use and use-def chains are likely to
	     be inconsistent.  */
	  if (link->ref != ref)
	    abort ();
	  /* Now have an empty chain.  */
	  *phead = NULL;
	}
      else
	{
	  /* Multiple refs.  One of them must be us.  */
	  if (link->ref == ref)
	    *phead = link->next;
	  else
	    {
	      /* Follow chain.  */
	      for (; link->next; link = link->next)
		{
		  if (link->next->ref == ref)
		    {
		      /* Unlink from list.  */
		      link->next = link->next->next;
		      return link->next;
		    }
		}
	    }
	}
    }
  return link;
}


/* Unlink REF from all def-use/use-def chains, etc.  */
int
df_ref_remove (df, ref)
     struct df *df;
     struct ref *ref;
{
  if (DF_REF_REG_DEF_P (ref))
    {
      df_def_unlink (df, ref);
      df_ref_unlink (&df->insns[DF_REF_INSN_UID (ref)].defs, ref);
    }
  else
    {
      df_use_unlink (df, ref);
      df_ref_unlink (&df->insns[DF_REF_INSN_UID (ref)].uses, ref);
    }
  return 1;
}


/* Unlink DEF from use-def and reg-def chains.  */
static void 
df_def_unlink (df, def)
     struct df *df ATTRIBUTE_UNUSED;
     struct ref *def;
{
  struct df_link *du_link;
  unsigned int dregno = DF_REF_REGNO (def);

  /* Follow def-use chain to find all the uses of this def.  */
  for (du_link = DF_REF_CHAIN (def); du_link; du_link = du_link->next)
    {
      struct ref *use = du_link->ref;

      /* Unlink this def from the use-def chain.  */
      df_ref_unlink (&DF_REF_CHAIN (use), def);
    }
  DF_REF_CHAIN (def) = 0;

  /* Unlink def from reg-def chain.  */
  df_ref_unlink (&df->regs[dregno].defs, def);

  df->defs[DF_REF_ID (def)] = 0;
}


/* Unlink use from def-use and reg-use chains.  */
static void 
df_use_unlink (df, use)
     struct df *df ATTRIBUTE_UNUSED;
     struct ref *use;
{
  struct df_link *ud_link;
  unsigned int uregno = DF_REF_REGNO (use);

  /* Follow use-def chain to find all the defs of this use.  */
  for (ud_link = DF_REF_CHAIN (use); ud_link; ud_link = ud_link->next)
    {
      struct ref *def = ud_link->ref;

      /* Unlink this use from the def-use chain.  */
      df_ref_unlink (&DF_REF_CHAIN (def), use);
    }
  DF_REF_CHAIN (use) = 0;

  /* Unlink use from reg-use chain.  */
  df_ref_unlink (&df->regs[uregno].uses, use);

  df->uses[DF_REF_ID (use)] = 0;
}

/* Local routines for recording refs.  */


/* Create a new ref of type DF_REF_TYPE for register REG at address
   LOC within INSN of BB.  */
static struct ref *
df_ref_create (df, reg, loc, bb, insn, ref_type)
     struct df *df;     
     rtx reg;
     rtx *loc;
     basic_block bb;
     rtx insn;
     enum df_ref_type ref_type;
{
  struct ref *this_ref;
  unsigned int uid;
  
  this_ref = (struct ref *) obstack_alloc (&df_ref_obstack, 
					   sizeof (*this_ref));
  DF_REF_REG (this_ref) = reg;
  DF_REF_LOC (this_ref) = loc;
  DF_REF_BB (this_ref) = bb;
  DF_REF_INSN (this_ref) = insn;
  DF_REF_CHAIN (this_ref) = 0;
  DF_REF_TYPE (this_ref) = ref_type;
  uid = INSN_UID (insn);

  if (ref_type == DF_REF_REG_DEF)
    {
      if (df->def_id >= df->def_size)
	{
	  /* Make table 25 percent larger.  */
	  df->def_size += (df->def_size / 4);
	  df->defs = xrealloc (df->defs, 
			       df->def_size * sizeof (*df->defs));
	}
      DF_REF_ID (this_ref) = df->def_id;
      df->defs[df->def_id++] = this_ref;
    }
  else
    {
      if (df->use_id >= df->use_size)
	{
	  /* Make table 25 percent larger.  */
	  df->use_size += (df->use_size / 4);
	  df->uses = xrealloc (df->uses, 
			       df->use_size * sizeof (*df->uses));
	}
      DF_REF_ID (this_ref) = df->use_id;
      df->uses[df->use_id++] = this_ref;
    }
  return this_ref;
}


/* Create a new reference of type DF_REF_TYPE for a single register REG,
   used inside the LOC rtx of INSN.  */
static void
df_ref_record_1 (df, reg, loc, bb, insn, ref_type)
     struct df *df;
     rtx reg;
     rtx *loc;
     basic_block bb;
     rtx insn;
     enum df_ref_type ref_type;
{
  df_ref_create (df, reg, loc, bb, insn, ref_type);
}


/* Create new references of type DF_REF_TYPE for each part of register REG
   at address LOC within INSN of BB.  */
static void
df_ref_record (df, reg, loc, bb, insn, ref_type)
     struct df *df;
     rtx reg;
     rtx *loc;
     basic_block bb;
     rtx insn;
     enum df_ref_type ref_type;
{
  unsigned int regno;

  if (GET_CODE (reg) != REG && GET_CODE (reg) != SUBREG)
    abort ();

  /* For the reg allocator we are interested in some SUBREG rtx's, but not
     all.  Notably only those representing a word extraction from a multi-word
     reg.  As written in the docu those should have the form
     (subreg:SI (reg:M A) N), with size(SImode) > size(Mmode).
     XXX Is that true?  We could also use the global word_mode variable.  */
  if (GET_CODE (reg) == SUBREG
      && (GET_MODE_SIZE (GET_MODE (reg)) < GET_MODE_SIZE (word_mode)
          || GET_MODE_SIZE (GET_MODE (reg))
	       >= GET_MODE_SIZE (GET_MODE (SUBREG_REG (reg)))))
    {
      loc = &SUBREG_REG (reg);
      reg = *loc;
    }

  regno = REGNO (GET_CODE (reg) == SUBREG ? SUBREG_REG (reg) : reg);
  if (regno < FIRST_PSEUDO_REGISTER)
    {
      int i;
      int endregno;
      
      if (! (df->flags & DF_HARD_REGS))
	return;

      /* GET_MODE (reg) is correct here.  We don't want to go into a SUBREG
         for the mode, because we only want to add references to regs, which
	 are really referenced.  E.g. a (subreg:SI (reg:DI 0) 0) does _not_
	 reference the whole reg 0 in DI mode (which would also include
	 reg 1, at least, if 0 and 1 are SImode registers).  */
      endregno = regno + HARD_REGNO_NREGS (regno, GET_MODE (reg));

      for (i = regno; i < endregno; i++)
	df_ref_record_1 (df, gen_rtx_REG (reg_raw_mode[i], i),
			 loc, bb, insn, ref_type);
    }
  else
    {
      df_ref_record_1 (df, reg, loc, bb, insn, ref_type);
    }
}


/* Process all the registers defined in the rtx, X.  */
static void
df_def_record_1 (df, x, bb, insn)
     struct df *df;
     rtx x;
     basic_block bb;
     rtx insn;
{
  rtx *loc = &SET_DEST (x);
  rtx dst = *loc;

  /* Some targets place small structures in registers for
     return values of functions.  */
  if (GET_CODE (dst) == PARALLEL && GET_MODE (dst) == BLKmode)
    {
      int i;

      for (i = XVECLEN (dst, 0) - 1; i >= 0; i--)
	  df_def_record_1 (df, XVECEXP (dst, 0, i), bb, insn);
      return;
    }

  /* May be, we should flag the use of strict_low_part somehow.  Might be
     handy for the reg allocator.  */
#ifdef HANDLE_SUBREG
  while (GET_CODE (dst) == STRICT_LOW_PART
         || GET_CODE (dst) == ZERO_EXTRACT
	 || GET_CODE (dst) == SIGN_EXTRACT)
    {
      loc = &XEXP (dst, 0);
      dst = *loc;
    }
  /* For the reg allocator we are interested in exact register references.
     This means, we want to know, if only a part of a register is
     used/defd.  */
/*
  if (GET_CODE (dst) == SUBREG)
    {
      loc = &XEXP (dst, 0);
      dst = *loc;
    } */
#else

  while (GET_CODE (dst) == SUBREG
	 || GET_CODE (dst) == ZERO_EXTRACT
	 || GET_CODE (dst) == SIGN_EXTRACT
	 || GET_CODE (dst) == STRICT_LOW_PART)
    {
      loc = &XEXP (dst, 0);
      dst = *loc;
    }
#endif

  if (GET_CODE (dst) == REG
      || (GET_CODE (dst) == SUBREG && GET_CODE (SUBREG_REG (dst)) == REG))
      df_ref_record (df, dst, loc, bb, insn, DF_REF_REG_DEF);
}


/* Process all the registers defined in the pattern rtx, X.  */
static void
df_defs_record (df, x, bb, insn)
     struct df *df;
     rtx x;
     basic_block bb;
     rtx insn;
{
  RTX_CODE code = GET_CODE (x);

  if (code == SET || code == CLOBBER)
    {
      /* Mark the single def within the pattern.  */
      df_def_record_1 (df, x, bb, insn);
    }
  else if (code == PARALLEL)
    {
      int i;

      /* Mark the multiple defs within the pattern.  */
      for (i = XVECLEN (x, 0) - 1; i >= 0; i--)
	{
	  code = GET_CODE (XVECEXP (x, 0, i));
	  if (code == SET || code == CLOBBER)
	    df_def_record_1 (df, XVECEXP (x, 0, i), bb, insn);
	}
    }
}


/* Process all the registers used in the rtx at address LOC.  */
static void
df_uses_record (df, loc, ref_type, bb, insn)
     struct df *df;
     rtx *loc;
     enum df_ref_type ref_type;
     basic_block bb;
     rtx insn;
{
  RTX_CODE code;
  rtx x;

 retry:
  x = *loc;
  code = GET_CODE (x);
  switch (code)
    {
    case LABEL_REF:
    case SYMBOL_REF:
    case CONST_INT:
    case CONST:
    case CONST_DOUBLE:
    case PC:
    case ADDR_VEC:
    case ADDR_DIFF_VEC:
      return;

    case CLOBBER:
      /* If we are clobbering a MEM, mark any registers inside the address
	 as being used.  */
      if (GET_CODE (XEXP (x, 0)) == MEM)
	df_uses_record (df, &XEXP (XEXP (x, 0), 0), 
			DF_REF_REG_MEM_STORE, bb, insn);

      /* If we're clobbering a REG then we have a def so ignore.  */
      return;

    case MEM:
      df_uses_record (df, &XEXP (x, 0), DF_REF_REG_MEM_LOAD, bb, insn);
      return;

    case SUBREG:
      /* While we're here, optimize this case.  */
#if defined(HANDLE_SUBREG)

      /* In case the SUBREG is not of a register, don't optimize.  */
      if (GET_CODE (SUBREG_REG (x)) != REG)
	{
	  loc = &SUBREG_REG (x);
	  df_uses_record (df, loc, ref_type, bb, insn);
	  return;
	}
#else
      loc = &SUBREG_REG (x);
      x = *loc;
      if (GET_CODE (x) != REG)
	{
	  df_uses_record (df, loc, ref_type, bb, insn);
	  return;
	}
#endif

      /* ... Fall through ...  */

    case REG:
      /* See a register (or subreg) other than being set.  */
      df_ref_record (df, x, loc, bb, insn, ref_type);
      return;

    case SET:
      {
	rtx dst = SET_DEST (x);
	int use_dst = 0;

	/* If storing into MEM, don't show it as being used.  But do
	   show the address as being used.  */
	if (GET_CODE (dst) == MEM)
	  {
	    df_uses_record (df, &XEXP (dst, 0), 
			    DF_REF_REG_MEM_STORE,
			    bb, insn);
	    df_uses_record (df, &SET_SRC (x), DF_REF_REG_USE, bb, insn);
	    return;
	  }
	    
#if 1 && defined(HANDLE_SUBREG)
	/* Look for sets that perform a read-modify-write.  */
	while (GET_CODE (dst) == STRICT_LOW_PART
	       || GET_CODE (dst) == ZERO_EXTRACT
	       || GET_CODE (dst) == SIGN_EXTRACT)
	  {
	    if (GET_CODE (dst) == STRICT_LOW_PART)
	      {
		dst = XEXP (dst, 0);
		if (GET_CODE (dst) != SUBREG)
		  abort ();
		/* A strict_low_part uses the whole reg not only the subreg.  */
		df_uses_record (df, &SUBREG_REG (dst), DF_REF_REG_USE, bb, insn);
	      }
	    else
	      {
	        df_uses_record (df, &XEXP (dst, 0), DF_REF_REG_USE, bb, insn);
		dst = XEXP (dst, 0);
	      }
	  }
	if (GET_CODE (dst) == SUBREG)
	  {
	    /* Paradoxical or too small subreg's are read-mod-write.  */
            if (GET_MODE_SIZE (GET_MODE (dst)) < GET_MODE_SIZE (word_mode)
                || GET_MODE_SIZE (GET_MODE (dst))
	           >= GET_MODE_SIZE (GET_MODE (SUBREG_REG (dst))))
	      use_dst = 1;
	  }
	/* In the original code also some SUBREG rtx's were considered
	   read-modify-write (those with
	     REG_SIZE(SUBREG_REG(dst)) > REG_SIZE(dst) )
	   e.g. a (subreg:QI (reg:SI A) 0).  I can't see this.  The only
	   reason for a read cycle for reg A would be to somehow preserve
	   the bits outside of the subreg:QI.  But for this a strict_low_part
	   was necessary anyway, and this we handled already.  */
#else
	while (GET_CODE (dst) == STRICT_LOW_PART
	       || GET_CODE (dst) == ZERO_EXTRACT
	       || GET_CODE (dst) == SIGN_EXTRACT
	       || GET_CODE (dst) == SUBREG)
	  {
	    /* A SUBREG of a smaller size does not use the old value.  */
	    if (GET_CODE (dst) != SUBREG
		|| (REG_SIZE (SUBREG_REG (dst)) > REG_SIZE (dst)))
	      use_dst = 1;
	    dst = XEXP (dst, 0);
	  }
#endif

	if ((GET_CODE (dst) == PARALLEL && GET_MODE (dst) == BLKmode)
	    || GET_CODE (dst) == REG || GET_CODE (dst) == SUBREG)
	  {
#if 1 || !defined(HANDLE_SUBREG)
            if (use_dst)
	      df_uses_record (df, &SET_DEST (x), DF_REF_REG_USE, bb, insn);
#endif
	    df_uses_record (df, &SET_SRC (x), DF_REF_REG_USE, bb, insn);
	    return;
	  }
      }
      break;

    case RETURN:
      break;

    case ASM_OPERANDS:
    case UNSPEC_VOLATILE:
    case TRAP_IF:
    case ASM_INPUT:
      {
	/* Traditional and volatile asm instructions must be considered to use
	   and clobber all hard registers, all pseudo-registers and all of
	   memory.  So must TRAP_IF and UNSPEC_VOLATILE operations.

	   Consider for instance a volatile asm that changes the fpu rounding
	   mode.  An insn should not be moved across this even if it only uses
	   pseudo-regs because it might give an incorrectly rounded result. 

	   For now, just mark any regs we can find in ASM_OPERANDS as
	   used.  */

        /* For all ASM_OPERANDS, we must traverse the vector of input operands.
	   We can not just fall through here since then we would be confused
	   by the ASM_INPUT rtx inside ASM_OPERANDS, which do not indicate
	   traditional asms unlike their normal usage.  */
	if (code == ASM_OPERANDS)
	  {
	    int j;

	    for (j = 0; j < ASM_OPERANDS_INPUT_LENGTH (x); j++)
	      df_uses_record (df, &ASM_OPERANDS_INPUT (x, j), 
			      DF_REF_REG_USE, bb, insn);
	  }
	break;
      }

    case PRE_DEC:
    case POST_DEC:
    case PRE_INC:
    case POST_INC:
    case PRE_MODIFY:
    case POST_MODIFY:
      /* Catch the def of the register being modified.  */
      df_ref_record (df, XEXP (x, 0), &XEXP (x, 0), bb, insn, DF_REF_REG_DEF);

      /* ... Fall through to handle uses ... */

    default:
      break;
    }

  /* Recursively scan the operands of this expression.  */
  {
    register const char *fmt = GET_RTX_FORMAT (code);
    int i;
    
    for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
      {
	if (fmt[i] == 'e')
	  {
	    /* Tail recursive case: save a function call level.  */
	    if (i == 0)
	      {
		loc = &XEXP (x, 0);
		goto retry;
	      }
	    df_uses_record (df, &XEXP (x, i), ref_type, bb, insn);
	  }
	else if (fmt[i] == 'E')
	  {
	    int j;
	    for (j = 0; j < XVECLEN (x, i); j++)
	      df_uses_record (df, &XVECEXP (x, i, j), ref_type,
			      bb, insn);
	  }
      }
  }
}


/* Record all the df within INSN of basic block BB.  */
static void
df_insn_refs_record (df, bb, insn)
     struct df *df;
     basic_block bb;
     rtx insn;
{
  int i;

  if (INSN_P (insn))
    {
      /* Record register defs */
      df_defs_record (df, PATTERN (insn), bb, insn);
      
      if (GET_CODE (insn) == CALL_INSN)
	{
	  rtx note;
	  rtx x;
	  
	  /* Record the registers used to pass arguments.  */
	  for (note = CALL_INSN_FUNCTION_USAGE (insn); note;
	       note = XEXP (note, 1))
	    {
	      if (GET_CODE (XEXP (note, 0)) == USE)
		df_uses_record (df, &SET_DEST (XEXP (note, 0)), DF_REF_REG_USE,
				bb, insn);
	    }

	  /* The stack ptr is used (honorarily) by a CALL insn.  */
	  x = df_reg_use_gen (STACK_POINTER_REGNUM);
	  df_uses_record (df, &SET_DEST (x), DF_REF_REG_USE, bb, insn);
	  
	  if (df->flags & DF_HARD_REGS)
	    {
	      /* Calls may also reference any of the global registers,
		 so they are recorded as used.  */
	      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
		if (global_regs[i])
		  {
		    x = df_reg_use_gen (i);
		    df_uses_record (df, &SET_DEST (x),
				    DF_REF_REG_USE, bb, insn);
		  }
	    }
	}
      
      /* Record the register uses.  */
      df_uses_record (df, &PATTERN (insn), 
		      DF_REF_REG_USE, bb, insn);
      

      if (GET_CODE (insn) == CALL_INSN)
	{
	  rtx note;

	  if (df->flags & DF_HARD_REGS)
	    {
	      /* Each call clobbers all call-clobbered regs that are not
		 global or fixed and have not been explicitly defined
		 in the call pattern.  */
	      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
		if (call_used_regs[i] 
		    && ! global_regs[i]
		    && ! fixed_regs[i]
		    && ! df_insn_regno_def_p (df, bb, insn, i))
		  {
		    rtx reg_clob = df_reg_clobber_gen (i);
		    df_defs_record (df, reg_clob, bb, insn);
		  }
	    }
	  
	  /* There may be extra registers to be clobbered.  */
	  for (note = CALL_INSN_FUNCTION_USAGE (insn);
	       note;
	       note = XEXP (note, 1))
	    if (GET_CODE (XEXP (note, 0)) == CLOBBER)
	      df_defs_record (df, XEXP (note, 0), bb, insn);
	}
    }
}


/* Record all the refs within the basic block BB.  */
static void
df_bb_refs_record (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;

  /* Scan the block an insn at a time from beginning to end.  */
  for (insn = bb->head; ; insn = NEXT_INSN (insn))
    {
      if (INSN_P (insn))
	{
	  /* Record defs within INSN.  */
	  df_insn_refs_record (df, bb, insn);
	}
      if (insn == bb->end)
	break;
    }
}


/* Record all the refs in the basic blocks specified by BLOCKS.  */
static void
df_refs_record (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
    {
      df_bb_refs_record (df, bb);
    });
}

/* Dataflow analysis routines.  */


/* Create reg-def chains for basic block BB.  These are a list of
   definitions for each register.  */
static void
df_bb_reg_def_chain_create (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;
  
  /* Perhaps the defs should be sorted using a depth first search
     of the CFG (or possibly a breadth first search).  We currently
     scan the basic blocks in reverse order so that the first defs
     apprear at the start of the chain.  */
  
  for (insn = bb->end; insn && insn != PREV_INSN (bb->head);
       insn = PREV_INSN (insn))
    {
      struct df_link *link;
      unsigned int uid = INSN_UID (insn);

      if (! INSN_P (insn))
	continue;
      
      for (link = df->insns[uid].defs; link; link = link->next)
	{
	  struct ref *def = link->ref;
	  unsigned int dregno = DF_REF_REGNO (def);
	  
	  df->regs[dregno].defs
	    = df_link_create (def, df->regs[dregno].defs);
	}
    }
}


/* Create reg-def chains for each basic block within BLOCKS.  These
   are a list of definitions for each register.  */
static void
df_reg_def_chain_create (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP/*_REV*/ (blocks, 0, bb,
    {
      df_bb_reg_def_chain_create (df, bb);
    });
}


/* Create reg-use chains for basic block BB.  These are a list of uses
   for each register.  */
static void
df_bb_reg_use_chain_create (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;
  
  /* Scan in forward order so that the last uses appear at the
	 start of the chain.  */
  
  for (insn = bb->head; insn && insn != NEXT_INSN (bb->end);
       insn = NEXT_INSN (insn))
    {
      struct df_link *link;
      unsigned int uid = INSN_UID (insn);

      if (! INSN_P (insn))
	continue;
      
      for (link = df->insns[uid].uses; link; link = link->next)
	{
	  struct ref *use = link->ref;
	  unsigned int uregno = DF_REF_REGNO (use);
	  
	  df->regs[uregno].uses
	    = df_link_create (use, df->regs[uregno].uses);
	}
    }
}


/* Create reg-use chains for each basic block within BLOCKS.  These
   are a list of uses for each register.  */
static void
df_reg_use_chain_create (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
    {
      df_bb_reg_use_chain_create (df, bb);
    });
}


/* Create def-use chains from reaching use bitmaps for basic block BB.  */
static void
df_bb_du_chain_create (df, bb, ru)
     struct df *df;
     basic_block bb;
     bitmap ru;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  rtx insn;
  
  bitmap_copy (ru, bb_info->ru_out);
  
  /* For each def in BB create a linked list (chain) of uses
     reached from the def.  */
  for (insn = bb->end; insn && insn != PREV_INSN (bb->head);
       insn = PREV_INSN (insn))
    {
      struct df_link *def_link;
      struct df_link *use_link;
      unsigned int uid = INSN_UID (insn);

      if (! INSN_P (insn))
	continue;
      
      /* For each def in insn...  */
      for (def_link = df->insns[uid].defs; def_link; def_link = def_link->next)
	{
	  struct ref *def = def_link->ref;
	  unsigned int dregno = DF_REF_REGNO (def);
	  
	  DF_REF_CHAIN (def) = 0;

	  /* While the reg-use chains are not essential, it
	     is _much_ faster to search these short lists rather
	     than all the reaching uses, especially for large functions.  */
	  for (use_link = df->regs[dregno].uses; use_link; 
	       use_link = use_link->next)
	    {
	      struct ref *use = use_link->ref;
	      
	      if (bitmap_bit_p (ru, DF_REF_ID (use)))
		{
		  DF_REF_CHAIN (def) 
		    = df_link_create (use, DF_REF_CHAIN (def));
		  
		  bitmap_clear_bit (ru, DF_REF_ID (use));
		}
	    }
	}

      /* For each use in insn...  */
      for (use_link = df->insns[uid].uses; use_link; use_link = use_link->next)
	{
	  struct ref *use = use_link->ref;
	  bitmap_set_bit (ru, DF_REF_ID (use));
	}
    }
}


/* Create def-use chains from reaching use bitmaps for basic blocks
   in BLOCKS.  */
static void
df_du_chain_create (df, blocks)
     struct df *df;
     bitmap blocks;
{
  bitmap ru;
  basic_block bb;

  ru = BITMAP_XMALLOC ();

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
    {
      df_bb_du_chain_create (df, bb, ru);
    });

  BITMAP_XFREE (ru);
}


/* Create use-def chains from reaching def bitmaps for basic block BB.  */
static void
df_bb_ud_chain_create (df, bb)
     struct df *df;
     basic_block bb;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  struct ref **reg_def_last = df->reg_def_last;
  rtx insn;
  
  memset (reg_def_last, 0, df->n_regs * sizeof (struct ref *));
  
  /* For each use in BB create a linked list (chain) of defs
     that reach the use.  */
  for (insn = bb->head; insn && insn != NEXT_INSN (bb->end);
       insn = NEXT_INSN (insn))
    {
      unsigned int uid = INSN_UID (insn);
      struct df_link *use_link;
      struct df_link *def_link;

      if (! INSN_P (insn))
	continue;

      /* For each use in insn...  */      
      for (use_link = df->insns[uid].uses; use_link; use_link = use_link->next)
	{
	  struct ref *use = use_link->ref;
	  unsigned int regno = DF_REF_REGNO (use);
	  
	  DF_REF_CHAIN (use) = 0;

	  /* Has regno been defined in this BB yet?  If so, use
	     the last def as the single entry for the use-def
	     chain for this use.  Otherwise, we need to add all
	     the defs using this regno that reach the start of
	     this BB.  */
	  if (reg_def_last[regno])
	    {
	      DF_REF_CHAIN (use) 
		= df_link_create (reg_def_last[regno], 0);
	    }
	  else
	    {
	      /* While the reg-def chains are not essential, it is
		 _much_ faster to search these short lists rather than
		 all the reaching defs, especially for large
		 functions.  */
	      for (def_link = df->regs[regno].defs; def_link; 
		   def_link = def_link->next)
		{
		  struct ref *def = def_link->ref;
	      
		  if (bitmap_bit_p (bb_info->rd_in, DF_REF_ID (def)))
		    {
		      DF_REF_CHAIN (use) 
			= df_link_create (def, DF_REF_CHAIN (use));
		    }
		}
	    }
	}
      

      /* For each def in insn...record the last def of each reg.  */
      for (def_link = df->insns[uid].defs; def_link; def_link = def_link->next)
	{
	  struct ref *def = def_link->ref;
	  int dregno = DF_REF_REGNO (def);
	  
	  reg_def_last[dregno] = def;
	}
    }
}


/* Create use-def chains from reaching def bitmaps for basic blocks
   within BLOCKS.  */
static void
df_ud_chain_create (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
    {
      df_bb_ud_chain_create (df, bb);
    });
}


/* Use depth first order, and the worklist, to figure out what block
   to look at next. */

static int
df_visit_next (df, blocks)
     struct df *df ATTRIBUTE_UNUSED;
     sbitmap blocks;
{
  int i=0;
  for (i = 0; i < n_basic_blocks; i++)
    if (TEST_BIT (blocks, df->rc_order[i]))
      return df->rc_order[i];
  return sbitmap_first_set_bit (blocks);
}

/* Calculate reaching defs for each basic block in BLOCKS, i.e., the
   defs that are live at the start of a basic block.  */
static void
df_rd_global_compute (df, blocks)
     struct df *df ATTRIBUTE_UNUSED;
     bitmap blocks;
{
  int i;
  basic_block bb;
  sbitmap worklist;
  
  worklist = sbitmap_alloc (n_basic_blocks);
  sbitmap_zero (worklist);

  /* Copy the blocklist to the worklist */
  EXECUTE_IF_SET_IN_BITMAP (blocks, 0, i, 
  {
    SET_BIT (worklist, i);
  });
  
  /* We assume that only the basic blocks in WORKLIST have been
     modified.  */
  FOR_EACH_BB_IN_SBITMAP (worklist, 0, bb,
    {
      struct bb_info *bb_info = DF_BB_INFO (df, bb);
      
      bitmap_copy (bb_info->rd_out, bb_info->rd_gen);
    });
  
  while ((i = df_visit_next (df, worklist)) >= 0)
    {
      struct bb_info *bb_info;
      edge e;
      int changed;

      /* Remove this block from the worklist.  */
      RESET_BIT (worklist, i);
      

      bb = BASIC_BLOCK (i);  
      bb_info = DF_BB_INFO (df, bb);

      /* Calculate union of predecessor outs.  */
      bitmap_zero (bb_info->rd_in);
      for (e = bb->pred; e != 0; e = e->pred_next)
	{
	  struct bb_info *pred_refs = DF_BB_INFO (df, e->src);
	  
	  if (e->src == ENTRY_BLOCK_PTR)
	    continue;

	  bitmap_a_or_b (bb_info->rd_in, bb_info->rd_in, 
			  pred_refs->rd_out);
	}
      
      /* RD_OUT is the set of defs that are live at the end of the
	 BB.  These are the defs that are either generated by defs
	 (RD_GEN) within the BB or are live at the start (RD_IN)
	 and are not killed by other defs (RD_KILL).  */
      changed = bitmap_union_of_diff (bb_info->rd_out, bb_info->rd_gen,
				       bb_info->rd_in, bb_info->rd_kill);

      if (changed)
	{
	  /* Add each of this block's successors to the worklist.  */
	  for (e = bb->succ; e != 0; e = e->succ_next)
	    {
	      if (e->dest == EXIT_BLOCK_PTR)
		continue;
	      
	      SET_BIT (worklist, i);
	    }
	}
    }
  sbitmap_free (worklist);
}


/* Calculate reaching uses for each basic block within BLOCKS, i.e.,
   the uses that are live at the start of a basic block.  */
static void
df_ru_global_compute (df, blocks)
     struct df *df ATTRIBUTE_UNUSED;
     bitmap blocks;
{
  int i;
  basic_block bb;
  sbitmap worklist;

  worklist = sbitmap_alloc (n_basic_blocks);
  sbitmap_zero (worklist);
  
  EXECUTE_IF_SET_IN_BITMAP (blocks, 0, i, 
  {
    SET_BIT (worklist, i);
  });

  /* We assume that only the basic blocks in WORKLIST have been
     modified.  */
  FOR_EACH_BB_IN_SBITMAP (worklist, 0, bb,
    {
      struct bb_info *bb_info = DF_BB_INFO (df, bb);

      bitmap_copy (bb_info->ru_in, bb_info->ru_gen);
    });


  while ((i = df_visit_next (df, worklist)) >= 0)
    {
      struct bb_info *bb_info;
      edge e;
      int changed;
      
      /* Remove this block from the worklist.  */
      RESET_BIT (worklist, i);
      
      bb = BASIC_BLOCK (i);  
      bb_info = DF_BB_INFO (df, bb);

      /* Calculate union of successor ins.  */
      bitmap_zero (bb_info->ru_out);
      for (e = bb->succ; e != 0; e = e->succ_next)
	{
	  struct bb_info *succ_refs = DF_BB_INFO (df, e->dest);
	  
	  if (e->dest == EXIT_BLOCK_PTR)
	    continue;
	  
	  bitmap_a_or_b (bb_info->ru_out, bb_info->ru_out, 
			  succ_refs->ru_in);
	}

      /* RU_IN is the set of uses that are live at the start of the
	 BB.  These are the uses that are either generated within the
	 BB (RU_GEN) or are live at the end (RU_OUT) and are not uses
	 killed by defs within the BB (RU_KILL).  */
      changed = bitmap_union_of_diff (bb_info->ru_in, bb_info->ru_gen,
				       bb_info->ru_out, bb_info->ru_kill);

      if (changed)
	{
	  /* Add each of this block's predecessors to the worklist.  */
	  for (e = bb->pred; e != 0; e = e->pred_next)
	    {
	      if (e->src == ENTRY_BLOCK_PTR)
		continue;

	      SET_BIT (worklist, i);	      
	    }
	}
    }

  sbitmap_free (worklist);
}


/* Calculate live registers for each basic block within BLOCKS.  */
static void
df_lr_global_compute (df, blocks)
     struct df *df ATTRIBUTE_UNUSED;
     bitmap blocks;
{
  int i;
  basic_block bb;
  bitmap worklist;

  worklist = BITMAP_XMALLOC ();
  bitmap_copy (worklist, blocks);

  /* We assume that only the basic blocks in WORKLIST have been
     modified.  */
  FOR_EACH_BB_IN_BITMAP (worklist, 0, bb,
    {
      struct bb_info *bb_info = DF_BB_INFO (df, bb);

      bitmap_copy (bb_info->lr_in, bb_info->lr_use);
    });

  while ((i = bitmap_last_set_bit (worklist)) >= 0)
    {
      struct bb_info *bb_info = DF_BB_INFO (df, bb);
      edge e;
      int changed;
      
      /* Remove this block from the worklist.  */
      bitmap_clear_bit (worklist, i);

      bb = BASIC_BLOCK (i);  
      bb_info = DF_BB_INFO (df, bb);

      /* Calculate union of successor ins.  */
      bitmap_zero (bb_info->lr_out);
      for (e = bb->succ; e != 0; e = e->succ_next)
	{
	  struct bb_info *succ_refs = DF_BB_INFO (df, e->dest);
	  
	  if (e->dest == EXIT_BLOCK_PTR)
	    continue;
	  
	  bitmap_a_or_b (bb_info->lr_out, bb_info->lr_out, 
			  succ_refs->lr_in);
	}

      /* LR_IN is the set of uses that are live at the start of the
	 BB.  These are the uses that are either generated by uses
	 (LR_USE) within the BB or are live at the end (LR_OUT)
	 and are not killed by other uses (LR_DEF).  */
      changed = bitmap_union_of_diff (bb_info->lr_in, bb_info->lr_use,
				       bb_info->lr_out, bb_info->lr_def);

      if (changed)
	{
	  /* Add each of this block's predecessors to the worklist.  */
	  for (e = bb->pred; e != 0; e = e->pred_next)
	    {
	      if (e->src == ENTRY_BLOCK_PTR)
		continue;

	      bitmap_set_bit (worklist, e->src->index);
	    }
	}
    }
  BITMAP_XFREE (worklist);
}


/* Compute local reaching def info for basic block BB.  */
static void
df_bb_rd_local_compute (df, bb)
     struct df *df;
     basic_block bb;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  rtx insn;
  
  for (insn = bb->head; insn && insn != NEXT_INSN (bb->end);
       insn = NEXT_INSN (insn))
    {
      unsigned int uid = INSN_UID (insn);
      struct df_link *def_link;

      if (! INSN_P (insn))
	continue;
      
      for (def_link = df->insns[uid].defs; def_link; def_link = def_link->next)
	{
	  struct ref *def = def_link->ref;
	  unsigned int regno = DF_REF_REGNO (def);
	  struct df_link *def2_link;

	  for (def2_link = df->regs[regno].defs; def2_link; 
	       def2_link = def2_link->next)
	    {
	      struct ref *def2 = def2_link->ref;

	      /* Add all defs of this reg to the set of kills.  This
		 is greedy since many of these defs will not actually
		 be killed by this BB but it keeps things a lot
		 simpler.  */
	      bitmap_set_bit (bb_info->rd_kill, DF_REF_ID (def2));
	      
	      /* Zap from the set of gens for this BB.  */
	      bitmap_clear_bit (bb_info->rd_gen, DF_REF_ID (def2));
	    }

	  bitmap_set_bit (bb_info->rd_gen, DF_REF_ID (def));
	}
    }
  
  bb_info->rd_valid = 1;
}


/* Compute local reaching def info for each basic block within BLOCKS.  */
static void
df_rd_local_compute (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
  {
    df_bb_rd_local_compute (df, bb);
  });
}


/* Compute local reaching use (upward exposed use) info for basic
   block BB.  */
static void
df_bb_ru_local_compute (df, bb)
     struct df *df;
     basic_block bb;
{
  /* This is much more tricky than computing reaching defs.  With
     reaching defs, defs get killed by other defs.  With upwards
     exposed uses, these get killed by defs with the same regno.  */

  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  rtx insn;

  for (insn = bb->end; insn && insn != PREV_INSN (bb->head);
       insn = PREV_INSN (insn))
    {
      unsigned int uid = INSN_UID (insn);
      struct df_link *def_link;
      struct df_link *use_link;

      if (! INSN_P (insn))
	continue;
      
      for (def_link = df->insns[uid].defs; def_link; def_link = def_link->next)
	{
	  struct ref *def = def_link->ref;
	  unsigned int dregno = DF_REF_REGNO (def);

	  for (use_link = df->regs[dregno].uses; use_link; 
	       use_link = use_link->next)
	    {
	      struct ref *use = use_link->ref;

	      /* Add all uses of this reg to the set of kills.  This
		 is greedy since many of these uses will not actually
		 be killed by this BB but it keeps things a lot
		 simpler.  */
	      bitmap_set_bit (bb_info->ru_kill, DF_REF_ID (use));
	      
	      /* Zap from the set of gens for this BB.  */
	      bitmap_clear_bit (bb_info->ru_gen, DF_REF_ID (use));
	    }
	}
      
      for (use_link = df->insns[uid].uses; use_link; use_link = use_link->next)
	{
	  struct ref *use = use_link->ref;
	  /* Add use to set of gens in this BB.  */
	  bitmap_set_bit (bb_info->ru_gen, DF_REF_ID (use));
	}
    }
  bb_info->ru_valid = 1;
}


/* Compute local reaching use (upward exposed use) info for each basic
   block within BLOCKS.  */
static void
df_ru_local_compute (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
  {
    df_bb_ru_local_compute (df, bb);
  });
}


/* Compute local live variable info for basic block BB.  */
static void
df_bb_lr_local_compute (df, bb)
     struct df *df;
     basic_block bb;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  rtx insn;
  
  for (insn = bb->end; insn && insn != PREV_INSN (bb->head);
       insn = PREV_INSN (insn))
    {
      unsigned int uid = INSN_UID (insn);
      struct df_link *link;

      if (! INSN_P (insn))
	continue;
      
      for (link = df->insns[uid].defs; link; link = link->next)
	{
	  struct ref *def = link->ref;
	  unsigned int dregno = DF_REF_REGNO (def);
	  
	  /* Add def to set of defs in this BB.  */
	  bitmap_set_bit (bb_info->lr_def, dregno);
	  
	  bitmap_clear_bit (bb_info->lr_use, dregno);
	}
      
      for (link = df->insns[uid].uses; link; link = link->next)
	{
	  struct ref *use = link->ref;
	  /* Add use to set of uses in this BB.  */
	  bitmap_set_bit (bb_info->lr_use, DF_REF_REGNO (use));
	}
    }
  bb_info->lr_valid = 1;
}


/* Compute local live variable info for each basic block within BLOCKS.  */
static void
df_lr_local_compute (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
  {
    df_bb_lr_local_compute (df, bb);
  });
}


/* Compute register info: lifetime, bb, and number of defs and uses
   for basic block BB.  */
static void
df_bb_reg_info_compute (df, bb, live)
     struct df *df;
     basic_block bb;
  bitmap live;
{
  struct reg_info *reg_info = df->regs;
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  rtx insn;
  
  bitmap_copy (live, bb_info->lr_out);
  
  for (insn = bb->end; insn && insn != PREV_INSN (bb->head);
       insn = PREV_INSN (insn))
    {
      unsigned int uid = INSN_UID (insn);
      unsigned int regno;
      struct df_link *link;
      
      if (! INSN_P (insn))
	continue;
      
      for (link = df->insns[uid].defs; link; link = link->next)
	{
	  struct ref *def = link->ref;
	  unsigned int dregno = DF_REF_REGNO (def);
	  
	  /* Kill this register.  */
	  bitmap_clear_bit (live, dregno);
	  reg_info[dregno].n_defs++;
	}
      
      for (link = df->insns[uid].uses; link; link = link->next)
	{
	  struct ref *use = link->ref;
	  unsigned int uregno = DF_REF_REGNO (use);
	  
	  /* This register is now live.  */
	  bitmap_set_bit (live, uregno);
	  reg_info[uregno].n_uses++;
	}
      
      /* Increment lifetimes of all live registers.  */
      EXECUTE_IF_SET_IN_BITMAP (live, 0, regno,
      { 
	reg_info[regno].lifetime++;
      });
    }
}


/* Compute register info: lifetime, bb, and number of defs and uses.  */
static void
df_reg_info_compute (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;
  bitmap live;

  live = BITMAP_XMALLOC ();

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
  {
    df_bb_reg_info_compute (df, bb, live);
  });

  BITMAP_XFREE (live);
}


/* Assign LUIDs for BB.  */
static int
df_bb_luids_set (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;
  int luid = 0;

  /* The LUIDs are monotonically increasing for each basic block.  */

  for (insn = bb->head; ; insn = NEXT_INSN (insn))
    {
      if (INSN_P (insn))
	DF_INSN_LUID (df, insn) = luid++;
      DF_INSN_LUID (df, insn) = luid;

      if (insn == bb->end)
	break;
    }
  return luid;
}


/* Assign LUIDs for each basic block within BLOCKS.  */
static int
df_luids_set (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;
  int total = 0;

  FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
    {
      total += df_bb_luids_set (df, bb);
    });
  return total;
}


/* Perform dataflow analysis using existing DF structure for blocks
   within BLOCKS.  If BLOCKS is zero, use all basic blocks in the CFG.  */
static void
df_analyse_1 (df, blocks, flags, update)
     struct df *df;
     bitmap blocks;
     int flags;
     int update;
{
  int aflags;
  int dflags;

  dflags = 0;
  aflags = flags;
  if (flags & DF_UD_CHAIN)
    aflags |= DF_RD | DF_RD_CHAIN;

  if (flags & DF_DU_CHAIN)
    aflags |= DF_RU;

  if (flags & DF_RU)
    aflags |= DF_RU_CHAIN;

  if (flags & DF_REG_INFO)
    aflags |= DF_LR;

  if (! blocks)
      blocks = df->all_blocks;

  df->flags = flags;
  if (update)
    {
      df_refs_update (df);
      /* More fine grained incremental dataflow analysis would be
	 nice.  For now recompute the whole shebang for the
	 modified blocks.  */
      // df_refs_unlink (df, blocks);
      /* All the def-use, use-def chains can be potentially
	 modified by changes in one block.  The size of the
	 bitmaps can also change.  */
    }
  else
    {
      /* Scan the function for all register defs and uses.  */
      df_refs_queue (df);
      df_refs_record (df, blocks);

      /* Link all the new defs and uses to the insns.  */
      df_refs_process (df);
    }

  /* Allocate the bitmaps now the total number of defs and uses are
     known.  If the number of defs or uses have changed, then
     these bitmaps need to be reallocated.  */
  df_bitmaps_alloc (df, aflags);

  /* Set the LUIDs for each specified basic block.  */
  df_luids_set (df, blocks);

  /* Recreate reg-def and reg-use chains from scratch so that first
     def is at the head of the reg-def chain and the last use is at
     the head of the reg-use chain.  This is only important for
     regs local to a basic block as it speeds up searching.  */
  if (aflags & DF_RD_CHAIN)
    {
      df_reg_def_chain_create (df, blocks);
    }

  if (aflags & DF_RU_CHAIN)
    {
      df_reg_use_chain_create (df, blocks);
    }

  df->dfs_order = xmalloc (sizeof(int) * n_basic_blocks);
  df->rc_order = xmalloc (sizeof(int) * n_basic_blocks);
  
  flow_depth_first_order_compute (df->dfs_order, df->rc_order);

  if (aflags & DF_RD)
    {
      /* Compute the sets of gens and kills for the defs of each bb.  */
      df_rd_local_compute (df, df->flags & DF_RD ? blocks : df->all_blocks);

      /* Compute the global reaching definitions.  */
      df_rd_global_compute (df, df->all_blocks);
    }

  if (aflags & DF_UD_CHAIN)
    {
      /* Create use-def chains.  */
      df_ud_chain_create (df, df->all_blocks);

      if (! (flags & DF_RD))
	dflags |= DF_RD;
    }
     
  if (aflags & DF_RU)
    {
      /* Compute the sets of gens and kills for the upwards exposed
	 uses in each bb.  */
      df_ru_local_compute (df, df->flags & DF_RU ? blocks : df->all_blocks);
      
      /* Compute the global reaching uses.  */
      df_ru_global_compute (df, df->all_blocks);
    }

  if (aflags & DF_DU_CHAIN)
    {
      /* Create def-use chains.  */
      df_du_chain_create (df, df->all_blocks);

      if (! (flags & DF_RU))
	dflags |= DF_RU;
    }

  /* Free up bitmaps that are no longer required.  */
  if (dflags)
     df_bitmaps_free (df, dflags);

  if (aflags & DF_LR)
    {
      /* Compute the sets of defs and uses of live variables.  */
      df_lr_local_compute (df, df->flags & DF_LR ? blocks : df->all_blocks);
      
      /* Compute the global live variables.  */
      df_lr_global_compute (df, df->all_blocks);
    }

  if (aflags & DF_REG_INFO)
    {
      df_reg_info_compute (df, df->all_blocks);
    } 
  free (df->dfs_order);
  free (df->rc_order);
}


/* Initialise dataflow analysis.  */
struct df *
df_init ()
{
  struct df *df;

  df = xcalloc (1, sizeof (struct df));

  /* Squirrel away a global for debugging.  */
  ddf = df;
  
  return df;
}


/* Start queuing refs.  */
static int
df_refs_queue (df)
     struct df *df;
{
  df->def_id_save = df->def_id;
  df->use_id_save = df->use_id;
  /* ???? Perhaps we should save current obstack state so that we can
     unwind it.  */
  return 0;
}


/* Process queued refs.  */
static int
df_refs_process (df)
     struct df *df;
{
  unsigned int i;

  /* Build new insn-def chains.  */
  for (i = df->def_id_save; i != df->def_id; i++)
    {
      struct ref *def = df->defs[i];
      unsigned int uid = DF_REF_INSN_UID (def);

      /* Add def to head of def list for INSN.  */
      df->insns[uid].defs
	= df_link_create (def, df->insns[uid].defs);
    }

  /* Build new insn-use chains.  */
  for (i = df->use_id_save; i != df->use_id; i++)
    {
      struct ref *use = df->uses[i];
      unsigned int uid = DF_REF_INSN_UID (use);

      /* Add use to head of use list for INSN.  */
      df->insns[uid].uses
	= df_link_create (use, df->insns[uid].uses);
    }
  return 0;
}


/* Update refs for basic block BB.  */
static int 
df_bb_refs_update (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;
  int count = 0;

  /* While we have to scan the chain of insns for this BB, we don't
     need to allocate and queue a long chain of BB/INSN pairs.  Using
     a bitmap for insns_modified saves memory and avoids queuing
     duplicates.  */

  for (insn = bb->head; ; insn = NEXT_INSN (insn))
    {
      unsigned int uid;

      uid = INSN_UID (insn);

      if (bitmap_bit_p (df->insns_modified, uid))
	{
	  /* Delete any allocated refs of this insn.  MPH,  FIXME.  */
	  df_insn_refs_unlink (df, bb, insn);
	  
	  /* Scan the insn for refs.  */
	  df_insn_refs_record (df, bb, insn);
	  

	  bitmap_clear_bit (df->insns_modified, uid);	  
	  count++;
	}
      if (insn == bb->end)
	break;
    }
  return count;
}


/* Process all the modified/deleted insns that were queued.  */
static int
df_refs_update (df)
     struct df *df;
{
  basic_block bb;
  int count = 0;

  if ((unsigned int)max_reg_num () >= df->reg_size)
    df_reg_table_realloc (df, 0);

  df_refs_queue (df);

  FOR_EACH_BB_IN_BITMAP (df->bbs_modified, 0, bb,
    {
      count += df_bb_refs_update (df, bb);
    });

  df_refs_process (df);
  return count;
}


/* Return non-zero if any of the requested blocks in the bitmap
   BLOCKS have been modified.  */
static int
df_modified_p (df, blocks)
     struct df *df;
     bitmap blocks;
{
  unsigned int j;
  int update = 0;

  for (j = 0; j < df->n_bbs; j++)
    if (bitmap_bit_p (df->bbs_modified, j)
	&& (! blocks || (blocks == (bitmap) -1) || bitmap_bit_p (blocks, j)))
    {
      update = 1;
      break;
    }

  return update;
}


/* Analyse dataflow info for the basic blocks specified by the bitmap
   BLOCKS, or for the whole CFG if BLOCKS is zero, or just for the
   modified blocks if BLOCKS is -1.  */
int
df_analyse (df, blocks, flags)
     struct df *df;
     bitmap blocks;
     int flags;
{
  int update;

  /* We could deal with additional basic blocks being created by
     rescanning everything again.  */
  if (df->n_bbs && df->n_bbs != (unsigned int)n_basic_blocks)
    abort ();

  update = df_modified_p (df, blocks);
  if (update || (flags != df->flags))
    {
      if (! blocks)
	{
	  if (df->n_bbs)
	    {
	      /* Recompute everything from scratch.  */
	      df_free (df);
	    }
	  /* Allocate and initialise data structures.  */
	  df_alloc (df, max_reg_num ());
	  df_analyse_1 (df, 0, flags, 0);
	  update = 1;
	}
      else
	{
	  if (blocks == (bitmap) -1)
	    blocks = df->bbs_modified;

	  if (! df->n_bbs)
	    abort ();

	  df_analyse_1 (df, blocks, flags, 1);
	  bitmap_zero (df->bbs_modified);
	}
    }
  return update;
}


/* Free all the dataflow info and the DF structure.  */
void
df_finish (df)
     struct df *df;
{
  df_free (df);
  free (df);
}


/* Unlink INSN from its reference information.  */
static void
df_insn_refs_unlink (df, bb, insn)
     struct df *df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
{
  struct df_link *link;
  unsigned int uid;
  
  uid = INSN_UID (insn);

  /* Unlink all refs defined by this insn.  */
  for (link = df->insns[uid].defs; link; link = link->next)
    df_def_unlink (df, link->ref);

  /* Unlink all refs used by this insn.  */
  for (link = df->insns[uid].uses; link; link = link->next)
    df_use_unlink (df, link->ref);

  df->insns[uid].defs = 0;
  df->insns[uid].uses = 0;
}


/* Unlink all the insns within BB from their reference information.  */
static void
df_bb_refs_unlink (df, bb)
     struct df *df;
     basic_block bb;
{
  rtx insn;

  /* Scan the block an insn at a time from beginning to end.  */
  for (insn = bb->head; ; insn = NEXT_INSN (insn))
    {
      if (INSN_P (insn))
	{
	  /* Unlink refs for INSN.  */
	  df_insn_refs_unlink (df, bb, insn);
	}
      if (insn == bb->end)
	break;
    }
}


/* Unlink all the refs in the basic blocks specified by BLOCKS.  */
static void
df_refs_unlink (df, blocks)
     struct df *df;
     bitmap blocks;
{
  basic_block bb;

  if (blocks)
    {
      FOR_EACH_BB_IN_BITMAP (blocks, 0, bb,
      {
	df_bb_refs_unlink (df, bb);
      });
    }
  else
    {
      FOR_ALL_BBS (bb,
      {
	df_bb_refs_unlink (df, bb);
      });
    }
}

/* Functions to modify insns.  */


/* Delete INSN and all its reference information.  */
rtx
df_insn_delete (df, bb, insn)
     struct df *df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
{
  /* If the insn is a jump, we should perhaps call delete_insn to
     handle the JUMP_LABEL?  */

  /* We should not be deleting the NOTE_INSN_BASIC_BLOCK or label.  */
  if (insn == bb->head)
    abort ();
  if (insn == bb->end)
    bb->end = PREV_INSN (insn);  

  /* Delete the insn.  */
  PUT_CODE (insn, NOTE);
  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
  NOTE_SOURCE_FILE (insn) = 0;

  df_insn_modify (df, bb, insn);

  return NEXT_INSN (insn);
}


/* Mark that INSN within BB may have changed  (created/modified/deleted).
   This may be called multiple times for the same insn.  There is no
   harm calling this function if the insn wasn't changed; it will just
   slow down the rescanning of refs.  */
void
df_insn_modify (df, bb, insn)
     struct df *df;
     basic_block bb;
     rtx insn;
{
  unsigned int uid;

  uid = INSN_UID (insn);

  bitmap_set_bit (df->bbs_modified, bb->index);
  bitmap_set_bit (df->insns_modified, uid);

#if 0
  /* For incremental updating on the fly, perhaps we could make a copy
     of all the refs of the original insn and turn them into
     anti-refs.  When df_refs_update finds these anti-refs, it annihilates
     the original refs.  If validate_change fails then these anti-refs
     will just get ignored.  */
  */
#endif
}


typedef struct replace_args
{
  rtx match;
  rtx replacement;
  rtx insn;
  int modified;
} replace_args;


/* Replace mem pointed to by PX with its associated pseudo register.
   DATA is actually a pointer to a structure describing the
   instruction currently being scanned and the MEM we are currently
   replacing.  */
static int
df_rtx_mem_replace (px, data)
     rtx *px;
     void *data;
{
  replace_args *args = (replace_args *) data;
  rtx mem = *px;

  if (mem == NULL_RTX)
    return 0;

  switch (GET_CODE (mem))
    {
    case MEM:
      break;

    case CONST_DOUBLE:
      /* We're not interested in the MEM associated with a
	 CONST_DOUBLE, so there's no need to traverse into one.  */
      return -1;

    default:
      /* This is not a MEM.  */
      return 0;
    }

  if (!rtx_equal_p (args->match, mem))
    /* This is not the MEM we are currently replacing.  */
    return 0;

  /* Actually replace the MEM.  */
  validate_change (args->insn, px, args->replacement, 1);
  args->modified++;

  return 0;
}


int
df_insn_mem_replace (df, bb, insn, mem, reg)
     struct df *df;
     basic_block bb;
     rtx insn;
     rtx mem;
     rtx reg;
{
  replace_args args;

  args.insn = insn;
  args.match = mem;
  args.replacement = reg;
  args.modified = 0;

  /* Seach and replace all matching mems within insn.  */
  for_each_rtx (&insn, df_rtx_mem_replace, &args);

  if (args.modified)
    df_insn_modify (df, bb, insn);

  /* ???? FIXME.  We may have a new def or one or more new uses of REG
     in INSN.  REG should be a new pseudo so it won't affect the
     dataflow information that we currently have.  We should add
     the new uses and defs to INSN and then recreate the chains
     when df_analyse is called.  */
  return args.modified;
}


/* Replace one register with another.  Called through for_each_rtx; PX
   points to the rtx being scanned.  DATA is actually a pointer to a
   structure of arguments.  */
static int
df_rtx_reg_replace (px, data)
     rtx *px;
     void *data;
{
  rtx x = *px;
  replace_args *args = (replace_args *) data;

  if (x == NULL_RTX)
    return 0;

  if (x == args->match)
    {
      validate_change (args->insn, px, args->replacement, 1);
      args->modified++;
    }

  return 0;
}


/* Replace the reg within every ref on CHAIN that is within the set
   BLOCKS of basic blocks with NEWREG.  Also update the regs within
   REG_NOTES.  */
void
df_refs_reg_replace (df, blocks, chain, oldreg, newreg)
     struct df *df;
     bitmap blocks;
     struct df_link *chain;
     rtx oldreg;
     rtx newreg;
{
  struct df_link *link;
  replace_args args;

  if (! blocks)
    blocks = df->all_blocks;

  args.match = oldreg;
  args.replacement = newreg;
  args.modified = 0;

  for (link = chain; link; link = link->next)
    {
      struct ref *ref = link->ref;
      rtx insn = DF_REF_INSN (ref);

      if (! INSN_P (insn))
	continue;

      if (bitmap_bit_p (blocks, DF_REF_BBNO (ref)))
	{
	  df_ref_reg_replace (df, ref, oldreg, newreg);

	  /* Replace occurrences of the reg within the REG_NOTES.  */
	  if ((! link->next || DF_REF_INSN (ref)
	      != DF_REF_INSN (link->next->ref))
	      && REG_NOTES (insn))
	    {
	      args.insn = insn;
	      for_each_rtx (&REG_NOTES (insn), df_rtx_reg_replace, &args);
	    }
	}
      else
	{
	  /* Temporary check to ensure that we have a grip on which
	     regs should be replaced.  */
	  abort ();
	}
    }
}


/* Replace all occurrences of register OLDREG with register NEWREG in
   blocks defined by bitmap BLOCKS.  This also replaces occurrences of
   OLDREG in the REG_NOTES but only for insns containing OLDREG.  This
   routine expects the reg-use and reg-def chains to be valid.  */
int
df_reg_replace (df, blocks, oldreg, newreg)
     struct df *df;
     bitmap blocks;
     rtx oldreg;
     rtx newreg;
{
  unsigned int oldregno = REGNO (oldreg);

  df_refs_reg_replace (df, blocks, df->regs[oldregno].defs, oldreg, newreg);
  df_refs_reg_replace (df, blocks, df->regs[oldregno].uses, oldreg, newreg);
  return 1;
}


/* Try replacing the reg within REF with NEWREG.  Do not modify
   def-use/use-def chains.  */
int
df_ref_reg_replace (df, ref, oldreg, newreg)
     struct df *df;
     struct ref *ref;
     rtx oldreg;
     rtx newreg;
{
  /* Check that insn was deleted by being converted into a NOTE.  If
   so ignore this insn.  */
  if (! INSN_P (DF_REF_INSN (ref)))
    return 0;

  if (oldreg && oldreg != DF_REF_REG (ref))
    abort ();

  if (! validate_change (DF_REF_INSN (ref), DF_REF_LOC (ref), newreg, 1))
    return 0;

  df_insn_modify (df, DF_REF_BB (ref), DF_REF_INSN (ref));
  return 1;
}


struct ref*
df_bb_def_use_swap (df, bb, def_insn, use_insn, regno)
     struct df * df;
     basic_block bb;
     rtx def_insn;
     rtx use_insn;
     unsigned int regno;
{
  struct ref *def;
  struct ref *use;
  int def_uid;
  int use_uid;
  struct df_link *link;

  def = df_bb_insn_regno_first_def_find (df, bb, def_insn, regno);
  if (! def)
    return 0;

  use = df_bb_insn_regno_last_use_find (df, bb, use_insn, regno);
  if (! use)
    return 0;

  /* The USE no longer exists.  */
  use_uid = INSN_UID (use_insn);
  df_use_unlink (df, use);
  df_ref_unlink (&df->insns[use_uid].uses, use);

  /* The DEF requires shifting so remove it from DEF_INSN
     and add it to USE_INSN by reusing LINK.  */
  def_uid = INSN_UID (def_insn);
  link = df_ref_unlink (&df->insns[def_uid].defs, def);
  link->ref = def;
  link->next = df->insns[use_uid].defs;
  df->insns[use_uid].defs = link;

#if 0
  link = df_ref_unlink (&df->regs[regno].defs, def);
  link->ref = def;
  link->next = df->regs[regno].defs;
  df->insns[regno].defs = link;
#endif

  DF_REF_INSN (def) = use_insn;
  return def;
}


/* Record df between FIRST_INSN and LAST_INSN inclusive.  All new 
   insns must be processed by this routine.  */
static void
df_insns_modify (df, bb, first_insn, last_insn)
     struct df *df;
     basic_block bb;
     rtx first_insn;
     rtx last_insn;
{
  rtx insn;

  for (insn = first_insn; ; insn = NEXT_INSN (insn))
    {
      unsigned int uid;

      /* A non-const call should not have slipped through the net.  If
	 it does, we need to create a new basic block.  Ouch.  The
	 same applies for a label.  */
      if ((GET_CODE (insn) == CALL_INSN
	   && ! CONST_CALL_P (insn))
	  || GET_CODE (insn) == CODE_LABEL)
	abort ();

      uid = INSN_UID (insn);

      if (uid >= df->insn_size)
	df_insn_table_realloc (df, 0);

      df_insn_modify (df, bb, insn);

      if (insn == last_insn)
	break;
    }
}


/* Emit PATTERN before INSN within BB.  */
rtx
df_pattern_emit_before (df, pattern, bb, insn)
     struct df *df ATTRIBUTE_UNUSED;
     rtx pattern;
     basic_block bb;
     rtx insn;
{
  rtx ret_insn;
  rtx prev_insn = PREV_INSN (insn);

  /* We should not be inserting before the start of the block.  */
  if (insn == bb->head)
    abort ();
  ret_insn = emit_insn_before (pattern, insn);
  if (ret_insn == insn)
    return ret_insn;
  
  df_insns_modify (df, bb, NEXT_INSN (prev_insn), ret_insn);
  return ret_insn;
}


/* Emit PATTERN after INSN within BB.  */
rtx
df_pattern_emit_after (df, pattern, bb, insn)
     struct df *df;
     rtx pattern;
     basic_block bb;
     rtx insn;
{
  rtx ret_insn;

  ret_insn = emit_insn_after (pattern, insn);
  if (ret_insn == insn)
    return ret_insn;

  if (bb->end == insn)
    bb->end = ret_insn;

  df_insns_modify (df, bb, NEXT_INSN (insn), ret_insn);
  return ret_insn;
}


/* Emit jump PATTERN after INSN within BB.  */
rtx
df_jump_pattern_emit_after (df, pattern, bb, insn)
     struct df *df;
     rtx pattern;
     basic_block bb;
     rtx insn;
{
  rtx ret_insn;

  ret_insn = emit_jump_insn_after (pattern, insn);
  if (ret_insn == insn)
    return ret_insn;

  if (bb->end == insn)
    bb->end = ret_insn;

  df_insns_modify (df, bb, NEXT_INSN (insn), ret_insn);
  return ret_insn;
}


/* Move INSN within BB before BEFORE_INSN within BEFORE_BB.

   This function should only be used to move loop invariant insns
   out of a loop where it has been proven that the def-use info
   will still be valid.  */
rtx
df_insn_move_before (df, bb, insn, before_bb, before_insn)
     struct df *df;
     basic_block bb;
     rtx insn;
     basic_block before_bb;
     rtx before_insn;
{
  struct df_link *link;
  unsigned int uid;

  if (! bb)
    return df_pattern_emit_before (df, insn, before_bb, before_insn);

  uid = INSN_UID (insn);

  /* Change bb for all df defined and used by this insn.  */
  for (link = df->insns[uid].defs; link; link = link->next)  
    DF_REF_BB (link->ref) = before_bb;
  for (link = df->insns[uid].uses; link; link = link->next)  
    DF_REF_BB (link->ref) = before_bb;

  /* The lifetimes of the registers used in this insn will be reduced
     while the lifetimes of the registers defined in this insn
     are likely to be increased.  */

  /* ???? Perhaps all the insns moved should be stored on a list
     which df_analyse removes when it recalculates data flow.  */

  return emit_block_insn_before (insn, before_insn, before_bb);
}

/* Functions to query dataflow information.  */


int
df_insn_regno_def_p (df, bb, insn, regno)
     struct df *df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
     unsigned int regno;
{
  unsigned int uid;
  struct df_link *link;

  uid = INSN_UID (insn);

  for (link = df->insns[uid].defs; link; link = link->next)  
    {
      struct ref *def = link->ref;
      
      if (DF_REF_REGNO (def) == regno)
	return 1;
    }

  return 0;
}


static int
df_def_dominates_all_uses_p (df, def)
     struct df *df ATTRIBUTE_UNUSED;
     struct ref *def;
{
  struct df_link *du_link;

  /* Follow def-use chain to find all the uses of this def.  */
  for (du_link = DF_REF_CHAIN (def); du_link; du_link = du_link->next)
    {
      struct ref *use = du_link->ref;
      struct df_link *ud_link;
      
      /* Follow use-def chain to check all the defs for this use.  */
      for (ud_link = DF_REF_CHAIN (use); ud_link; ud_link = ud_link->next)
	if (ud_link->ref != def)
	  return 0;
    }
  return 1;
}


int
df_insn_dominates_all_uses_p (df, bb, insn)
     struct df *df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
{
  unsigned int uid;
  struct df_link *link;

  uid = INSN_UID (insn);

  for (link = df->insns[uid].defs; link; link = link->next)  
    {
      struct ref *def = link->ref;
      
      if (! df_def_dominates_all_uses_p (df, def))
	return 0;
    }

  return 1;
}


/* Return non-zero if all DF dominates all the uses within the bitmap
   BLOCKS.  */
static int
df_def_dominates_uses_p (df, def, blocks)
     struct df *df ATTRIBUTE_UNUSED;
     struct ref *def;
     bitmap blocks;
{
  struct df_link *du_link;

  /* Follow def-use chain to find all the uses of this def.  */
  for (du_link = DF_REF_CHAIN (def); du_link; du_link = du_link->next)
    {
      struct ref *use = du_link->ref;
      struct df_link *ud_link;

      /* Only worry about the uses within BLOCKS.  For example,
      consider a register defined within a loop that is live at the
      loop exits.  */
      if (bitmap_bit_p (blocks, DF_REF_BBNO (use)))
	{
	  /* Follow use-def chain to check all the defs for this use.  */
	  for (ud_link = DF_REF_CHAIN (use); ud_link; ud_link = ud_link->next)
	    if (ud_link->ref != def)
	      return 0;
	}
    }
  return 1;
}


/* Return non-zero if all the defs of INSN within BB dominates
   all the corresponding uses.  */
int
df_insn_dominates_uses_p (df, bb, insn, blocks)
     struct df *df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
     bitmap blocks;
{
  unsigned int uid;
  struct df_link *link;

  uid = INSN_UID (insn);

  for (link = df->insns[uid].defs; link; link = link->next)  
    {
      struct ref *def = link->ref;

      /* Only consider the defs within BLOCKS.  */
      if (bitmap_bit_p (blocks, DF_REF_BBNO (def))
	  && ! df_def_dominates_uses_p (df, def, blocks))
	return 0;
    }
  return 1;
}


/* Return the basic block that REG referenced in or NULL if referenced
   in multiple basic blocks.  */
basic_block
df_regno_bb (df, regno)
     struct df *df;
     unsigned int regno;
{
  struct df_link *defs = df->regs[regno].defs;
  struct df_link *uses = df->regs[regno].uses;
  struct ref *def = defs ? defs->ref : 0;
  struct ref *use = uses ? uses->ref : 0;
  basic_block bb_def = def ? DF_REF_BB (def) : 0;
  basic_block bb_use = use ? DF_REF_BB (use) : 0;

  /* Compare blocks of first def and last use.  ???? FIXME.  What if
     the reg-def and reg-use lists are not correctly ordered.  */
  return bb_def == bb_use ? bb_def : 0;
}


/* Return non-zero if REG used in multiple basic blocks.  */
int
df_reg_global_p (df, reg)
     struct df *df;
     rtx reg;
{
  return df_regno_bb (df, REGNO (reg)) != 0;
}


/* Return total lifetime (in insns) of REG.  */
int
df_reg_lifetime (df, reg)
     struct df *df;
     rtx reg;
{
  return df->regs[REGNO (reg)].lifetime;
}


/* Return non-zero if REG live at start of BB.  */
int
df_bb_reg_live_start_p (df, bb, reg)
     struct df *df ATTRIBUTE_UNUSED;
     basic_block bb;
     rtx reg;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);

#ifdef ENABLE_CHECKING
  if (! bb_info->lr_in)
    abort ();
#endif
  
  return bitmap_bit_p (bb_info->lr_in, REGNO (reg));
}


/* Return non-zero if REG live at end of BB.  */
int
df_bb_reg_live_end_p (df, bb, reg)
     struct df *df ATTRIBUTE_UNUSED;
     basic_block bb;
     rtx reg;
{
  struct bb_info *bb_info = DF_BB_INFO (df, bb);
  
#ifdef ENABLE_CHECKING
  if (! bb_info->lr_in)
    abort ();
#endif

  return bitmap_bit_p (bb_info->lr_out, REGNO (reg));
}


/* Return -1 if life of REG1 before life of REG2, 1 if life of REG1
   after life of REG2, or 0, if the lives overlap.  */
int
df_bb_regs_lives_compare (df, bb, reg1, reg2)
     struct df *df;
     basic_block bb;
     rtx reg1;
     rtx reg2;
{
  unsigned int regno1 = REGNO (reg1);
  unsigned int regno2 = REGNO (reg2);
  struct ref *def1;
  struct ref *use1;
  struct ref *def2;
  struct ref *use2;

 
  /* The regs must be local to BB.  */
  if (df_regno_bb (df, regno1) != bb
      || df_regno_bb (df, regno2) != bb)
    abort ();

  def2 = df_bb_regno_first_def_find (df, bb, regno2);
  use1 = df_bb_regno_last_use_find (df, bb, regno1);

  if (DF_INSN_LUID (df, DF_REF_INSN (def2))
      > DF_INSN_LUID (df, DF_REF_INSN (use1)))
    return -1;

  def1 = df_bb_regno_first_def_find (df, bb, regno1);
  use2 = df_bb_regno_last_use_find (df, bb, regno2);

  if (DF_INSN_LUID (df, DF_REF_INSN (def1))
      > DF_INSN_LUID (df, DF_REF_INSN (use2)))
    return 1;

  return 0;
}


/* Return last use of REGNO within BB.  */
static struct ref *
df_bb_regno_last_use_find (df, bb, regno)
     struct df * df;
     basic_block bb ATTRIBUTE_UNUSED;
     unsigned int regno;
{
  struct df_link *link;

  /* This assumes that the reg-use list is ordered such that for any
     BB, the last use is found first.  However, since the BBs are not
     ordered, the first use in the chain is not necessarily the last
     use in the function.  */
  for (link = df->regs[regno].uses; link; link = link->next)  
    {
      struct ref *use = link->ref;

      if (DF_REF_BB (use) == bb)
	return use;
    }
  return 0;
}


/* Return first def of REGNO within BB.  */
static struct ref *
df_bb_regno_first_def_find (df, bb, regno)
     struct df * df;
     basic_block bb ATTRIBUTE_UNUSED;
     unsigned int regno;
{
  struct df_link *link;

  /* This assumes that the reg-def list is ordered such that for any
     BB, the first def is found first.  However, since the BBs are not
     ordered, the first def in the chain is not necessarily the first
     def in the function.  */
  for (link = df->regs[regno].defs; link; link = link->next)  
    {
      struct ref *def = link->ref;

      if (DF_REF_BB (def) == bb)
	return def;
    }
  return 0;
}


/* Return first use of REGNO inside INSN within BB.  */
static struct ref *
df_bb_insn_regno_last_use_find (df, bb, insn, regno)
     struct df * df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
     unsigned int regno;
{
  unsigned int uid;
  struct df_link *link;

  uid = INSN_UID (insn);

  for (link = df->insns[uid].uses; link; link = link->next)  
    {
      struct ref *use = link->ref;

      if (DF_REF_REGNO (use) == regno)
	return use;
    }

  return 0;
}


/* Return first def of REGNO inside INSN within BB.  */
static struct ref *
df_bb_insn_regno_first_def_find (df, bb, insn, regno)
     struct df * df;
     basic_block bb ATTRIBUTE_UNUSED;
     rtx insn;
     unsigned int regno;
{
  unsigned int uid;
  struct df_link *link;

  uid = INSN_UID (insn);

  for (link = df->insns[uid].defs; link; link = link->next)  
    {
      struct ref *def = link->ref;

      if (DF_REF_REGNO (def) == regno)
	return def;
    }

  return 0;
}


/* Return insn using REG if the BB contains only a single
   use and def of REG.  */
rtx
df_bb_single_def_use_insn_find (df, bb, insn, reg)
     struct df * df;
     basic_block bb;
     rtx insn;
     rtx reg;
{
  struct ref *def;
  struct ref *use;
  struct df_link *du_link;

  def = df_bb_insn_regno_first_def_find (df, bb, insn, REGNO (reg));

  if (! def)
    abort ();

  du_link = DF_REF_CHAIN (def);

  if (! du_link)
    return NULL_RTX;

  use = du_link->ref;

  /* Check if def is dead.  */
  if (! use)
    return NULL_RTX;

  /* Check for multiple uses.  */
  if (du_link->next)
    return NULL_RTX;
  
  return DF_REF_INSN (use);
}

/* Functions for debugging/dumping dataflow information.  */


/* Dump a def-use or use-def chain for REF to FILE.  */
static void
df_chain_dump (link, file)
     struct df_link *link;
     FILE *file;
{
  fprintf (file, "{ ");
  for (; link; link = link->next)
    {
      fprintf (file, "%c%d ",
	       DF_REF_REG_DEF_P (link->ref) ? 'd' : 'u',
	       DF_REF_ID (link->ref));
    }
  fprintf (file, "}");
}

static void
df_chain_dump_regno (link, file)
     struct df_link *link;
     FILE *file;
{
  fprintf (file, "{ ");
  for (; link; link = link->next)
    {
      fprintf (file, "%c%d(%d) ",
               DF_REF_REG_DEF_P (link->ref) ? 'd' : 'u',
               DF_REF_ID (link->ref),
               DF_REF_REGNO (link->ref));
    }
  fprintf (file, "}");
}

/* Dump dataflow info.  */
void
df_dump (df, flags, file)
     struct df *df;
     int flags;
     FILE *file;
{
  unsigned int i;
  unsigned int j;

  if (! df || ! file)
    return;

  fprintf (file, "\nDataflow summary:\n");
  fprintf (file, "n_regs = %d, n_defs = %d, n_uses = %d, n_bbs = %d\n",
	   df->n_regs, df->n_defs, df->n_uses, df->n_bbs);

  if (flags & DF_RD)
    {
      fprintf (file, "Reaching defs:\n");
      for (i = 0; i < df->n_bbs; i++)
	{
	  basic_block bb = BASIC_BLOCK (i);  
	  struct bb_info *bb_info = DF_BB_INFO (df, bb);      
	  
	  if (! bb_info->rd_in)
	    continue;

	  fprintf (file, "bb %d in  \t", i);
	  dump_bitmap (file, bb_info->rd_in);
	  fprintf (file, "bb %d gen \t", i);
	  dump_bitmap (file, bb_info->rd_gen);
	  fprintf (file, "bb %d kill\t", i);
	  dump_bitmap (file, bb_info->rd_kill);
	  fprintf (file, "bb %d out \t", i);
	  dump_bitmap (file, bb_info->rd_out);
	}
    }

  if (flags & DF_UD_CHAIN)
    {
      fprintf (file, "Use-def chains:\n");
      for (j = 0; j < df->n_defs; j++)
	{
	  if (df->defs[j])
	    {
	      fprintf (file, "d%d bb %d luid %d insn %d reg %d ",
		       j, DF_REF_BBNO (df->defs[j]),
		       DF_INSN_LUID (df, DF_REF_INSN (df->defs[j])),
		       DF_REF_INSN_UID (df->defs[j]),
		       DF_REF_REGNO (df->defs[j]));
	      df_chain_dump (DF_REF_CHAIN (df->defs[j]), file);
	      fprintf (file, "\n");
	    }
	}
    }

  if (flags & DF_RU)
    {
      fprintf (file, "Reaching uses:\n");
      for (i = 0; i < df->n_bbs; i++)
	{
	  basic_block bb = BASIC_BLOCK (i);  
	  struct bb_info *bb_info = DF_BB_INFO (df, bb);      
	  
	  if (! bb_info->ru_in)
	    continue;

	  fprintf (file, "bb %d in  \t", i);
	  dump_bitmap (file, bb_info->ru_in);
	  fprintf (file, "bb %d gen \t", i);
	  dump_bitmap (file, bb_info->ru_gen);
	  fprintf (file, "bb %d kill\t", i);
	  dump_bitmap (file, bb_info->ru_kill);
	  fprintf (file, "bb %d out \t", i);
	  dump_bitmap (file, bb_info->ru_out);
	}
    }

  if (flags & DF_DU_CHAIN)
    {
      fprintf (file, "Def-use chains:\n");
      for (j = 0; j < df->n_uses; j++)
	{
	  if (df->uses[j])
	    {
	      fprintf (file, "u%d bb %d luid %d insn %d reg %d ",
		       j, DF_REF_BBNO (df->uses[j]),
		       DF_INSN_LUID (df, DF_REF_INSN (df->uses[j])),
		       DF_REF_INSN_UID (df->uses[j]),
		       DF_REF_REGNO (df->uses[j]));
	      df_chain_dump (DF_REF_CHAIN (df->uses[j]), file);
	      fprintf (file, "\n");
	    }
	}
    }

  if (flags & DF_LR)
    {
      fprintf (file, "Live regs:\n");
      for (i = 0; i < df->n_bbs; i++)
	{
	  basic_block bb = BASIC_BLOCK (i);  
	  struct bb_info *bb_info = DF_BB_INFO (df, bb);      
	  
	  if (! bb_info->lr_in)
	    continue;

	  fprintf (file, "bb %d in  \t", i);
	  dump_bitmap (file, bb_info->lr_in);
	  fprintf (file, "bb %d use \t", i);
	  dump_bitmap (file, bb_info->lr_use);
	  fprintf (file, "bb %d def \t", i);
	  dump_bitmap (file, bb_info->lr_def);
	  fprintf (file, "bb %d out \t", i);
	  dump_bitmap (file, bb_info->lr_out);
	}
    }

  if (flags & (DF_REG_INFO | DF_RD_CHAIN | DF_RU_CHAIN))
    {
      struct reg_info *reg_info = df->regs;

      fprintf (file, "Register info:\n");
      for (j = 0; j < df->n_regs; j++)
	{
	  if (((flags & DF_REG_INFO) 
	       && (reg_info[j].n_uses || reg_info[j].n_defs))
	      || ((flags & DF_RD_CHAIN) && reg_info[j].defs)
	      || ((flags & DF_RU_CHAIN) && reg_info[j].uses))
	  {
	    fprintf (file, "reg %d", j);
	    if ((flags & DF_RD_CHAIN) && (flags & DF_RU_CHAIN))
	      {
		basic_block bb = df_regno_bb (df, j);
		
		if (bb)
		  fprintf (file, " bb %d", bb->index);
		else
		  fprintf (file, " bb ?");
	      }
	    if (flags & DF_REG_INFO)
	      {
		fprintf (file, " life %d", reg_info[j].lifetime);
	      }

	    if ((flags & DF_REG_INFO) || (flags & DF_RD_CHAIN))
	      {
		fprintf (file, " defs ");
		if (flags & DF_REG_INFO)
		  fprintf (file, "%d ", reg_info[j].n_defs);
		if (flags & DF_RD_CHAIN)
		  df_chain_dump (reg_info[j].defs, file);
	      }

	    if ((flags & DF_REG_INFO) || (flags & DF_RU_CHAIN))
	      {
		fprintf (file, " uses ");
		if (flags & DF_REG_INFO)
		  fprintf (file, "%d ", reg_info[j].n_uses);
		if (flags & DF_RU_CHAIN)
		  df_chain_dump (reg_info[j].uses, file);
	      }

	    fprintf (file, "\n");
	  }
	}
    }
  fprintf (file, "\n");
}


void
df_insn_debug (df, insn, file)
     struct df *df;
     rtx insn;
     FILE *file;
{
  unsigned int uid;
  int bbi;

  uid = INSN_UID (insn);
  if (uid >= df->insn_size)
    return;

  if (df->insns[uid].defs)
    bbi = DF_REF_BBNO (df->insns[uid].defs->ref);
  else  if (df->insns[uid].uses)
    bbi = DF_REF_BBNO (df->insns[uid].uses->ref);
  else
    bbi = -1;

  fprintf (file, "insn %d bb %d luid %d defs ",
	   uid, bbi, DF_INSN_LUID (df, insn));
  df_chain_dump (df->insns[uid].defs, file);
  fprintf (file, " uses ");
  df_chain_dump (df->insns[uid].uses, file);
  fprintf (file, "\n");
}

void
df_insn_debug_regno (df, insn, file)
     struct df *df;
     rtx insn;
     FILE *file;
{
  unsigned int uid;
  int bbi;

  uid = INSN_UID (insn);
  if (uid >= df->insn_size)
    return;

  if (df->insns[uid].defs)
    bbi = DF_REF_BBNO (df->insns[uid].defs->ref);
  else  if (df->insns[uid].uses)
    bbi = DF_REF_BBNO (df->insns[uid].uses->ref);
  else
    bbi = -1;

  fprintf (file, "insn %d bb %d luid %d defs ",
           uid, bbi, DF_INSN_LUID (df, insn));
  df_chain_dump_regno (df->insns[uid].defs, file);
  fprintf (file, " uses ");
  df_chain_dump_regno (df->insns[uid].uses, file);
  fprintf (file, "\n");
}

static void
df_regno_debug (df, regno, file)
     struct df *df;
     unsigned int regno;
     FILE *file;
{
  if (regno >= df->reg_size)
    return;

  fprintf (file, "reg %d life %d defs ",
	   regno, df->regs[regno].lifetime);
  df_chain_dump (df->regs[regno].defs, file);
  fprintf (file, " uses ");
  df_chain_dump (df->regs[regno].uses, file);
  fprintf (file, "\n");
}


static void
df_ref_debug (df, ref, file)
     struct df *df;
     struct ref *ref; 
     FILE *file;
{
  fprintf (file, "%c%d ",
	   DF_REF_REG_DEF_P (ref) ? 'd' : 'u',
	   DF_REF_ID (ref));
  fprintf (file, "reg %d bb %d luid %d insn %d chain ", 
	   DF_REF_REGNO (ref),
	   DF_REF_BBNO (ref), 
	   DF_INSN_LUID (df, DF_REF_INSN (ref)),
	   INSN_UID (DF_REF_INSN (ref)));
  df_chain_dump (DF_REF_CHAIN (ref), file);
  fprintf (file, "\n");
}


void 
debug_df_insn (insn)
     rtx insn;
{
  df_insn_debug (ddf, insn, stderr);
  debug_rtx (insn);
}


void 
debug_df_reg (reg)
     rtx reg;
{
  df_regno_debug (ddf, REGNO (reg), stderr);
}


void 
debug_df_regno (regno)
     unsigned int regno;
{
  df_regno_debug (ddf, regno, stderr);
}


void 
debug_df_ref (ref)
     struct ref *ref;
{
  df_ref_debug (ddf, ref, stderr);
}


void 
debug_df_defno (defno)
     unsigned int defno;
{
  df_ref_debug (ddf, ddf->defs[defno], stderr);
}


void 
debug_df_useno (defno)
     unsigned int defno;
{
  df_ref_debug (ddf, ddf->uses[defno], stderr);
}


void 
debug_df_chain (link)
     struct df_link *link;
{
  df_chain_dump (link, stderr);
  fputc ('\n', stderr);
}
