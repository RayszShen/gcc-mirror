/* Compute cover class of the allocnos and their hard register costs.
   Copyright (C) 2006, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Vladimir Makarov <vmakarov@redhat.com>.

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
Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "hard-reg-set.h"
#include "rtl.h"
#include "expr.h"
#include "tm_p.h"
#include "flags.h"
#include "basic-block.h"
#include "regs.h"
#include "addresses.h"
#include "insn-config.h"
#include "recog.h"
#include "toplev.h"
#include "target.h"
#include "params.h"
#include "ira-int.h"

/* The file contains code is analogous to one in regclass but the code
   works on the allocno basis.  */

struct costs;

static void record_reg_classes (int, int, rtx *, enum machine_mode *,
				const char **, rtx, struct costs **,
				enum reg_class *);
static inline int ok_for_index_p_nonstrict (rtx);
static inline int ok_for_base_p_nonstrict (rtx, enum machine_mode,
					   enum rtx_code, enum rtx_code);
static void record_address_regs (enum machine_mode, rtx x, int,
				 enum rtx_code, enum rtx_code, int scale);
static void record_operand_costs (rtx, struct costs **, enum reg_class *);
static rtx scan_one_insn (rtx);
static void print_costs (FILE *);
static void process_bb_node_for_costs (loop_tree_node_t);
static void find_allocno_class_costs (void);
static void process_bb_node_for_hard_reg_moves (loop_tree_node_t);
static void setup_allocno_cover_class_and_costs (void);
static void free_ira_costs (void);

#ifdef FORBIDDEN_INC_DEC_CLASSES
/* Indexed by n, is nonzero if (REG n) is used in an auto-inc or
   auto-dec context.  */
static char *in_inc_dec;
#endif

/* The `costs' struct records the cost of using a hard register of
   each class and of using memory for each allocno.  We use this data
   to set up register and costs.  */
struct costs
{
  int mem_cost;
  /* Costs for important register classes start here.  */
  int cost [1];
};

/* Initialized once.  It is a size of the allocated struct costs.  */
static int max_struct_costs_size;

/* Allocated and initialized once, and used to initialize cost values
   for each insn.  */
static struct costs *init_cost;

/* Allocated once, and used for temporary purposes.  */
static struct costs *temp_costs;

/* Allocated once, and used for the cost calculation.  */
static struct costs *op_costs [MAX_RECOG_OPERANDS];
static struct costs *this_op_costs [MAX_RECOG_OPERANDS];

/* Record the initial and accumulated cost of each class for each
   allocno.  */
static struct costs *total_costs;

/* Classes used for cost calculation.  */
static enum reg_class *cost_classes;

/* The size of the previous array.  */
static int cost_classes_num;

/* The array containing order numbers of cost classes.  */
static int cost_class_nums [N_REG_CLASSES];

/* It is the current size of struct costs.  */
static int struct_costs_size;

/* Return pointer to structure containing costs of allocno with given
   NUM in array ARR.  */
#define COSTS_OF_ALLOCNO(arr, num) \
  ((struct costs *) ((char *) (arr) + (num) * struct_costs_size))

/* Record register class preferences of each allocno.  */
static enum reg_class *allocno_pref;

/* Allocated buffers for allocno_pref.  */
static enum reg_class *allocno_pref_buffer;

/* Frequency of executions of the current insn.  */
static int frequency;

/* Compute the cost of loading X into (if TO_P is nonzero) or from (if
   TO_P is zero) a register of class CLASS in mode MODE.  X must not
   be a pseudo register.  */
static int
copy_cost (rtx x, enum machine_mode mode, enum reg_class class, int to_p,
	   secondary_reload_info *prev_sri)
{
  secondary_reload_info sri;
  enum reg_class secondary_class = NO_REGS;

  /* If X is a SCRATCH, there is actually nothing to move since we are
     assuming optimal allocation.  */
  if (GET_CODE (x) == SCRATCH)
    return 0;

  /* Get the class we will actually use for a reload.  */
  class = PREFERRED_RELOAD_CLASS (x, class);

  /* If we need a secondary reload for an intermediate, the cost is
     that to load the input into the intermediate register, then to
     copy it.  */
  sri.prev_sri = prev_sri;
  sri.extra_cost = 0;
  secondary_class = targetm.secondary_reload (to_p, x, class, mode, &sri);

  if (register_move_cost [mode] == NULL)
    init_register_move_cost (mode);

  if (secondary_class != NO_REGS)
    return (move_cost [mode] [secondary_class] [class]
	    + sri.extra_cost
	    + copy_cost (x, mode, secondary_class, to_p, &sri));

  /* For memory, use the memory move cost, for (hard) registers, use
     the cost to move between the register classes, and use 2 for
     everything else (constants).  */
  if (MEM_P (x) || class == NO_REGS)
    return sri.extra_cost + memory_move_cost [mode] [class] [to_p != 0];
  else if (REG_P (x))
    return
      (sri.extra_cost
       + move_cost [mode] [REGNO_REG_CLASS (REGNO (x))] [class]);
  else
    /* If this is a constant, we may eventually want to call rtx_cost
       here.  */
    return sri.extra_cost + COSTS_N_INSNS (1);
}



/* Record the cost of using memory or registers of various classes for
   the operands in INSN.

   N_ALTS is the number of alternatives.
   N_OPS is the number of operands.
   OPS is an array of the operands.
   MODES are the modes of the operands, in case any are VOIDmode.
   CONSTRAINTS are the constraints to use for the operands.  This array
   is modified by this procedure.

   This procedure works alternative by alternative.  For each
   alternative we assume that we will be able to allocate all allocnos
   to their ideal register class and calculate the cost of using that
   alternative.  Then we compute for each operand that is a
   pseudo-register, the cost of having the allocno allocated to each
   register class and using it in that alternative.  To this cost is
   added the cost of the alternative.

   The cost of each class for this insn is its lowest cost among all
   the alternatives.  */
static void
record_reg_classes (int n_alts, int n_ops, rtx *ops,
		    enum machine_mode *modes, const char **constraints,
		    rtx insn, struct costs **op_costs,
		    enum reg_class *allocno_pref)
{
  int alt;
  int i, j, k;
  rtx set;

