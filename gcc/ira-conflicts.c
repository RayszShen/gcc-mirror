/* IRA conflict builder.
   Copyright (C) 2006, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Vladimir Makarov <vmakarov@redhat.com>.

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
#include "regs.h"
#include "rtl.h"
#include "tm_p.h"
#include "target.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "recog.h"
#include "toplev.h"
#include "params.h"
#include "df.h"
#include "sparseset.h"
#include "ira-int.h"

/* This file contains code responsible for allocno conflict creation,
   allocno copy creation and allocno info accumulation on upper level
   regions.  */

/* allocnos_num array of arrays of bits, recording whether two
   allocno's conflict (can't go in the same hardware register).

   Some arrays will be used as conflict bit vector of the
   corresponding allocnos see function build_allocno_conflicts.  */
static INT_TYPE **conflicts;

/* Macro to test a conflict of A1 and A2 in `conflicts'.  */
#define CONFLICT_ALLOCNO_P(A1, A2)					\
  (ALLOCNO_MIN (A1) <= ALLOCNO_CONFLICT_ID (A2)				\
   && ALLOCNO_CONFLICT_ID (A2) <= ALLOCNO_MAX (A1)			\
   && TEST_ALLOCNO_SET_BIT (conflicts[ALLOCNO_NUM (A1)],		\
	  		    ALLOCNO_CONFLICT_ID (A2),			\
			    ALLOCNO_MIN (A1),				\
			    ALLOCNO_MAX (A1)))



/* The function builds allocno conflict table by processing allocno
   live ranges.  */
static void
build_conflict_bit_table (void)
{
  int i, num, id, allocated_words_num, conflict_bit_vec_words_num;
  unsigned int j;
  enum reg_class cover_class;
  allocno_t allocno, live_a;
  allocno_live_range_t r;
  allocno_iterator ai;
  sparseset allocnos_live;
  int allocno_set_words;

  allocno_set_words = (allocnos_num + INT_BITS - 1) / INT_BITS;
  allocnos_live = sparseset_alloc (allocnos_num);
  conflicts = ira_allocate (sizeof (INT_TYPE *) * allocnos_num);
  allocated_words_num = 0;
  FOR_EACH_ALLOCNO (allocno, ai)
    {
      num = ALLOCNO_NUM (allocno);
      if (ALLOCNO_MAX (allocno) < ALLOCNO_MIN (allocno))
	{
	  conflicts[num] = NULL;
	  continue;
	}
      conflict_bit_vec_words_num
	= (ALLOCNO_MAX (allocno) - ALLOCNO_MIN (allocno) + INT_BITS) / INT_BITS;
      allocated_words_num += conflict_bit_vec_words_num;
      conflicts[num]
	= ira_allocate (sizeof (INT_TYPE) * conflict_bit_vec_words_num);
      memset (conflicts[num], 0,
	      sizeof (INT_TYPE) * conflict_bit_vec_words_num);
    }
  if (internal_flag_ira_verbose > 0 && ira_dump_file != NULL)
    fprintf
      (ira_dump_file,
       "+++Allocating %ld bytes for conflict table (uncompressed size %ld)\n",
       (long) allocated_words_num * sizeof (INT_TYPE),
       (long) allocno_set_words * allocnos_num * sizeof (INT_TYPE));
  for (i = 0; i < max_point; i++)
    {
      for (r = start_point_ranges[i]; r != NULL; r = r->start_next)
	{
	  allocno = r->allocno;
	  num = ALLOCNO_NUM (allocno);
	  id = ALLOCNO_CONFLICT_ID (allocno);
	  cover_class = ALLOCNO_COVER_CLASS (allocno);
	  sparseset_set_bit (allocnos_live, num);
	  EXECUTE_IF_SET_IN_SPARSESET (allocnos_live, j)
	    {
	      live_a = allocnos[j];
	      if (cover_class == ALLOCNO_COVER_CLASS (live_a)
		  /* Don't set up conflict for the allocno with itself.  */
		  && num != (int) j)
		{
		  SET_ALLOCNO_SET_BIT (conflicts[num],
				       ALLOCNO_CONFLICT_ID (live_a),
				       ALLOCNO_MIN (allocno),
				       ALLOCNO_MAX (allocno));
		  SET_ALLOCNO_SET_BIT (conflicts[j], id,
				       ALLOCNO_MIN (live_a),
				       ALLOCNO_MAX (live_a));
		}
	    }
	}
	  
      for (r = finish_point_ranges[i]; r != NULL; r = r->finish_next)
	sparseset_clear_bit (allocnos_live, ALLOCNO_NUM (r->allocno));
    }
  sparseset_free (allocnos_live);
}



