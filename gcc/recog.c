/* Subroutines used by or related to instruction recognition.
   Copyright (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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


#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "regs.h"
#include "expr.h"
#include "function.h"
#include "flags.h"
#include "real.h"
#include "toplev.h"
#include "basic-block.h"
#include "output.h"
#include "reload.h"

#ifndef STACK_PUSH_CODE
#ifdef STACK_GROWS_DOWNWARD
#define STACK_PUSH_CODE PRE_DEC
#else
#define STACK_PUSH_CODE PRE_INC
#endif
#endif

#ifndef STACK_POP_CODE
#ifdef STACK_GROWS_DOWNWARD
#define STACK_POP_CODE POST_INC
#else
#define STACK_POP_CODE POST_DEC
#endif
#endif

static void validate_replace_rtx_1	PARAMS ((rtx *, rtx, rtx, rtx));
static rtx *find_single_use_1		PARAMS ((rtx, rtx *));
static void validate_replace_src_1 	PARAMS ((rtx *, void *));
static rtx split_insn			PARAMS ((rtx));

/* Nonzero means allow operands to be volatile.
   This should be 0 if you are generating rtl, such as if you are calling
   the functions in optabs.c and expmed.c (most of the time).
   This should be 1 if all valid insns need to be recognized,
   such as in regclass.c and final.c and reload.c.

   init_recog and init_recog_no_volatile are responsible for setting this.  */

int volatile_ok;

struct recog_data recog_data;

/* Contains a vector of operand_alternative structures for every operand.
   Set up by preprocess_constraints.  */
struct operand_alternative recog_op_alt[MAX_RECOG_OPERANDS][MAX_RECOG_ALTERNATIVES];

/* On return from `constrain_operands', indicate which alternative
   was satisfied.  */

int which_alternative;

/* Nonzero after end of reload pass.
   Set to 1 or 0 by toplev.c.
   Controls the significance of (SUBREG (MEM)).  */

int reload_completed;

/* Initialize data used by the function `recog'.
   This must be called once in the compilation of a function
   before any insn recognition may be done in the function.  */

void
init_recog_no_volatile ()
{
  volatile_ok = 0;
}

void
init_recog ()
{
  volatile_ok = 1;
}

/* Try recognizing the instruction INSN,
   and return the code number that results.
   Remember the code so that repeated calls do not
   need to spend the time for actual rerecognition.

   This function is the normal interface to instruction recognition.
   The automatically-generated function `recog' is normally called
   through this one.  (The only exception is in combine.c.)  */

int
recog_memoized_1 (insn)
     rtx insn;
{
  if (INSN_CODE (insn) < 0)
    INSN_CODE (insn) = recog (PATTERN (insn), insn, 0);
  return INSN_CODE (insn);
}

/* Check that X is an insn-body for an `asm' with operands
   and that the operands mentioned in it are legitimate.  */

int
check_asm_operands (x)
     rtx x;
{
  int noperands;
  rtx *operands;
  const char **constraints;
  int i;

  /* Post-reload, be more strict with things.  */
  if (reload_completed)
    {
      /* ??? Doh!  We've not got the wrapping insn.  Cook one up.  */
      extract_insn (make_insn_raw (x));
      constrain_operands (1);
      return which_alternative >= 0;
    }

  noperands = asm_noperands (x);
  if (noperands < 0)
    return 0;
  if (noperands == 0)
    return 1;

  operands = (rtx *) alloca (noperands * sizeof (rtx));
  constraints = (const char **) alloca (noperands * sizeof (char *));

  decode_asm_operands (x, operands, NULL, constraints, NULL);

  for (i = 0; i < noperands; i++)
    {
      const char *c = constraints[i];
      if (c[0] == '%')
	c++;
      if (ISDIGIT ((unsigned char) c[0]) && c[1] == '\0')
	c = constraints[c[0] - '0'];

      if (! asm_operand_ok (operands[i], c))
	return 0;
    }

  return 1;
}

/* Static data for the next two routines.  */

typedef struct change_t
{
  rtx object;
  int old_code;
  rtx *loc;
  rtx old;
} change_t;

static change_t *changes;
static int changes_allocated;

static int num_changes = 0;

/* Validate a proposed change to OBJECT.  LOC is the location in the rtl
   at which NEW will be placed.  If OBJECT is zero, no validation is done,
   the change is simply made.

   Two types of objects are supported:  If OBJECT is a MEM, memory_address_p
   will be called with the address and mode as parameters.  If OBJECT is
   an INSN, CALL_INSN, or JUMP_INSN, the insn will be re-recognized with
   the change in place.

   IN_GROUP is non-zero if this is part of a group of changes that must be
   performed as a group.  In that case, the changes will be stored.  The
   function `apply_change_group' will validate and apply the changes.

   If IN_GROUP is zero, this is a single change.  Try to recognize the insn
   or validate the memory reference with the change applied.  If the result
   is not valid for the machine, suppress the change and return zero.
   Otherwise, perform the change and return 1.  */

int
validate_change (object, loc, new, in_group)
    rtx object;
    rtx *loc;
    rtx new;
    int in_group;
{
  rtx old = *loc;

  if (old == new || rtx_equal_p (old, new))
    return 1;

  if (in_group == 0 && num_changes != 0)
    abort ();

  *loc = new;

  /* Save the information describing this change.  */
  if (num_changes >= changes_allocated)
    {
      if (changes_allocated == 0)
	/* This value allows for repeated substitutions inside complex
	   indexed addresses, or changes in up to 5 insns.  */
	changes_allocated = MAX_RECOG_OPERANDS * 5;
      else
	changes_allocated *= 2;

      changes =
	(change_t*) xrealloc (changes,
			      sizeof (change_t) * changes_allocated);
    }

  changes[num_changes].object = object;
  changes[num_changes].loc = loc;
  changes[num_changes].old = old;

  if (object && GET_CODE (object) != MEM)
    {
      /* Set INSN_CODE to force rerecognition of insn.  Save old code in
	 case invalid.  */
      changes[num_changes].old_code = INSN_CODE (object);
      INSN_CODE (object) = -1;
    }

  num_changes++;

  /* If we are making a group of changes, return 1.  Otherwise, validate the
     change group we made.  */

  if (in_group)
    return 1;
  else
    return apply_change_group ();
}

/* This subroutine of apply_change_group verifies whether the changes to INSN
   were valid; i.e. whether INSN can still be recognized.  */

int
insn_invalid_p (insn)
     rtx insn;
{
  rtx pat = PATTERN (insn);
  int num_clobbers = 0;
  /* If we are before reload and the pattern is a SET, see if we can add
     clobbers.  */
  int icode = recog (pat, insn,
		     (GET_CODE (pat) == SET
		      && ! reload_completed && ! reload_in_progress)
		     ? &num_clobbers : 0);
  int is_asm = icode < 0 && asm_noperands (PATTERN (insn)) >= 0;


  /* If this is an asm and the operand aren't legal, then fail.  Likewise if
     this is not an asm and the insn wasn't recognized.  */
  if ((is_asm && ! check_asm_operands (PATTERN (insn)))
      || (!is_asm && icode < 0))
    return 1;

  /* If we have to add CLOBBERs, fail if we have to add ones that reference
     hard registers since our callers can't know if they are live or not.
     Otherwise, add them.  */
  if (num_clobbers > 0)
    {
      rtx newpat;

      if (added_clobbers_hard_reg_p (icode))
	return 1;

      newpat = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (num_clobbers + 1));
      XVECEXP (newpat, 0, 0) = pat;
      add_clobbers (newpat, icode);
      PATTERN (insn) = pat = newpat;
    }

  /* After reload, verify that all constraints are satisfied.  */
  if (reload_completed)
    {
      extract_insn (insn);

      if (! constrain_operands (1))
	return 1;
    }

  INSN_CODE (insn) = icode;
  return 0;
}

/* Return number of changes made and not validated yet.  */
int
num_changes_pending ()
{
  return num_changes;
}

/* Apply a group of changes previously issued with `validate_change'.
   Return 1 if all changes are valid, zero otherwise.  */

int
apply_change_group ()
{
  int i;
  rtx last_validated = NULL_RTX;

  /* The changes have been applied and all INSN_CODEs have been reset to force
     rerecognition.

     The changes are valid if we aren't given an object, or if we are
     given a MEM and it still is a valid address, or if this is in insn
     and it is recognized.  In the latter case, if reload has completed,
     we also require that the operands meet the constraints for
     the insn.  */

  for (i = 0; i < num_changes; i++)
    {
      rtx object = changes[i].object;

      /* if there is no object to test or if it is the same as the one we
         already tested, ignore it.  */
      if (object == 0 || object == last_validated)
	continue;

      if (GET_CODE (object) == MEM)
	{
	  if (! memory_address_p (GET_MODE (object), XEXP (object, 0)))
	    break;
	}
      else if (insn_invalid_p (object))
	{
	  rtx pat = PATTERN (object);

	  /* Perhaps we couldn't recognize the insn because there were
	     extra CLOBBERs at the end.  If so, try to re-recognize
	     without the last CLOBBER (later iterations will cause each of
	     them to be eliminated, in turn).  But don't do this if we
	     have an ASM_OPERAND.  */
	  if (GET_CODE (pat) == PARALLEL
	      && GET_CODE (XVECEXP (pat, 0, XVECLEN (pat, 0) - 1)) == CLOBBER
	      && asm_noperands (PATTERN (object)) < 0)
	    {
	      rtx newpat;

	      if (XVECLEN (pat, 0) == 2)
		newpat = XVECEXP (pat, 0, 0);
	      else
		{
		  int j;

		  newpat
		    = gen_rtx_PARALLEL (VOIDmode,
					rtvec_alloc (XVECLEN (pat, 0) - 1));
		  for (j = 0; j < XVECLEN (newpat, 0); j++)
		    XVECEXP (newpat, 0, j) = XVECEXP (pat, 0, j);
		}

	      /* Add a new change to this group to replace the pattern
		 with this new pattern.  Then consider this change
		 as having succeeded.  The change we added will
		 cause the entire call to fail if things remain invalid.

		 Note that this can lose if a later change than the one
		 we are processing specified &XVECEXP (PATTERN (object), 0, X)
		 but this shouldn't occur.  */

	      validate_change (object, &PATTERN (object), newpat, 1);
	      continue;
	    }
	  else if (GET_CODE (pat) == USE || GET_CODE (pat) == CLOBBER)
	    /* If this insn is a CLOBBER or USE, it is always valid, but is
	       never recognized.  */
	    continue;
	  else
	    break;
	}
      last_validated = object;
    }

  if (i == num_changes)
    {
      basic_block bb;

      for (i = 0; i < num_changes; i++)
	if (changes[i].object
	    && INSN_P (changes[i].object)
	    && (bb = BLOCK_FOR_INSN (changes[i].object)))
	  bb->flags |= BB_DIRTY;

      num_changes = 0;
      return 1;
    }
  else
    {
      cancel_changes (0);
      return 0;
    }
}

/* Return the number of changes so far in the current group.  */

int
num_validated_changes ()
{
  return num_changes;
}

/* Retract the changes numbered NUM and up.  */

void
cancel_changes (num)
     int num;
{
  int i;

  /* Back out all the changes.  Do this in the opposite order in which
     they were made.  */
  for (i = num_changes - 1; i >= num; i--)
    {
      *changes[i].loc = changes[i].old;
      if (changes[i].object && GET_CODE (changes[i].object) != MEM)
	INSN_CODE (changes[i].object) = changes[i].old_code;
    }
  num_changes = num;
}

/* Replace every occurrence of FROM in X with TO.  Mark each change with
   validate_change passing OBJECT.  */

