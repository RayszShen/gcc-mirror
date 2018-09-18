/* Subroutines used to transform array subscripting expressions into
   forms that are more amenable to d-form instruction selection for p9
   little-endian VSX code.
   Copyright (C) 1991-2018 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "rtl.h"
#include "tree.h"
#include "memmodel.h"
#include "df.h"
#include "tm_p.h"
#include "ira.h"
#include "print-tree.h"
#include "varasm.h"
#include "explow.h"
#include "expr.h"
#include "output.h"
#include "tree-pass.h"
#include "rtx-vector-builder.h"

#include "cfgloop.h"
#include "print-rtl.h"
#include "tree-pretty-print.h"

#include "genrtl.h"

/* This pass transforms array indexing expressions from a form that
   favors selection of X-form instructions into a form that favors
   selection of D-form instructions.

   Showing favor for D-form instructions is especially important when
   targeting Power9, as the Power9 architecture added a number of new
   D-form instruction capabilities.  */

/* This is based on the union-find logic in web.c.  web_entry_base is
   defined in df.h.  */
class indexing_web_entry : public web_entry_base
{
 public:
  rtx_insn *insn;		/* Pointer to the insn */
  basic_block bb;		/* Pointer to the enclosing basic block */
};

/* Print on FILE the indexes for the predecessors of basic_block BB.  */

#ifdef REMOVE_ME
static void
print_pred_bbs (FILE *file, basic_block bb)
{
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->preds)
    fprintf (file, "bb_%d ", e->src->index);
}


/* Print on FILE the indexes for the successors of basic_block BB.  */

static void
print_succ_bbs (FILE *file, basic_block bb)
{
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->succs)
    fprintf (file, "bb_%d ", e->dest->index);
}
#endif

struct equivalence_member {
  struct equivalence_member *next;
  unsigned int uid;
  const char *index_defs;
  const char *base_defs;
  unsigned int index_originator;
  long long int index_delta;
  unsigned int base_originator;
  long long int base_delta;
};

/* Can't use malloc, so preallocate all of these.  */
#define MaxMembers 64
struct equivalence_member malloc_replacement[MaxMembers];
static int next_equivalence = 0;


/* fix me: remove dependency on maximum number of equivalence classes.  */
#define MaxEquivalences 32

static struct equivalence_member *equivalence_classes[MaxEquivalences];
static int num_equivalences = 0;

static void fatal (const char *msg) {

  fprintf (stderr, "Fatal error: %s\n", msg);
  exit (-1);
}

#define MAX_DEFS_SPACE 4096
static char defs_buf[MAX_DEFS_SPACE];
static char *next_buf_space = defs_buf;

static int count_links (struct df_link *def_link) {
  if (def_link == NULL) return 0;
  else return 1 + count_links (def_link->next);
}

static const char *empty_string = "";

#define CONSTANT_DEFINITION empty_string

static const char *
preserve (const char *defs) {
  char *cp;

  for (cp = defs_buf; cp < next_buf_space; cp += strlen (cp) + 1) {
    if (!strcmp (cp, defs))
      return cp;
  }
  next_buf_space += strlen (defs) + 1;
  if (next_buf_space > &defs_buf[MAX_DEFS_SPACE])
    fatal ("string buffer space overflowed");
  strcpy (cp, defs);
  return cp;
}

static char *place_int (char *buf, unsigned int val) {
  char tbuf [32];
  int last_digit = 31;
  tbuf[last_digit--] = '\0';

  if (val == 0)
    tbuf [last_digit] = '0';
  else {
    while (val) {
      tbuf[last_digit--] = '0' + (val % 10);
      val /= 10;
    }
    last_digit++;
  }
  strcpy (buf, tbuf + last_digit);
  return buf + strlen (tbuf + last_digit);
}

static const char *
help_canonize (int count, struct df_link *def_link) {
  int ids[16];
  int i = 0;

  /* FIXME: KELVIN TO REPLACE ARBITRARY RESTRICTION OF 16.  */
  if (count > 16)
    fatal ("too many defs in help_canonize");

  while (def_link != NULL) {
    ids[i++] = DF_REF_ID (def_link->ref);
    def_link = def_link->next;
  }

  /* bubble sort to put ids in ascending order. */
  for (int end = count - 1; end > 0; end--) {
    for (int j = 0; j < end; j++) {
      if (ids[j] > ids[j+1]) {
	int swap = ids[j];
	ids[j] = ids[j+1];
	ids[j+1] = swap;
      }
    }  /* ids[end] has largest entry in ids[0..end]  */
  }

  char buf[1024];
  char *cp = buf;
  for (int i = 0; i < count; i++) {
    *cp = '\0';			/* not needed: helps debug.  */
    cp = place_int (cp, ids[i]);
    *cp++ = ':';
  }
  cp--;
  *cp = '\0';

  return preserve (buf);
}

static const char *
canonize (struct df_link *def_link) {

  /* prototyping: don't worry about efficiency for the moment.  */
  int count = count_links (def_link);
  return help_canonize (count, def_link);
}