/* The function returns TRUE, if the operand constraint STR is
   commutative.  */
static bool
commutative_constraint_p (const char *str)
{
  bool ignore_p;
  int c;

  for (ignore_p = false;;)
    {
      c = *str;
      if (c == '\0')
	break;
      str += CONSTRAINT_LEN (c, str);
      if (c == '#')
	ignore_p = true;
      else if (c == ',')
	ignore_p = false;
      else if (! ignore_p)
	{
	  /* Usually `%' is the first constraint character but the
	     documentation does not require this.  */
	  if (c == '%')
	    return true;
	}
    }
  return false;
}

/* The function returns number of the operand which should be the same
   in any case as operand with number OP_NUM (or negative value if
   there is no such operand).  If USE_COMMUT_OP_P is TRUE, the
   function makes temporarily commutative operand exchange before
   this.  The function takes only really possible alternatives into
   consideration.  */
static int
get_dup_num (int op_num, bool use_commut_op_p)
{
  int curr_alt, c, original, dup;
  bool ignore_p, commut_op_used_p;
  const char *str;
  rtx op;

  if (op_num < 0 || recog_data.n_alternatives == 0)
    return -1;
  op = recog_data.operand[op_num];
  ira_assert (REG_P (op));
  commut_op_used_p = true;
  if (use_commut_op_p)
    {
      if (commutative_constraint_p (recog_data.constraints[op_num]))
	op_num++;
      else if (op_num > 0 && commutative_constraint_p (recog_data.constraints
						       [op_num - 1]))
	op_num--;
      else
	commut_op_used_p = false;
    }
  str = recog_data.constraints[op_num];
  for (ignore_p = false, original = -1, curr_alt = 0;;)
    {
      c = *str;
      if (c == '\0')
	break;
      if (c == '#')
	ignore_p = true;
      else if (c == ',')
	{
	  curr_alt++;
	  ignore_p = false;
	}
      else if (! ignore_p)
	switch (c)
	  {
	  case 'X':
	    return -1;
	    
	  case 'm':
	  case 'o':
	    /* Accept a register which might be placed in memory.  */
	    return -1;
	    break;

	  case 'V':
	  case '<':
	  case '>':
	    break;

	  case 'p':
	    GO_IF_LEGITIMATE_ADDRESS (VOIDmode, op, win_p);
	    break;
	    
	  win_p:
	    return -1;
	  
	  case 'g':
	    return -1;
	    
	  case 'r':
	  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	  case 'h': case 'j': case 'k': case 'l':
	  case 'q': case 't': case 'u':
	  case 'v': case 'w': case 'x': case 'y': case 'z':
	  case 'A': case 'B': case 'C': case 'D':
	  case 'Q': case 'R': case 'S': case 'T': case 'U':
	  case 'W': case 'Y': case 'Z':
	    {
	      enum reg_class cl;

	      cl = (c == 'r'
		    ? GENERAL_REGS : REG_CLASS_FROM_CONSTRAINT (c, str));
	      if (cl != NO_REGS)
		return -1;
#ifdef EXTRA_CONSTRAINT_STR
	      else if (EXTRA_CONSTRAINT_STR (op, c, str))
		return -1;
#endif
	      break;
	    }
	    
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	    if (original != -1 && original != c)
	      return -1;
	    original = c;
	    break;
	  }
      str += CONSTRAINT_LEN (c, str);
    }
  if (original == -1)
    return -1;
  dup = original - '0';
  if (use_commut_op_p)
    {
      if (commutative_constraint_p (recog_data.constraints[dup]))
	dup++;
      else if (dup > 0
	       && commutative_constraint_p (recog_data.constraints[dup -1]))
	dup--;
      else if (! commut_op_used_p)
	return -1;
    }
  return dup;
}

/* The function returns operand which should be in any case the same
   as operand with number OP_NUM.  If USE_COMMUT_OP_P is TRUE, the
   function makes temporarily commutative operand exchange before
   this.  */