static void
validate_replace_rtx_1 (loc, from, to, object)
     rtx *loc;
     rtx from, to, object;
{
  int i, j;
  const char *fmt;
  rtx x = *loc;
  enum rtx_code code;
  enum machine_mode op0_mode = VOIDmode;
  int prev_changes = num_changes;
  rtx new;

  if (!x)
    return;

  code = GET_CODE (x);
  fmt = GET_RTX_FORMAT (code);
  if (fmt[0] == 'e')
    op0_mode = GET_MODE (XEXP (x, 0));

  /* X matches FROM if it is the same rtx or they are both referring to the
     same register in the same mode.  Avoid calling rtx_equal_p unless the
     operands look similar.  */

  if (x == from
      || (GET_CODE (x) == REG && GET_CODE (from) == REG
	  && GET_MODE (x) == GET_MODE (from)
	  && REGNO (x) == REGNO (from))
      || (GET_CODE (x) == GET_CODE (from) && GET_MODE (x) == GET_MODE (from)
	  && rtx_equal_p (x, from)))
    {
      validate_change (object, loc, to, 1);
      return;
    }

  /* Call ourself recursively to perform the replacements.  */

  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e')
	validate_replace_rtx_1 (&XEXP (x, i), from, to, object);
      else if (fmt[i] == 'E')
	for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	  validate_replace_rtx_1 (&XVECEXP (x, i, j), from, to, object);
    }

  /* If we didn't substitute, there is nothing more to do.  */
  if (num_changes == prev_changes)
    return;

  /* Allow substituted expression to have different mode.  This is used by
     regmove to change mode of pseudo register.  */
  if (fmt[0] == 'e' && GET_MODE (XEXP (x, 0)) != VOIDmode)
    op0_mode = GET_MODE (XEXP (x, 0));

  /* Do changes needed to keep rtx consistent.  Don't do any other
     simplifications, as it is not our job.  */

  if ((GET_RTX_CLASS (code) == '<' || GET_RTX_CLASS (code) == 'c')
      && swap_commutative_operands_p (XEXP (x, 0), XEXP (x, 1)))
    {
      validate_change (object, loc,
		       gen_rtx_fmt_ee (GET_RTX_CLASS (code) == 'c' ? code
				       : swap_condition (code),
				       GET_MODE (x), XEXP (x, 1),
				       XEXP (x, 0)), 1);
      x = *loc;
      code = GET_CODE (x);
    }

  switch (code)
    {
    case PLUS:
      /* If we have a PLUS whose second operand is now a CONST_INT, use
         plus_constant to try to simplify it.
         ??? We may want later to remove this, once simplification is
         separated from this function.  */
      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	validate_change (object, loc,
			 simplify_gen_binary
			 (PLUS, GET_MODE (x), XEXP (x, 0), XEXP (x, 1)), 1);
      break;
    case MINUS:
      if (GET_CODE (XEXP (x, 1)) == CONST_INT
	  || GET_CODE (XEXP (x, 1)) == CONST_DOUBLE)
	validate_change (object, loc,
			 simplify_gen_binary
			 (PLUS, GET_MODE (x), XEXP (x, 0),
			  simplify_gen_unary (NEG,
					      GET_MODE (x), XEXP (x, 1),
					      GET_MODE (x))), 1);
      break;
    case ZERO_EXTEND:
    case SIGN_EXTEND:
      if (GET_MODE (XEXP (x, 0)) == VOIDmode)
	{
	  new = simplify_gen_unary (code, GET_MODE (x), XEXP (x, 0),
				    op0_mode);
	  /* If any of the above failed, substitute in something that
	     we know won't be recognized.  */
	  if (!new)
	    new = gen_rtx_CLOBBER (GET_MODE (x), const0_rtx);
	  validate_change (object, loc, new, 1);
	}
      break;
    case SUBREG:
      /* All subregs possible to simplify should be simplified.  */
      new = simplify_subreg (GET_MODE (x), SUBREG_REG (x), op0_mode,
			     SUBREG_BYTE (x));

      /* Subregs of VOIDmode operands are incorrect.  */
      if (!new && GET_MODE (SUBREG_REG (x)) == VOIDmode)
	new = gen_rtx_CLOBBER (GET_MODE (x), const0_rtx);
      if (new)
	validate_change (object, loc, new, 1);
      break;
    case ZERO_EXTRACT:
    case SIGN_EXTRACT:
      /* If we are replacing a register with memory, try to change the memory
         to be the mode required for memory in extract operations (this isn't
         likely to be an insertion operation; if it was, nothing bad will
         happen, we might just fail in some cases).  */

      if (GET_CODE (XEXP (x, 0)) == MEM
	  && GET_CODE (XEXP (x, 1)) == CONST_INT
	  && GET_CODE (XEXP (x, 2)) == CONST_INT
	  && !mode_dependent_address_p (XEXP (XEXP (x, 0), 0))
	  && !MEM_VOLATILE_P (XEXP (x, 0)))
	{
	  enum machine_mode wanted_mode = VOIDmode;
	  enum machine_mode is_mode = GET_MODE (XEXP (x, 0));
	  int pos = INTVAL (XEXP (x, 2));

	  if (GET_CODE (x) == ZERO_EXTRACT)
	    {
	      enum machine_mode new_mode
		= mode_for_extraction (EP_extzv, 1);
	      if (new_mode != MAX_MACHINE_MODE)
		wanted_mode = new_mode;
	    }
	  else if (GET_CODE (x) == SIGN_EXTRACT)
	    {
	      enum machine_mode new_mode
		= mode_for_extraction (EP_extv, 1);
	      if (new_mode != MAX_MACHINE_MODE)
		wanted_mode = new_mode;
	    }

	  /* If we have a narrower mode, we can do something.  */
	  if (wanted_mode != VOIDmode
	      && GET_MODE_SIZE (wanted_mode) < GET_MODE_SIZE (is_mode))
	    {
	      int offset = pos / BITS_PER_UNIT;
	      rtx newmem;

	      /* If the bytes and bits are counted differently, we
	         must adjust the offset.  */
	      if (BYTES_BIG_ENDIAN != BITS_BIG_ENDIAN)
		offset =
		  (GET_MODE_SIZE (is_mode) - GET_MODE_SIZE (wanted_mode) -
		   offset);

	      pos %= GET_MODE_BITSIZE (wanted_mode);

	      newmem = adjust_address_nv (XEXP (x, 0), wanted_mode, offset);

	      validate_change (object, &XEXP (x, 2), GEN_INT (pos), 1);
	      validate_change (object, &XEXP (x, 0), newmem, 1);
	    }
	}

      break;

    default:
      break;
    }
}

/* Try replacing every occurrence of FROM in subexpression LOC of INSN
   with TO.  After all changes have been made, validate by seeing
   if INSN is still valid.  */

int
validate_replace_rtx_subexp (from, to, insn, loc)
     rtx from, to, insn, *loc;
{
  validate_replace_rtx_1 (loc, from, to, insn);
  return apply_change_group ();
}

/* Try replacing every occurrence of FROM in INSN with TO.  After all
   changes have been made, validate by seeing if INSN is still valid.  */

int
validate_replace_rtx (from, to, insn)
     rtx from, to, insn;
{
  validate_replace_rtx_1 (&PATTERN (insn), from, to, insn);
  return apply_change_group ();
}

/* Try replacing every occurrence of FROM in INSN with TO.  */

void
validate_replace_rtx_group (from, to, insn)
     rtx from, to, insn;
{
  validate_replace_rtx_1 (&PATTERN (insn), from, to, insn);
}

/* Function called by note_uses to replace used subexpressions.  */
struct validate_replace_src_data
{
  rtx from;			/* Old RTX */
  rtx to;			/* New RTX */
  rtx insn;			/* Insn in which substitution is occurring.  */
};

static void
validate_replace_src_1 (x, data)
     rtx *x;
     void *data;
{
  struct validate_replace_src_data *d
    = (struct validate_replace_src_data *) data;

  validate_replace_rtx_1 (x, d->from, d->to, d->insn);
}

/* Try replacing every occurrence of FROM in INSN with TO, avoiding
   SET_DESTs.  */

void
validate_replace_src_group (from, to, insn)
     rtx from, to, insn;
{
  struct validate_replace_src_data d;

  d.from = from;
  d.to = to;
  d.insn = insn;
  note_uses (&PATTERN (insn), validate_replace_src_1, &d);
}

/* Same as validate_repalace_src_group, but validate by seeing if
   INSN is still valid.  */
int
validate_replace_src (from, to, insn)
     rtx from, to, insn;
{
  validate_replace_src_group (from, to, insn);
  return apply_change_group ();
}

#ifdef HAVE_cc0
/* Return 1 if the insn using CC0 set by INSN does not contain
   any ordered tests applied to the condition codes.
   EQ and NE tests do not count.  */

int
next_insn_tests_no_inequality (insn)
     rtx insn;
{
  rtx next = next_cc0_user (insn);

  /* If there is no next insn, we have to take the conservative choice.  */
  if (next == 0)
    return 0;

  return ((GET_CODE (next) == JUMP_INSN
	   || GET_CODE (next) == INSN
	   || GET_CODE (next) == CALL_INSN)
	  && ! inequality_comparisons_p (PATTERN (next)));
}

#if 0  /* This is useless since the insn that sets the cc's
	  must be followed immediately by the use of them.  */
/* Return 1 if the CC value set up by INSN is not used.  */

int
next_insns_test_no_inequality (insn)
     rtx insn;
{
  rtx next = NEXT_INSN (insn);

  for (; next != 0; next = NEXT_INSN (next))
    {
      if (GET_CODE (next) == CODE_LABEL
	  || GET_CODE (next) == BARRIER)
	return 1;
      if (GET_CODE (next) == NOTE)
	continue;
      if (inequality_comparisons_p (PATTERN (next)))
	return 0;
      if (sets_cc0_p (PATTERN (next)) == 1)
	return 1;
      if (! reg_mentioned_p (cc0_rtx, PATTERN (next)))
	return 1;
    }
  return 1;
}
#endif
#endif

/* This is used by find_single_use to locate an rtx that contains exactly one
   use of DEST, which is typically either a REG or CC0.  It returns a
   pointer to the innermost rtx expression containing DEST.  Appearances of
   DEST that are being used to totally replace it are not counted.  */

static rtx *
find_single_use_1 (dest, loc)
     rtx dest;
     rtx *loc;
{
  rtx x = *loc;
  enum rtx_code code = GET_CODE (x);
  rtx *result = 0;
  rtx *this_result;
  int i;
  const char *fmt;

  switch (code)
    {
    case CONST_INT:
    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CLOBBER:
      return 0;

    case SET:
      /* If the destination is anything other than CC0, PC, a REG or a SUBREG
	 of a REG that occupies all of the REG, the insn uses DEST if
	 it is mentioned in the destination or the source.  Otherwise, we
	 need just check the source.  */
      if (GET_CODE (SET_DEST (x)) != CC0
	  && GET_CODE (SET_DEST (x)) != PC
	  && GET_CODE (SET_DEST (x)) != REG
	  && ! (GET_CODE (SET_DEST (x)) == SUBREG
		&& GET_CODE (SUBREG_REG (SET_DEST (x))) == REG
		&& (((GET_MODE_SIZE (GET_MODE (SUBREG_REG (SET_DEST (x))))
		      + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD)
		    == ((GET_MODE_SIZE (GET_MODE (SET_DEST (x)))
			 + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD))))
	break;

      return find_single_use_1 (dest, &SET_SRC (x));

    case MEM:
    case SUBREG:
      return find_single_use_1 (dest, &XEXP (x, 0));

    default:
      break;
    }

  /* If it wasn't one of the common cases above, check each expression and
     vector of this code.  Look for a unique usage of DEST.  */

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e')
	{
	  if (dest == XEXP (x, i)
	      || (GET_CODE (dest) == REG && GET_CODE (XEXP (x, i)) == REG
		  && REGNO (dest) == REGNO (XEXP (x, i))))
	    this_result = loc;
	  else
	    this_result = find_single_use_1 (dest, &XEXP (x, i));

	  if (result == 0)
	    result = this_result;
	  else if (this_result)
	    /* Duplicate usage.  */
	    return 0;
	}
      else if (fmt[i] == 'E')
	{
	  int j;

	  for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	    {
	      if (XVECEXP (x, i, j) == dest
		  || (GET_CODE (dest) == REG
		      && GET_CODE (XVECEXP (x, i, j)) == REG
		      && REGNO (XVECEXP (x, i, j)) == REGNO (dest)))
		this_result = loc;
	      else
		this_result = find_single_use_1 (dest, &XVECEXP (x, i, j));

	      if (result == 0)
		result = this_result;
	      else if (this_result)
		return 0;
	    }
	}
    }

  return result;
}

/* See if DEST, produced in INSN, is used only a single time in the
   sequel.  If so, return a pointer to the innermost rtx expression in which
   it is used.

   If PLOC is non-zero, *PLOC is set to the insn containing the single use.

   This routine will return usually zero either before flow is called (because
   there will be no LOG_LINKS notes) or after reload (because the REG_DEAD
   note can't be trusted).

   If DEST is cc0_rtx, we look only at the next insn.  In that case, we don't
   care about REG_DEAD notes or LOG_LINKS.

   Otherwise, we find the single use by finding an insn that has a
   LOG_LINKS pointing at INSN and has a REG_DEAD note for DEST.  If DEST is
   only referenced once in that insn, we know that it must be the first
   and last insn referencing DEST.  */

rtx *
find_single_use (dest, insn, ploc)
     rtx dest;
     rtx insn;
     rtx *ploc;
{
  rtx next;
  rtx *result;
  rtx link;

#ifdef HAVE_cc0
  if (dest == cc0_rtx)
    {
      next = NEXT_INSN (insn);
      if (next == 0
	  || (GET_CODE (next) != INSN && GET_CODE (next) != JUMP_INSN))
	return 0;

      result = find_single_use_1 (dest, &PATTERN (next));
      if (result && ploc)
	*ploc = next;
      return result;
    }
#endif

  if (reload_completed || reload_in_progress || GET_CODE (dest) != REG)
    return 0;

  for (next = next_nonnote_insn (insn);
       next != 0 && GET_CODE (next) != CODE_LABEL;
       next = next_nonnote_insn (next))
    if (INSN_P (next) && dead_or_set_p (next, dest))
      {
	for (link = LOG_LINKS (next); link; link = XEXP (link, 1))
	  if (XEXP (link, 0) == insn)
	    break;

	if (link)
	  {
	    result = find_single_use_1 (dest, &PATTERN (next));
	    if (ploc)
	      *ploc = next;
	    return result;
	  }
      }

  return 0;
}

/* Return 1 if OP is a valid general operand for machine mode MODE.
   This is either a register reference, a memory reference,
   or a constant.  In the case of a memory reference, the address
   is checked for general validity for the target machine.

   Register and memory references must have mode MODE in order to be valid,
   but some constants have no machine mode and are valid for any mode.

   If MODE is VOIDmode, OP is checked for validity for whatever mode
   it has.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.

   For an explanation of this function's behavior for registers of
   class NO_REGS, see the comment for `register_operand'.  */

int
general_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  enum rtx_code code = GET_CODE (op);

  if (mode == VOIDmode)
    mode = GET_MODE (op);

  /* Don't accept CONST_INT or anything similar
     if the caller wants something floating.  */
  if (GET_MODE (op) == VOIDmode && mode != VOIDmode
      && GET_MODE_CLASS (mode) != MODE_INT
      && GET_MODE_CLASS (mode) != MODE_PARTIAL_INT)
    return 0;

  if (GET_CODE (op) == CONST_INT
      && mode != VOIDmode
      && trunc_int_for_mode (INTVAL (op), mode) != INTVAL (op))
    return 0;

  if (CONSTANT_P (op))
    return ((GET_MODE (op) == VOIDmode || GET_MODE (op) == mode
	     || mode == VOIDmode)
#ifdef LEGITIMATE_PIC_OPERAND_P
	    && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op))
