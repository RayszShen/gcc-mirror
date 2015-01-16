/* Control flow graph building code for GNU compiler.
   Copyright (C) 1987-2015 Free Software Foundation, Inc.

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


#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "predict.h"
#include "hashtab.h"
#include "function.h"
#include "dominance.h"
#include "cfg.h"
#include "cfgrtl.h"
#include "cfganal.h"
#include "cfgbuild.h"
#include "basic-block.h"
#include "regs.h"
#include "flags.h"
#include "except.h"
#include "statistics.h"
#include "real.h"
#include "fixed-value.h"
#include "insn-config.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "calls.h"
#include "emit-rtl.h"
#include "varasm.h"
#include "stmt.h"
#include "expr.h"
#include "diagnostic-core.h"
#include "timevar.h"
#include "sbitmap.h"

static void make_edges (basic_block, basic_block, int);
static void make_label_edge (sbitmap, basic_block, rtx, int);
static void find_bb_boundaries (basic_block);
static void compute_outgoing_frequencies (basic_block);

/* Return true if insn is something that should be contained inside basic
   block.  */

bool
inside_basic_block_p (const rtx_insn *insn)
{
  switch (GET_CODE (insn))
    {
    case CODE_LABEL:
      /* Avoid creating of basic block for jumptables.  */
      return (NEXT_INSN (insn) == 0
	      || ! JUMP_TABLE_DATA_P (NEXT_INSN (insn)));

    case JUMP_INSN:
    case CALL_INSN:
    case INSN:
    case DEBUG_INSN:
      return true;

    case JUMP_TABLE_DATA:
    case BARRIER:
    case NOTE:
      return false;

    default:
      gcc_unreachable ();
    }
}

/* Return true if INSN may cause control flow transfer, so it should be last in
   the basic block.  */

bool
control_flow_insn_p (const rtx_insn *insn)
{
  switch (GET_CODE (insn))
    {
    case NOTE:
    case CODE_LABEL:
    case DEBUG_INSN:
      return false;

    case JUMP_INSN:
      return true;

    case CALL_INSN:
      /* Noreturn and sibling call instructions terminate the basic blocks
	 (but only if they happen unconditionally).  */
      if ((SIBLING_CALL_P (insn)
	   || find_reg_note (insn, REG_NORETURN, 0))
	  && GET_CODE (PATTERN (insn)) != COND_EXEC)
	return true;

      /* Call insn may return to the nonlocal goto handler.  */
      if (can_nonlocal_goto (insn))
	return true;
      break;

    case INSN:
      /* Treat trap instructions like noreturn calls (same provision).  */
      if (GET_CODE (PATTERN (insn)) == TRAP_IF
	  && XEXP (PATTERN (insn), 0) == const1_rtx)
	return true;
      if (!cfun->can_throw_non_call_exceptions)
	return false;
      break;

    case JUMP_TABLE_DATA:
    case BARRIER:
      /* It is nonsense to reach this when looking for the
	 end of basic block, but before dead code is eliminated
	 this may happen.  */
      return false;

    default:
      gcc_unreachable ();
    }

  return can_throw_internal (insn);
}


/* Create an edge between two basic blocks.  FLAGS are auxiliary information
   about the edge that is accumulated between calls.  */

/* Create an edge from a basic block to a label.  */

static void
make_label_edge (sbitmap edge_cache, basic_block src, rtx label, int flags)
{
  gcc_assert (LABEL_P (label));

  /* If the label was never emitted, this insn is junk, but avoid a
     crash trying to refer to BLOCK_FOR_INSN (label).  This can happen
     as a result of a syntax error and a diagnostic has already been
     printed.  */

  if (INSN_UID (label) == 0)
    return;

  cached_make_edge (edge_cache, src, BLOCK_FOR_INSN (label), flags);
}

/* Create the edges generated by INSN in REGION.  */

void
rtl_make_eh_edge (sbitmap edge_cache, basic_block src, rtx insn)
{
  eh_landing_pad lp = get_eh_landing_pad_from_rtx (insn);

  if (lp)
    {
      rtx label = lp->landing_pad;

      /* During initial rtl generation, use the post_landing_pad.  */
      if (label == NULL)
	{
	  gcc_assert (lp->post_landing_pad);
	  label = label_rtx (lp->post_landing_pad);
	}

      make_label_edge (edge_cache, src, label,
		       EDGE_ABNORMAL | EDGE_EH
		       | (CALL_P (insn) ? EDGE_ABNORMAL_CALL : 0));
    }
}