static rtx
get_dup (int op_num, bool use_commut_op_p)
{
  int n = get_dup_num (op_num, use_commut_op_p);

  if (n < 0)
    return NULL_RTX;
  else
    return recog_data.operand[n];
}

/* Process registers REG1 and REG2 in move INSN with execution
   frequency FREQ.  The function also processes the registers in a
   potential move insn (INSN == NULL in this case) with frequency
   FREQ.  The function can modify hard register costs of the
   corresponding allocnos or create a copy involving the corresponding
   allocnos.  The function does nothing if the both registers are hard
   registers.  When nothing is changed, the function returns
   FALSE.  */
static bool
process_regs_for_copy (rtx reg1, rtx reg2, rtx insn, int freq)
{
  int hard_regno, cost, index;
  allocno_t a;
  enum reg_class class, cover_class;
  enum machine_mode mode;
  copy_t cp;

  gcc_assert (REG_P (reg1) && REG_P (reg2));
  if (HARD_REGISTER_P (reg1))
    {
      if (HARD_REGISTER_P (reg2))
	return false;
      hard_regno = REGNO (reg1);
      a = ira_curr_regno_allocno_map[REGNO (reg2)];
    }
  else if (HARD_REGISTER_P (reg2))
    {
      hard_regno = REGNO (reg2);
      a = ira_curr_regno_allocno_map[REGNO (reg1)];
    }
  else
    {
      cp = add_allocno_copy (ira_curr_regno_allocno_map[REGNO (reg1)],
			     ira_curr_regno_allocno_map[REGNO (reg2)],
			     freq, insn, ira_curr_loop_tree_node);
      bitmap_set_bit (ira_curr_loop_tree_node->local_copies, cp->num); 
      return true;
    }
  class = REGNO_REG_CLASS (hard_regno);
  mode = ALLOCNO_MODE (a);
  cover_class = ALLOCNO_COVER_CLASS (a);
  if (! class_subset_p[class][cover_class])
    return false;
  if (reg_class_size[class] <= (unsigned) CLASS_MAX_NREGS (class, mode))
    /* It is already taken into account in ira-costs.c.  */
    return false;
  index = class_hard_reg_index[cover_class][hard_regno];
  if (index < 0)
    return false;
  if (HARD_REGISTER_P (reg1))
    cost = register_move_cost[mode][cover_class][class] * freq;
  else
    cost = register_move_cost[mode][class][cover_class] * freq;
  allocate_and_set_costs
    (&ALLOCNO_HARD_REG_COSTS (a), cover_class,
     ALLOCNO_COVER_CLASS_COST (a));
  allocate_and_set_costs
    (&ALLOCNO_CONFLICT_HARD_REG_COSTS (a), cover_class, 0);
  ALLOCNO_HARD_REG_COSTS (a)[index] -= cost;
  ALLOCNO_CONFLICT_HARD_REG_COSTS (a)[index] -= cost;
  return true;
}

/* The function processes all output registers of the current insn and
   input register REG (its operand number OP_NUM) which dies in the
   insn as if there were a move insn between them with frequency
   FREQ.  */
static void
process_reg_shuffles (rtx reg, int op_num, int freq)
{
  int i;
  rtx another_reg;

  gcc_assert (REG_P (reg));
  for (i = 0; i < recog_data.n_operands; i++)
    {
      another_reg = recog_data.operand[i];
      
      if (!REG_P (another_reg) || op_num == i
	  || recog_data.operand_type[i] != OP_OUT)
	continue;
      
      process_regs_for_copy (reg, another_reg, NULL_RTX, freq);
    }
}

/* The function processes INSN and create allocno copies if
   necessary.  For example, it might be because INSN is a
   pseudo-register move or INSN is two operand insn.  */