  /* Process each alternative, each time minimizing an operand's cost
     with the cost for each operand in that alternative.  */
  for (alt = 0; alt < n_alts; alt++)
    {
      enum reg_class classes [MAX_RECOG_OPERANDS];
      int allows_mem [MAX_RECOG_OPERANDS];
      int class;
      int alt_fail = 0;
      int alt_cost = 0, op_cost_add;

      for (i = 0; i < n_ops; i++)
	{
	  unsigned char c;
	  const char *p = constraints [i];
	  rtx op = ops [i];
	  enum machine_mode mode = modes [i];
	  int allows_addr = 0;
	  int win = 0;

	  /* Initially show we know nothing about the register class.  */
	  classes [i] = NO_REGS;
	  allows_mem [i] = 0;

	  /* If this operand has no constraints at all, we can
	     conclude nothing about it since anything is valid.  */
	  if (*p == 0)
	    {
	      if (REG_P (op) && REGNO (op) >= FIRST_PSEUDO_REGISTER)
		memset (this_op_costs [i], 0, struct_costs_size);
	      continue;
	    }

	  /* If this alternative is only relevant when this operand
	     matches a previous operand, we do different things
	     depending on whether this operand is a allocno-reg or not.
	     We must process any modifiers for the operand before we
	     can make this test.  */
	  while (*p == '%' || *p == '=' || *p == '+' || *p == '&')
	    p++;

	  if (p [0] >= '0' && p [0] <= '0' + i && (p [1] == ',' || p [1] == 0))
	    {
	      /* Copy class and whether memory is allowed from the
		 matching alternative.  Then perform any needed cost
		 computations and/or adjustments.  */
	      j = p [0] - '0';
	      classes [i] = classes [j];
	      allows_mem [i] = allows_mem [j];

	      if (! REG_P (op) || REGNO (op) < FIRST_PSEUDO_REGISTER)
		{
		  /* If this matches the other operand, we have no
		     added cost and we win.  */
		  if (rtx_equal_p (ops [j], op))
		    win = 1;
		  /* If we can put the other operand into a register,
		     add to the cost of this alternative the cost to
		     copy this operand to the register used for the
		     other operand.  */
		  else if (classes [j] != NO_REGS)
		    {
		      alt_cost += copy_cost (op, mode, classes [j], 1, NULL);
		      win = 1;
		    }
		}
	      else if (! REG_P (ops [j])
		       || REGNO (ops [j]) < FIRST_PSEUDO_REGISTER)
		{
		  /* This op is a allocno but the one it matches is
		     not.  */

		  /* If we can't put the other operand into a
		     register, this alternative can't be used.  */

		  if (classes [j] == NO_REGS)
		    alt_fail = 1;
		  /* Otherwise, add to the cost of this alternative
		     the cost to copy the other operand to the
		     register used for this operand.  */
		  else
		    alt_cost
		      += copy_cost (ops [j], mode, classes [j], 1, NULL);
		}
	      else
		{
		  /* The costs of this operand are not the same as the
		     other operand since move costs are not symmetric.
		     Moreover, if we cannot tie them, this alternative
		     needs to do a copy, which is one instruction.  */
		  struct costs *pp = this_op_costs [i];

		  if (register_move_cost [mode] == NULL)
		    init_register_move_cost (mode);

		  for (k = 0; k < cost_classes_num; k++)
		    {
		      class = cost_classes [k];
		      pp->cost [k]
			= ((recog_data.operand_type [i] != OP_OUT
			    ? register_may_move_in_cost [mode]
			      [class] [classes [i]] * frequency : 0)
			   + (recog_data.operand_type [i] != OP_IN
			      ? register_may_move_out_cost [mode]
			        [classes [i]] [class] * frequency : 0));
		    }

		  /* If the alternative actually allows memory, make
		     things a bit cheaper since we won't need an extra
		     insn to load it.  */
		  pp->mem_cost
		    = ((recog_data.operand_type [i] != OP_IN
			? memory_move_cost [mode] [classes [i]] [0]
			: 0) * frequency
		       + (recog_data.operand_type [i] != OP_OUT
			  ? memory_move_cost [mode] [classes [i]] [1]
			  : 0) * frequency - allows_mem [i] * frequency / 2);

		  /* If we have assigned a class to this allocno in our
		     first pass, add a cost to this alternative
		     corresponding to what we would add if this allocno
		     were not in the appropriate class.  We could use
		     cover class here but it is less accurate
		     approximation.  */
		  if (allocno_pref)
		    {
		      enum reg_class pref_class
			= allocno_pref [ALLOCNO_NUM
					(ira_curr_regno_allocno_map
					 [REGNO (op)])];

		      if (pref_class == NO_REGS)
			alt_cost
			  += ((recog_data.operand_type [i] != OP_IN
			       ? memory_move_cost [mode] [classes [i]] [0]
			       : 0)
			      + (recog_data.operand_type [i] != OP_OUT
				 ? memory_move_cost [mode] [classes [i]] [1]
				 : 0));
		      else if (reg_class_intersect
			       [pref_class] [classes [i]] == NO_REGS)
			alt_cost
			  += (register_move_cost
			      [mode] [pref_class] [classes [i]]);
		    }
		  if (REGNO (ops [i]) != REGNO (ops [j])
		      && ! find_reg_note (insn, REG_DEAD, op))
		    alt_cost += 2;

		  /* This is in place of ordinary cost computation for
		     this operand, so skip to the end of the
		     alternative (should be just one character).  */
		  while (*p && *p++ != ',')
		    ;

		  constraints [i] = p;
		  continue;
		}
	    }
	  
	  /* Scan all the constraint letters.  See if the operand
	     matches any of the constraints.  Collect the valid
	     register classes and see if this operand accepts
	     memory.  */
	  while ((c = *p))
	    {
	      switch (c)
		{
		case ',':
		  break;
		case '*':
		  /* Ignore the next letter for this pass.  */
		  c = *++p;
		  break;

		case '?':
		  alt_cost += 2;
		case '!':  case '#':  case '&':
		case '0':  case '1':  case '2':  case '3':  case '4':
		case '5':  case '6':  case '7':  case '8':  case '9':
		  break;

		case 'p':
		  allows_addr = 1;
		  win = address_operand (op, GET_MODE (op));
		  /* We know this operand is an address, so we want it
		     to be allocated to a register that can be the
		     base of an address, i.e. BASE_REG_CLASS.  */
		  classes [i]
		    = reg_class_union [classes [i]]
		      [base_reg_class (VOIDmode, ADDRESS, SCRATCH)];
		  break;

		case 'm':  case 'o':  case 'V':
		  /* It doesn't seem worth distinguishing between
		     offsettable and non-offsettable addresses
		     here.  */
		  allows_mem [i] = 1;
		  if (MEM_P (op))
		    win = 1;
		  break;

		case '<':
		  if (MEM_P (op)
		      && (GET_CODE (XEXP (op, 0)) == PRE_DEC
			  || GET_CODE (XEXP (op, 0)) == POST_DEC))
		    win = 1;
		  break;

		case '>':
		  if (MEM_P (op)
		      && (GET_CODE (XEXP (op, 0)) == PRE_INC
			  || GET_CODE (XEXP (op, 0)) == POST_INC))
		    win = 1;
		  break;

		case 'E':
		case 'F':
		  if (GET_CODE (op) == CONST_DOUBLE
		      || (GET_CODE (op) == CONST_VECTOR
			  && (GET_MODE_CLASS (GET_MODE (op))
			      == MODE_VECTOR_FLOAT)))
		    win = 1;
		  break;

		case 'G':
		case 'H':
		  if (GET_CODE (op) == CONST_DOUBLE
		      && CONST_DOUBLE_OK_FOR_CONSTRAINT_P (op, c, p))
		    win = 1;
		  break;

		case 's':
		  if (GET_CODE (op) == CONST_INT
		      || (GET_CODE (op) == CONST_DOUBLE
			  && GET_MODE (op) == VOIDmode))
		    break;

		case 'i':
		  if (CONSTANT_P (op)
		      && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op)))
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
		      && CONST_OK_FOR_CONSTRAINT_P (INTVAL (op), c, p))
		    win = 1;
		  break;

		case 'X':
		  win = 1;
		  break;

		case 'g':
		  if (MEM_P (op)
		      || (CONSTANT_P (op)
			  && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (op))))
		    win = 1;
		  allows_mem [i] = 1;
		case 'r':
		  classes [i] = reg_class_union [classes [i]] [GENERAL_REGS];
		  break;