#endif
	    && LEGITIMATE_CONSTANT_P (op));

  /* Except for certain constants with VOIDmode, already checked for,
     OP's mode must match MODE if MODE specifies a mode.  */

  if (GET_MODE (op) != mode)
    return 0;

  if (code == SUBREG)
    {
      rtx sub = SUBREG_REG (op);

#ifdef INSN_SCHEDULING
      /* On machines that have insn scheduling, we want all memory
	 reference to be explicit, so outlaw paradoxical SUBREGs.  */
      if (GET_CODE (sub) == MEM
	  && GET_MODE_SIZE (mode) > GET_MODE_SIZE (GET_MODE (sub)))
	return 0;
#endif
      /* Avoid memories with nonzero SUBREG_BYTE, as offsetting the memory
         may result in incorrect reference.  We should simplify all valid
         subregs of MEM anyway.  But allow this after reload because we
	 might be called from cleanup_subreg_operands.

	 ??? This is a kludge.  */
      if (!reload_completed && SUBREG_BYTE (op) != 0
	  && GET_CODE (sub) == MEM)
	return 0;

      /* FLOAT_MODE subregs can't be paradoxical.  Combine will occasionally
 	 create such rtl, and we must reject it.  */
      if (GET_MODE_CLASS (GET_MODE (op)) == MODE_FLOAT
	  && GET_MODE_SIZE (GET_MODE (op)) > GET_MODE_SIZE (GET_MODE (sub)))
	return 0;

      op = sub;
      code = GET_CODE (op);
    }

  if (code == REG)
    /* A register whose class is NO_REGS is not a general operand.  */
    return (REGNO (op) >= FIRST_PSEUDO_REGISTER
	    || REGNO_REG_CLASS (REGNO (op)) != NO_REGS);

  if (code == MEM)
    {
      rtx y = XEXP (op, 0);

      if (! volatile_ok && MEM_VOLATILE_P (op))
	return 0;

      if (GET_CODE (y) == ADDRESSOF)
	return 1;

      /* Use the mem's mode, since it will be reloaded thus.  */
      mode = GET_MODE (op);
      GO_IF_LEGITIMATE_ADDRESS (mode, y, win);
    }

  /* Pretend this is an operand for now; we'll run force_operand
     on its replacement in fixup_var_refs_1.  */
  if (code == ADDRESSOF)
    return 1;

  return 0;

 win:
  return 1;
}

/* Return 1 if OP is a valid memory address for a memory reference
   of mode MODE.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.  */

int
address_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return memory_address_p (mode, op);
}

/* Return 1 if OP is a register reference of mode MODE.
   If MODE is VOIDmode, accept a register in any mode.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.

   As a special exception, registers whose class is NO_REGS are
   not accepted by `register_operand'.  The reason for this change
   is to allow the representation of special architecture artifacts
   (such as a condition code register) without extending the rtl
   definitions.  Since registers of class NO_REGS cannot be used
   as registers in any case where register classes are examined,
   it is most consistent to keep this function from accepting them.  */

int
register_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_MODE (op) != mode && mode != VOIDmode)
    return 0;

  if (GET_CODE (op) == SUBREG)
    {
      rtx sub = SUBREG_REG (op);

      /* Before reload, we can allow (SUBREG (MEM...)) as a register operand
	 because it is guaranteed to be reloaded into one.
	 Just make sure the MEM is valid in itself.
	 (Ideally, (SUBREG (MEM)...) should not exist after reload,
	 but currently it does result from (SUBREG (REG)...) where the
	 reg went on the stack.)  */
      if (! reload_completed && GET_CODE (sub) == MEM)
	return general_operand (op, mode);

#ifdef CLASS_CANNOT_CHANGE_MODE
      if (GET_CODE (sub) == REG
	  && REGNO (sub) < FIRST_PSEUDO_REGISTER
	  && (TEST_HARD_REG_BIT
	      (reg_class_contents[(int) CLASS_CANNOT_CHANGE_MODE],
	       REGNO (sub)))
	  && CLASS_CANNOT_CHANGE_MODE_P (mode, GET_MODE (sub))
	  && GET_MODE_CLASS (GET_MODE (sub)) != MODE_COMPLEX_INT
	  && GET_MODE_CLASS (GET_MODE (sub)) != MODE_COMPLEX_FLOAT)
	return 0;
#endif

      /* FLOAT_MODE subregs can't be paradoxical.  Combine will occasionally
	 create such rtl, and we must reject it.  */
      if (GET_MODE_CLASS (GET_MODE (op)) == MODE_FLOAT
	  && GET_MODE_SIZE (GET_MODE (op)) > GET_MODE_SIZE (GET_MODE (sub)))
	return 0;

      op = sub;
    }

  /* If we have an ADDRESSOF, consider it valid since it will be
     converted into something that will not be a MEM.  */
  if (GET_CODE (op) == ADDRESSOF)
    return 1;

  /* We don't consider registers whose class is NO_REGS
     to be a register operand.  */
  return (GET_CODE (op) == REG
	  && (REGNO (op) >= FIRST_PSEUDO_REGISTER
	      || REGNO_REG_CLASS (REGNO (op)) != NO_REGS));
}

/* Return 1 for a register in Pmode; ignore the tested mode.  */

int
pmode_register_operand (op, mode)
     rtx op;
     enum machine_mode mode ATTRIBUTE_UNUSED;
{
  return register_operand (op, Pmode);
}

/* Return 1 if OP should match a MATCH_SCRATCH, i.e., if it is a SCRATCH
   or a hard register.  */

int
scratch_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_MODE (op) != mode && mode != VOIDmode)
    return 0;

  return (GET_CODE (op) == SCRATCH
	  || (GET_CODE (op) == REG
	      && REGNO (op) < FIRST_PSEUDO_REGISTER));
}

/* Return 1 if OP is a valid immediate operand for mode MODE.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.  */

int
immediate_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  /* Don't accept CONST_INT or anything similar
     if the caller wants something floating.  */
  if (GET_MODE (op) == VOIDmode && mode != VOIDmode
      && GET_MODE_CLASS (mode) != MODE_INT
      && GET_MODE_CLASS (mode) != MODE_PARTIAL_INT)
    return 0;

  if (GET_CODE (op) == CONST_INT
      && mode != VOIDmode
      && trunc_int_for_mode (INTVAL (op), mode) != INTVAL (op))
    return 0;

  /* Accept CONSTANT_P_RTX, since it will be gone by CSE1 and
     result in 0/1.  It seems a safe assumption that this is
     in range for everyone.  */
  if (GET_CODE (op) == CONSTANT_P_RTX)
    return 1;

  return (CONSTANT_P (op)
	  && (GET_MODE (op) == mode || mode == VOIDmode
	      || GET_MODE (op) == VOIDmode)
#ifdef LEGITIMATE_PIC_OPERAND_P
	  && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op))
#endif
	  && LEGITIMATE_CONSTANT_P (op));
}

/* Returns 1 if OP is an operand that is a CONST_INT.  */

int
const_int_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE (op) != CONST_INT)
    return 0;

  if (mode != VOIDmode
      && trunc_int_for_mode (INTVAL (op), mode) != INTVAL (op))
    return 0;

  return 1;
}

/* Returns 1 if OP is an operand that is a constant integer or constant
   floating-point number.  */

int
const_double_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  /* Don't accept CONST_INT or anything similar
     if the caller wants something floating.  */
  if (GET_MODE (op) == VOIDmode && mode != VOIDmode
      && GET_MODE_CLASS (mode) != MODE_INT
      && GET_MODE_CLASS (mode) != MODE_PARTIAL_INT)
    return 0;

  return ((GET_CODE (op) == CONST_DOUBLE || GET_CODE (op) == CONST_INT)
	  && (mode == VOIDmode || GET_MODE (op) == mode
	      || GET_MODE (op) == VOIDmode));
}

/* Return 1 if OP is a general operand that is not an immediate operand.  */

int
nonimmediate_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return (general_operand (op, mode) && ! CONSTANT_P (op));
}

/* Return 1 if OP is a register reference or immediate value of mode MODE.  */

int
nonmemory_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (CONSTANT_P (op))
    {
      /* Don't accept CONST_INT or anything similar
	 if the caller wants something floating.  */
      if (GET_MODE (op) == VOIDmode && mode != VOIDmode
	  && GET_MODE_CLASS (mode) != MODE_INT
	  && GET_MODE_CLASS (mode) != MODE_PARTIAL_INT)
	return 0;

      if (GET_CODE (op) == CONST_INT
	  && mode != VOIDmode
	  && trunc_int_for_mode (INTVAL (op), mode) != INTVAL (op))
	return 0;

      return ((GET_MODE (op) == VOIDmode || GET_MODE (op) == mode
	       || mode == VOIDmode)
#ifdef LEGITIMATE_PIC_OPERAND_P
	      && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op))
#endif
	      && LEGITIMATE_CONSTANT_P (op));
    }

  if (GET_MODE (op) != mode && mode != VOIDmode)
    return 0;

  if (GET_CODE (op) == SUBREG)
    {
      /* Before reload, we can allow (SUBREG (MEM...)) as a register operand
	 because it is guaranteed to be reloaded into one.
	 Just make sure the MEM is valid in itself.
	 (Ideally, (SUBREG (MEM)...) should not exist after reload,
	 but currently it does result from (SUBREG (REG)...) where the
	 reg went on the stack.)  */
      if (! reload_completed && GET_CODE (SUBREG_REG (op)) == MEM)
	return general_operand (op, mode);
      op = SUBREG_REG (op);
    }

  /* We don't consider registers whose class is NO_REGS
     to be a register operand.  */
  return (GET_CODE (op) == REG
	  && (REGNO (op) >= FIRST_PSEUDO_REGISTER
	      || REGNO_REG_CLASS (REGNO (op)) != NO_REGS));
}

/* Return 1 if OP is a valid operand that stands for pushing a
   value of mode MODE onto the stack.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.  */

int
push_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  unsigned int rounded_size = GET_MODE_SIZE (mode);

#ifdef PUSH_ROUNDING
  rounded_size = PUSH_ROUNDING (rounded_size);
#endif

  if (GET_CODE (op) != MEM)
    return 0;

  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;

  op = XEXP (op, 0);

  if (rounded_size == GET_MODE_SIZE (mode))
    {
      if (GET_CODE (op) != STACK_PUSH_CODE)
	return 0;
    }
  else
    {
      if (GET_CODE (op) != PRE_MODIFY
	  || GET_CODE (XEXP (op, 1)) != PLUS
	  || XEXP (XEXP (op, 1), 0) != XEXP (op, 0)
	  || GET_CODE (XEXP (XEXP (op, 1), 1)) != CONST_INT
#ifdef STACK_GROWS_DOWNWARD
	  || INTVAL (XEXP (XEXP (op, 1), 1)) != - (int) rounded_size
#else
	  || INTVAL (XEXP (XEXP (op, 1), 1)) != rounded_size
#endif
	  )
	return 0;
    }

  return XEXP (op, 0) == stack_pointer_rtx;
}

/* Return 1 if OP is a valid operand that stands for popping a
   value of mode MODE off the stack.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.  */

int
pop_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE (op) != MEM)
    return 0;

  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;

  op = XEXP (op, 0);

  if (GET_CODE (op) != STACK_POP_CODE)
    return 0;

  return XEXP (op, 0) == stack_pointer_rtx;
}

/* Return 1 if ADDR is a valid memory address for mode MODE.  */

int
memory_address_p (mode, addr)
     enum machine_mode mode ATTRIBUTE_UNUSED;
     rtx addr;
{
  if (GET_CODE (addr) == ADDRESSOF)
    return 1;

  GO_IF_LEGITIMATE_ADDRESS (mode, addr, win);
  return 0;

 win:
  return 1;
}