static void
insert_into_equivalence_class (unsigned int uid,
			       const char *index_defs, const char *base_defs,
			       unsigned int index_originator,
			       long long int index_delta,
			       unsigned int base_originator,
			       long long int base_delta)
{
  struct equivalence_member *new_instance;

  new_instance = &malloc_replacement[next_equivalence++];
  if (next_equivalence > MaxMembers)
    fatal ("too many equivalence instances");

  new_instance->uid = uid;
  new_instance->index_defs = index_defs;
  new_instance->base_defs = base_defs;
  new_instance->index_originator = index_originator;
  new_instance->index_delta = index_delta;
  new_instance->base_originator = base_originator;
  new_instance->base_delta = base_delta;

  /* See if this matches an existing equivalence class.  */
  for (int i = 0; i < num_equivalences; i++) {
    struct equivalence_member *em = equivalence_classes[i];
    if ((em->index_defs == index_defs) && (em->base_defs == base_defs)) {
      new_instance->next = em;
      equivalence_classes[i] = new_instance;
      return;
    }
  }

  new_instance->next = NULL;
  equivalence_classes[num_equivalences++] = new_instance;
  if (num_equivalences >= MaxEquivalences)
    fatal ("too many equivalence classes");
}

static void
fixup_equivalences (indexing_web_entry *insn_entry) {

  /* FIXME: USE ALLOCA */
#define MY_LARGEST_EQUIVALENCE_CLASS	32
  unsigned int num_elements = 0;
  unsigned int insn_numbers [MY_LARGEST_EQUIVALENCE_CLASS];

  for (int i = 0; i < num_equivalences; i++)
    {
      struct equivalence_member *em = equivalence_classes[i];
      struct equivalence_member *em2 = em;

      for (num_elements = 0; em != NULL; em = em->next) {
	insn_numbers [num_elements++] = em->uid;
	if (num_elements >= MY_LARGEST_EQUIVALENCE_CLASS)
	  fatal ("too many insns in equivalence class");
      }

      /* No fixup desired if there's only one element in equivalence class.  */
      if (num_elements <= 1) continue;

      /* Within the equivalence class, we must find the insn that
	 dominates all others.  Any insn that is not in a domination
	 relationship with this insn must be removed from the class.  */
      unsigned int the_dominator = insn_numbers [0];

      for (unsigned int j = 1; j < num_elements; j++) {
	if (dominated_by_p (CDI_DOMINATORS,
			    insn_entry [the_dominator].bb,
			    insn_entry [insn_numbers [j]].bb)) {
	  the_dominator = insn_numbers[j];
	}
      }

      /* the_dominator dominates all but those that it is not "comparable"
	 to.  Remove non-comparables.  */

      /* FIXME: USE ALLOCA */
      unsigned int excluded [MY_LARGEST_EQUIVALENCE_CLASS];
      int num_excluded = 0;

      for (unsigned int j = 0; j < num_elements - 1; j++)
	if (!dominated_by_p (CDI_DOMINATORS,
			     insn_entry [insn_numbers [j]].bb,
			     insn_entry [the_dominator].bb))
	  excluded [num_excluded++] = insn_numbers [j];

      /* So many exclusions that there's nothing left to optimized.  */
      if (num_elements - num_excluded <= 1) continue;

      long long int dominator_delta = 0;
      rtx derived_ptr_reg = gen_reg_rtx (Pmode);

      /* first, output the dominator insn.  */
      int originating_insn = 0;
      for (em = em2; em != NULL; em = em->next)
	{
	  if (em->uid == the_dominator) {

	    dominator_delta = em->index_delta + em->base_delta;

	    /* Code works for both 32-bit and 64-bit targets.  */
	    rtx_insn *insn = insn_entry [em->uid].insn;
	    rtx body = PATTERN (insn);
	    rtx base_reg, index_reg;
	    rtx addr, mem;
	    rtx new_init_expr;
	    rtx new_delta_expr = NULL;
	    bool is_load;

	    originating_insn = INSN_UID (insn);
	    if (dump_file) {
	      fprintf (dump_file, "Replacing originating insn %d: ",
		       originating_insn);
	      print_inline_rtx (dump_file, insn, 2);
	      fprintf (dump_file, "\n");
	    }

	    gcc_assert (GET_CODE (body) == SET);
	    if (GET_CODE (SET_SRC (body)) == MEM)
	      {
		/* originating instruction is a load */
		mem = SET_SRC (body);
		addr = XEXP (SET_SRC (body), 0);
		is_load = true;
	      }
	    else
	      { /* originating instruction is a store */
		gcc_assert (GET_CODE (SET_DEST (body)) == MEM);
		mem = SET_DEST (body);
		addr = XEXP (SET_DEST (body), 0);
		is_load = false;
	      }

	    enum rtx_code code = GET_CODE (addr);
	    gcc_assert ((code == PLUS) || (code == MINUS));
	    base_reg = XEXP (addr, 0);
	    index_reg = XEXP (addr, 1);

	    if (code == PLUS)
	      new_init_expr = gen_rtx_PLUS (Pmode, base_reg, index_reg);
	    else
	      new_init_expr = gen_rtx_MINUS (Pmode, base_reg, index_reg);
	    new_init_expr = gen_rtx_SET (derived_ptr_reg, new_init_expr);

	    rtx_insn *new_insn = emit_insn_before (new_init_expr, insn);
	    set_block_for_insn (new_insn, BLOCK_FOR_INSN (insn));
	    INSN_CODE (new_insn) = -1; /* force re-recogniition. */
	    df_insn_rescan (new_insn);

	    if (dump_file) {
	      fprintf (dump_file, "with insn %d: ", INSN_UID (new_insn));
	      print_inline_rtx (dump_file, new_insn, 2);
	      fprintf (dump_file, "\n");
	    }

	    if (dominator_delta)
	      {
		rtx ci = gen_rtx_raw_CONST_INT (Pmode, dominator_delta);
		new_delta_expr = gen_rtx_PLUS (Pmode, derived_ptr_reg, ci);
		new_delta_expr = gen_rtx_SET (derived_ptr_reg, new_delta_expr);
		new_insn = emit_insn_before (new_delta_expr, insn);
		set_block_for_insn (new_insn, BLOCK_FOR_INSN (insn));
		INSN_CODE (new_insn) = -1; /* force re-recognition.  */
		df_insn_rescan (new_insn);

		if (dump_file) {
		  fprintf (dump_file, "and with insn %d: ",
			   INSN_UID (new_insn));
		  print_inline_rtx (dump_file, new_insn, 2);
		  fprintf (dump_file, "\n");
		}
	      }

	    rtx new_mem = gen_rtx_MEM (GET_MODE (mem), derived_ptr_reg);
	    MEM_COPY_ATTRIBUTES (new_mem, mem);

	    rtx new_expr;
	    if (is_load)
	      new_expr = gen_rtx_SET (SET_DEST (body), new_mem);
	    else
	      new_expr = gen_rtx_SET (new_mem, SET_SRC (body));

	    new_insn = emit_insn_before (new_expr, insn);
	    set_block_for_insn (new_insn, BLOCK_FOR_INSN (insn));
	    INSN_CODE (new_insn) = -1;
	    df_insn_rescan (new_insn);

	    if (dump_file) {
	      fprintf (dump_file, "and with insn %d: ", INSN_UID (new_insn));
	      print_inline_rtx (dump_file, new_insn, 2);
	      fprintf (dump_file, "\n");
	    }

	    df_insn_delete (insn);
	    remove_insn (insn);
	    insn->set_deleted ();
	  }
	}

      /* next, output the other insns.  */
      for (em = em2; em != NULL; em = em->next)
	{
	  if (em->uid != the_dominator) {

	    bool dont_replace = false;
	    for (int j = 0; j < num_excluded; j++) {
	      if (excluded [j] == em->uid)
		dont_replace = true;
	    }

	    if (dont_replace) {
	      fprintf (dump_file,
		       "Not replacing insn %d from equivalence class\n",
		       em->uid);
	      fprintf (dump_file, " as it is not dominated by originating insn %d\n",
		       originating_insn);
	    } else {

	      long long int dominated_delta = em->index_delta + em->base_delta;
	      dominated_delta -= dominator_delta;

	      rtx_insn *insn = insn_entry [em->uid].insn;
	      rtx body = PATTERN (insn);
	      rtx mem;
	      bool is_load;

	      if (dump_file) {
		fprintf (dump_file, "Replacing propagating insn %d: ",
			 INSN_UID (insn));
		print_inline_rtx (dump_file, insn, 2);
		fprintf (dump_file, "\n");
	      }

	      gcc_assert (GET_CODE (body) == SET);

	      if (GET_CODE (SET_SRC (body)) == MEM)
		{
		  /* propagating instruction is a load */
		  mem = SET_SRC (body);
		  is_load = true;
		}
	      else
		{ /* propagating instruction is a store */
		  gcc_assert (GET_CODE (SET_DEST (body)) == MEM);
		  mem = SET_DEST (body);
		  is_load = false;
		}

	      rtx ci = gen_rtx_raw_CONST_INT (Pmode, dominated_delta);
	      rtx addr_expr = gen_rtx_PLUS (Pmode, derived_ptr_reg, ci);
	      rtx new_mem = gen_rtx_MEM (GET_MODE (mem), addr_expr);
	      MEM_COPY_ATTRIBUTES (new_mem, mem);

	      rtx new_expr;
	      if (is_load)
		new_expr = gen_rtx_SET (SET_DEST (body), new_mem);
	      else
		new_expr = gen_rtx_SET (new_mem, SET_SRC (body));

	      rtx_insn *new_insn = emit_insn_before (new_expr, insn);
	      set_block_for_insn (new_insn, BLOCK_FOR_INSN (insn));
	      INSN_CODE (new_insn) = -1;
	      df_insn_rescan (new_insn);

	      if (dump_file) {
		fprintf (dump_file, "with insn %d: ", INSN_UID (new_insn));
		print_inline_rtx (dump_file, new_insn, 2);
		fprintf (dump_file, "\n");
	      }

	      df_insn_delete (insn);
	      remove_insn (insn);
	      insn->set_deleted ();
	    }
	  }
	}
    }
}