		default:
		  if (REG_CLASS_FROM_CONSTRAINT (c, p) != NO_REGS)
		    classes [i] = reg_class_union [classes [i]]
		                  [REG_CLASS_FROM_CONSTRAINT (c, p)];
#ifdef EXTRA_CONSTRAINT_STR
		  else if (EXTRA_CONSTRAINT_STR (op, c, p))
		    win = 1;

		  if (EXTRA_MEMORY_CONSTRAINT (c, p))
		    {
		      /* Every MEM can be reloaded to fit.  */
		      allows_mem [i] = 1;
		      if (MEM_P (op))
			win = 1;
		    }
		  if (EXTRA_ADDRESS_CONSTRAINT (c, p))
		    {
		      /* Every address can be reloaded to fit.  */
		      allows_addr = 1;
		      if (address_operand (op, GET_MODE (op)))
			win = 1;
		      /* We know this operand is an address, so we
			 want it to be allocated to a register that
			 can be the base of an address,
			 i.e. BASE_REG_CLASS.  */
		      classes [i]
			= reg_class_union [classes [i]]
			  [base_reg_class (VOIDmode, ADDRESS, SCRATCH)];
		    }
#endif
		  break;
		}
	      p += CONSTRAINT_LEN (c, p);
	      if (c == ',')
		break;
	    }

	  constraints [i] = p;

	  /* How we account for this operand now depends on whether it
	     is a pseudo register or not.  If it is, we first check if
	     any register classes are valid.  If not, we ignore this
	     alternative, since we want to assume that all allocnos get
	     allocated for register preferencing.  If some register
	     class is valid, compute the costs of moving the allocno
	     into that class.  */
	  if (REG_P (op) && REGNO (op) >= FIRST_PSEUDO_REGISTER)
	    {
	      if (classes [i] == NO_REGS)
		{
		  /* We must always fail if the operand is a REG, but
		     we did not find a suitable class.

		     Otherwise we may perform an uninitialized read
		     from this_op_costs after the `continue' statement
		     below.  */
		  alt_fail = 1;
		}
	      else
		{
		  struct costs *pp = this_op_costs [i];

		  if (register_move_cost [mode] == NULL)
		    init_register_move_cost (mode);

		  for (k = 0; k < cost_classes_num; k++)
		    {
		      class = cost_classes [k];
		      pp->cost [k]
			= ((recog_data.operand_type [i] != OP_OUT
			    ? register_may_move_in_cost [mode]
			      [class] [classes [i]] * frequency : 0)
			   + (recog_data.operand_type [i] != OP_IN
			      ? register_may_move_out_cost [mode]
			        [classes [i]] [class] * frequency : 0));
		    }

		  /* If the alternative actually allows memory, make
		     things a bit cheaper since we won't need an extra
		     insn to load it.  */
		  pp->mem_cost
		    = ((recog_data.operand_type [i] != OP_IN
		        ? memory_move_cost [mode] [classes [i]] [0]
			: 0) * frequency
		       + (recog_data.operand_type [i] != OP_OUT
			  ? memory_move_cost [mode] [classes [i]] [1]
			  : 0) * frequency - allows_mem [i] * frequency / 2);

		  /* If we have assigned a class to this allocno in our
		     first pass, add a cost to this alternative
		     corresponding to what we would add if this allocno
		     were not in the appropriate class.  We could use
		     cover class here but it is less accurate
		     approximation.  */
		  if (allocno_pref)
		    {
		      enum reg_class pref_class
			= allocno_pref [ALLOCNO_NUM
					(ira_curr_regno_allocno_map
					 [REGNO (op)])];

		      if (pref_class == NO_REGS)
			alt_cost
			  += ((recog_data.operand_type [i] != OP_IN
			       ? memory_move_cost [mode] [classes [i]] [0]
			       : 0)
			      + (recog_data.operand_type [i] != OP_OUT
				 ? memory_move_cost [mode] [classes [i]] [1]
				 : 0));
		      else if (reg_class_intersect
			       [pref_class] [classes [i]] == NO_REGS)
			alt_cost
			  += (register_move_cost
			      [mode] [pref_class] [classes [i]]);
		    }
		}
	    }

	  /* Otherwise, if this alternative wins, either because we
	     have already determined that or if we have a hard
	     register of the proper class, there is no cost for this
	     alternative.  */
	  else if (win || (REG_P (op)
			   && reg_fits_class_p (op, classes [i],
						0, GET_MODE (op))))
	    ;

	  /* If registers are valid, the cost of this alternative
	     includes copying the object to and/or from a
	     register.  */
	  else if (classes [i] != NO_REGS)
	    {
	      if (recog_data.operand_type [i] != OP_OUT)
		alt_cost += copy_cost (op, mode, classes [i], 1, NULL);

	      if (recog_data.operand_type [i] != OP_IN)
		alt_cost += copy_cost (op, mode, classes [i], 0, NULL);
	    }
	  /* The only other way this alternative can be used is if
	     this is a constant that could be placed into memory.  */
	  else if (CONSTANT_P (op) && (allows_addr || allows_mem [i]))
	    alt_cost += memory_move_cost [mode] [classes [i]] [1];
	  else
	    alt_fail = 1;
	}

      if (alt_fail)
	continue;

      op_cost_add = alt_cost * frequency;
      /* Finally, update the costs with the information we've
	 calculated about this alternative.  */
      for (i = 0; i < n_ops; i++)
	if (REG_P (ops [i])
	    && REGNO (ops [i]) >= FIRST_PSEUDO_REGISTER)
	  {
	    struct costs *pp = op_costs [i], *qq = this_op_costs [i];
	    int scale = 1 + (recog_data.operand_type [i] == OP_INOUT);

	    pp->mem_cost = MIN (pp->mem_cost,
				(qq->mem_cost + op_cost_add) * scale);

	    for (k = 0; k < cost_classes_num; k++)
	      pp->cost [k]
		= MIN (pp->cost [k],
		       (qq->cost [k] + op_cost_add) * scale);
	  }
    }

  /* If this insn is a single set copying operand 1 to operand 0 and
     one operand is a allocno with the other a hard reg or a allocno
     that prefers a register that is in its own register class then we
     may want to adjust the cost of that register class to -1.

     Avoid the adjustment if the source does not die to avoid
     stressing of register allocator by preferrencing two colliding
     registers into single class.

     Also avoid the adjustment if a copy between registers of the
     class is expensive (ten times the cost of a default copy is
     considered arbitrarily expensive).  This avoids losing when the
     preferred class is very expensive as the source of a copy
     instruction.  */
  if ((set = single_set (insn)) != 0
      && ops [0] == SET_DEST (set) && ops [1] == SET_SRC (set)
      && REG_P (ops [0]) && REG_P (ops [1])
      && find_regno_note (insn, REG_DEAD, REGNO (ops [1])))
    for (i = 0; i <= 1; i++)
      if (REGNO (ops [i]) >= FIRST_PSEUDO_REGISTER)
	{
	  unsigned int regno = REGNO (ops [!i]);
	  enum machine_mode mode = GET_MODE (ops [!i]);
	  int class;
	  unsigned int nr;

	  if (regno < FIRST_PSEUDO_REGISTER)
	    for (k = 0; k < cost_classes_num; k++)
	      {
		class = cost_classes [k];
		if (TEST_HARD_REG_BIT (reg_class_contents [class], regno)
		    && (reg_class_size [class]
			== (unsigned) CLASS_MAX_NREGS (class, mode)))
		  {
		    if (reg_class_size [class] == 1)
		      op_costs [i]->cost [k] = -frequency;
		    else
		      {
			for (nr = 0;
			     nr < (unsigned) hard_regno_nregs [regno] [mode];
			     nr++)
			  if (! TEST_HARD_REG_BIT (reg_class_contents [class],
						   regno + nr))
			    break;
			
			if (nr == (unsigned) hard_regno_nregs [regno] [mode])
			  op_costs [i]->cost [k] = -frequency;
		      }
		  }
	      }
	}
}



