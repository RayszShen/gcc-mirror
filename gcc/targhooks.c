/* Default target hook functions.
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* The migration of target macros to target hooks works as follows:

   1. Create a target hook that uses the existing target macros to
      implement the same functionality.

   2. Convert all the MI files to use the hook instead of the macro.

   3. Repeat for a majority of the remaining target macros.  This will
      take some time.

   4. Tell target maintainers to start migrating.

   5. Eventually convert the backends to override the hook instead of
      defining the macros.  This will take some time too.

   6. TBD when, poison the macros.  Unmigrated targets will break at
      this point.

   Note that we expect steps 1-3 to be done by the people that
   understand what the MI does with each macro, and step 5 to be done
   by the target maintainers for their respective targets.

   Note that steps 1 and 2 don't have to be done together, but no
   target can override the new hook until step 2 is complete for it.

   Once the macros are poisoned, we will revert to the old migration
   rules - migrate the macro, callers, and targets all at once.  This
   comment can thus be removed at that point.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "machmode.h"
#include "rtl.h"
#include "tree.h"
#include "expr.h"
#include "output.h"
#include "toplev.h"
#include "function.h"
#include "target.h"
#include "tm_p.h"
#include "target-def.h"
#include "ggc.h"
#include "regs.h"
#include "hard-reg-set.h"


void
default_external_libcall (rtx fun ATTRIBUTE_UNUSED)
{
#ifdef ASM_OUTPUT_EXTERNAL_LIBCALL
  ASM_OUTPUT_EXTERNAL_LIBCALL(asm_out_file, fun);
#endif
}

enum machine_mode
default_cc_modes_compatible (enum machine_mode m1, enum machine_mode m2)
{
  if (m1 == m2)
    return m1;
  return VOIDmode;
}

bool
default_return_in_memory (tree type,
			  tree fntype ATTRIBUTE_UNUSED)
{
#ifndef RETURN_IN_MEMORY
  return (TYPE_MODE (type) == BLKmode);
#else
  return RETURN_IN_MEMORY (type);
#endif
}

rtx
default_expand_builtin_saveregs (void)
{
  error ("__builtin_saveregs not supported by this target");
  return const0_rtx;
}

void
default_setup_incoming_varargs (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
				enum machine_mode mode ATTRIBUTE_UNUSED,
				tree type ATTRIBUTE_UNUSED,
				int *pretend_arg_size ATTRIBUTE_UNUSED,
				int second_time ATTRIBUTE_UNUSED)
{
}

/* The default implementation of TARGET_BUILTIN_SETJMP_FRAME_VALUE.  */

rtx
default_builtin_setjmp_frame_value (void)
{
  return virtual_stack_vars_rtx;
}

/* Generic hook that takes a CUMULATIVE_ARGS pointer and returns false.  */

bool
hook_bool_CUMULATIVE_ARGS_false (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED)
{
  return false;
}

bool
default_pretend_outgoing_varargs_named (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED)
{
  return (targetm.calls.setup_incoming_varargs
	  != default_setup_incoming_varargs);
}

enum machine_mode
default_eh_return_filter_mode (void)
{
  return word_mode;
}

/* The default implementation of TARGET_SHIFT_TRUNCATION_MASK.  */

unsigned HOST_WIDE_INT
default_shift_truncation_mask (enum machine_mode mode)
{
  return SHIFT_COUNT_TRUNCATED ? GET_MODE_BITSIZE (mode) - 1 : 0;
}

/* Generic hook that takes a CUMULATIVE_ARGS pointer and returns true.  */

bool
hook_bool_CUMULATIVE_ARGS_true (CUMULATIVE_ARGS * a ATTRIBUTE_UNUSED)
{
  return true;
}


/* The generic C++ ABI specifies this is a 64-bit value.  */
tree
default_cxx_guard_type (void)
{
  return long_long_integer_type_node;
}


/* Returns the size of the cookie to use when allocating an array
   whose elements have the indicated TYPE.  Assumes that it is already
   known that a cookie is needed.  */

tree
default_cxx_get_cookie_size (tree type)
{
  tree cookie_size;

  /* We need to allocate an additional max (sizeof (size_t), alignof
     (true_type)) bytes.  */
  tree sizetype_size;
  tree type_align;

  sizetype_size = size_in_bytes (sizetype);
  type_align = size_int (TYPE_ALIGN_UNIT (type));
  if (INT_CST_LT_UNSIGNED (type_align, sizetype_size))
    cookie_size = sizetype_size;
  else
    cookie_size = type_align;

  return cookie_size;
}