/* Return 1 if OP is a valid memory reference with mode MODE,
   including a valid address.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.  */

int
memory_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  rtx inner;

  if (! reload_completed)
    /* Note that no SUBREG is a memory operand before end of reload pass,
       because (SUBREG (MEM...)) forces reloading into a register.  */
    return GET_CODE (op) == MEM && general_operand (op, mode);

  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;

  inner = op;
  if (GET_CODE (inner) == SUBREG)
    inner = SUBREG_REG (inner);

  return (GET_CODE (inner) == MEM && general_operand (op, mode));
}

/* Return 1 if OP is a valid indirect memory reference with mode MODE;
   that is, a memory reference whose address is a general_operand.  */

int
indirect_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  /* Before reload, a SUBREG isn't in memory (see memory_operand, above).  */
  if (! reload_completed
      && GET_CODE (op) == SUBREG && GET_CODE (SUBREG_REG (op)) == MEM)
    {
      int offset = SUBREG_BYTE (op);
      rtx inner = SUBREG_REG (op);

      if (mode != VOIDmode && GET_MODE (op) != mode)
	return 0;

      /* The only way that we can have a general_operand as the resulting
	 address is if OFFSET is zero and the address already is an operand
	 or if the address is (plus Y (const_int -OFFSET)) and Y is an
	 operand.  */

      return ((offset == 0 && general_operand (XEXP (inner, 0), Pmode))
	      || (GET_CODE (XEXP (inner, 0)) == PLUS
		  && GET_CODE (XEXP (XEXP (inner, 0), 1)) == CONST_INT
		  && INTVAL (XEXP (XEXP (inner, 0), 1)) == -offset
		  && general_operand (XEXP (XEXP (inner, 0), 0), Pmode)));
    }

  return (GET_CODE (op) == MEM
	  && memory_operand (op, mode)
	  && general_operand (XEXP (op, 0), Pmode));
}

/* Return 1 if this is a comparison operator.  This allows the use of
   MATCH_OPERATOR to recognize all the branch insns.  */

int
comparison_operator (op, mode)
    rtx op;
    enum machine_mode mode;
{
  return ((mode == VOIDmode || GET_MODE (op) == mode)
	  && GET_RTX_CLASS (GET_CODE (op)) == '<');
}

/* If BODY is an insn body that uses ASM_OPERANDS,
   return the number of operands (both input and output) in the insn.
   Otherwise return -1.  */

int
asm_noperands (body)
     rtx body;
{
  switch (GET_CODE (body))
    {
    case ASM_OPERANDS:
      /* No output operands: return number of input operands.  */
      return ASM_OPERANDS_INPUT_LENGTH (body);
    case SET:
      if (GET_CODE (SET_SRC (body)) == ASM_OPERANDS)
	/* Single output operand: BODY is (set OUTPUT (asm_operands ...)).  */
	return ASM_OPERANDS_INPUT_LENGTH (SET_SRC (body)) + 1;
      else
	return -1;
    case PARALLEL:
      if (GET_CODE (XVECEXP (body, 0, 0)) == SET
	  && GET_CODE (SET_SRC (XVECEXP (body, 0, 0))) == ASM_OPERANDS)
	{
	  /* Multiple output operands, or 1 output plus some clobbers:
	     body is [(set OUTPUT (asm_operands ...))... (clobber (reg ...))...].  */
	  int i;
	  int n_sets;

	  /* Count backwards through CLOBBERs to determine number of SETs.  */
	  for (i = XVECLEN (body, 0); i > 0; i--)
	    {
	      if (GET_CODE (XVECEXP (body, 0, i - 1)) == SET)
		break;
	      if (GET_CODE (XVECEXP (body, 0, i - 1)) != CLOBBER)
		return -1;
	    }

	  /* N_SETS is now number of output operands.  */
	  n_sets = i;

	  /* Verify that all the SETs we have
	     came from a single original asm_operands insn
	     (so that invalid combinations are blocked).  */
	  for (i = 0; i < n_sets; i++)
	    {
	      rtx elt = XVECEXP (body, 0, i);
	      if (GET_CODE (elt) != SET)
		return -1;
	      if (GET_CODE (SET_SRC (elt)) != ASM_OPERANDS)
		return -1;
	      /* If these ASM_OPERANDS rtx's came from different original insns
	         then they aren't allowed together.  */
	      if (ASM_OPERANDS_INPUT_VEC (SET_SRC (elt))
		  != ASM_OPERANDS_INPUT_VEC (SET_SRC (XVECEXP (body, 0, 0))))
		return -1;
	    }
	  return (ASM_OPERANDS_INPUT_LENGTH (SET_SRC (XVECEXP (body, 0, 0)))
		  + n_sets);
	}
      else if (GET_CODE (XVECEXP (body, 0, 0)) == ASM_OPERANDS)
	{
	  /* 0 outputs, but some clobbers:
	     body is [(asm_operands ...) (clobber (reg ...))...].  */
	  int i;

	  /* Make sure all the other parallel things really are clobbers.  */
	  for (i = XVECLEN (body, 0) - 1; i > 0; i--)
	    if (GET_CODE (XVECEXP (body, 0, i)) != CLOBBER)
	      return -1;

	  return ASM_OPERANDS_INPUT_LENGTH (XVECEXP (body, 0, 0));
	}
      else
	return -1;
    default:
      return -1;
    }
}

/* Assuming BODY is an insn body that uses ASM_OPERANDS,
   copy its operands (both input and output) into the vector OPERANDS,
   the locations of the operands within the insn into the vector OPERAND_LOCS,
   and the constraints for the operands into CONSTRAINTS.
   Write the modes of the operands into MODES.
   Return the assembler-template.

   If MODES, OPERAND_LOCS, CONSTRAINTS or OPERANDS is 0,
   we don't store that info.  */

const char *
decode_asm_operands (body, operands, operand_locs, constraints, modes)
     rtx body;
     rtx *operands;
     rtx **operand_locs;
     const char **constraints;
     enum machine_mode *modes;
{
  int i;
  int noperands;
  const char *template = 0;

  if (GET_CODE (body) == SET && GET_CODE (SET_SRC (body)) == ASM_OPERANDS)
    {
      rtx asmop = SET_SRC (body);
      /* Single output operand: BODY is (set OUTPUT (asm_operands ....)).  */

      noperands = ASM_OPERANDS_INPUT_LENGTH (asmop) + 1;

      for (i = 1; i < noperands; i++)
	{
	  if (operand_locs)
	    operand_locs[i] = &ASM_OPERANDS_INPUT (asmop, i - 1);
	  if (operands)
	    operands[i] = ASM_OPERANDS_INPUT (asmop, i - 1);
	  if (constraints)
	    constraints[i] = ASM_OPERANDS_INPUT_CONSTRAINT (asmop, i - 1);
	  if (modes)
	    modes[i] = ASM_OPERANDS_INPUT_MODE (asmop, i - 1);
	}

      /* The output is in the SET.
	 Its constraint is in the ASM_OPERANDS itself.  */
      if (operands)
	operands[0] = SET_DEST (body);
      if (operand_locs)
	operand_locs[0] = &SET_DEST (body);
      if (constraints)
	constraints[0] = ASM_OPERANDS_OUTPUT_CONSTRAINT (asmop);
      if (modes)
	modes[0] = GET_MODE (SET_DEST (body));
      template = ASM_OPERANDS_TEMPLATE (asmop);
    }
  else if (GET_CODE (body) == ASM_OPERANDS)
    {
      rtx asmop = body;
      /* No output operands: BODY is (asm_operands ....).  */

      noperands = ASM_OPERANDS_INPUT_LENGTH (asmop);

      /* The input operands are found in the 1st element vector.  */
      /* Constraints for inputs are in the 2nd element vector.  */
      for (i = 0; i < noperands; i++)
	{
	  if (operand_locs)
	    operand_locs[i] = &ASM_OPERANDS_INPUT (asmop, i);
	  if (operands)
	    operands[i] = ASM_OPERANDS_INPUT (asmop, i);
	  if (constraints)
	    constraints[i] = ASM_OPERANDS_INPUT_CONSTRAINT (asmop, i);
	  if (modes)
	    modes[i] = ASM_OPERANDS_INPUT_MODE (asmop, i);
	}
      template = ASM_OPERANDS_TEMPLATE (asmop);
    }
  else if (GET_CODE (body) == PARALLEL
	   && GET_CODE (XVECEXP (body, 0, 0)) == SET
	   && GET_CODE (SET_SRC (XVECEXP (body, 0, 0))) == ASM_OPERANDS)
    {
      rtx asmop = SET_SRC (XVECEXP (body, 0, 0));
      int nparallel = XVECLEN (body, 0); /* Includes CLOBBERs.  */
      int nin = ASM_OPERANDS_INPUT_LENGTH (asmop);
      int nout = 0;		/* Does not include CLOBBERs.  */

      /* At least one output, plus some CLOBBERs.  */

      /* The outputs are in the SETs.
	 Their constraints are in the ASM_OPERANDS itself.  */
      for (i = 0; i < nparallel; i++)
	{
	  if (GET_CODE (XVECEXP (body, 0, i)) == CLOBBER)
	    break;		/* Past last SET */

	  if (operands)
	    operands[i] = SET_DEST (XVECEXP (body, 0, i));
	  if (operand_locs)
	    operand_locs[i] = &SET_DEST (XVECEXP (body, 0, i));
	  if (constraints)
	    constraints[i] = XSTR (SET_SRC (XVECEXP (body, 0, i)), 1);
	  if (modes)
	    modes[i] = GET_MODE (SET_DEST (XVECEXP (body, 0, i)));
	  nout++;
	}

      for (i = 0; i < nin; i++)
	{
	  if (operand_locs)
	    operand_locs[i + nout] = &ASM_OPERANDS_INPUT (asmop, i);
	  if (operands)
	    operands[i + nout] = ASM_OPERANDS_INPUT (asmop, i);
	  if (constraints)
	    constraints[i + nout] = ASM_OPERANDS_INPUT_CONSTRAINT (asmop, i);
	  if (modes)
	    modes[i + nout] = ASM_OPERANDS_INPUT_MODE (asmop, i);
	}

      template = ASM_OPERANDS_TEMPLATE (asmop);
    }
  else if (GET_CODE (body) == PARALLEL
	   && GET_CODE (XVECEXP (body, 0, 0)) == ASM_OPERANDS)
    {
      /* No outputs, but some CLOBBERs.  */

      rtx asmop = XVECEXP (body, 0, 0);
      int nin = ASM_OPERANDS_INPUT_LENGTH (asmop);

      for (i = 0; i < nin; i++)
	{
	  if (operand_locs)
	    operand_locs[i] = &ASM_OPERANDS_INPUT (asmop, i);
	  if (operands)
	    operands[i] = ASM_OPERANDS_INPUT (asmop, i);
	  if (constraints)
	    constraints[i] = ASM_OPERANDS_INPUT_CONSTRAINT (asmop, i);
	  if (modes)
	    modes[i] = ASM_OPERANDS_INPUT_MODE (asmop, i);
	}

      template = ASM_OPERANDS_TEMPLATE (asmop);
    }

  return template;
}

/* Check if an asm_operand matches it's constraints.
   Return > 0 if ok, = 0 if bad, < 0 if inconclusive.  */