/* Wrapper around REGNO_OK_FOR_INDEX_P, to allow pseudo registers.  */
static inline int
ok_for_index_p_nonstrict (rtx reg)
{
  unsigned regno = REGNO (reg);

  return regno >= FIRST_PSEUDO_REGISTER || REGNO_OK_FOR_INDEX_P (regno);
}

/* A version of regno_ok_for_base_p for use during regclass, when all
   allocnos should count as OK.  Arguments as for
   regno_ok_for_base_p.  */
static inline int
ok_for_base_p_nonstrict (rtx reg, enum machine_mode mode,
			 enum rtx_code outer_code, enum rtx_code index_code)
{
  unsigned regno = REGNO (reg);

  if (regno >= FIRST_PSEUDO_REGISTER)
    return TRUE;
  return ok_for_base_p_1 (regno, mode, outer_code, index_code);
}

/* Record the pseudo registers we must reload into hard registers in a
   subexpression of a memory address, X.

   If CONTEXT is 0, we are looking at the base part of an address,
   otherwise we are looking at the index part.

   MODE is the mode of the memory reference; OUTER_CODE and INDEX_CODE
   give the context that the rtx appears in.  These three arguments
   are passed down to base_reg_class.

   SCALE is twice the amount to multiply the cost by (it is twice so
   we can represent half-cost adjustments).  */
static void
record_address_regs (enum machine_mode mode, rtx x, int context,
		     enum rtx_code outer_code, enum rtx_code index_code,
		     int scale)
{
  enum rtx_code code = GET_CODE (x);
  enum reg_class class;

  if (context == 1)
    class = INDEX_REG_CLASS;
  else
    class = base_reg_class (mode, outer_code, index_code);

  switch (code)
    {
    case CONST_INT:
    case CONST:
    case CC0:
    case PC:
    case SYMBOL_REF:
    case LABEL_REF:
      return;

    case PLUS:
      /* When we have an address that is a sum, we must determine
	 whether registers are "base" or "index" regs.  If there is a
	 sum of two registers, we must choose one to be the "base".
	 Luckily, we can use the REG_POINTER to make a good choice
	 most of the time.  We only need to do this on machines that
	 can have two registers in an address and where the base and
	 index register classes are different.

	 ??? This code used to set REGNO_POINTER_FLAG in some cases,
	 but that seems bogus since it should only be set when we are
	 sure the register is being used as a pointer.  */
      {
	rtx arg0 = XEXP (x, 0);
	rtx arg1 = XEXP (x, 1);
	enum rtx_code code0 = GET_CODE (arg0);
	enum rtx_code code1 = GET_CODE (arg1);

	/* Look inside subregs.  */
	if (code0 == SUBREG)
	  arg0 = SUBREG_REG (arg0), code0 = GET_CODE (arg0);
	if (code1 == SUBREG)
	  arg1 = SUBREG_REG (arg1), code1 = GET_CODE (arg1);

	/* If this machine only allows one register per address, it
	   must be in the first operand.  */
	if (MAX_REGS_PER_ADDRESS == 1)
	  record_address_regs (mode, arg0, 0, PLUS, code1, scale);

	/* If index and base registers are the same on this machine,
	   just record registers in any non-constant operands.  We
	   assume here, as well as in the tests below, that all
	   addresses are in canonical form.  */
	else if (INDEX_REG_CLASS == base_reg_class (VOIDmode, PLUS, SCRATCH))
	  {
	    record_address_regs (mode, arg0, context, PLUS, code1, scale);
	    if (! CONSTANT_P (arg1))
	      record_address_regs (mode, arg1, context, PLUS, code0, scale);
	  }

	/* If the second operand is a constant integer, it doesn't
	   change what class the first operand must be.  */
	else if (code1 == CONST_INT || code1 == CONST_DOUBLE)
	  record_address_regs (mode, arg0, context, PLUS, code1, scale);
	/* If the second operand is a symbolic constant, the first
	   operand must be an index register.  */
	else if (code1 == SYMBOL_REF || code1 == CONST || code1 == LABEL_REF)
	  record_address_regs (mode, arg0, 1, PLUS, code1, scale);
	/* If both operands are registers but one is already a hard
	   register of index or reg-base class, give the other the
	   class that the hard register is not.  */
	else if (code0 == REG && code1 == REG
		 && REGNO (arg0) < FIRST_PSEUDO_REGISTER
		 && (ok_for_base_p_nonstrict (arg0, mode, PLUS, REG)
		     || ok_for_index_p_nonstrict (arg0)))
	  record_address_regs (mode, arg1,
			       ok_for_base_p_nonstrict (arg0, mode, PLUS, REG)
			       ? 1 : 0,
			       PLUS, REG, scale);
	else if (code0 == REG && code1 == REG
		 && REGNO (arg1) < FIRST_PSEUDO_REGISTER
		 && (ok_for_base_p_nonstrict (arg1, mode, PLUS, REG)
		     || ok_for_index_p_nonstrict (arg1)))
	  record_address_regs (mode, arg0,
			       ok_for_base_p_nonstrict (arg1, mode, PLUS, REG)
			       ? 1 : 0,
			       PLUS, REG, scale);
	/* If one operand is known to be a pointer, it must be the
	   base with the other operand the index.  Likewise if the
	   other operand is a MULT.  */
	else if ((code0 == REG && REG_POINTER (arg0)) || code1 == MULT)
	  {
	    record_address_regs (mode, arg0, 0, PLUS, code1, scale);
	    record_address_regs (mode, arg1, 1, PLUS, code0, scale);
	  }
	else if ((code1 == REG && REG_POINTER (arg1)) || code0 == MULT)
	  {
	    record_address_regs (mode, arg0, 1, PLUS, code1, scale);
	    record_address_regs (mode, arg1, 0, PLUS, code0, scale);
	  }
	/* Otherwise, count equal chances that each might be a base or
	   index register.  This case should be rare.  */
	else
	  {
	    record_address_regs (mode, arg0, 0, PLUS, code1, scale / 2);
	    record_address_regs (mode, arg0, 1, PLUS, code1, scale / 2);
	    record_address_regs (mode, arg1, 0, PLUS, code0, scale / 2);
	    record_address_regs (mode, arg1, 1, PLUS, code0, scale / 2);
	  }
      }
      break;

      /* Double the importance of a allocno that is incremented or
	 decremented, since it would take two extra insns if it ends
	 up in the wrong place.  */
    case POST_MODIFY:
    case PRE_MODIFY:
      record_address_regs (mode, XEXP (x, 0), 0, code,
			   GET_CODE (XEXP (XEXP (x, 1), 1)), 2 * scale);
      if (REG_P (XEXP (XEXP (x, 1), 1)))
	record_address_regs (mode, XEXP (XEXP (x, 1), 1), 1, code, REG,
			     2 * scale);
      break;

    case POST_INC:
    case PRE_INC:
    case POST_DEC:
    case PRE_DEC:
      /* Double the importance of a allocno that is incremented or
	 decremented, since it would take two extra insns if it ends
	 up in the wrong place.  If the operand is a pseudo-register,
	 show it is being used in an INC_DEC context.  */
#ifdef FORBIDDEN_INC_DEC_CLASSES
      if (REG_P (XEXP (x, 0))
	  && REGNO (XEXP (x, 0)) >= FIRST_PSEUDO_REGISTER)
	in_inc_dec [ALLOCNO_NUM (ira_curr_regno_allocno_map
				 [REGNO (XEXP (x, 0))])] = 1;
#endif
      record_address_regs (mode, XEXP (x, 0), 0, code, SCRATCH, 2 * scale);
      break;

    case REG:
      {
	struct costs *pp;
	int i, k;

	if (REGNO (x) < FIRST_PSEUDO_REGISTER)
	  break;

	pp = COSTS_OF_ALLOCNO (total_costs,
			       ALLOCNO_NUM (ira_curr_regno_allocno_map
					    [REGNO (x)]));
	pp->mem_cost += (memory_move_cost [Pmode] [class] [1] * scale) / 2;
	if (register_move_cost [Pmode] == NULL)
	  init_register_move_cost (Pmode);
	for (k = 0; k < cost_classes_num; k++)
	  {
	    i = cost_classes [k];
	    pp->cost [k] += (register_may_move_in_cost [Pmode] [i] [class]
			     * scale) / 2;
	  }
      }
      break;

    default:
      {
	const char *fmt = GET_RTX_FORMAT (code);
	int i;
	for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
	  if (fmt [i] == 'e')
	    record_address_regs (mode, XEXP (x, i), context, code, SCRATCH,
				 scale);
      }
    }
}