static void
add_insn_allocno_copies (rtx insn)
{
  rtx set, operand, dup;
  const char *str;
  bool commut_p, bound_p;
  int i, j, freq;
  
  freq = REG_FREQ_FROM_BB (BLOCK_FOR_INSN (insn));
  if (freq == 0)
    freq = 1;
  if ((set = single_set (insn)) != NULL_RTX
      && REG_P (SET_DEST (set)) && REG_P (SET_SRC (set))
      && ! side_effects_p (set)
      && find_reg_note (insn, REG_DEAD, SET_SRC (set)) != NULL_RTX
      && find_reg_note (insn, REG_RETVAL, NULL_RTX) == NULL_RTX)
    process_regs_for_copy (SET_DEST (set), SET_SRC (set), insn, freq);
  else
    {
      extract_insn (insn);
      for (i = 0; i < recog_data.n_operands; i++)
	{
	  operand = recog_data.operand[i];
	  if (REG_P (operand)
	      && find_reg_note (insn, REG_DEAD, operand) != NULL_RTX)
	    {
	      str = recog_data.constraints[i];
	      while (*str == ' ' && *str == '\t')
		str++;
	      bound_p = false;
	      for (j = 0, commut_p = false; j < 2; j++, commut_p = true)
		if ((dup = get_dup (i, commut_p)) != NULL_RTX
		    && REG_P (dup) && GET_MODE (operand) == GET_MODE (dup)
		    && process_regs_for_copy (operand, dup, NULL_RTX, freq))
		  bound_p = true;
	      if (bound_p)
		continue;
	      /* If an operand dies, prefer its hard register for the
		 output operands by decreasing the hard register cost
		 or creating the corresponding allocno copies.  The
		 cost will not correspond to a real move insn cost, so
		 make the frequency smaller.  */
	      process_reg_shuffles (operand, i, freq < 8 ? 1 : freq / 8);
	    }
	}
    }
}

/* The function adds copies originated from BB given by
   LOOP_TREE_NODE.  */
static void
add_copies (loop_tree_node_t loop_tree_node)
{
  basic_block bb;
  rtx insn;

  bb = loop_tree_node->bb;
  if (bb == NULL)
    return;
  FOR_BB_INSNS (bb, insn)
    if (INSN_P (insn))
      add_insn_allocno_copies (insn);
}

/* The function propagates copy info for allocno A to the
   corresponding allocno on upper loop tree level.  So allocnos on
   upper levels accumulate information about the corresponding
   allocnos in nested regions.  */
static void
propagate_allocno_copy_info (allocno_t a)
{
  int regno;
  allocno_t parent_a, another_a, another_parent_a;
  loop_tree_node_t parent;
  copy_t cp, next_cp;

  regno = ALLOCNO_REGNO (a);
  if ((parent = ALLOCNO_LOOP_TREE_NODE (a)->parent) != NULL
      && (parent_a = parent->regno_allocno_map[regno]) != NULL)
    {
      for (cp = ALLOCNO_COPIES (a); cp != NULL; cp = next_cp)
	{
	  if (cp->first == a)
	    {
	      next_cp = cp->next_first_allocno_copy;
	      another_a = cp->second;
	    }
	  else if (cp->second == a)
	    {
	      next_cp = cp->next_second_allocno_copy;
	      another_a = cp->first;
	    }
	  else
	    gcc_unreachable ();
	  if ((another_parent_a = (parent->regno_allocno_map
				   [ALLOCNO_REGNO (another_a)])) != NULL)
	    add_allocno_copy (parent_a, another_parent_a, cp->freq,
			      cp->insn, cp->loop_tree_node);
	}
    }
}

/* The function propagates copy info to the corresponding allocnos on
   upper loop tree level.  */
static void
propagate_copy_info (void)
{
  int i;
  allocno_t a;

  for (i = max_reg_num () - 1; i >= FIRST_PSEUDO_REGISTER; i--)
    for (a = regno_allocno_map[i];
	 a != NULL;
	 a = ALLOCNO_NEXT_REGNO_ALLOCNO (a))
      propagate_allocno_copy_info (a);
}

/* The function returns TRUE if live ranges of allocnos A1 and A2
   intersect.  It is used to find a conflict for new allocnos or
   allocnos with the different cover classes.  */
bool
allocno_live_ranges_intersect_p (allocno_t a1, allocno_t a2)
{
  allocno_live_range_t r1, r2;

  if (a1 == a2)
    return false;
  if (ALLOCNO_REG (a1) != NULL && ALLOCNO_REG (a2) != NULL
      && (ORIGINAL_REGNO (ALLOCNO_REG (a1))
	  == ORIGINAL_REGNO (ALLOCNO_REG (a2))))
    return false;
  /* Remember the ranges are always kept ordered.  */
  for (r1 = ALLOCNO_LIVE_RANGES (a1), r2 = ALLOCNO_LIVE_RANGES (a2);
       r1 != NULL && r2 != NULL;)
    {
      if (r1->start > r2->finish)
	r1 = r1->next;
      else if (r2->start > r1->finish)
	r2 = r2->next;
      else
	return true;
    }
  return false;
}