/* States of basic block as seen by find_many_sub_basic_blocks.  */
enum state {
  /* Basic blocks created via split_block belong to this state.
     make_edges will examine these basic blocks to see if we need to
     create edges going out of them.  */
  BLOCK_NEW = 0,

  /* Basic blocks that do not need examining belong to this state.
     These blocks will be left intact.  In particular, make_edges will
     not create edges going out of these basic blocks.  */
  BLOCK_ORIGINAL,

  /* Basic blocks that may need splitting (due to a label appearing in
     the middle, etc) belong to this state.  After splitting them,
     make_edges will create edges going out of them as needed.  */
  BLOCK_TO_SPLIT
};

#define STATE(BB) (enum state) ((size_t) (BB)->aux)
#define SET_STATE(BB, STATE) ((BB)->aux = (void *) (size_t) (STATE))

/* Used internally by purge_dead_tablejump_edges, ORed into state.  */
#define BLOCK_USED_BY_TABLEJUMP		32
#define FULL_STATE(BB) ((size_t) (BB)->aux)

/* Identify the edges going out of basic blocks between MIN and MAX,
   inclusive, that have their states set to BLOCK_NEW or
   BLOCK_TO_SPLIT.

   UPDATE_P should be nonzero if we are updating CFG and zero if we
   are building CFG from scratch.  */