/* Calculate the costs of insn operands.  */
static void
record_operand_costs (rtx insn, struct costs **op_costs,
		      enum reg_class *allocno_pref)
{
  const char *constraints [MAX_RECOG_OPERANDS];
  enum machine_mode modes [MAX_RECOG_OPERANDS];
  int i;

  for (i = 0; i < recog_data.n_operands; i++)
    {
      constraints [i] = recog_data.constraints [i];
      modes [i] = recog_data.operand_mode [i];
    }

  /* If we get here, we are set up to record the costs of all the
     operands for this insn.  Start by initializing the costs.  Then
     handle any address registers.  Finally record the desired classes
     for any allocnos, doing it twice if some pair of operands are
     commutative.  */
  for (i = 0; i < recog_data.n_operands; i++)
    {
      memmove (op_costs [i], init_cost, struct_costs_size);

      if (GET_CODE (recog_data.operand [i]) == SUBREG)
	recog_data.operand [i] = SUBREG_REG (recog_data.operand [i]);

      if (MEM_P (recog_data.operand [i]))
	record_address_regs (GET_MODE (recog_data.operand [i]),
			     XEXP (recog_data.operand [i], 0),
			     0, MEM, SCRATCH, frequency * 2);
      else if (constraints [i] [0] == 'p'
	       || EXTRA_ADDRESS_CONSTRAINT (constraints [i] [0],
					    constraints [i]))
	record_address_regs (VOIDmode, recog_data.operand [i], 0, ADDRESS,
			     SCRATCH, frequency * 2);
    }

  /* Check for commutative in a separate loop so everything will have
     been initialized.  We must do this even if one operand is a
     constant--see addsi3 in m68k.md.  */
  for (i = 0; i < (int) recog_data.n_operands - 1; i++)
    if (constraints [i] [0] == '%')
      {
	const char *xconstraints [MAX_RECOG_OPERANDS];
	int j;

	/* Handle commutative operands by swapping the constraints.
	   We assume the modes are the same.  */
	for (j = 0; j < recog_data.n_operands; j++)
	  xconstraints [j] = constraints [j];

	xconstraints [i] = constraints [i+1];
	xconstraints [i+1] = constraints [i];
	record_reg_classes (recog_data.n_alternatives, recog_data.n_operands,
			    recog_data.operand, modes,
			    xconstraints, insn, op_costs, allocno_pref);
      }
  record_reg_classes (recog_data.n_alternatives, recog_data.n_operands,
		      recog_data.operand, modes,
		      constraints, insn, op_costs, allocno_pref);
}



/* Process one insn INSN.  Scan it and record each time it would save
   code to put a certain allocnos in a certain class.  Return the last
   insn processed, so that the scan can be continued from there.  */
static rtx
scan_one_insn (rtx insn)
{
  enum rtx_code pat_code;
  rtx set, note;
  int i, k;

  if (!INSN_P (insn))
    return insn;

  pat_code = GET_CODE (PATTERN (insn));
  if (pat_code == USE || pat_code == CLOBBER || pat_code == ASM_INPUT
      || pat_code == ADDR_VEC || pat_code == ADDR_DIFF_VEC)
    return insn;

  set = single_set (insn);
  extract_insn (insn);

  /* If this insn loads a parameter from its stack slot, then it
     represents a savings, rather than a cost, if the parameter is
     stored in memory.  Record this fact.  */
  if (set != 0 && REG_P (SET_DEST (set)) && MEM_P (SET_SRC (set))
      && (note = find_reg_note (insn, REG_EQUIV, NULL_RTX)) != NULL_RTX
      && MEM_P (XEXP (note, 0)))
    {
      COSTS_OF_ALLOCNO (total_costs,
			ALLOCNO_NUM (ira_curr_regno_allocno_map
				     [REGNO (SET_DEST (set))]))->mem_cost
	-= (memory_move_cost [GET_MODE (SET_DEST (set))] [GENERAL_REGS] [1]
	    * frequency);
      record_address_regs (GET_MODE (SET_SRC (set)), XEXP (SET_SRC (set), 0),
			   0, MEM, SCRATCH, frequency * 2);
    }

  record_operand_costs (insn, op_costs, allocno_pref);

  /* Now add the cost for each operand to the total costs for its
     allocno.  */
  for (i = 0; i < recog_data.n_operands; i++)
    if (REG_P (recog_data.operand [i])
	&& REGNO (recog_data.operand [i]) >= FIRST_PSEUDO_REGISTER)
      {
	int regno = REGNO (recog_data.operand [i]);
	struct costs *p
	  = COSTS_OF_ALLOCNO (total_costs,
			      ALLOCNO_NUM (ira_curr_regno_allocno_map
					   [regno]));
	struct costs *q = op_costs [i];

	p->mem_cost += q->mem_cost;
	for (k = 0; k < cost_classes_num; k++)
	  p->cost [k] += q->cost [k];
      }

  return insn;
}