int
asm_operand_ok (op, constraint)
     rtx op;
     const char *constraint;
{
  int result = 0;

  /* Use constrain_operands after reload.  */
  if (reload_completed)
    abort ();

  while (*constraint)
    {
      char c = *constraint++;
      switch (c)
	{
	case '=':
	case '+':
	case '*':
	case '%':
	case '?':
	case '!':
	case '#':
	case '&':
	case ',':
	  break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  /* For best results, our caller should have given us the
	     proper matching constraint, but we can't actually fail
	     the check if they didn't.  Indicate that results are
	     inconclusive.  */
	  while (ISDIGIT (*constraint))
	    constraint++;
	  result = -1;
	  break;

	case 'p':
	  if (address_operand (op, VOIDmode))
	    return 1;
	  break;

	case 'm':
	case 'V': /* non-offsettable */
	  if (memory_operand (op, VOIDmode))
	    return 1;
	  break;

	case 'o': /* offsettable */
	  if (offsettable_nonstrict_memref_p (op))
	    return 1;
	  break;

	case '<':
	  /* ??? Before flow, auto inc/dec insns are not supposed to exist,
	     excepting those that expand_call created.  Further, on some
	     machines which do not have generalized auto inc/dec, an inc/dec
	     is not a memory_operand.

	     Match any memory and hope things are resolved after reload.  */

	  if (GET_CODE (op) == MEM
	      && (1
		  || GET_CODE (XEXP (op, 0)) == PRE_DEC
		  || GET_CODE (XEXP (op, 0)) == POST_DEC))
	    return 1;
	  break;

	case '>':
	  if (GET_CODE (op) == MEM
	      && (1
		  || GET_CODE (XEXP (op, 0)) == PRE_INC
		  || GET_CODE (XEXP (op, 0)) == POST_INC))
	    return 1;
	  break;

	case 'E':
	case 'F':
	  if (GET_CODE (op) == CONST_DOUBLE
	      || (GET_CODE (op) == CONST_VECTOR
		  && GET_MODE_CLASS (GET_MODE (op)) == MODE_VECTOR_FLOAT))
	    return 1;
	  break;

	case 'G':
	  if (GET_CODE (op) == CONST_DOUBLE
	      && CONST_DOUBLE_OK_FOR_LETTER_P (op, 'G'))
	    return 1;
	  break;
	case 'H':
	  if (GET_CODE (op) == CONST_DOUBLE
	      && CONST_DOUBLE_OK_FOR_LETTER_P (op, 'H'))
	    return 1;
	  break;

	case 's':
	  if (GET_CODE (op) == CONST_INT
	      || (GET_CODE (op) == CONST_DOUBLE
		  && GET_MODE (op) == VOIDmode))
	    break;
	  /* FALLTHRU */

	case 'i':
	  if (CONSTANT_P (op)
#ifdef LEGITIMATE_PIC_OPERAND_P
	      && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op))
#endif
	      )
	    return 1;
	  break;

	case 'n':
	  if (GET_CODE (op) == CONST_INT
	      || (GET_CODE (op) == CONST_DOUBLE
		  && GET_MODE (op) == VOIDmode))
	    return 1;
	  break;

	case 'I':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'I'))
	    return 1;
	  break;
	case 'J':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'J'))
	    return 1;
	  break;
	case 'K':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'K'))
	    return 1;
	  break;
	case 'L':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'L'))
	    return 1;
	  break;
	case 'M':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'M'))
	    return 1;
	  break;
	case 'N':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'N'))
	    return 1;
	  break;
	case 'O':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'O'))
	    return 1;
	  break;
	case 'P':
	  if (GET_CODE (op) == CONST_INT
	      && CONST_OK_FOR_LETTER_P (INTVAL (op), 'P'))
	    return 1;
	  break;

	case 'X':
	  return 1;

	case 'g':
	  if (general_operand (op, VOIDmode))
	    return 1;
	  break;

	default:
	  /* For all other letters, we first check for a register class,
	     otherwise it is an EXTRA_CONSTRAINT.  */
	  if (REG_CLASS_FROM_LETTER (c) != NO_REGS)
	    {
	    case 'r':
	      if (GET_MODE (op) == BLKmode)
		break;
	      if (register_operand (op, VOIDmode))
		return 1;
	    }
#ifdef EXTRA_CONSTRAINT
	  if (EXTRA_CONSTRAINT (op, c))
	    return 1;
	  if (EXTRA_MEMORY_CONSTRAINT (c))
	    {
	      /* Every memory operand can be reloaded to fit.  */
	      if (memory_operand (op, VOIDmode))
	        return 1;
	    }
	  if (EXTRA_ADDRESS_CONSTRAINT (c))
	    {
	      /* Every address operand can be reloaded to fit.  */
	      if (address_operand (op, VOIDmode))
	        return 1;
	    }
#endif
	  break;
	}
    }

  return result;
}

/* Given an rtx *P, if it is a sum containing an integer constant term,
   return the location (type rtx *) of the pointer to that constant term.
   Otherwise, return a null pointer.  */

rtx *
find_constant_term_loc (p)
     rtx *p;
{
  rtx *tem;
  enum rtx_code code = GET_CODE (*p);

  /* If *P IS such a constant term, P is its location.  */

  if (code == CONST_INT || code == SYMBOL_REF || code == LABEL_REF
      || code == CONST)
    return p;

  /* Otherwise, if not a sum, it has no constant term.  */

  if (GET_CODE (*p) != PLUS)
    return 0;

  /* If one of the summands is constant, return its location.  */

  if (XEXP (*p, 0) && CONSTANT_P (XEXP (*p, 0))
      && XEXP (*p, 1) && CONSTANT_P (XEXP (*p, 1)))
    return p;

  /* Otherwise, check each summand for containing a constant term.  */

  if (XEXP (*p, 0) != 0)
    {
      tem = find_constant_term_loc (&XEXP (*p, 0));
      if (tem != 0)
	return tem;
    }

  if (XEXP (*p, 1) != 0)
    {
      tem = find_constant_term_loc (&XEXP (*p, 1));
      if (tem != 0)
	return tem;
    }

  return 0;
}

/* Return 1 if OP is a memory reference
   whose address contains no side effects
   and remains valid after the addition
   of a positive integer less than the
   size of the object being referenced.

   We assume that the original address is valid and do not check it.

   This uses strict_memory_address_p as a subroutine, so
   don't use it before reload.  */

int
offsettable_memref_p (op)
     rtx op;
{
  return ((GET_CODE (op) == MEM)
	  && offsettable_address_p (1, GET_MODE (op), XEXP (op, 0)));
}

/* Similar, but don't require a strictly valid mem ref:
   consider pseudo-regs valid as index or base regs.  */

int
offsettable_nonstrict_memref_p (op)
     rtx op;
{
  return ((GET_CODE (op) == MEM)
	  && offsettable_address_p (0, GET_MODE (op), XEXP (op, 0)));
}

/* Return 1 if Y is a memory address which contains no side effects
   and would remain valid after the addition of a positive integer
   less than the size of that mode.

   We assume that the original address is valid and do not check it.
   We do check that it is valid for narrower modes.

   If STRICTP is nonzero, we require a strictly valid address,
   for the sake of use in reload.c.  */

int
offsettable_address_p (strictp, mode, y)
     int strictp;
     enum machine_mode mode;
     rtx y;
{
  enum rtx_code ycode = GET_CODE (y);
  rtx z;
  rtx y1 = y;
  rtx *y2;
  int (*addressp) PARAMS ((enum machine_mode, rtx)) =
    (strictp ? strict_memory_address_p : memory_address_p);
  unsigned int mode_sz = GET_MODE_SIZE (mode);

  if (CONSTANT_ADDRESS_P (y))
    return 1;

  /* Adjusting an offsettable address involves changing to a narrower mode.
     Make sure that's OK.  */

  if (mode_dependent_address_p (y))
    return 0;

  /* ??? How much offset does an offsettable BLKmode reference need?
     Clearly that depends on the situation in which it's being used.
     However, the current situation in which we test 0xffffffff is
     less than ideal.  Caveat user.  */
  if (mode_sz == 0)
    mode_sz = BIGGEST_ALIGNMENT / BITS_PER_UNIT;

  /* If the expression contains a constant term,
     see if it remains valid when max possible offset is added.  */

  if ((ycode == PLUS) && (y2 = find_constant_term_loc (&y1)))
    {
      int good;

      y1 = *y2;
      *y2 = plus_constant (*y2, mode_sz - 1);
      /* Use QImode because an odd displacement may be automatically invalid
	 for any wider mode.  But it should be valid for a single byte.  */
      good = (*addressp) (QImode, y);

      /* In any case, restore old contents of memory.  */
      *y2 = y1;
      return good;
    }

  if (GET_RTX_CLASS (ycode) == 'a')
    return 0;

  /* The offset added here is chosen as the maximum offset that
     any instruction could need to add when operating on something
     of the specified mode.  We assume that if Y and Y+c are
     valid addresses then so is Y+d for all 0<d<c.  adjust_address will
     go inside a LO_SUM here, so we do so as well.  */
  if (GET_CODE (y) == LO_SUM
      && mode != BLKmode
      && mode_sz <= GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT)
    z = gen_rtx_LO_SUM (GET_MODE (y), XEXP (y, 0),
			plus_constant (XEXP (y, 1), mode_sz - 1));
  else
    z = plus_constant (y, mode_sz - 1);

  /* Use QImode because an odd displacement may be automatically invalid
     for any wider mode.  But it should be valid for a single byte.  */
  return (*addressp) (QImode, z);
}

/* Return 1 if ADDR is an address-expression whose effect depends
   on the mode of the memory reference it is used in.

   Autoincrement addressing is a typical example of mode-dependence
   because the amount of the increment depends on the mode.  */

int
mode_dependent_address_p (addr)
  rtx addr ATTRIBUTE_UNUSED; /* Maybe used in GO_IF_MODE_DEPENDENT_ADDRESS.  */
{
  GO_IF_MODE_DEPENDENT_ADDRESS (addr, win);
  return 0;
  /* Label `win' might (not) be used via GO_IF_MODE_DEPENDENT_ADDRESS.  */
 win: ATTRIBUTE_UNUSED_LABEL
  return 1;
}

/* Return 1 if OP is a general operand
   other than a memory ref with a mode dependent address.  */

int
mode_independent_operand (op, mode)
     enum machine_mode mode;
     rtx op;
{
  rtx addr;

  if (! general_operand (op, mode))
    return 0;

  if (GET_CODE (op) != MEM)
    return 1;

  addr = XEXP (op, 0);
  GO_IF_MODE_DEPENDENT_ADDRESS (addr, lose);
  return 1;
  /* Label `lose' might (not) be used via GO_IF_MODE_DEPENDENT_ADDRESS.  */
 lose: ATTRIBUTE_UNUSED_LABEL
  return 0;
}

/* Like extract_insn, but save insn extracted and don't extract again, when
   called again for the same insn expecting that recog_data still contain the
   valid information.  This is used primary by gen_attr infrastructure that
   often does extract insn again and again.  */
void
extract_insn_cached (insn)
     rtx insn;
{
  if (recog_data.insn == insn && INSN_CODE (insn) >= 0)
    return;
  extract_insn (insn);
  recog_data.insn = insn;
}
/* Do cached extract_insn, constrain_operand and complain about failures.
   Used by insn_attrtab.  */
void
extract_constrain_insn_cached (insn)
     rtx insn;
{
  extract_insn_cached (insn);
  if (which_alternative == -1
      && !constrain_operands (reload_completed))
    fatal_insn_not_found (insn);
}
/* Do cached constrain_operand and complain about failures.  */
int
constrain_operands_cached (strict)
	int strict;
{
  if (which_alternative == -1)
    return constrain_operands (strict);
  else
    return 1;
}

/* Analyze INSN and fill in recog_data.  */

void
extract_insn (insn)
     rtx insn;
{
  int i;
  int icode;
  int noperands;
  rtx body = PATTERN (insn);

  recog_data.insn = NULL;
  recog_data.n_operands = 0;
  recog_data.n_alternatives = 0;
  recog_data.n_dups = 0;
  which_alternative = -1;

  switch (GET_CODE (body))
    {
    case USE:
    case CLOBBER:
    case ASM_INPUT:
    case ADDR_VEC:
    case ADDR_DIFF_VEC:
      return;

    case SET:
      if (GET_CODE (SET_SRC (body)) == ASM_OPERANDS)
	goto asm_insn;
      else
	goto normal_insn;
    case PARALLEL:
      if ((GET_CODE (XVECEXP (body, 0, 0)) == SET
	   && GET_CODE (SET_SRC (XVECEXP (body, 0, 0))) == ASM_OPERANDS)
	  || GET_CODE (XVECEXP (body, 0, 0)) == ASM_OPERANDS)
	goto asm_insn;
      else
	goto normal_insn;
    case ASM_OPERANDS:
    asm_insn:
      recog_data.n_operands = noperands = asm_noperands (body);
      if (noperands >= 0)
	{
	  /* This insn is an `asm' with operands.  */

	  /* expand_asm_operands makes sure there aren't too many operands.  */
	  if (noperands > MAX_RECOG_OPERANDS)
	    abort ();

	  /* Now get the operand values and constraints out of the insn.  */
	  decode_asm_operands (body, recog_data.operand,
			       recog_data.operand_loc,
			       recog_data.constraints,
			       recog_data.operand_mode);
	  if (noperands > 0)
	    {
	      const char *p =  recog_data.constraints[0];
	      recog_data.n_alternatives = 1;
	      while (*p)
		recog_data.n_alternatives += (*p++ == ',');
	    }
	  break;
	}
      fatal_insn_not_found (insn);

    default:
    normal_insn:
      /* Ordinary insn: recognize it, get the operands via insn_extract
	 and get the constraints.  */

      icode = recog_memoized (insn);
      if (icode < 0)
	fatal_insn_not_found (insn);

      recog_data.n_operands = noperands = insn_data[icode].n_operands;
      recog_data.n_alternatives = insn_data[icode].n_alternatives;
      recog_data.n_dups = insn_data[icode].n_dups;

      insn_extract (insn);

      for (i = 0; i < noperands; i++)
	{
	  recog_data.constraints[i] = insn_data[icode].operand[i].constraint;
	  recog_data.operand_mode[i] = insn_data[icode].operand[i].mode;
	  /* VOIDmode match_operands gets mode from their real operand.  */
	  if (recog_data.operand_mode[i] == VOIDmode)
	    recog_data.operand_mode[i] = GET_MODE (recog_data.operand[i]);
	}
    }
  for (i = 0; i < noperands; i++)
    recog_data.operand_type[i]
      = (recog_data.constraints[i][0] == '=' ? OP_OUT
	 : recog_data.constraints[i][0] == '+' ? OP_INOUT
	 : OP_IN);

  if (recog_data.n_alternatives > MAX_RECOG_ALTERNATIVES)
    abort ();
}