/* The function returns TRUE if live ranges of pseudo-registers REGNO1
   and REGNO2 intersect.  It should be used when there is only one
   region.  Currently it is used during the reload work.  */
bool
pseudo_live_ranges_intersect_p (int regno1, int regno2)
{
  allocno_t a1, a2;

  ira_assert (regno1 >= FIRST_PSEUDO_REGISTER
	      && regno2 >= FIRST_PSEUDO_REGISTER);
  /* Reg info caclulated by dataflow infrastructure can be different
     from one calculated by regclass.  */
  if ((a1 = ira_loop_tree_root->regno_allocno_map[regno1]) == NULL
      || (a2 = ira_loop_tree_root->regno_allocno_map[regno2]) == NULL)
    return false;
  return allocno_live_ranges_intersect_p (a1, a2);
}

/* Remove copies involving conflicting allocnos.  We can not do this
   at the copy creation time because there are no conflicts at that
   time yet.  */
static void
remove_conflict_allocno_copies (void)
{
  int i;
  allocno_t a;
  allocno_iterator ai;
  copy_t cp, next_cp;
  VEC(copy_t,heap) *conflict_allocno_copy_vec;

  conflict_allocno_copy_vec = VEC_alloc (copy_t, heap, get_max_uid ());
  FOR_EACH_ALLOCNO (a, ai)
    {
      for (cp = ALLOCNO_COPIES (a); cp != NULL; cp = next_cp)
	if (cp->first == a)
	  next_cp = cp->next_first_allocno_copy;
	else
	  {
	    next_cp = cp->next_second_allocno_copy;
	    VEC_safe_push (copy_t, heap, conflict_allocno_copy_vec, cp);
	  }
    }
  for (i = 0; VEC_iterate (copy_t, conflict_allocno_copy_vec, i, cp); i++)
    if (CONFLICT_ALLOCNO_P (cp->first, cp->second))
      remove_allocno_copy_from_list (cp);
  VEC_free (copy_t, heap, conflict_allocno_copy_vec);
}

/* The function builds conflict vectors or bit conflict vectors
   (whatever is more profitable) of all allocnos from the conflict
   table.  */