/* Dump allocnos costs.  */
static void
print_costs (FILE *f)
{
  int k;
  allocno_t a;
  allocno_iterator ai;

  fprintf (f, "\n");
  FOR_EACH_ALLOCNO (a, ai)
    {
      int i, class;
      basic_block bb;
      int regno = ALLOCNO_REGNO (a);

      i = ALLOCNO_NUM (a);
      fprintf (f, "  a%d(r%d,", i, regno);
      if ((bb = ALLOCNO_LOOP_TREE_NODE (a)->bb) != NULL)
	fprintf (f, "b%d", bb->index);
      else
	fprintf (f, "l%d", ALLOCNO_LOOP_TREE_NODE (a)->loop->num);
      fprintf (f, ") costs:");
      for (k = 0; k < cost_classes_num; k++)
	{
	  class = cost_classes [k];
	  if (contains_reg_of_mode [class] [PSEUDO_REGNO_MODE (regno)]
#ifdef FORBIDDEN_INC_DEC_CLASSES
	      && (! in_inc_dec [i] || ! forbidden_inc_dec_class [class])
#endif
#ifdef CANNOT_CHANGE_MODE_CLASS
	      && ! invalid_mode_change_p (i, (enum reg_class) class,
					  PSEUDO_REGNO_MODE (regno))
#endif
	      )
	    fprintf (f, " %s:%d", reg_class_names [class],
		     COSTS_OF_ALLOCNO (total_costs, i)->cost [k]);
	}
      fprintf (f, " MEM:%i\n", COSTS_OF_ALLOCNO (total_costs, i)->mem_cost);
    }
}

/* The function traverses basic blocks represented by LOOP_TREE_NODE
   to find the costs of the allocnos.  */
static void
process_bb_node_for_costs (loop_tree_node_t loop_tree_node)
{
  basic_block bb;
  rtx insn;

  bb = loop_tree_node->bb;
  if (bb == NULL)
    return;
  frequency = REG_FREQ_FROM_BB (bb);
  if (frequency == 0)
    frequency = 1;
  FOR_BB_INSNS (bb, insn)
    insn = scan_one_insn (insn);
}

/* Entry function to find costs of each class for pesudos and their
   best classes. */
static void
find_allocno_class_costs (void)
{
  int i, k;
  int pass;
  basic_block bb;

  init_recog ();
#ifdef FORBIDDEN_INC_DEC_CLASSES
  in_inc_dec = ira_allocate (sizeof (char) * allocnos_num);
#endif /* FORBIDDEN_INC_DEC_CLASSES */

  allocno_pref = NULL;
  /* Normally we scan the insns once and determine the best class to
     use for each allocno.  However, if -fexpensive_optimizations are
     on, we do so twice, the second time using the tentative best
     classes to guide the selection.  */
  for (pass = 0; pass <= flag_expensive_optimizations; pass++)
    {
      if (internal_flag_ira_verbose > 0 && ira_dump_file)
	fprintf (ira_dump_file, "\nPass %i for finding allocno costs\n\n",
		 pass);
      if (pass != flag_expensive_optimizations)
	{
	  for (cost_classes_num = 0;
	       cost_classes_num < reg_class_cover_size;
	       cost_classes_num++)
	    {
	      cost_classes [cost_classes_num]
		= reg_class_cover [cost_classes_num];
	      cost_class_nums [cost_classes [cost_classes_num]]
		= cost_classes_num;
	    }
	  struct_costs_size
	    = sizeof (struct costs) + sizeof (int) * (cost_classes_num - 1);
	}
      else
	{
	  for (cost_classes_num = 0;
	       cost_classes_num < important_classes_num;
	       cost_classes_num++)
	    {
	      cost_classes [cost_classes_num]
		= important_classes [cost_classes_num];
	      cost_class_nums [cost_classes [cost_classes_num]]
		= cost_classes_num;
	    }
	  struct_costs_size
	    = sizeof (struct costs) + sizeof (int) * (cost_classes_num - 1);
	}
      /* Zero out our accumulation of the cost of each class for each
	 allocno.  */
      memset (total_costs, 0, allocnos_num * struct_costs_size);
#ifdef FORBIDDEN_INC_DEC_CLASSES
      memset (in_inc_dec, 0, allocnos_num * sizeof (char));
#endif

      /* Scan the instructions and record each time it would save code
	 to put a certain allocno in a certain class.  */
      traverse_loop_tree (FALSE, ira_loop_tree_root,
			  process_bb_node_for_costs, NULL);

      /* Now for each allocno look at how desirable each class is and
	 find which class is preferred.  */
      if (pass == 0)
	allocno_pref = allocno_pref_buffer;

      for (i = max_reg_num () - 1; i >= FIRST_PSEUDO_REGISTER; i--)
	{
	  allocno_t a, father_a;
	  int class, a_num, father_a_num;
	  loop_tree_node_t father;
	  int best_cost;
	  enum reg_class best, common_class;
#ifdef FORBIDDEN_INC_DEC_CLASSES
	  int inc_dec_p = FALSE;
#endif

	  if (regno_allocno_map [i] == NULL)
	    continue;
	  memset (temp_costs, 0, struct_costs_size);
	  for (a = regno_allocno_map [i];
	       a != NULL;
	       a = ALLOCNO_NEXT_REGNO_ALLOCNO (a))
	    {
	      a_num = ALLOCNO_NUM (a);
	      if ((flag_ira_algorithm == IRA_ALGORITHM_REGIONAL
		   || flag_ira_algorithm == IRA_ALGORITHM_MIXED)
		  && (father = ALLOCNO_LOOP_TREE_NODE (a)->father) != NULL
		  && (father_a = father->regno_allocno_map [i]) != NULL)
		{
		  father_a_num = ALLOCNO_NUM (father_a);
		  for (k = 0; k < cost_classes_num; k++)
		    COSTS_OF_ALLOCNO (total_costs, father_a_num)->cost [k]
		      += COSTS_OF_ALLOCNO (total_costs, a_num)->cost [k];
		  COSTS_OF_ALLOCNO (total_costs, father_a_num)->mem_cost
		    += COSTS_OF_ALLOCNO (total_costs, a_num)->mem_cost;
		}
	      for (k = 0; k < cost_classes_num; k++)
		temp_costs->cost [k]
		  += COSTS_OF_ALLOCNO (total_costs, a_num)->cost [k];
	      temp_costs->mem_cost
		+= COSTS_OF_ALLOCNO (total_costs, a_num)->mem_cost;
#ifdef FORBIDDEN_INC_DEC_CLASSES
	      if (in_inc_dec [a_num])
		inc_dec_p = TRUE;
#endif
	    }
	  best_cost = (1 << (HOST_BITS_PER_INT - 2)) - 1;
	  best = ALL_REGS;
	  for (k = 0; k < cost_classes_num; k++)
	    {
	      class = cost_classes [k];
	      /* Ignore classes that are too small for this operand or
		 invalid for an operand that was auto-incremented.  */
	      if (! contains_reg_of_mode  [class] [PSEUDO_REGNO_MODE (i)]
#ifdef FORBIDDEN_INC_DEC_CLASSES
		  || (inc_dec_p && forbidden_inc_dec_class [class])
#endif
#ifdef CANNOT_CHANGE_MODE_CLASS
		  || invalid_mode_change_p (i, (enum reg_class) class,
					    PSEUDO_REGNO_MODE (i))
#endif
		  )
		;
	      else if (temp_costs->cost [k] < best_cost)
		{
		  best_cost = temp_costs->cost [k];
		  best = (enum reg_class) class;
		}
	      else if (temp_costs->cost [k] == best_cost)
		best = reg_class_union [best] [class];
	    }
	  if (best_cost > temp_costs->mem_cost)
	    common_class = NO_REGS;
	  else
	    {
	      common_class = best;
	      if (class_subset_p [best] [class_translate [best]])
		common_class = class_translate [best];
	    }
	  for (a = regno_allocno_map [i];
	       a != NULL;
	       a = ALLOCNO_NEXT_REGNO_ALLOCNO (a))
	    {
	      a_num = ALLOCNO_NUM (a);
	      if (common_class == NO_REGS)
		best = NO_REGS;
	      else
		{	      
		  /* Finding best class which is cover class for the
		     register.  */
		  best_cost = (1 << (HOST_BITS_PER_INT - 2)) - 1;
		  best = ALL_REGS;
		  for (k = 0; k < cost_classes_num; k++)
		    {
		      class = cost_classes [k];
		      if (! class_subset_p [class] [common_class])
			continue;
		      /* Ignore classes that are too small for this
			 operand or invalid for an operand that was
			 auto-incremented.  */
		      if (! contains_reg_of_mode [class] [PSEUDO_REGNO_MODE
							  (i)]
#ifdef FORBIDDEN_INC_DEC_CLASSES
			  || (inc_dec_p && forbidden_inc_dec_class [class])
#endif
#ifdef CANNOT_CHANGE_MODE_CLASS
			  || invalid_mode_change_p (i, (enum reg_class) class,
						    PSEUDO_REGNO_MODE (i))
#endif
			  )
			;
		      else if (COSTS_OF_ALLOCNO (total_costs, a_num)->cost [k]
			       < best_cost)
			{
			  best_cost
			    = COSTS_OF_ALLOCNO (total_costs, a_num)->cost [k];
			  best = (enum reg_class) class;
			}
		      else if (COSTS_OF_ALLOCNO (total_costs, a_num)->cost [k]
			       == best_cost)
			best = reg_class_union [best] [class];
		    }
		}
	      if (internal_flag_ira_verbose > 2 && ira_dump_file != NULL
		  && (pass == 0 || allocno_pref [a_num] != best))
		{
		  fprintf (ira_dump_file, "    a%d (r%d,", a_num, i);
		  if ((bb = ALLOCNO_LOOP_TREE_NODE (a)->bb) != NULL)
		    fprintf (ira_dump_file, "b%d", bb->index);
		  else
		    fprintf (ira_dump_file, "l%d",
			     ALLOCNO_LOOP_TREE_NODE (a)->loop->num);
		  fprintf (ira_dump_file, ") best %s, cover %s\n",
			   reg_class_names [best],
			   reg_class_names [class_translate [best]]);
		}
	      allocno_pref [a_num] = best;
	    }
	}
      
      if (internal_flag_ira_verbose > 4 && ira_dump_file)
	{
	  print_costs (ira_dump_file);
	  fprintf (ira_dump_file,"\n");
	}
    }

#ifdef FORBIDDEN_INC_DEC_CLASSES
  ira_free (in_inc_dec);
#endif
}