/* After calling extract_insn, you can use this function to extract some
   information from the constraint strings into a more usable form.
   The collected data is stored in recog_op_alt.  */
void
preprocess_constraints ()
{
  int i;

  memset (recog_op_alt, 0, sizeof recog_op_alt);
  for (i = 0; i < recog_data.n_operands; i++)
    {
      int j;
      struct operand_alternative *op_alt;
      const char *p = recog_data.constraints[i];

      op_alt = recog_op_alt[i];

      for (j = 0; j < recog_data.n_alternatives; j++)
	{
	  op_alt[j].class = NO_REGS;
	  op_alt[j].constraint = p;
	  op_alt[j].matches = -1;
	  op_alt[j].matched = -1;

	  if (*p == '\0' || *p == ',')
	    {
	      op_alt[j].anything_ok = 1;
	      continue;
	    }

	  for (;;)
	    {
	      char c = *p++;
	      if (c == '#')
		do
		  c = *p++;
		while (c != ',' && c != '\0');
	      if (c == ',' || c == '\0')
		break;

	      switch (c)
		{
		case '=': case '+': case '*': case '%':
		case 'E': case 'F': case 'G': case 'H':
		case 's': case 'i': case 'n':
		case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P':
		  /* These don't say anything we care about.  */
		  break;

		case '?':
		  op_alt[j].reject += 6;
		  break;
		case '!':
		  op_alt[j].reject += 600;
		  break;
		case '&':
		  op_alt[j].earlyclobber = 1;
		  break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		  {
		    char *end;
		    op_alt[j].matches = strtoul (p - 1, &end, 10);
		    recog_op_alt[op_alt[j].matches][j].matched = i;
		    p = end;
		  }
		  break;

		case 'm':
		  op_alt[j].memory_ok = 1;
		  break;
		case '<':
		  op_alt[j].decmem_ok = 1;
		  break;
		case '>':
		  op_alt[j].incmem_ok = 1;
		  break;
		case 'V':
		  op_alt[j].nonoffmem_ok = 1;
		  break;
		case 'o':
		  op_alt[j].offmem_ok = 1;
		  break;
		case 'X':
		  op_alt[j].anything_ok = 1;
		  break;

		case 'p':
		  op_alt[j].is_address = 1;
		  op_alt[j].class = reg_class_subunion[(int) op_alt[j].class]
		    [(int) MODE_BASE_REG_CLASS (VOIDmode)];
		  break;

		case 'g': case 'r':
		  op_alt[j].class = reg_class_subunion[(int) op_alt[j].class][(int) GENERAL_REGS];
		  break;

		default:
		  if (EXTRA_MEMORY_CONSTRAINT (c))
		    {
		      op_alt[j].memory_ok = 1;
		      break;
		    }
		  if (EXTRA_ADDRESS_CONSTRAINT (c))
		    {
		      op_alt[j].is_address = 1;
		      op_alt[j].class = reg_class_subunion[(int) op_alt[j].class]
		        [(int) MODE_BASE_REG_CLASS (VOIDmode)];
		      break;
		    }

		  op_alt[j].class = reg_class_subunion[(int) op_alt[j].class][(int) REG_CLASS_FROM_LETTER ((unsigned char) c)];
		  break;
		}
	    }
	}
    }
}

/* Check the operands of an insn against the insn's operand constraints
   and return 1 if they are valid.
   The information about the insn's operands, constraints, operand modes
   etc. is obtained from the global variables set up by extract_insn.

   WHICH_ALTERNATIVE is set to a number which indicates which
   alternative of constraints was matched: 0 for the first alternative,
   1 for the next, etc.

   In addition, when two operands are match
   and it happens that the output operand is (reg) while the
   input operand is --(reg) or ++(reg) (a pre-inc or pre-dec),
   make the output operand look like the input.
   This is because the output operand is the one the template will print.

   This is used in final, just before printing the assembler code and by
   the routines that determine an insn's attribute.

   If STRICT is a positive non-zero value, it means that we have been
   called after reload has been completed.  In that case, we must
   do all checks strictly.  If it is zero, it means that we have been called
   before reload has completed.  In that case, we first try to see if we can
   find an alternative that matches strictly.  If not, we try again, this
   time assuming that reload will fix up the insn.  This provides a "best
   guess" for the alternative and is used to compute attributes of insns prior
   to reload.  A negative value of STRICT is used for this internal call.  */

struct funny_match
{
  int this, other;
};

int
constrain_operands (strict)
     int strict;
{
  const char *constraints[MAX_RECOG_OPERANDS];
  int matching_operands[MAX_RECOG_OPERANDS];
  int earlyclobber[MAX_RECOG_OPERANDS];
  int c;

  struct funny_match funny_match[MAX_RECOG_OPERANDS];
  int funny_match_index;

  which_alternative = 0;
  if (recog_data.n_operands == 0 || recog_data.n_alternatives == 0)
    return 1;

  for (c = 0; c < recog_data.n_operands; c++)
    {
      constraints[c] = recog_data.constraints[c];
      matching_operands[c] = -1;
    }

  do
    {
      int opno;
      int lose = 0;
      funny_match_index = 0;

      for (opno = 0; opno < recog_data.n_operands; opno++)
	{
	  rtx op = recog_data.operand[opno];
	  enum machine_mode mode = GET_MODE (op);
	  const char *p = constraints[opno];
	  int offset = 0;
	  int win = 0;
	  int val;

	  earlyclobber[opno] = 0;

	  /* A unary operator may be accepted by the predicate, but it
	     is irrelevant for matching constraints.  */
	  if (GET_RTX_CLASS (GET_CODE (op)) == '1')
	    op = XEXP (op, 0);

	  if (GET_CODE (op) == SUBREG)
	    {
	      if (GET_CODE (SUBREG_REG (op)) == REG
		  && REGNO (SUBREG_REG (op)) < FIRST_PSEUDO_REGISTER)
		offset = subreg_regno_offset (REGNO (SUBREG_REG (op)),
					      GET_MODE (SUBREG_REG (op)),
					      SUBREG_BYTE (op),
					      GET_MODE (op));
	      op = SUBREG_REG (op);
	    }

	  /* An empty constraint or empty alternative
	     allows anything which matched the pattern.  */
	  if (*p == 0 || *p == ',')
	    win = 1;

	  while (*p && (c = *p++) != ',')
	    switch (c)
	      {
	      case '?':  case '!': case '*':  case '%':
	      case '=':  case '+':
		break;

	      case '#':
		/* Ignore rest of this alternative as far as
		   constraint checking is concerned.  */
		while (*p && *p != ',')
		  p++;
		break;

	      case '&':
		earlyclobber[opno] = 1;
		break;

	      case '0':  case '1':  case '2':  case '3':  case '4':
	      case '5':  case '6':  case '7':  case '8':  case '9':
		{
		  /* This operand must be the same as a previous one.
		     This kind of constraint is used for instructions such
		     as add when they take only two operands.

		     Note that the lower-numbered operand is passed first.

		     If we are not testing strictly, assume that this
		     constraint will be satisfied.  */

		  char *end;
		  int match;

		  match = strtoul (p - 1, &end, 10);
		  p = end;

		  if (strict < 0)
		    val = 1;
		  else
		    {
		      rtx op1 = recog_data.operand[match];
		      rtx op2 = recog_data.operand[opno];

		      /* A unary operator may be accepted by the predicate,
			 but it is irrelevant for matching constraints.  */
		      if (GET_RTX_CLASS (GET_CODE (op1)) == '1')
			op1 = XEXP (op1, 0);
		      if (GET_RTX_CLASS (GET_CODE (op2)) == '1')
			op2 = XEXP (op2, 0);

		      val = operands_match_p (op1, op2);
		    }

		  matching_operands[opno] = match;
		  matching_operands[match] = opno;

		  if (val != 0)
		    win = 1;

		  /* If output is *x and input is *--x, arrange later
		     to change the output to *--x as well, since the
		     output op is the one that will be printed.  */
		  if (val == 2 && strict > 0)
		    {
		      funny_match[funny_match_index].this = opno;
		      funny_match[funny_match_index++].other = match;
		    }
		}
		break;

	      case 'p':
		/* p is used for address_operands.  When we are called by
		   gen_reload, no one will have checked that the address is
		   strictly valid, i.e., that all pseudos requiring hard regs
		   have gotten them.  */
		if (strict <= 0
		    || (strict_memory_address_p (recog_data.operand_mode[opno],
						 op)))
		  win = 1;
		break;

		/* No need to check general_operand again;
		   it was done in insn-recog.c.  */
	      case 'g':
		/* Anything goes unless it is a REG and really has a hard reg
		   but the hard reg is not in the class GENERAL_REGS.  */
		if (strict < 0
		    || GENERAL_REGS == ALL_REGS
		    || GET_CODE (op) != REG
		    || (reload_in_progress
			&& REGNO (op) >= FIRST_PSEUDO_REGISTER)
		    || reg_fits_class_p (op, GENERAL_REGS, offset, mode))
		  win = 1;
		break;

	      case 'X':
		/* This is used for a MATCH_SCRATCH in the cases when
		   we don't actually need anything.  So anything goes
		   any time.  */
		win = 1;
		break;

	      case 'm':
		if (GET_CODE (op) == MEM
		    /* Before reload, accept what reload can turn into mem.  */
		    || (strict < 0 && CONSTANT_P (op))
		    /* During reload, accept a pseudo  */
		    || (reload_in_progress && GET_CODE (op) == REG
			&& REGNO (op) >= FIRST_PSEUDO_REGISTER))
		  win = 1;
		break;

	      case '<':
		if (GET_CODE (op) == MEM
		    && (GET_CODE (XEXP (op, 0)) == PRE_DEC
			|| GET_CODE (XEXP (op, 0)) == POST_DEC))
		  win = 1;
		break;

	      case '>':
		if (GET_CODE (op) == MEM
		    && (GET_CODE (XEXP (op, 0)) == PRE_INC
			|| GET_CODE (XEXP (op, 0)) == POST_INC))
		  win = 1;
		break;

	      case 'E':
	      case 'F':
		if (GET_CODE (op) == CONST_DOUBLE
		    || (GET_CODE (op) == CONST_VECTOR
			&& GET_MODE_CLASS (GET_MODE (op)) == MODE_VECTOR_FLOAT))
		  win = 1;
		break;

	      case 'G':
	      case 'H':
		if (GET_CODE (op) == CONST_DOUBLE
		    && CONST_DOUBLE_OK_FOR_LETTER_P (op, c))
		  win = 1;
		break;

	      case 's':
		if (GET_CODE (op) == CONST_INT
		    || (GET_CODE (op) == CONST_DOUBLE
			&& GET_MODE (op) == VOIDmode))
		  break;
	      case 'i':
		if (CONSTANT_P (op))
		  win = 1;
		break;

	      case 'n':
		if (GET_CODE (op) == CONST_INT
		    || (GET_CODE (op) == CONST_DOUBLE
			&& GET_MODE (op) == VOIDmode))
		  win = 1;
		break;

	      case 'I':
	      case 'J':
	      case 'K':
	      case 'L':
	      case 'M':
	      case 'N':
	      case 'O':
	      case 'P':
		if (GET_CODE (op) == CONST_INT
		    && CONST_OK_FOR_LETTER_P (INTVAL (op), c))
		  win = 1;
		break;

	      case 'V':
		if (GET_CODE (op) == MEM
		    && ((strict > 0 && ! offsettable_memref_p (op))
			|| (strict < 0
			    && !(CONSTANT_P (op) || GET_CODE (op) == MEM))
			|| (reload_in_progress
			    && !(GET_CODE (op) == REG
				 && REGNO (op) >= FIRST_PSEUDO_REGISTER))))
		  win = 1;
		break;

	      case 'o':
		if ((strict > 0 && offsettable_memref_p (op))
		    || (strict == 0 && offsettable_nonstrict_memref_p (op))
		    /* Before reload, accept what reload can handle.  */
		    || (strict < 0
			&& (CONSTANT_P (op) || GET_CODE (op) == MEM))
		    /* During reload, accept a pseudo  */
		    || (reload_in_progress && GET_CODE (op) == REG
			&& REGNO (op) >= FIRST_PSEUDO_REGISTER))
		  win = 1;
		break;

	      default:
		{
		  enum reg_class class;

		  class = (c == 'r' ? GENERAL_REGS : REG_CLASS_FROM_LETTER (c));
		  if (class != NO_REGS)
		    {
		      if (strict < 0
			  || (strict == 0
			      && GET_CODE (op) == REG
			      && REGNO (op) >= FIRST_PSEUDO_REGISTER)
			  || (strict == 0 && GET_CODE (op) == SCRATCH)
			  || (GET_CODE (op) == REG
			      && reg_fits_class_p (op, class, offset, mode)))
		        win = 1;
		    }
#ifdef EXTRA_CONSTRAINT
		  else if (EXTRA_CONSTRAINT (op, c))
		    win = 1;

		  if (EXTRA_MEMORY_CONSTRAINT (c))
		    {
		      /* Every memory operand can be reloaded to fit,
			 so copy the condition from the 'm' case.  */
		      if (GET_CODE (op) == MEM
		          /* Before reload, accept what reload can turn into mem.  */
		          || (strict < 0 && CONSTANT_P (op))
		          /* During reload, accept a pseudo  */
		          || (reload_in_progress && GET_CODE (op) == REG
			      && REGNO (op) >= FIRST_PSEUDO_REGISTER))
			win = 1;
		    }
		  if (EXTRA_ADDRESS_CONSTRAINT (c))
		    {
		      /* Every address operand can be reloaded to fit,
			 so copy the condition from the 'p' case.  */
		      if (strict <= 0
		          || (strict_memory_address_p (recog_data.operand_mode[opno],
						       op)))
		        win = 1;
		    }
#endif
		  break;
		}
	      }

	  constraints[opno] = p;
	  /* If this operand did not win somehow,
	     this alternative loses.  */
	  if (! win)
	    lose = 1;
	}
      /* This alternative won; the operands are ok.
	 Change whichever operands this alternative says to change.  */
      if (! lose)
	{
	  int opno, eopno;

	  /* See if any earlyclobber operand conflicts with some other
	     operand.  */

	  if (strict > 0)
	    for (eopno = 0; eopno < recog_data.n_operands; eopno++)
	      /* Ignore earlyclobber operands now in memory,
		 because we would often report failure when we have
		 two memory operands, one of which was formerly a REG.  */
	      if (earlyclobber[eopno]
		  && GET_CODE (recog_data.operand[eopno]) == REG)
		for (opno = 0; opno < recog_data.n_operands; opno++)
		  if ((GET_CODE (recog_data.operand[opno]) == MEM
		       || recog_data.operand_type[opno] != OP_OUT)
		      && opno != eopno
		      /* Ignore things like match_operator operands.  */
		      && *recog_data.constraints[opno] != 0
		      && ! (matching_operands[opno] == eopno
			    && operands_match_p (recog_data.operand[opno],
						 recog_data.operand[eopno]))
		      && ! safe_from_earlyclobber (recog_data.operand[opno],
						   recog_data.operand[eopno]))
		    lose = 1;

	  if (! lose)
	    {
	      while (--funny_match_index >= 0)
		{
		  recog_data.operand[funny_match[funny_match_index].other]
		    = recog_data.operand[funny_match[funny_match_index].this];
		}

	      return 1;
	    }
	}

      which_alternative++;
    }
  while (which_alternative < recog_data.n_alternatives);

  which_alternative = -1;
  /* If we are about to reject this, but we are not to test strictly,
     try a very loose test.  Only return failure if it fails also.  */
  if (strict == 0)
    return constrain_operands (-1);
  else
    return 0;
}