static void
make_edges (basic_block min, basic_block max, int update_p)
{
  basic_block bb;
  sbitmap edge_cache = NULL;

  /* Heavy use of computed goto in machine-generated code can lead to
     nearly fully-connected CFGs.  In that case we spend a significant
     amount of time searching the edge lists for duplicates.  */
  if (forced_labels || cfun->cfg->max_jumptable_ents > 100)
    edge_cache = sbitmap_alloc (last_basic_block_for_fn (cfun));

  /* By nature of the way these get numbered, ENTRY_BLOCK_PTR->next_bb block
     is always the entry.  */
  if (min == ENTRY_BLOCK_PTR_FOR_FN (cfun)->next_bb)
    make_edge (ENTRY_BLOCK_PTR_FOR_FN (cfun), min, EDGE_FALLTHRU);

  FOR_BB_BETWEEN (bb, min, max->next_bb, next_bb)
    {
      rtx_insn *insn;
      enum rtx_code code;
      edge e;
      edge_iterator ei;

      if (STATE (bb) == BLOCK_ORIGINAL)
	continue;

      /* If we have an edge cache, cache edges going out of BB.  */
      if (edge_cache)
	{
	  bitmap_clear (edge_cache);
	  if (update_p)
	    {
	      FOR_EACH_EDGE (e, ei, bb->succs)
		if (e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun))
		  bitmap_set_bit (edge_cache, e->dest->index);
	    }
	}

      if (LABEL_P (BB_HEAD (bb))
	  && LABEL_ALT_ENTRY_P (BB_HEAD (bb)))
	cached_make_edge (NULL, ENTRY_BLOCK_PTR_FOR_FN (cfun), bb, 0);

      /* Examine the last instruction of the block, and discover the
	 ways we can leave the block.  */

      insn = BB_END (bb);
      code = GET_CODE (insn);

      /* A branch.  */
      if (code == JUMP_INSN)
	{
	  rtx tmp;
	  rtx_jump_table_data *table;

	  /* Recognize a non-local goto as a branch outside the
	     current function.  */
	  if (find_reg_note (insn, REG_NON_LOCAL_GOTO, NULL_RTX))
	    ;

	  /* Recognize a tablejump and do the right thing.  */
	  else if (tablejump_p (insn, NULL, &table))
	    {
	      rtvec vec = table->get_labels ();
	      int j;

	      for (j = GET_NUM_ELEM (vec) - 1; j >= 0; --j)
		make_label_edge (edge_cache, bb,
				 XEXP (RTVEC_ELT (vec, j), 0), 0);

	      /* Some targets (eg, ARM) emit a conditional jump that also
		 contains the out-of-range target.  Scan for these and
		 add an edge if necessary.  */
	      if ((tmp = single_set (insn)) != NULL
		  && SET_DEST (tmp) == pc_rtx
		  && GET_CODE (SET_SRC (tmp)) == IF_THEN_ELSE
		  && GET_CODE (XEXP (SET_SRC (tmp), 2)) == LABEL_REF)
		make_label_edge (edge_cache, bb,
				 LABEL_REF_LABEL (XEXP (SET_SRC (tmp), 2)), 0);
	    }

	  /* If this is a computed jump, then mark it as reaching
	     everything on the forced_labels list.  */
	  else if (computed_jump_p (insn))
	    {
	      for (rtx_insn_list *x = forced_labels; x; x = x->next ())
		make_label_edge (edge_cache, bb, x->insn (), EDGE_ABNORMAL);
	    }

	  /* Returns create an exit out.  */
	  else if (returnjump_p (insn))
	    cached_make_edge (edge_cache, bb, EXIT_BLOCK_PTR_FOR_FN (cfun), 0);

	  /* Recognize asm goto and do the right thing.  */
	  else if ((tmp = extract_asm_operands (PATTERN (insn))) != NULL)
	    {
	      int i, n = ASM_OPERANDS_LABEL_LENGTH (tmp);
	      for (i = 0; i < n; ++i)
		make_label_edge (edge_cache, bb,
				 XEXP (ASM_OPERANDS_LABEL (tmp, i), 0), 0);
	    }

	  /* Otherwise, we have a plain conditional or unconditional jump.  */
	  else
	    {
	      gcc_assert (JUMP_LABEL (insn));
	      make_label_edge (edge_cache, bb, JUMP_LABEL (insn), 0);
	    }
	}

      /* If this is a sibling call insn, then this is in effect a combined call
	 and return, and so we need an edge to the exit block.  No need to
	 worry about EH edges, since we wouldn't have created the sibling call
	 in the first place.  */
      if (code == CALL_INSN && SIBLING_CALL_P (insn))
	cached_make_edge (edge_cache, bb, EXIT_BLOCK_PTR_FOR_FN (cfun),
			  EDGE_SIBCALL | EDGE_ABNORMAL);

      /* If this is a CALL_INSN, then mark it as reaching the active EH
	 handler for this CALL_INSN.  If we're handling non-call
	 exceptions then any insn can reach any of the active handlers.
	 Also mark the CALL_INSN as reaching any nonlocal goto handler.  */
      else if (code == CALL_INSN || cfun->can_throw_non_call_exceptions)
	{
	  /* Add any appropriate EH edges.  */
	  rtl_make_eh_edge (edge_cache, bb, insn);

	  if (code == CALL_INSN)
	    {
	      if (can_nonlocal_goto (insn))
		{
		  /* ??? This could be made smarter: in some cases it's
		     possible to tell that certain calls will not do a
		     nonlocal goto.  For example, if the nested functions
		     that do the nonlocal gotos do not have their addresses
		     taken, then only calls to those functions or to other
		     nested functions that use them could possibly do
		     nonlocal gotos.  */
		  for (rtx_insn_list *x = nonlocal_goto_handler_labels;
		       x;
		       x = x->next ())
		    make_label_edge (edge_cache, bb, x->insn (),
				     EDGE_ABNORMAL | EDGE_ABNORMAL_CALL);
		}

	      if (flag_tm)
		{
		  rtx note;
		  for (note = REG_NOTES (insn); note; note = XEXP (note, 1))
		    if (REG_NOTE_KIND (note) == REG_TM)
		      make_label_edge (edge_cache, bb, XEXP (note, 0),
				       EDGE_ABNORMAL | EDGE_ABNORMAL_CALL);
		}
	    }
	}

      /* Find out if we can drop through to the next block.  */
      insn = NEXT_INSN (insn);
      e = find_edge (bb, EXIT_BLOCK_PTR_FOR_FN (cfun));
      if (e && e->flags & EDGE_FALLTHRU)
	insn = NULL;

      while (insn
	     && NOTE_P (insn)
	     && NOTE_KIND (insn) != NOTE_INSN_BASIC_BLOCK)
	insn = NEXT_INSN (insn);

      if (!insn)
	cached_make_edge (edge_cache, bb, EXIT_BLOCK_PTR_FOR_FN (cfun),
			  EDGE_FALLTHRU);
      else if (bb->next_bb != EXIT_BLOCK_PTR_FOR_FN (cfun))
	{
	  if (insn == BB_HEAD (bb->next_bb))
	    cached_make_edge (edge_cache, bb, bb->next_bb, EDGE_FALLTHRU);
	}
    }

  if (edge_cache)
    sbitmap_free (edge_cache);
}