/* Process moves involving hard regs to modify allocno hard register
   costs.  We can do this only after determining allocno cover class.
   If a hard register forms a register class, than moves with the hard
   register are already taken into account slightly in class costs for
   the allocno.  */
static void
process_bb_node_for_hard_reg_moves (loop_tree_node_t loop_tree_node)
{
  int i, freq, cost, src_regno, dst_regno, hard_regno, to_p;
  allocno_t a;
  enum reg_class class, cover_class, hard_reg_class;
  enum machine_mode mode;
  basic_block bb;
  rtx insn, set, src, dst;

  bb = loop_tree_node->bb;
  if (bb == NULL)
    return;
  freq = REG_FREQ_FROM_BB (bb);
  if (freq == 0)
    freq = 1;
  FOR_BB_INSNS (bb, insn)
    {
      if (! INSN_P (insn))
	continue;
      set = single_set (insn);
      if (set == NULL_RTX)
	continue;
      dst = SET_DEST (set);
      src = SET_SRC (set);
      if (! REG_P (dst) || ! REG_P (src))
	continue;
      dst_regno = REGNO (dst);
      src_regno = REGNO (src);
      if (dst_regno >= FIRST_PSEUDO_REGISTER
	  && src_regno < FIRST_PSEUDO_REGISTER)
	{
	  hard_regno = src_regno;
	  to_p = TRUE;
	  a = ira_curr_regno_allocno_map [dst_regno];
	}
      else if (src_regno >= FIRST_PSEUDO_REGISTER
	       && dst_regno < FIRST_PSEUDO_REGISTER)
	{
	  hard_regno = dst_regno;
	  to_p = FALSE;
	  a = ira_curr_regno_allocno_map [src_regno];
	}
      else
	continue;
      class = ALLOCNO_COVER_CLASS (a);
      if (! TEST_HARD_REG_BIT (reg_class_contents [class], hard_regno))
	continue;
      i = class_hard_reg_index [class] [hard_regno];
      if (i < 0)
	continue;
      mode = ALLOCNO_MODE (a);
      hard_reg_class = REGNO_REG_CLASS (hard_regno);
      cost = (to_p ? register_move_cost [mode] [hard_reg_class] [class]
	      : register_move_cost [mode] [class] [hard_reg_class]) * freq;
      allocate_and_set_costs (&ALLOCNO_HARD_REG_COSTS (a), class,
			      ALLOCNO_COVER_CLASS_COST (a));
      allocate_and_set_costs (&ALLOCNO_CONFLICT_HARD_REG_COSTS (a), class, 0);
      ALLOCNO_HARD_REG_COSTS (a) [i] -= cost;
      ALLOCNO_CONFLICT_HARD_REG_COSTS (a) [i] -= cost;
      ALLOCNO_COVER_CLASS_COST (a) = MIN (ALLOCNO_COVER_CLASS_COST (a),
					  ALLOCNO_HARD_REG_COSTS (a) [i]);
      if (flag_ira_algorithm == IRA_ALGORITHM_REGIONAL
	  || flag_ira_algorithm == IRA_ALGORITHM_MIXED)
	{
	  loop_tree_node_t father;
	  int regno = ALLOCNO_REGNO (a);
	  
	  for (;;)
	    {
	      if ((father = ALLOCNO_LOOP_TREE_NODE (a)->father) == NULL)
	        break;
	      if ((a = father->regno_allocno_map [regno]) == NULL)
		break;
	      cover_class = ALLOCNO_COVER_CLASS (a);
	      allocate_and_set_costs
		(&ALLOCNO_HARD_REG_COSTS (a), cover_class,
		 ALLOCNO_COVER_CLASS_COST (a));
	      allocate_and_set_costs (&ALLOCNO_CONFLICT_HARD_REG_COSTS (a),
				      cover_class, 0);
	      ALLOCNO_HARD_REG_COSTS (a) [i] -= cost;
	      ALLOCNO_CONFLICT_HARD_REG_COSTS (a) [i] -= cost;
	      ALLOCNO_COVER_CLASS_COST (a)
		= MIN (ALLOCNO_COVER_CLASS_COST (a),
		       ALLOCNO_HARD_REG_COSTS (a) [i]);
	    }
	}
    }
}

/* After we find hard register and memory costs for allocnos, define
   its cover class and modify hard register cost because insns moving
   allocno to/from hard registers.  */