#ifdef TO_REMOVE
static void
dump_equivalences (indexing_web_entry *insn_entry) {

  if (dump_file)
    {
#define LARGEST_EQUIVALENCE_CLASS	32
      unsigned int num_elements = 0;
      unsigned int insn_numbers [LARGEST_EQUIVALENCE_CLASS];

      for (int i = 0; i < num_equivalences; i++)
	{
	  struct equivalence_member *em = equivalence_classes[i];
	  struct equivalence_member *em2 = em;

	  fprintf (dump_file, "Equivalence class %d:\n", i);
	  fprintf (dump_file, "  index_defs: %s, base_defs: %s\n",
		   em->index_defs, em->base_defs);
	  for (num_elements = 0; em != NULL; em = em->next) {
	    insn_numbers [num_elements++] = em->uid;
	    if (num_elements >= LARGEST_EQUIVALENCE_CLASS)
	      fatal ("too many insns in equivalence class");
	  }

	  /* Within the equivalence class, we must find the insn that
	     dominates all the others.  Any insn that is not in a
	     domination relationship with this insn must be excluded
	     from the class.  */
	  /* Bubble the dominating insn to the end of the array.  */

	  unsigned int the_dominator = insn_numbers [0];


	  for (unsigned int j = 1; j < num_elements; j++) {

	    /*
	    fprintf (dump_file, "checking domination of %d vs %d\n",
		     the_dominator, insn_numbers[j]);
	    */
	    if (dominated_by_p (CDI_DOMINATORS,
				insn_entry [the_dominator].bb,
				insn_entry [insn_numbers [j]].bb)) {
	      the_dominator = insn_numbers[j];
	      /*
	      fprintf (dump_file, "they are out of order, so updating\n");
	      */
	    }
	    /*
	    else
	      fprintf (dump_file, "they are in order, so not updating\n");
	    */
	  }
	  /* insn_numbers [num_elements - 1] holds candidate class dominator,
	     but I haven't compared with every other element, and
	     some may not be comparable.  If any elements are not
	     comparable, remove them from the equivalence class.  */
	  for (unsigned int j = 0; j < num_elements - 1; j++)
	    if (!dominated_by_p (CDI_DOMINATORS,
				 insn_entry [insn_numbers [j]].bb,
				 insn_entry [the_dominator].bb))
	      {
		fprintf (dump_file,
			 " removing insn %d from equivalence class\n",
			 insn_numbers [j]);
	      }

	  fprintf (dump_file, "  The insn that dominates all others is %d\n",
		   the_dominator);

	  long long int dominator_delta = 0;

	  /* first, output the dominator insn.  */
	  for (em = em2; em != NULL; em = em->next)
	    {
	      if (em->uid == the_dominator) {

		fprintf (dump_file,
			 "  insn: %u, index: %d, index_delta: %lld, "
			 "base: %d, base_delta: %lld\n",
			 em->uid, em->index_originator, em->index_delta,
			 em->base_originator, em->base_delta);

		fprintf (dump_file, "Replace this insn with:\n");
		fprintf (dump_file, " derived_ptr = r%d (base) + r%d (index);\n",
			 em->base_originator, em->index_originator);
		dominator_delta = em->index_delta + em->base_delta;

		if (dominator_delta)
		  fprintf (dump_file, " derived_ptr += %lld;\n",
			   dominator_delta);
		fprintf (dump_file, "Mem [derived_ptr]\n");
	      }
	    }

	  /* next, output the other insns.  */
	  for (em = em2; em != NULL; em = em->next)
	    {
	      if (em->uid != the_dominator) {
		fprintf (dump_file,
			 "  insn: %u, index: %d, index_delta: %lld, "
			 "base: %d, base_delta: %lld\n",
			 em->uid, em->index_originator, em->index_delta,
			 em->base_originator, em->base_delta);

		fprintf (dump_file, "Replace this insn with:\n");
		fprintf (dump_file, "Mem [derived_ptr + %lld];\n",
			 em->base_delta + em->index_delta - dominator_delta);
	      }
	    }
	}
    }
}
#endif