/* Return 1 iff OPERAND (assumed to be a REG rtx)
   is a hard reg in class CLASS when its regno is offset by OFFSET
   and changed to mode MODE.
   If REG occupies multiple hard regs, all of them must be in CLASS.  */

int
reg_fits_class_p (operand, class, offset, mode)
     rtx operand;
     enum reg_class class;
     int offset;
     enum machine_mode mode;
{
  int regno = REGNO (operand);
  if (regno < FIRST_PSEUDO_REGISTER
      && TEST_HARD_REG_BIT (reg_class_contents[(int) class],
			    regno + offset))
    {
      int sr;
      regno += offset;
      for (sr = HARD_REGNO_NREGS (regno, mode) - 1;
	   sr > 0; sr--)
	if (! TEST_HARD_REG_BIT (reg_class_contents[(int) class],
				 regno + sr))
	  break;
      return sr == 0;
    }

  return 0;
}

/* Split single instruction.  Helper function for split_all_insns.
   Return last insn in the sequence if successful, or NULL if unsuccessful.  */
static rtx
split_insn (insn)
     rtx insn;
{
  rtx set;
  if (!INSN_P (insn))
    ;
  /* Don't split no-op move insns.  These should silently
     disappear later in final.  Splitting such insns would
     break the code that handles REG_NO_CONFLICT blocks.  */

  else if ((set = single_set (insn)) != NULL && set_noop_p (set))
    {
      /* Nops get in the way while scheduling, so delete them
         now if register allocation has already been done.  It
         is too risky to try to do this before register
         allocation, and there are unlikely to be very many
         nops then anyways.  */
      if (reload_completed)
	delete_insn_and_edges (insn);
    }
  else
    {
      /* Split insns here to get max fine-grain parallelism.  */
      rtx first = PREV_INSN (insn);
      rtx last = try_split (PATTERN (insn), insn, 1);

      if (last != insn)
	{
	  /* try_split returns the NOTE that INSN became.  */
	  PUT_CODE (insn, NOTE);
	  NOTE_SOURCE_FILE (insn) = 0;
	  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;

	  /* ??? Coddle to md files that generate subregs in post-
	     reload splitters instead of computing the proper
	     hard register.  */
	  if (reload_completed && first != last)
	    {
	      first = NEXT_INSN (first);
	      while (1)
		{
		  if (INSN_P (first))
		    cleanup_subreg_operands (first);
		  if (first == last)
		    break;
		  first = NEXT_INSN (first);
		}
	    }
	  return last;
	}
    }
  return NULL_RTX;
}
/* Split all insns in the function.  If UPD_LIFE, update life info after.  */

void
split_all_insns (upd_life)
     int upd_life;
{
  sbitmap blocks;
  int changed;
  basic_block bb;

  blocks = sbitmap_alloc (last_basic_block);
  sbitmap_zero (blocks);
  changed = 0;

  FOR_EACH_BB_REVERSE (bb)
    {
      rtx insn, next;
      bool finish = false;

      for (insn = bb->head; !finish ; insn = next)
	{
	  rtx last;

	  /* Can't use `next_real_insn' because that might go across
	     CODE_LABELS and short-out basic blocks.  */
	  next = NEXT_INSN (insn);
	  finish = (insn == bb->end);
	  last = split_insn (insn);
	  if (last)
	    {
	      /* The split sequence may include barrier, but the
		 BB boundary we are interested in will be set to previous
		 one.  */

	      while (GET_CODE (last) == BARRIER)
		last = PREV_INSN (last);
	      SET_BIT (blocks, bb->index);
	      changed = 1;
	      insn = last;
	    }
	}
    }

  if (changed)
    {
      find_many_sub_basic_blocks (blocks);
    }

  if (changed && upd_life)
    {
      count_or_remove_death_notes (blocks, 1);
      update_life_info (blocks, UPDATE_LIFE_LOCAL, PROP_DEATH_NOTES);
    }
#ifdef ENABLE_CHECKING
  verify_flow_info ();
#endif

  sbitmap_free (blocks);
}

/* Same as split_all_insns, but do not expect CFG to be available.
   Used by machine depedent reorg passes.  */

void
split_all_insns_noflow ()
{
  rtx next, insn;

  for (insn = get_insns (); insn; insn = next)
    {
      next = NEXT_INSN (insn);
      split_insn (insn);
    }
  return;
}

#ifdef HAVE_peephole2
struct peep2_insn_data
{
  rtx insn;
  regset live_before;
};

static struct peep2_insn_data peep2_insn_data[MAX_INSNS_PER_PEEP2 + 1];
static int peep2_current;

/* A non-insn marker indicating the last insn of the block.
   The live_before regset for this element is correct, indicating
   global_live_at_end for the block.  */
#define PEEP2_EOB	pc_rtx

/* Return the Nth non-note insn after `current', or return NULL_RTX if it
   does not exist.  Used by the recognizer to find the next insn to match
   in a multi-insn pattern.  */

rtx
peep2_next_insn (n)
     int n;
{
  if (n >= MAX_INSNS_PER_PEEP2 + 1)
    abort ();

  n += peep2_current;
  if (n >= MAX_INSNS_PER_PEEP2 + 1)
    n -= MAX_INSNS_PER_PEEP2 + 1;

  if (peep2_insn_data[n].insn == PEEP2_EOB)
    return NULL_RTX;
  return peep2_insn_data[n].insn;
}

/* Return true if REGNO is dead before the Nth non-note insn
   after `current'.  */

int
peep2_regno_dead_p (ofs, regno)
     int ofs;
     int regno;
{
  if (ofs >= MAX_INSNS_PER_PEEP2 + 1)
    abort ();

  ofs += peep2_current;
  if (ofs >= MAX_INSNS_PER_PEEP2 + 1)
    ofs -= MAX_INSNS_PER_PEEP2 + 1;

  if (peep2_insn_data[ofs].insn == NULL_RTX)
    abort ();

  return ! REGNO_REG_SET_P (peep2_insn_data[ofs].live_before, regno);
}

/* Similarly for a REG.  */

int
peep2_reg_dead_p (ofs, reg)
     int ofs;
     rtx reg;
{
  int regno, n;

  if (ofs >= MAX_INSNS_PER_PEEP2 + 1)
    abort ();

  ofs += peep2_current;
  if (ofs >= MAX_INSNS_PER_PEEP2 + 1)
    ofs -= MAX_INSNS_PER_PEEP2 + 1;

  if (peep2_insn_data[ofs].insn == NULL_RTX)
    abort ();

  regno = REGNO (reg);
  n = HARD_REGNO_NREGS (regno, GET_MODE (reg));
  while (--n >= 0)
    if (REGNO_REG_SET_P (peep2_insn_data[ofs].live_before, regno + n))
      return 0;
  return 1;
}

/* Try to find a hard register of mode MODE, matching the register class in
   CLASS_STR, which is available at the beginning of insn CURRENT_INSN and
   remains available until the end of LAST_INSN.  LAST_INSN may be NULL_RTX,
   in which case the only condition is that the register must be available
   before CURRENT_INSN.
   Registers that already have bits set in REG_SET will not be considered.

   If an appropriate register is available, it will be returned and the
   corresponding bit(s) in REG_SET will be set; otherwise, NULL_RTX is
   returned.  */

rtx
peep2_find_free_register (from, to, class_str, mode, reg_set)
     int from, to;
     const char *class_str;
     enum machine_mode mode;
     HARD_REG_SET *reg_set;
{
  static int search_ofs;
  enum reg_class class;
  HARD_REG_SET live;
  int i;

  if (from >= MAX_INSNS_PER_PEEP2 + 1 || to >= MAX_INSNS_PER_PEEP2 + 1)
    abort ();

  from += peep2_current;
  if (from >= MAX_INSNS_PER_PEEP2 + 1)
    from -= MAX_INSNS_PER_PEEP2 + 1;
  to += peep2_current;
  if (to >= MAX_INSNS_PER_PEEP2 + 1)
    to -= MAX_INSNS_PER_PEEP2 + 1;

  if (peep2_insn_data[from].insn == NULL_RTX)
    abort ();
  REG_SET_TO_HARD_REG_SET (live, peep2_insn_data[from].live_before);

  while (from != to)
    {
      HARD_REG_SET this_live;

      if (++from >= MAX_INSNS_PER_PEEP2 + 1)
	from = 0;
      if (peep2_insn_data[from].insn == NULL_RTX)
	abort ();
      REG_SET_TO_HARD_REG_SET (this_live, peep2_insn_data[from].live_before);
      IOR_HARD_REG_SET (live, this_live);
    }

  class = (class_str[0] == 'r' ? GENERAL_REGS
	   : REG_CLASS_FROM_LETTER (class_str[0]));

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      int raw_regno, regno, success, j;

      /* Distribute the free registers as much as possible.  */
      raw_regno = search_ofs + i;
      if (raw_regno >= FIRST_PSEUDO_REGISTER)
	raw_regno -= FIRST_PSEUDO_REGISTER;
#ifdef REG_ALLOC_ORDER
      regno = reg_alloc_order[raw_regno];
#else
      regno = raw_regno;
#endif

      /* Don't allocate fixed registers.  */
      if (fixed_regs[regno])
	continue;
      /* Make sure the register is of the right class.  */
      if (! TEST_HARD_REG_BIT (reg_class_contents[class], regno))
	continue;
      /* And can support the mode we need.  */
      if (! HARD_REGNO_MODE_OK (regno, mode))
	continue;
      /* And that we don't create an extra save/restore.  */
      if (! call_used_regs[regno] && ! regs_ever_live[regno])
	continue;
      /* And we don't clobber traceback for noreturn functions.  */
      if ((regno == FRAME_POINTER_REGNUM || regno == HARD_FRAME_POINTER_REGNUM)
	  && (! reload_completed || frame_pointer_needed))
	continue;

      success = 1;
      for (j = HARD_REGNO_NREGS (regno, mode) - 1; j >= 0; j--)
	{
	  if (TEST_HARD_REG_BIT (*reg_set, regno + j)
	      || TEST_HARD_REG_BIT (live, regno + j))
	    {
	      success = 0;
	      break;
	    }
	}
      if (success)
	{
	  for (j = HARD_REGNO_NREGS (regno, mode) - 1; j >= 0; j--)
	    SET_HARD_REG_BIT (*reg_set, regno + j);

	  /* Start the next search with the next register.  */
	  if (++raw_regno >= FIRST_PSEUDO_REGISTER)
	    raw_regno = 0;
	  search_ofs = raw_regno;

	  return gen_rtx_REG (mode, regno);
	}
    }

  search_ofs = 0;
  return NULL_RTX;
}