static void
build_allocno_conflicts (void)
{
  int i, j, px, parent_num;
  bool free_p;
  int conflict_bit_vec_words_num;
  loop_tree_node_t parent;
  allocno_t a, parent_a, another_a, another_parent_a, *conflict_allocnos, *vec;
  INT_TYPE *allocno_conflicts;
  allocno_set_iterator asi;

  conflict_allocnos = ira_allocate (sizeof (allocno_t) * allocnos_num);
  for (i = max_reg_num () - 1; i >= FIRST_PSEUDO_REGISTER; i--)
    for (a = regno_allocno_map[i];
	 a != NULL;
	 a = ALLOCNO_NEXT_REGNO_ALLOCNO (a))
      {
	allocno_conflicts = conflicts[ALLOCNO_NUM (a)];
	px = 0;
	FOR_EACH_ALLOCNO_IN_SET (allocno_conflicts,
				 ALLOCNO_MIN (a), ALLOCNO_MAX (a), j, asi)
	  {
	    another_a = conflict_id_allocno_map[j];
	    ira_assert (ALLOCNO_COVER_CLASS (a)
			== ALLOCNO_COVER_CLASS (another_a));
	    conflict_allocnos[px++] = another_a;
	  }
	if (conflict_vector_profitable_p (a, px))
	  {
	    free_p = true;
	    allocate_allocno_conflict_vec (a, px);
	    vec = ALLOCNO_CONFLICT_ALLOCNO_ARRAY (a);
	    memcpy (vec, conflict_allocnos, sizeof (allocno_t) * px);
	    vec[px] = NULL;
	    ALLOCNO_CONFLICT_ALLOCNOS_NUM (a) = px;
	  }
	else
	  {
	    free_p = false;
	    ALLOCNO_CONFLICT_ALLOCNO_ARRAY (a) = conflicts[ALLOCNO_NUM (a)];
	    if (ALLOCNO_MAX (a) < ALLOCNO_MIN (a))
	      conflict_bit_vec_words_num = 0;
	    else
	      conflict_bit_vec_words_num
		= (ALLOCNO_MAX (a) - ALLOCNO_MIN (a) + INT_BITS) / INT_BITS;
	    ALLOCNO_CONFLICT_ALLOCNO_ARRAY_SIZE (a)
	      = conflict_bit_vec_words_num * sizeof (INT_TYPE);
	  }
	if ((parent = ALLOCNO_LOOP_TREE_NODE (a)->parent) == NULL
	    || (parent_a = parent->regno_allocno_map[i]) == NULL)
	  {
	    if (free_p)
	      ira_free (conflicts[ALLOCNO_NUM (a)]);
	    continue;
	  }
	ira_assert (ALLOCNO_COVER_CLASS (a) == ALLOCNO_COVER_CLASS (parent_a));
	parent_num = ALLOCNO_NUM (parent_a);
	FOR_EACH_ALLOCNO_IN_SET (allocno_conflicts,
				 ALLOCNO_MIN (a), ALLOCNO_MAX (a), j, asi)
	  {
	    another_a = conflict_id_allocno_map[j];
	    ira_assert (ALLOCNO_COVER_CLASS (a)
			== ALLOCNO_COVER_CLASS (another_a));
	    if ((another_parent_a = (parent->regno_allocno_map
				     [ALLOCNO_REGNO (another_a)])) == NULL)
	      continue;
	    ira_assert (ALLOCNO_NUM (another_parent_a) >= 0);
	    ira_assert (ALLOCNO_COVER_CLASS (another_a)
			== ALLOCNO_COVER_CLASS (another_parent_a));
	    SET_ALLOCNO_SET_BIT (conflicts[parent_num],
				 ALLOCNO_CONFLICT_ID (another_parent_a),
				 ALLOCNO_MIN (parent_a),
				 ALLOCNO_MAX (parent_a));
	  }
	if (free_p)
	  ira_free (conflicts[ALLOCNO_NUM (a)]);
      }
  ira_free (conflict_allocnos);
  ira_free (conflicts);
}



/* The function propagates information about allocnos modified inside
   the loop given by its LOOP_TREE_NODE to its parent.  */
static void
propagate_modified_regnos (loop_tree_node_t loop_tree_node)
{
  if (loop_tree_node == ira_loop_tree_root)
    return;
  ira_assert (loop_tree_node->bb == NULL);
  bitmap_ior_into (loop_tree_node->parent->modified_regnos,
		   loop_tree_node->modified_regnos);
}



/* The function prints hard reg set SET with TITLE to FILE.  */
static void
print_hard_reg_set (FILE *file, const char *title, HARD_REG_SET set)
{
  int i, start;

  fprintf (file, title);
  for (start = -1, i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      if (TEST_HARD_REG_BIT (set, i))
	{
	  if (i == 0 || ! TEST_HARD_REG_BIT (set, i - 1))
	    start = i;
	}
      if (start >= 0
	  && (i == FIRST_PSEUDO_REGISTER - 1 || ! TEST_HARD_REG_BIT (set, i)))
	{
	  if (start == i - 1)
	    fprintf (file, " %d", start);
	  else if (start == i - 2)
	    fprintf (file, " %d %d", start, start + 1);
	  else
	    fprintf (file, " %d-%d", start, i - 1);
	  start = -1;
	}
    }
  fprintf (file, "\n");
}

/* The function prints information about allocno or only regno (if
   REG_P) conflicts to FILE.  */