/* Return non-zero if an only if use represents a compile-time constant.  */
static int
represents_constant_p (df_ref use)
{
  struct df_link *def_link = DF_REF_CHAIN (use);

  /* If there is no definition, or the definition is
     artificial, or there are multiple definitions, this
     is an originating use.  */
  if (!def_link || !def_link->ref
      || DF_REF_IS_ARTIFICIAL (def_link->ref) || def_link->next)
    return false;
  else
    {
      rtx def_insn = DF_REF_INSN (def_link->ref);
      /* unsigned uid = INSN_UID (def_insn); not needed? */
      rtx body = PATTERN (def_insn);
      if (GET_CODE (body) == CONST_INT)
	return true;
      else if (GET_CODE (body) == SET)
	{
	  /* recurse on the use that defines this variable */
	  struct df_insn_info *inner_insn_info = DF_INSN_INFO_GET (def_insn);
	  df_ref inner_use;
	  FOR_EACH_INSN_INFO_USE (inner_use, inner_insn_info)
	    {
	      if (!represents_constant_p (inner_use))
		return false;
	    }
	  /* There were multiple defs but they are all constant.  */
	  return true;
	}
      else			/* treat unrecognized codes as not constant */
	return false;
    }
}

/* An originator represents the first point at which my value is
   derived from potentially more than one input definition, or the
   point at which my value is defined by an algebraic expression
   involving only constants,

   If my value depends on a constant combined with a single variable
   or a simple propagation of a single variable, I continue my search
   for the originator by examining the origin of the source variable's
   value.

   The value of *ADJUSTMENT is overwritten with the constant value that is
   added to the originator expression to obtain the value intended to
   be represented by DEF_LINK.  In the case that find_true_originator
   returns -1, the value held in *ADJUSTMENT is undefined.

   Returns -1 if there is no true originator.  In general, the search
   for an originator expression only spans SET operations that are
   based on simple algebraic expressions.  */