/* Return true if a parameter must be passed by reference.  This version
   of the TARGET_PASS_BY_REFERENCE hook uses just MUST_PASS_IN_STACK.  */

bool
hook_pass_by_reference_must_pass_in_stack (CUMULATIVE_ARGS *c ATTRIBUTE_UNUSED,
	enum machine_mode mode ATTRIBUTE_UNUSED, tree type ATTRIBUTE_UNUSED,
	bool named_arg ATTRIBUTE_UNUSED)
{
  return targetm.calls.must_pass_in_stack (mode, type);
}

/* Return true if a parameter follows callee copies conventions.  This
   version of the hook is true for all named arguments.  */

bool
hook_callee_copies_named (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
			  enum machine_mode mode ATTRIBUTE_UNUSED,
			  tree type ATTRIBUTE_UNUSED, bool named)
{
  return named;
}

/* Emit any directives required to unwind this instruction.  */

void
default_unwind_emit (FILE * stream ATTRIBUTE_UNUSED,
		     rtx insn ATTRIBUTE_UNUSED)
{
  /* Should never happen.  */
  gcc_unreachable ();
}

/* True if MODE is valid for the target.  By "valid", we mean able to
   be manipulated in non-trivial ways.  In particular, this means all
   the arithmetic is supported.

   By default we guess this means that any C type is supported.  If
   we can't map the mode back to a type that would be available in C,
   then reject it.  Special case, here, is the double-word arithmetic
   supported by optabs.c.  */

bool
default_scalar_mode_supported_p (enum machine_mode mode)
{
  int precision = GET_MODE_PRECISION (mode);

  switch (GET_MODE_CLASS (mode))
    {
    case MODE_PARTIAL_INT:
    case MODE_INT:
      if (precision == CHAR_TYPE_SIZE)
	return true;
      if (precision == SHORT_TYPE_SIZE)
	return true;
      if (precision == INT_TYPE_SIZE)
	return true;
      if (precision == LONG_TYPE_SIZE)
	return true;
      if (precision == LONG_LONG_TYPE_SIZE)
	return true;
      if (precision == 2 * BITS_PER_WORD)
	return true;
      return false;

    case MODE_FLOAT:
      if (precision == FLOAT_TYPE_SIZE)
	return true;
      if (precision == DOUBLE_TYPE_SIZE)
	return true;
      if (precision == LONG_DOUBLE_TYPE_SIZE)
	return true;
      return false;

    default:
      gcc_unreachable ();
    }
}

/* NULL if INSN insn is valid within a low-overhead loop, otherwise returns
   an error message.
  
   This function checks whether a given INSN is valid within a low-overhead
   loop.  If INSN is invalid it returns the reason for that, otherwise it
   returns NULL. A called function may clobber any special registers required
   for low-overhead looping. Additionally, some targets (eg, PPC) use the count
   register for branch on table instructions. We reject the doloop pattern in
   these cases.  */

const char *
default_invalid_within_doloop (rtx insn)
{
  if (CALL_P (insn))
    return "Function call in loop.";
  
  if (JUMP_P (insn)
      && (GET_CODE (PATTERN (insn)) == ADDR_DIFF_VEC
	  || GET_CODE (PATTERN (insn)) == ADDR_VEC))
    return "Computed branch in the loop.";
  
  return NULL;
}

bool
hook_bool_CUMULATIVE_ARGS_mode_tree_bool_false (
	CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
	enum machine_mode mode ATTRIBUTE_UNUSED,
	tree type ATTRIBUTE_UNUSED, bool named ATTRIBUTE_UNUSED)
{
  return false;
}

bool
hook_bool_CUMULATIVE_ARGS_mode_tree_bool_true (
	CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
	enum machine_mode mode ATTRIBUTE_UNUSED,
	tree type ATTRIBUTE_UNUSED, bool named ATTRIBUTE_UNUSED)
{
  return true;
}

int
hook_int_CUMULATIVE_ARGS_mode_tree_bool_0 (
	CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
	enum machine_mode mode ATTRIBUTE_UNUSED,
	tree type ATTRIBUTE_UNUSED, bool named ATTRIBUTE_UNUSED)
{
  return 0;
}

const char *
hook_invalid_arg_for_unprototyped_fn (
	tree typelist ATTRIBUTE_UNUSED,
	tree funcdecl ATTRIBUTE_UNUSED,
	tree val ATTRIBUTE_UNUSED)
{
  return NULL;
}

/* Initialize the stack protection decls.  */

/* Stack protection related decls living in libgcc.  */
static GTY(()) tree stack_chk_guard_decl;