/* Perform the peephole2 optimization pass.  */

void
peephole2_optimize (dump_file)
     FILE *dump_file ATTRIBUTE_UNUSED;
{
  regset_head rs_heads[MAX_INSNS_PER_PEEP2 + 2];
  rtx insn, prev;
  regset live;
  int i;
  basic_block bb;
#ifdef HAVE_conditional_execution
  sbitmap blocks;
  bool changed;
#endif
  bool do_cleanup_cfg = false;
  bool do_rebuild_jump_labels = false;

  /* Initialize the regsets we're going to use.  */
  for (i = 0; i < MAX_INSNS_PER_PEEP2 + 1; ++i)
    peep2_insn_data[i].live_before = INITIALIZE_REG_SET (rs_heads[i]);
  live = INITIALIZE_REG_SET (rs_heads[i]);

#ifdef HAVE_conditional_execution
  blocks = sbitmap_alloc (last_basic_block);
  sbitmap_zero (blocks);
  changed = false;
#else
  count_or_remove_death_notes (NULL, 1);
#endif

  FOR_EACH_BB_REVERSE (bb)
    {
      struct propagate_block_info *pbi;

      /* Indicate that all slots except the last holds invalid data.  */
      for (i = 0; i < MAX_INSNS_PER_PEEP2; ++i)
	peep2_insn_data[i].insn = NULL_RTX;

      /* Indicate that the last slot contains live_after data.  */
      peep2_insn_data[MAX_INSNS_PER_PEEP2].insn = PEEP2_EOB;
      peep2_current = MAX_INSNS_PER_PEEP2;

      /* Start up propagation.  */
      COPY_REG_SET (live, bb->global_live_at_end);
      COPY_REG_SET (peep2_insn_data[MAX_INSNS_PER_PEEP2].live_before, live);

#ifdef HAVE_conditional_execution
      pbi = init_propagate_block_info (bb, live, NULL, NULL, 0);
#else
      pbi = init_propagate_block_info (bb, live, NULL, NULL, PROP_DEATH_NOTES);
#endif

      for (insn = bb->end; ; insn = prev)
	{
	  prev = PREV_INSN (insn);
	  if (INSN_P (insn))
	    {
	      rtx try, before_try, x;
	      int match_len;
	      rtx note;
	      bool was_call = false;

	      /* Record this insn.  */
	      if (--peep2_current < 0)
		peep2_current = MAX_INSNS_PER_PEEP2;
	      peep2_insn_data[peep2_current].insn = insn;
	      propagate_one_insn (pbi, insn);
	      COPY_REG_SET (peep2_insn_data[peep2_current].live_before, live);

	      /* Match the peephole.  */
	      try = peephole2_insns (PATTERN (insn), insn, &match_len);
	      if (try != NULL)
		{
		  /* If we are splitting a CALL_INSN, look for the CALL_INSN
		     in SEQ and copy our CALL_INSN_FUNCTION_USAGE and other
		     cfg-related call notes.  */
		  for (i = 0; i <= match_len; ++i)
		    {
		      int j;
		      rtx old_insn, new_insn, note;

		      j = i + peep2_current;
		      if (j >= MAX_INSNS_PER_PEEP2 + 1)
			j -= MAX_INSNS_PER_PEEP2 + 1;
		      old_insn = peep2_insn_data[j].insn;
		      if (GET_CODE (old_insn) != CALL_INSN)
			continue;
		      was_call = true;

		      new_insn = try;
		      while (new_insn != NULL_RTX)
			{
			  if (GET_CODE (new_insn) == CALL_INSN)
			    break;
			  new_insn = NEXT_INSN (new_insn);
			}

		      if (new_insn == NULL_RTX)
			abort ();

		      CALL_INSN_FUNCTION_USAGE (new_insn)
			= CALL_INSN_FUNCTION_USAGE (old_insn);

		      for (note = REG_NOTES (old_insn);
			   note;
			   note = XEXP (note, 1))
			switch (REG_NOTE_KIND (note))
			  {
			  case REG_NORETURN:
			  case REG_SETJMP:
			  case REG_ALWAYS_RETURN:
			    REG_NOTES (new_insn)
			      = gen_rtx_EXPR_LIST (REG_NOTE_KIND (note),
						   XEXP (note, 0),
						   REG_NOTES (new_insn));
			  default:
			    /* Discard all other reg notes.  */
			    break;
			  }

		      /* Croak if there is another call in the sequence.  */
		      while (++i <= match_len)
			{
			  j = i + peep2_current;
			  if (j >= MAX_INSNS_PER_PEEP2 + 1)
			    j -= MAX_INSNS_PER_PEEP2 + 1;
			  old_insn = peep2_insn_data[j].insn;
			  if (GET_CODE (old_insn) == CALL_INSN)
			    abort ();
			}
		      break;
		    }

		  i = match_len + peep2_current;
		  if (i >= MAX_INSNS_PER_PEEP2 + 1)
		    i -= MAX_INSNS_PER_PEEP2 + 1;

		  note = find_reg_note (peep2_insn_data[i].insn,
					REG_EH_REGION, NULL_RTX);

		  /* Replace the old sequence with the new.  */
		  try = emit_insn_after_scope (try, peep2_insn_data[i].insn,
					       INSN_SCOPE (peep2_insn_data[i].insn));
		  before_try = PREV_INSN (insn);
		  delete_insn_chain (insn, peep2_insn_data[i].insn);

		  /* Re-insert the EH_REGION notes.  */
		  if (note || (was_call && nonlocal_goto_handler_labels))
		    {
		      edge eh_edge;

		      for (eh_edge = bb->succ; eh_edge
			   ; eh_edge = eh_edge->succ_next)
			if (eh_edge->flags & (EDGE_EH | EDGE_ABNORMAL_CALL))
			  break;

		      for (x = try ; x != before_try ; x = PREV_INSN (x))
			if (GET_CODE (x) == CALL_INSN
			    || (flag_non_call_exceptions
				&& may_trap_p (PATTERN (x))
				&& !find_reg_note (x, REG_EH_REGION, NULL)))
			  {
			    if (note)
			      REG_NOTES (x)
			        = gen_rtx_EXPR_LIST (REG_EH_REGION,
						     XEXP (note, 0),
						     REG_NOTES (x));

			    if (x != bb->end && eh_edge)
			      {
				edge nfte, nehe;
				int flags;

				nfte = split_block (bb, x);
				flags = (eh_edge->flags
					 & (EDGE_EH | EDGE_ABNORMAL));
				if (GET_CODE (x) == CALL_INSN)
				  flags |= EDGE_ABNORMAL_CALL;
				nehe = make_edge (nfte->src, eh_edge->dest,
						  flags);

				nehe->probability = eh_edge->probability;
				nfte->probability
				  = REG_BR_PROB_BASE - nehe->probability;

			        do_cleanup_cfg |= purge_dead_edges (nfte->dest);
#ifdef HAVE_conditional_execution
				SET_BIT (blocks, nfte->dest->index);
				changed = true;
#endif
				bb = nfte->src;
				eh_edge = nehe;
			      }
			  }

		      /* Converting possibly trapping insn to non-trapping is
			 possible.  Zap dummy outgoing edges.  */
		      do_cleanup_cfg |= purge_dead_edges (bb);
		    }

#ifdef HAVE_conditional_execution
		  /* With conditional execution, we cannot back up the
		     live information so easily, since the conditional
		     death data structures are not so self-contained.
		     So record that we've made a modification to this
		     block and update life information at the end.  */
		  SET_BIT (blocks, bb->index);
		  changed = true;

		  for (i = 0; i < MAX_INSNS_PER_PEEP2 + 1; ++i)
		    peep2_insn_data[i].insn = NULL_RTX;
		  peep2_insn_data[peep2_current].insn = PEEP2_EOB;
#else
		  /* Back up lifetime information past the end of the
		     newly created sequence.  */
		  if (++i >= MAX_INSNS_PER_PEEP2 + 1)
		    i = 0;
		  COPY_REG_SET (live, peep2_insn_data[i].live_before);

		  /* Update life information for the new sequence.  */
		  x = try;
		  do
		    {
		      if (INSN_P (x))
			{
			  if (--i < 0)
			    i = MAX_INSNS_PER_PEEP2;
			  peep2_insn_data[i].insn = x;
			  propagate_one_insn (pbi, x);
			  COPY_REG_SET (peep2_insn_data[i].live_before, live);
			}
		      x = PREV_INSN (x);
		    }
		  while (x != prev);

		  /* ??? Should verify that LIVE now matches what we
		     had before the new sequence.  */

		  peep2_current = i;
#endif

		  /* If we generated a jump instruction, it won't have
		     JUMP_LABEL set.  Recompute after we're done.  */
		  for (x = try; x != before_try; x = PREV_INSN (x))
		    if (GET_CODE (x) == JUMP_INSN)
		      {
		        do_rebuild_jump_labels = true;
			break;
		      }
		}
	    }

	  if (insn == bb->head)
	    break;
	}

      free_propagate_block_info (pbi);
    }

  for (i = 0; i < MAX_INSNS_PER_PEEP2 + 1; ++i)
    FREE_REG_SET (peep2_insn_data[i].live_before);
  FREE_REG_SET (live);

  if (do_rebuild_jump_labels)
    rebuild_jump_labels (get_insns ());

  /* If we eliminated EH edges, we may be able to merge blocks.  Further,
     we've changed global life since exception handlers are no longer
     reachable.  */
  if (do_cleanup_cfg)
    {
      cleanup_cfg (0);
      update_life_info (0, UPDATE_LIFE_GLOBAL_RM_NOTES, PROP_DEATH_NOTES);
    }
#ifdef HAVE_conditional_execution
  else
    {
      count_or_remove_death_notes (blocks, 1);
      update_life_info (blocks, UPDATE_LIFE_LOCAL, PROP_DEATH_NOTES);
    }
  sbitmap_free (blocks);
#endif
}
#endif /* HAVE_peephole2 */

/* Common predicates for use with define_bypass.  */

/* True if the dependency between OUT_INSN and IN_INSN is on the store
   data not the address operand(s) of the store.  IN_INSN must be
   single_set.  OUT_INSN must be either a single_set or a PARALLEL with
   SETs inside.  */

int
store_data_bypass_p (out_insn, in_insn)
     rtx out_insn, in_insn;
{
  rtx out_set, in_set;

  in_set = single_set (in_insn);
  if (! in_set)
    abort ();

  if (GET_CODE (SET_DEST (in_set)) != MEM)
    return false;

  out_set = single_set (out_insn);
  if (out_set)
    {
      if (reg_mentioned_p (SET_DEST (out_set), SET_DEST (in_set)))
	return false;
    }
  else
    {
      rtx out_pat;
      int i;

      out_pat = PATTERN (out_insn);
      if (GET_CODE (out_pat) != PARALLEL)
	abort ();

      for (i = 0; i < XVECLEN (out_pat, 0); i++)
	{
	  rtx exp = XVECEXP (out_pat, 0, i);

	  if (GET_CODE (exp) == CLOBBER)
	    continue;

	  if (GET_CODE (exp) != SET)
	    abort ();

	  if (reg_mentioned_p (SET_DEST (exp), SET_DEST (in_set)))
	    return false;
	}
    }

  return true;
}

/* True if the dependency between OUT_INSN and IN_INSN is in the IF_THEN_ELSE
   condition, and not the THEN or ELSE branch.  OUT_INSN may be either a single
   or multiple set; IN_INSN should be single_set for truth, but for convenience
   of insn categorization may be any JUMP or CALL insn.  */

int
if_test_bypass_p (out_insn, in_insn)
     rtx out_insn, in_insn;
{
  rtx out_set, in_set;

  in_set = single_set (in_insn);
  if (! in_set)
    {
      if (GET_CODE (in_insn) == JUMP_INSN || GET_CODE (in_insn) == CALL_INSN)
	return false;
      abort ();
    }

  if (GET_CODE (SET_SRC (in_set)) != IF_THEN_ELSE)
    return false;
  in_set = SET_SRC (in_set);

  out_set = single_set (out_insn);
  if (out_set)
    {
      if (reg_mentioned_p (SET_DEST (out_set), XEXP (in_set, 1))
	  || reg_mentioned_p (SET_DEST (out_set), XEXP (in_set, 2)))
	return false;
    }
  else
    {
      rtx out_pat;
      int i;

      out_pat = PATTERN (out_insn);
      if (GET_CODE (out_pat) != PARALLEL)
	abort ();

      for (i = 0; i < XVECLEN (out_pat, 0); i++)
	{
	  rtx exp = XVECEXP (out_pat, 0, i);

	  if (GET_CODE (exp) == CLOBBER)
	    continue;

	  if (GET_CODE (exp) != SET)
	    abort ();

	  if (reg_mentioned_p (SET_DEST (out_set), XEXP (in_set, 1))
	      || reg_mentioned_p (SET_DEST (out_set), XEXP (in_set, 2)))
	    return false;
	}
    }

  return true;
}