static int
find_true_originator (struct df_link *def_link, long long int *adjustment)
{
  rtx def_insn = DF_REF_INSN (def_link->ref);
  unsigned uid2 = INSN_UID (def_insn);

  if (dump_file && (dump_flags & TDF_DETAILS)) {
    fprintf (dump_file,
	     " find_true_originator looking at insn %d\n",
	     uid2);
    print_inline_rtx (dump_file, def_insn, 2);
    fprintf (dump_file, "\n");
  }

  rtx inner_body = PATTERN (def_insn);
  if (GET_CODE (inner_body) == SET)
    {
      struct df_insn_info *inner_insn_info = DF_INSN_INFO_GET (def_insn);
      df_ref inner_use;

      /* We're only happy with multiple uses if all but one represent
	 constant values.  */
      int non_constant_uses = 0;
      int result = 0;
      FOR_EACH_INSN_INFO_USE (inner_use, inner_insn_info)
	{
	  if (!represents_constant_p (inner_use))
	    {
	      non_constant_uses++;
	      /* There should be only one non-constant use, and it should
		 satisfy find_true_originator.  */
	      struct df_link *def_link = DF_REF_CHAIN (inner_use);

	      /* If there is no definition, or the definition is
		 artificial, or there are multiple definitions, this
		 is an originating use.  */
	      if (!def_link || !def_link->ref
		  || DF_REF_IS_ARTIFICIAL (def_link->ref) || def_link->next) {
		if (dump_file && (dump_flags & TDF_DETAILS))
		  fprintf (dump_file, "Treat this as an originating use\n");
		result = uid2;
	      }
	      else {
		if (dump_file && (dump_flags & TDF_DETAILS))
		  fprintf (dump_file, "Recursing on the single used def\n");
		result = find_true_originator (def_link, adjustment);
	      }
	    }
	}

      if (non_constant_uses == 1) {

	/* If my SET looks like PLUS or MINUS CONST_INT, or if it's a
	   simple register copy, this is what I want.  Anything else is
	   a problem that I'm not ready to deal with.  Maybe I can
	   prepare myself to deal with other scenarios that are seen
	   to exist in common practice.  */

	rtx source_expr = SET_SRC (inner_body);
	int source_code = GET_CODE (source_expr);
	if (source_code == PLUS)
	  {
	    rtx op1 = XEXP (source_expr, 0);
	    rtx op2 = XEXP (source_expr, 1);

	    if ((GET_CODE (op1) == CONST_INT) && (GET_CODE (op2) != CONST_INT))
	      *adjustment += INTVAL (op1);
	    else if ((GET_CODE (op1) != CONST_INT)
		     && (GET_CODE (op2) == CONST_INT))
	      *adjustment += INTVAL (op2);
	    else if (dump_file && (dump_flags & TDF_DETAILS))
	      fprintf (dump_file,
		       " punting, PLUS arguments don't fit expectations\n");
	  }
	else if (source_code == MINUS)
	  {
	    rtx op1 = XEXP (source_expr, 0);
	    rtx op2 = XEXP (source_expr, 1);

	    if ((GET_CODE (op1) != CONST_INT) && (GET_CODE (op2) == CONST_INT))
	      *adjustment -= INTVAL (op1);
	    else
	      {
		fprintf (dump_file,
			 " punting, MINUS arguments don't fit expectations\n");
	      }
	  }
	else if (source_code != REG)
	  {
	    /* We don't handle ashift, for example.  */
	    if (dump_file && (dump_flags & TDF_DETAILS))
	      fprintf (dump_file,
		       " punt, SET subexpresion is unhappy with code %s\n",
		       GET_RTX_NAME(source_code));
	    return uid2;
	  }
	/* else, this is a register copy expression: no impact on
	   adjustment.  */
	return result;
      }
      else if (non_constant_uses > 1) /* Too many non-constant inputs */
	return uid2;
      else			/* All definition uses are constant. */
	return uid2;
    }
  else
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file,
		 " punt, because this is not a SET insn\n");
      return -1;
    }
}