tree
default_stack_protect_guard (void)
{
  tree t = stack_chk_guard_decl;

  if (t == NULL)
    {
      t = build_decl (VAR_DECL, get_identifier ("__stack_chk_guard"),
		      ptr_type_node);
      TREE_STATIC (t) = 1;
      TREE_PUBLIC (t) = 1;
      DECL_EXTERNAL (t) = 1;
      TREE_USED (t) = 1;
      TREE_THIS_VOLATILE (t) = 1;
      DECL_ARTIFICIAL (t) = 1;
      DECL_IGNORED_P (t) = 1;

      stack_chk_guard_decl = t;
    }

  return t;
}

static GTY(()) tree stack_chk_fail_decl;

tree 
default_external_stack_protect_fail (void)
{
  tree t = stack_chk_fail_decl;

  if (t == NULL_TREE)
    {
      t = build_function_type_list (void_type_node, NULL_TREE);
      t = build_decl (FUNCTION_DECL, get_identifier ("__stack_chk_fail"), t);
      TREE_STATIC (t) = 1;
      TREE_PUBLIC (t) = 1;
      DECL_EXTERNAL (t) = 1;
      TREE_USED (t) = 1;
      TREE_THIS_VOLATILE (t) = 1;
      TREE_NOTHROW (t) = 1;
      DECL_ARTIFICIAL (t) = 1;
      DECL_IGNORED_P (t) = 1;

      stack_chk_fail_decl = t;
    }

  return build_function_call_expr (t, NULL_TREE);
}

tree
default_hidden_stack_protect_fail (void)
{
#ifndef HAVE_GAS_HIDDEN
  return default_external_stack_protect_fail ();
#else
  tree t = stack_chk_fail_decl;

  if (!flag_pic)
    return default_external_stack_protect_fail ();

  if (t == NULL_TREE)
    {
      t = build_function_type_list (void_type_node, NULL_TREE);
      t = build_decl (FUNCTION_DECL,
		      get_identifier ("__stack_chk_fail_local"), t);
      TREE_STATIC (t) = 1;
      TREE_PUBLIC (t) = 1;
      DECL_EXTERNAL (t) = 1;
      TREE_USED (t) = 1;
      TREE_THIS_VOLATILE (t) = 1;
      TREE_NOTHROW (t) = 1;
      DECL_ARTIFICIAL (t) = 1;
      DECL_IGNORED_P (t) = 1;
      DECL_VISIBILITY_SPECIFIED (t) = 1;
      DECL_VISIBILITY (t) = VISIBILITY_HIDDEN;

      stack_chk_fail_decl = t;
    }

  return build_function_call_expr (t, NULL_TREE);
#endif
}

bool
hook_bool_rtx_commutative_p (rtx x, int outer_code ATTRIBUTE_UNUSED)
{
  return COMMUTATIVE_P (x);
}

rtx
default_function_value (tree ret_type ATTRIBUTE_UNUSED,
			tree fn_decl_or_type,
			bool outgoing ATTRIBUTE_UNUSED)
{
  /* The old interface doesn't handle receiving the function type.  */
  if (fn_decl_or_type
      && !DECL_P (fn_decl_or_type))
    fn_decl_or_type = NULL;

#ifdef FUNCTION_OUTGOING_VALUE
  if (outgoing)
    return FUNCTION_OUTGOING_VALUE (ret_type, fn_decl_or_type);
#endif

#ifdef FUNCTION_VALUE
  return FUNCTION_VALUE (ret_type, fn_decl_or_type);
#else
  return NULL_RTX;
#endif
}

/* Implementation of the apply_result_mode which assumes that all
   return values fit into a single hard register.
   For the sake of backward compatibility, this his is currently the
   default.  */
enum machine_mode
apply_result_mode_1reg (unsigned regno)
{
  return reg_raw_mode[regno];
}

/* Implementation of the apply_result_mode which tries to compute
   a mode for the largest number of hard registers that could
   hold a return value.
   At some point, this should become the default, when enough ports have
   been changed to take advantage of the apply_result_mode hook.  */
enum machine_mode
apply_result_mode_scanreg (unsigned regno)
{
  unsigned n;
  enum machine_mode mode, wider_mode;

  mode = reg_raw_mode[regno];
  gcc_assert (mode != VOIDmode);

  n = hard_regno_nregs[regno][mode];
  while (regno + n < FIRST_PSEUDO_REGISTER && call_used_regs[regno + n])
    {
      n++;
      wider_mode = choose_hard_reg_mode (regno, n, false);
      if (wider_mode != VOIDmode)
	mode = wider_mode;
    }
  return mode;
}

#include "gt-targhooks.h"