static void
print_conflicts (FILE *file, bool reg_p)
{
  allocno_t a;
  allocno_iterator ai;
  HARD_REG_SET conflicting_hard_regs;

  FOR_EACH_ALLOCNO (a, ai)
    {
      allocno_t conflict_a;
      allocno_conflict_iterator aci;
      basic_block bb;

      if (reg_p)
	fprintf (file, ";; r%d", ALLOCNO_REGNO (a));
      else
	{
	  fprintf (file, ";; a%d(r%d,", ALLOCNO_NUM (a), ALLOCNO_REGNO (a));
	  if ((bb = ALLOCNO_LOOP_TREE_NODE (a)->bb) != NULL)
	    fprintf (file, "b%d", bb->index);
	  else
	    fprintf (file, "l%d", ALLOCNO_LOOP_TREE_NODE (a)->loop->num);
	  fprintf (file, ")");
	}
      fprintf (file, " conflicts:");
      if (ALLOCNO_CONFLICT_ALLOCNO_ARRAY (a) != NULL)
	FOR_EACH_ALLOCNO_CONFLICT (a, conflict_a, aci)
	  {
	    if (reg_p)
	      fprintf (file, " r%d,", ALLOCNO_REGNO (conflict_a));
	    else
	      {
		fprintf (file, " a%d(r%d,", ALLOCNO_NUM (conflict_a),
			 ALLOCNO_REGNO (conflict_a));
		if ((bb = ALLOCNO_LOOP_TREE_NODE (conflict_a)->bb) != NULL)
		  fprintf (file, "b%d)", bb->index);
		else
		  fprintf (file, "l%d)",
			   ALLOCNO_LOOP_TREE_NODE (conflict_a)->loop->num);
	      }
	  }
      COPY_HARD_REG_SET (conflicting_hard_regs,
			 ALLOCNO_TOTAL_CONFLICT_HARD_REGS (a));
      AND_COMPL_HARD_REG_SET (conflicting_hard_regs, no_alloc_regs);
      AND_HARD_REG_SET (conflicting_hard_regs,
			reg_class_contents[ALLOCNO_COVER_CLASS (a)]);
      print_hard_reg_set (file, "\n;;     total conflict hard regs:",
			  conflicting_hard_regs);
      COPY_HARD_REG_SET (conflicting_hard_regs,
			 ALLOCNO_CONFLICT_HARD_REGS (a));
      AND_COMPL_HARD_REG_SET (conflicting_hard_regs, no_alloc_regs);
      AND_HARD_REG_SET (conflicting_hard_regs,
			reg_class_contents[ALLOCNO_COVER_CLASS (a)]);
      print_hard_reg_set (file, ";;     conflict hard regs:",
			  conflicting_hard_regs);
    }
  fprintf (file, "\n");
}

/* The function prints information about allocno or only regno (if
   REG_P) conflicts to stderr.  */
void
debug_conflicts (bool reg_p)
{
  print_conflicts (stderr, reg_p);
}



/* Entry function which builds allocno conflicts and allocno copies
   and accumulate some allocno info on upper level regions.  */
void
ira_build_conflicts (void)
{
  allocno_t a;
  allocno_iterator ai;

  if (optimize)
    {
      build_conflict_bit_table ();
      traverse_loop_tree (true, ira_loop_tree_root, NULL, add_copies);
      if (flag_ira_algorithm == IRA_ALGORITHM_REGIONAL
	  || flag_ira_algorithm == IRA_ALGORITHM_MIXED)
	propagate_copy_info ();
      /* We need finished conflict table for the subsequent call.  */
      remove_conflict_allocno_copies ();
      build_allocno_conflicts ();
    }
  FOR_EACH_ALLOCNO (a, ai)
    {
      if (ALLOCNO_CALLS_CROSSED_NUM (a) == 0)
	continue;
      if (! flag_caller_saves)
	{
	  IOR_HARD_REG_SET (ALLOCNO_TOTAL_CONFLICT_HARD_REGS (a),
			    call_used_reg_set);
	  if (ALLOCNO_CALLS_CROSSED_NUM (a) != 0)
	    IOR_HARD_REG_SET (ALLOCNO_CONFLICT_HARD_REGS (a),
			      call_used_reg_set);
	}
      else
	{
	  IOR_HARD_REG_SET (ALLOCNO_TOTAL_CONFLICT_HARD_REGS (a),
			    no_caller_save_reg_set);
	  if (ALLOCNO_CALLS_CROSSED_NUM (a) != 0)
	    IOR_HARD_REG_SET (ALLOCNO_CONFLICT_HARD_REGS (a),
			      no_caller_save_reg_set);
	}
    }
  if (optimize)
    {
      traverse_loop_tree (false, ira_loop_tree_root, NULL,
			  propagate_modified_regnos);
      if (internal_flag_ira_verbose > 2 && ira_dump_file != NULL)
	print_conflicts (ira_dump_file, false);
    }
}