/* Main entry point for this pass.

   This pass runs after loops have been unrolled.  The pass searches
   for sequences of code of the form:

   A0: *(array_base + offset)

   Aij:			(optional, for j = 1 .. Ki, with different Ki
                         for each value of i.  If Ki equals 0, there
                         are no constant adjustments to offset for
                         this value of i)
      offset += constant (i, j)

   Ai:                    (for i = 1 .. N)
      *(array_base + offset)

   where for each i = 1 to N and j = 1 to K-1
     A(i-1) dominates A(i), and
     A(i-1) dominates A(i, 1) and
     A(i,j) dominates A(i, j+1) and
     A(i,K) dominanes A(i) and
     there are no other assignments to offset between A0 and AN

   It replaces these sequences with:

   A0: derived_pointer = array_base + offset
       *(derived_pointer)

   Aij: leave this alone.  expect that subsequent optimization deletes
        this code as it may become dead (since we don't use the
        indexing expression following out code transformations.)

   Ai:
   *(derived_pointer + constant_i)
     (where constant_i equals sum of constant (n,j) for all n from 1
      to i paired with all j from 1 to Kn,

   Note that each function may match this pattern multiple times.

*/
unsigned int
rs6000_fix_indexing (function *fun)
{
#ifdef REMOVE_ME
  struct loop *loop;
  int verbosity = 3;
#endif

  calculate_dominance_info (CDI_DOMINATORS);

#ifdef REMOVE_ME
  if (dump_file) {
    fprintf (dump_file, "Kelvin is fixing indices\n");
    print_rtx_function (dump_file, fun, true);
  }

  /* this code should borrow from rs6000_analyze_swaps ()  */

  /* Not even sure if I want to restrict myself to loops, but if I
     did, this code might be useful.  */
  FOR_EACH_LOOP (loop, LI_FROM_INNERMOST)
    {
      if (dump_file && (dump_flags & TDF_DETAILS)) {
	fprintf (dump_file,
		 "Kelvin examining loop: %d\n", loop->num);
	if (loop->header)
	  fprintf (dump_file, "header = %d, ", loop->header->index);
	else {
	  fprintf (dump_file, "deleted - nothing to do here\n");
	  return 0;
	}

	if (loop->latch)
          fprintf (dump_file, "latch = %d, ", loop->latch->index);
        else
          fprintf (dump_file, "multiple latches, ");

        fprintf (dump_file, " niter = ");
        print_generic_expr (dump_file, loop->nb_iterations);
        fprintf (dump_file, ")\n");

	basic_block bb;
	fprintf (dump_file, "These are the blocks that comprise my loop\n");
	FOR_EACH_BB_FN (bb, cfun) {
	  if (bb->loop_father == loop) {
	    fprintf (dump_file, "bb_%d (preds = {", bb->index);
	    print_pred_bbs (dump_file, bb);
	    fprintf (dump_file, "}, succs = {");
	    print_succ_bbs (dump_file, bb);
	    fprintf (dump_file, "})\n");

	    fprintf (dump_file, "insns = {");

	    rtx_insn *last = BB_END (bb);
	    if (last)
	      last = NEXT_INSN (last);
	    for (rtx_insn *insn = BB_HEAD (bb);
		 insn != last; insn = NEXT_INSN (insn)) {
	      /*
                df_dump_insn_top (insn, dump_file);
                dump_insn_slim (dump_file, insn);
                df_dump_insn_bottom (insn, dump_file);

                probably borrow some code from rs6000-swaps...
	      */
	      dump_bb (dump_file, bb, 4, TDF_VOPS|TDF_MEMSYMS);
	    }
	    fprintf (dump_file, "}\n\n");
          }
        }
      }
    }
#endif
  
  /* From here on down, this code was copied and adopted from
     rs6000_analyze_swaps.  */

  basic_block bb;
  rtx_insn *insn, *curr_insn = 0;

  indexing_web_entry *insn_entry;
  /* Dataflow analysis for use-def chains.  */
  df_set_flags (DF_RD_PRUNE_DEAD_DEFS);
  df_chain_add_problem (DF_DU_CHAIN | DF_UD_CHAIN);
  df_analyze ();
  //  df_set_flags (DF_DEFER_INSN_RESCAN);

  insn_entry = XCNEWVEC (indexing_web_entry, get_max_uid ());

  /* Kelvin says we shouldn't need a pre-pass to recombine lvx and
     stvx patterns so we don't lose info.  */

  /* I'm also thinking I don't need the insn_entry data structure to
     represent "webs of insns".  */

  /* I'm looking for patterns such as the following:

      ;; Note that reg/v/f:DI <27> gets its value here, but do we
      ;; care?  What is important to us is that reg/v/f:DI <27> does
      ;; not change throughout our sequence of insns..

      (cinsn 2 (set (reg/v/f:DI <27> [ x ])
                    (reg:DI 3 [ x ])) "ddot-c.c":12
       (expr_list:REG_DEAD (reg:DI 3 [ x ])))

      ;;
      ;; vsr_35 = MEM [x + reg:DI <9>]
      ;;
      (cinsn 31 (set (reg:V2DF <35> [ vect__3.7 ])
                     (mem:V2DF (plus:DI (reg/v/f:DI <27> [ x ])
                                        (reg:DI <9> [ ivtmp.18 ]))
                      [1 MEM[base: x_20(D), index: ivtmp.18_35,
                       offset: 0B]+0 S16 A64])) "ddot-c.c":18)

      ;;
      ;; i += constant value 16

      ;;
      (cinsn 304 (set (reg:DI <70>)
                      (plus:DI (reg:DI <9> [ ivtmp.18 ])
                               (const_int 16)))
       (expr_list:REG_DEAD (reg:DI <9> [ ivtmp.18 ])))
      (cinsn 34 (set (reg:DI <9> [ ivtmp.18 ])
                     (reg:DI <70>)))

      ;; by the way, I don't think I really need to do dominator
      ;; analysis.  I can just use the du-chains to confirm that there
      ;; is only one way the information can flow to particular
      ;; locations in the code.

      What does the algorithm look like:
       1. Look for multiple array indexing expressions that refer to
          the same array base address.
       2. Group these into subsets for which the indexing expression
          derives from the same initial_value + some accumulation of
          constant values added thereto.

      Question: is it possible that if I look at the insns now, they
      will have adjusted register numbers?

   */


  /* Walk the insns to gather basic data.  */
  FOR_ALL_BB_FN (bb, fun)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "Scrutinizing bb %d\n", bb->index);

      FOR_BB_INSNS_SAFE (bb, insn, curr_insn)
	{
	  unsigned int uid = INSN_UID (insn);

	  insn_entry[uid].insn = insn;
	  insn_entry[uid].bb = BLOCK_FOR_INSN (insn);

	  if (dump_file && (dump_flags & TDF_DETAILS)) {
	    fprintf (dump_file, "\nLooking at insn: %d\n", uid);
	    df_dump_insn_top (insn, dump_file);
	    dump_insn_slim (dump_file, insn);
	    df_dump_insn_bottom (insn, dump_file);
	  }

	  /*
	   * first, look for all memory[base + index] expressions.
	   * then group these by base.
	   * then for all instructions in each group, scrutinize the index
	   * definition. Partition this group according to the origin
	   * variable upon which the the definitions of i are based.
	   *
	   * how do we define "origin variable"?
	   *
	   *  if i has multiple definitions, it is its own origin
	   *  variable.  Likewise, if i has a single definition and the
	   *  definition is NOT the sum or difference of a constant value
	   *  and some other variable, the i is its own origin variable.
	   *
	   *  Otherwise, i has the same origin variable as the expression
	   *  that represents its definition.
	   *
	   * After we've created these partitions, for each partition
	   * whose size is greater than 1:
	   *
	   *  1. introduce derived_ptr = base + origin_variable
	   *     immediately following the instruction that defines
	   *     origin_variable.
	   *
	   *  2. for each member of the partition, replace the expression
	   *     memory [base + index] with derived_ptr [constant], where
	   *     constant is the sum of all constant values added to the
	   *     origin variable to represent this particular value of i.
	   */

	  if (NONDEBUG_INSN_P (insn)) {
	    rtx body = PATTERN (insn);
	    if ((GET_CODE (body) == SET) && (GET_CODE (SET_SRC (body)) == MEM)) {
	      rtx mem = XEXP (SET_SRC (body), 0);

	      if (dump_file && (dump_flags & TDF_DETAILS)) {
		fprintf (dump_file, " this insn is fetching data from memory: ");
		print_inline_rtx (dump_file, mem, 2);
		fprintf (dump_file, "\n");
	      }
	      enum rtx_code code = GET_CODE (mem);
	      if ((code == PLUS) || (code == MINUS)) {
		rtx base_reg = XEXP (mem,0);
		rtx index_reg = XEXP (mem, 1);

		if (dump_file && (dump_flags & TDF_DETAILS)) {
		  fprintf (dump_file, " memory is base + index, ");
		  fprintf (dump_file, "base: ");
		  print_inline_rtx (dump_file, base_reg, 2);
		  fprintf (dump_file, "\n index: ");
		  print_inline_rtx (dump_file, index_reg, 2);
		  fprintf (dump_file, "\n");
		}

		if (REG_P (base_reg) && REG_P (index_reg)) {
		  int base_originator = -1;
		  unsigned long long int base_offset = 0;
		  const char *base_defs = NULL;
		  int index_originator = -1;
		  unsigned long long int index_offset = 0;
		  const char *index_defs = NULL;

		  struct df_insn_info *insn_info = DF_INSN_INFO_GET (insn);
		  /* Since insn is known to represent a sum or difference,
		     this insn is likely to use at least two input variables.  */
		  df_ref use;
		  FOR_EACH_INSN_INFO_USE (use, insn_info)
		    {
		      /* not used yet:
			 struct df_link *def_link = DF_REF_CHAIN (use);
		      */
		      if (rtx_equal_p (DF_REF_REG (use), base_reg)) {
			if (dump_file && (dump_flags & TDF_DETAILS)) {
			  fprintf (dump_file,
				   "Found use corresponding to base_reg\n");
			  df_ref_debug (use, dump_file);
			}
			struct df_link *def_link = DF_REF_CHAIN (use);

			/* If there is no definition, or the definition
			   is artificial, or there are multiple
			   definitions, this is an originating use.  */
			if (!def_link || !def_link->ref
			    || DF_REF_IS_ARTIFICIAL (def_link->ref)
			    || def_link->next) {
			  if (dump_file && (dump_flags & TDF_DETAILS))
			    fprintf (dump_file, "Use is originating!\n");
			  base_originator = uid;
			  base_offset = 0;
			  base_defs = canonize (def_link);
			}
			else
			  {
			    if (dump_file && (dump_flags & TDF_DETAILS))
			      fprintf (dump_file, "Use may be propagating\n");
			    /* There's only one definition.  Dig deeper.  */
			    long long int delta = 0;
			    int uid2 = find_true_originator (def_link, &delta);
			    if (uid2 > 0) {
			      base_originator = uid2;
			      base_offset = delta;
			      rtx_insn *insn2 = insn_entry[uid2].insn;
			      struct df_insn_info *insn_info2 =
				DF_INSN_INFO_GET (insn2);
			      df_ref use2;
			      use2 = DF_INSN_INFO_USES (insn_info2);
			      if (use2)
				{
				  if (DF_REF_NEXT_LOC (use2))
				    fatal ("expect one use for true originator");
				  base_defs = canonize (DF_REF_CHAIN (use2));
				}
			      else
				base_defs = CONSTANT_DEFINITION;

			    }
			    if (dump_file && (dump_flags & TDF_DETAILS)) {
			      fprintf (dump_file,
				       " propagates from originating insn %d,"
				       " with delta: %lld\n", uid2, delta);
			    }
			  }
		      }
		      else if (rtx_equal_p (DF_REF_REG (use), index_reg))
			{
			  if (dump_file && (dump_flags & TDF_DETAILS)) {
			    fprintf (dump_file,
				     "Found use corresponding to index\n");
			    df_ref_debug (use, dump_file);
			  }
			  struct df_link *def_link = DF_REF_CHAIN (use);

			  /* If there is no definition, or the definition
			     is artificial, or there are multiple
			     definitions, this is an originating use.  */
			  if (!def_link || !def_link->ref
			      || DF_REF_IS_ARTIFICIAL (def_link->ref)
			      || def_link->next) {
			    if (dump_file && (dump_flags & TDF_DETAILS))
			      fprintf (dump_file, "Use is originating!\n");
			    index_originator = uid;
			    index_offset = 0;
			    index_defs = canonize (def_link);
			  }
			  else {
			    if (dump_file && (dump_flags & TDF_DETAILS))
			      fprintf (dump_file, "Use may be propagating\n");
			    /* There's only one definition.  Dig deeper.  */
			    long long int delta = 0;
			    int uid2 = find_true_originator (def_link, &delta);
			    if (uid2 > 0) {
			      index_originator = uid2;
			      index_offset = delta;
			      rtx_insn *insn2 = insn_entry[uid2].insn;
			      struct df_insn_info *insn_info2 =
				DF_INSN_INFO_GET (insn2);
			      df_ref use2;
			      use2 = DF_INSN_INFO_USES (insn_info2);
			      if (use2) {
				if (DF_REF_NEXT_LOC (use2))
				  fatal ("expect one use for true originator");
				index_defs = canonize (DF_REF_CHAIN (use2));
			      }
			      else
				index_defs = CONSTANT_DEFINITION;
			    }
			    if (dump_file && (dump_flags & TDF_DETAILS)) {
			      fprintf (dump_file,
				       " propagates from originating insn %d,"
				       " with delta %lld\n", uid2, delta);
			    }
			  }
			}

		      if ((index_originator > 0) && (base_originator > 0))
			insert_into_equivalence_class (uid, index_defs,
						       base_defs,
						       index_originator,
						       index_offset,
						       base_originator,
						       base_offset);
		    }
		}
	      }

	      if ((GET_CODE (body) == SET)
		  && (GET_CODE (SET_DEST (body)) == MEM))
		{
		  rtx mem = XEXP (SET_DEST (body), 0);
		  /*  rtx base_reg = XEXP (mem, 0); */
		  /* unused at the moment: rtx index = XEXP (mem, 1); */
		  if (dump_file && (dump_flags & TDF_DETAILS)) {
		    fprintf (dump_file,
			     " this insn is storing data to memory: ");
		    print_inline_rtx (dump_file, mem, 2);
		    fprintf (dump_file, "\n");
		  }

		  /* FIXME: kelvin needs to copy load code into store
		     context  */

		}
	    }
	  }
	  else if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file,
		     " punting because base or index not registers\n");
	}

      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "\n");
    }