static void
setup_allocno_cover_class_and_costs (void)
{
  int i, j, n, regno;
  int *reg_costs;
  enum reg_class cover_class, class;
  enum machine_mode mode;
  allocno_t a;
  allocno_iterator ai;

  FOR_EACH_ALLOCNO (a, ai)
    {
      i = ALLOCNO_NUM (a);
      mode = ALLOCNO_MODE (a);
      cover_class = class_translate [allocno_pref [i]];
      ira_assert (allocno_pref [i] == NO_REGS || cover_class != NO_REGS);
      ALLOCNO_MEMORY_COST (a) = ALLOCNO_UPDATED_MEMORY_COST (a)
	= COSTS_OF_ALLOCNO (total_costs, i)->mem_cost;
      ALLOCNO_COVER_CLASS (a) = cover_class;
      if (cover_class == NO_REGS)
	continue;
      ALLOCNO_AVAILABLE_REGS_NUM (a) = available_class_regs [cover_class];
      ALLOCNO_COVER_CLASS_COST (a)
	= (COSTS_OF_ALLOCNO (total_costs, i)
	   ->cost [cost_class_nums [allocno_pref [i]]]);
      if (ALLOCNO_COVER_CLASS (a) != allocno_pref [i])
	{
	  n = class_hard_regs_num [cover_class];
	  ALLOCNO_HARD_REG_COSTS (a)
	    = reg_costs = allocate_cost_vector (cover_class);
	  for (j = n - 1; j >= 0; j--)
	    {
	      regno = class_hard_regs [cover_class] [j];
	      class = REGNO_REG_CLASS (regno);
	      reg_costs [j] = (COSTS_OF_ALLOCNO (total_costs, i)
			       ->cost [cost_class_nums [class]]);
	    }
	}
    }
  traverse_loop_tree (FALSE, ira_loop_tree_root,
		      process_bb_node_for_hard_reg_moves, NULL);
}



/* Function called once during compiler work.  It sets up init_cost
   whose values don't depend on the compiled function.  */
void
init_ira_costs_once (void)
{
  int i;

  init_cost = NULL;
  for (i = 0; i < MAX_RECOG_OPERANDS; i++)
    {
      op_costs [i] = NULL;
      this_op_costs [i] = NULL;
    }
  temp_costs = NULL;
  cost_classes = NULL;
}

/* The function frees different cost vectors.  */
static void
free_ira_costs (void)
{
  int i;

  if (init_cost != NULL)
    free (init_cost);
  init_cost = NULL;
  for (i = 0; i < MAX_RECOG_OPERANDS; i++)
    {
      if (op_costs [i] != NULL)
	free (op_costs [i]);
      if (this_op_costs [i] != NULL)
	free (this_op_costs [i]);
      op_costs [i] = this_op_costs [i] = NULL;
    }
  if (temp_costs != NULL)
    free (temp_costs);
  temp_costs = NULL;
  if (cost_classes != NULL)
    free (cost_classes);
  cost_classes = NULL;
}

/* The function called every time when register related information is
   changed.  */
void
init_ira_costs (void)
{
  int i;

  free_ira_costs ();
  max_struct_costs_size
    = sizeof (struct costs) + sizeof (int) * (important_classes_num - 1);
  /* Don't use ira_allocate because vectors live through several IRA calls.  */
  init_cost = xmalloc (max_struct_costs_size);
  init_cost->mem_cost = 1000000;
  for (i = 0; i < important_classes_num; i++)
    init_cost->cost [i] = 1000000;
  for (i = 0; i < MAX_RECOG_OPERANDS; i++)
    {
      op_costs [i] = xmalloc (max_struct_costs_size);
      this_op_costs [i] = xmalloc (max_struct_costs_size);
    }
  temp_costs = xmalloc (max_struct_costs_size);
  cost_classes = xmalloc (sizeof (enum reg_class) * important_classes_num);
}

/* Function called once at the end of compiler work.  */
void
finish_ira_costs_once (void)
{
  free_ira_costs ();
}



/* Entry function which defines cover class, memory and hard register
   costs for each allocno.  */
void
ira_costs (void)
{
  allocno_t a;
  allocno_iterator ai;

  total_costs = ira_allocate (max_struct_costs_size * allocnos_num);
  allocno_pref_buffer = ira_allocate (sizeof (enum reg_class) * allocnos_num);
  find_allocno_class_costs ();
  setup_allocno_cover_class_and_costs ();
  /* Because we could process operands only as subregs, check mode of
     the registers themselves too.  */
  FOR_EACH_ALLOCNO (a, ai)
    if (register_move_cost [ALLOCNO_MODE (a)] == NULL)
      init_register_move_cost (ALLOCNO_MODE (a));
  ira_free (allocno_pref_buffer);
  ira_free (total_costs);
}



/* This function changes hard register costs for allocnos which lives
   trough function calls.  The function is called only when we found
   all intersected calls during building allocno conflicts.  */
void
tune_allocno_costs_and_cover_classes (void)
{
  int j, k, n, regno, freq;
  int cost, min_cost, *reg_costs;
  enum reg_class cover_class, class;
  enum machine_mode mode;
  allocno_t a;
  rtx call, *allocno_calls;
  HARD_REG_SET clobbered_regs;
  allocno_iterator ai;

  FOR_EACH_ALLOCNO (a, ai)
    {
      cover_class = ALLOCNO_COVER_CLASS (a);
      if (cover_class == NO_REGS)
	continue;
      mode = ALLOCNO_MODE (a);
      n = class_hard_regs_num [cover_class];
      min_cost = INT_MAX;
      if (ALLOCNO_CALLS_CROSSED_NUM (a) != 0)
	{
	  allocate_and_set_costs
	    (&ALLOCNO_HARD_REG_COSTS (a), cover_class,
	     ALLOCNO_COVER_CLASS_COST (a));
	  reg_costs = ALLOCNO_HARD_REG_COSTS (a);
	  for (j = n - 1; j >= 0; j--)
	    {
	      regno = class_hard_regs [cover_class] [j];
	      class = REGNO_REG_CLASS (regno);
	      cost = 0;
	      if (! flag_ira_ipra)
		{
		  /* ??? If only part is call clobbered.  */
		  if (! hard_reg_not_in_set_p (regno, mode, call_used_reg_set))
		    {
		      cost += (ALLOCNO_CALL_FREQ (a)
			       * (memory_move_cost [mode] [class] [0]
				  + memory_move_cost [mode] [class] [1]));
		    }
		}
	      else
		{
		  allocno_calls
		    = (VEC_address (rtx, regno_calls [ALLOCNO_REGNO (a)])
		       + ALLOCNO_CALLS_CROSSED_START (a));
		  ira_assert (allocno_calls != NULL); 
		  for (k = ALLOCNO_CALLS_CROSSED_NUM (a) - 1; k >= 0; k--)
		    {
		      call = allocno_calls [k];
		      freq = REG_FREQ_FROM_BB (BLOCK_FOR_INSN (call));
		      if (freq == 0)
			freq = 1;
		      get_call_invalidated_used_regs (call, &clobbered_regs,
						      FALSE);
		      /* ??? If only part is call clobbered.  */
		      if (! hard_reg_not_in_set_p (regno, mode,
						   clobbered_regs))
			cost
			  += freq * (memory_move_cost [mode] [class] [0]
				     + memory_move_cost [mode] [class] [1]);
		    }
		}
#ifdef IRA_HARD_REGNO_ADD_COST_MULTIPLIER
	      cost += ((memory_move_cost [mode] [class] [0]
			+ memory_move_cost [mode] [class] [1])
		       * ALLOCNO_FREQ (a)
		       * IRA_HARD_REGNO_ADD_COST_MULTIPLIER (regno) / 2);
#endif
	      reg_costs [j] += cost;
	      if (min_cost > reg_costs [j])
		min_cost = reg_costs [j];
	    }
	}
      if (min_cost != INT_MAX)
	ALLOCNO_COVER_CLASS_COST (a) = min_cost;
    }
}