static void
mark_tablejump_edge (rtx label)
{
  basic_block bb;

  gcc_assert (LABEL_P (label));
  /* See comment in make_label_edge.  */
  if (INSN_UID (label) == 0)
    return;
  bb = BLOCK_FOR_INSN (label);
  SET_STATE (bb, FULL_STATE (bb) | BLOCK_USED_BY_TABLEJUMP);
}

static void
purge_dead_tablejump_edges (basic_block bb, rtx_jump_table_data *table)
{
  rtx_insn *insn = BB_END (bb);
  rtx tmp;
  rtvec vec;
  int j;
  edge_iterator ei;
  edge e;

  vec = table->get_labels ();

  for (j = GET_NUM_ELEM (vec) - 1; j >= 0; --j)
    mark_tablejump_edge (XEXP (RTVEC_ELT (vec, j), 0));

  /* Some targets (eg, ARM) emit a conditional jump that also
     contains the out-of-range target.  Scan for these and
     add an edge if necessary.  */
  if ((tmp = single_set (insn)) != NULL
       && SET_DEST (tmp) == pc_rtx
       && GET_CODE (SET_SRC (tmp)) == IF_THEN_ELSE
       && GET_CODE (XEXP (SET_SRC (tmp), 2)) == LABEL_REF)
    mark_tablejump_edge (LABEL_REF_LABEL (XEXP (SET_SRC (tmp), 2)));

  for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
    {
      if (FULL_STATE (e->dest) & BLOCK_USED_BY_TABLEJUMP)
	SET_STATE (e->dest, FULL_STATE (e->dest)
			    & ~(size_t) BLOCK_USED_BY_TABLEJUMP);
      else if (!(e->flags & (EDGE_ABNORMAL | EDGE_EH)))
	{
	  remove_edge (e);
	  continue;
	}
      ei_next (&ei);
    }
}

/* Scan basic block BB for possible BB boundaries inside the block
   and create new basic blocks in the progress.  */

static void
find_bb_boundaries (basic_block bb)
{
  basic_block orig_bb = bb;
  rtx_insn *insn = BB_HEAD (bb);
  rtx_insn *end = BB_END (bb), *x;
  rtx_jump_table_data *table;
  rtx_insn *flow_transfer_insn = NULL;
  edge fallthru = NULL;

  if (insn == BB_END (bb))
    return;

  if (LABEL_P (insn))
    insn = NEXT_INSN (insn);

  /* Scan insn chain and try to find new basic block boundaries.  */
  while (1)
    {
      enum rtx_code code = GET_CODE (insn);

      /* In case we've previously seen an insn that effects a control
	 flow transfer, split the block.  */
      if ((flow_transfer_insn || code == CODE_LABEL)
	  && inside_basic_block_p (insn))
	{
	  fallthru = split_block (bb, PREV_INSN (insn));
	  if (flow_transfer_insn)
	    {
	      BB_END (bb) = flow_transfer_insn;

	      /* Clean up the bb field for the insns between the blocks.  */
	      for (x = NEXT_INSN (flow_transfer_insn);
		   x != BB_HEAD (fallthru->dest);
		   x = NEXT_INSN (x))
		if (!BARRIER_P (x))
		  set_block_for_insn (x, NULL);
	    }

	  bb = fallthru->dest;
	  remove_edge (fallthru);
	  flow_transfer_insn = NULL;
	  if (code == CODE_LABEL && LABEL_ALT_ENTRY_P (insn))
	    make_edge (ENTRY_BLOCK_PTR_FOR_FN (cfun), bb, 0);
	}
      else if (code == BARRIER)
	{
	  /* __builtin_unreachable () may cause a barrier to be emitted in
	     the middle of a BB.  We need to split it in the same manner as
	     if the barrier were preceded by a control_flow_insn_p insn.  */
	  if (!flow_transfer_insn)
	    flow_transfer_insn = prev_nonnote_insn_bb (insn);
	}

      if (control_flow_insn_p (insn))
	flow_transfer_insn = insn;
      if (insn == end)
	break;
      insn = NEXT_INSN (insn);
    }

  /* In case expander replaced normal insn by sequence terminating by
     return and barrier, or possibly other sequence not behaving like
     ordinary jump, we need to take care and move basic block boundary.  */
  if (flow_transfer_insn)
    {
      BB_END (bb) = flow_transfer_insn;

      /* Clean up the bb field for the insns that do not belong to BB.  */
      x = flow_transfer_insn;
      while (x != end)
	{
	  x = NEXT_INSN (x);
	  if (!BARRIER_P (x))
	    set_block_for_insn (x, NULL);
	}
    }

  /* We've possibly replaced the conditional jump by conditional jump
     followed by cleanup at fallthru edge, so the outgoing edges may
     be dead.  */
  purge_dead_edges (bb);

  /* purge_dead_edges doesn't handle tablejump's, but if we have split the
     basic block, we might need to kill some edges.  */
  if (bb != orig_bb && tablejump_p (BB_END (bb), NULL, &table))
    purge_dead_tablejump_edges (bb, table);
}