#ifdef TO_REMOVE
  dump_equivalences (insn_entry);
#endif
  fixup_equivalences (insn_entry);
  free_dominance_info (CDI_DOMINATORS);
  return 0;
}  // anon namespace


const pass_data pass_data_fix_indexing =
{
  RTL_PASS, /* type */
  "indexing", /* name */
  OPTGROUP_NONE, /* optinfo_flags, or could use OPTGROUP_LOOP */
  TV_NONE, /* tv_id, or could use TV_LOOP_UNROLL */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_df_finish, /* todo_flags_finish */
};

class pass_fix_indexing : public rtl_opt_pass
{
public:
  pass_fix_indexing(gcc::context *ctxt)
    : rtl_opt_pass(pass_data_fix_indexing, ctxt)
  {}

  /* opt_pass methods: */
  virtual bool gate (function *)
    {
      // This is most relevant to P9 targets since that architecture
      // introduces new D-form instructions, but this may pay off on
      // other architectures as well.  Might want to experiment.
      return (optimize > 0 && !BYTES_BIG_ENDIAN && TARGET_VSX
	      && TARGET_P9_VECTOR);
    }

  virtual unsigned int execute (function *fun)
    {
      return rs6000_fix_indexing (fun);
    }

  opt_pass *clone ()
    {
      return new pass_fix_indexing (m_ctxt);
    }

}; // class pass_fix_indexing

rtl_opt_pass *make_pass_fix_indexing (gcc::context *ctxt)
{
  return new pass_fix_indexing (ctxt);
}