/*  Assume that frequency of basic block B is known.  Compute frequencies
    and probabilities of outgoing edges.  */

static void
compute_outgoing_frequencies (basic_block b)
{
  edge e, f;
  edge_iterator ei;

  if (EDGE_COUNT (b->succs) == 2)
    {
      rtx note = find_reg_note (BB_END (b), REG_BR_PROB, NULL);
      int probability;

      if (note)
	{
	  probability = XINT (note, 0);
	  e = BRANCH_EDGE (b);
	  e->probability = probability;
	  e->count = apply_probability (b->count, probability);
	  f = FALLTHRU_EDGE (b);
	  f->probability = REG_BR_PROB_BASE - probability;
	  f->count = b->count - e->count;
	  return;
	}
      else
        {
          guess_outgoing_edge_probabilities (b);
        }
    }
  else if (single_succ_p (b))
    {
      e = single_succ_edge (b);
      e->probability = REG_BR_PROB_BASE;
      e->count = b->count;
      return;
    }
  else
    {
      /* We rely on BBs with more than two successors to have sane probabilities
         and do not guess them here. For BBs terminated by switch statements
         expanded to jump-table jump, we have done the right thing during
         expansion. For EH edges, we still guess the probabilities here.  */
      bool complex_edge = false;
      FOR_EACH_EDGE (e, ei, b->succs)
        if (e->flags & EDGE_COMPLEX)
          {
            complex_edge = true;
            break;
          }
      if (complex_edge)
        guess_outgoing_edge_probabilities (b);
    }

  if (b->count)
    FOR_EACH_EDGE (e, ei, b->succs)
      e->count = apply_probability (b->count, e->probability);
}

/* Assume that some pass has inserted labels or control flow
   instructions within a basic block.  Split basic blocks as needed
   and create edges.  */

void
find_many_sub_basic_blocks (sbitmap blocks)
{
  basic_block bb, min, max;

  FOR_EACH_BB_FN (bb, cfun)
    SET_STATE (bb,
	       bitmap_bit_p (blocks, bb->index) ? BLOCK_TO_SPLIT : BLOCK_ORIGINAL);

  FOR_EACH_BB_FN (bb, cfun)
    if (STATE (bb) == BLOCK_TO_SPLIT)
      find_bb_boundaries (bb);

  FOR_EACH_BB_FN (bb, cfun)
    if (STATE (bb) != BLOCK_ORIGINAL)
      break;

  min = max = bb;
  for (; bb != EXIT_BLOCK_PTR_FOR_FN (cfun); bb = bb->next_bb)
    if (STATE (bb) != BLOCK_ORIGINAL)
      max = bb;

  /* Now re-scan and wire in all edges.  This expect simple (conditional)
     jumps at the end of each new basic blocks.  */
  make_edges (min, max, 1);

  /* Update branch probabilities.  Expect only (un)conditional jumps
     to be created with only the forward edges.  */
  if (profile_status_for_fn (cfun) != PROFILE_ABSENT)
    FOR_BB_BETWEEN (bb, min, max->next_bb, next_bb)
      {
	edge e;
	edge_iterator ei;

	if (STATE (bb) == BLOCK_ORIGINAL)
	  continue;
	if (STATE (bb) == BLOCK_NEW)
	  {
	    bb->count = 0;
	    bb->frequency = 0;
	    FOR_EACH_EDGE (e, ei, bb->preds)
	      {
		bb->count += e->count;
		bb->frequency += EDGE_FREQUENCY (e);
	      }
	  }

	compute_outgoing_frequencies (bb);
      }

  FOR_EACH_BB_FN (bb, cfun)
    SET_STATE (bb, 0);
}
