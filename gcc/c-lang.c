/* Language-specific hook definitions for C front end.
   Copyright (C) 1991, 1995, 1997, 1998,
   1999, 2000, 2001, 2003 Free Software Foundation, Inc.

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
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "c-tree.h"
#include "c-common.h"
#include "ggc.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "tree-inline.h"
#include "diagnostic.h"
#include "c-pretty-print.h"

static void c_initialize_diagnostics (diagnostic_context *);

enum c_language_kind c_language = clk_c;

/* ### When changing hooks, consider if ObjC needs changing too!! ### */

#undef LANG_HOOKS_NAME
#define LANG_HOOKS_NAME "GNU C"
#undef LANG_HOOKS_INIT
#define LANG_HOOKS_INIT c_objc_common_init
#undef LANG_HOOKS_FINISH
#define LANG_HOOKS_FINISH c_common_finish
#undef LANG_HOOKS_INIT_OPTIONS
#define LANG_HOOKS_INIT_OPTIONS c_common_init_options
#undef LANG_HOOKS_INITIALIZE_DIAGNOSTITCS
#define LANG_HOOKS_INITIALIZE_DIAGNOSTITCS c_initialize_diagnostics
#undef LANG_HOOKS_HANDLE_OPTION
#define LANG_HOOKS_HANDLE_OPTION c_common_handle_option
#undef LANG_HOOKS_MISSING_ARGUMENT
#define LANG_HOOKS_MISSING_ARGUMENT c_common_missing_argument
#undef LANG_HOOKS_POST_OPTIONS
#define LANG_HOOKS_POST_OPTIONS c_common_post_options
#undef LANG_HOOKS_GET_ALIAS_SET
#define LANG_HOOKS_GET_ALIAS_SET c_common_get_alias_set
#undef LANG_HOOKS_SAFE_FROM_P
#define LANG_HOOKS_SAFE_FROM_P c_safe_from_p
#undef LANG_HOOKS_EXPAND_EXPR
#define LANG_HOOKS_EXPAND_EXPR c_expand_expr
#undef LANG_HOOKS_EXPAND_DECL
#define LANG_HOOKS_EXPAND_DECL c_expand_decl
#undef LANG_HOOKS_MARK_ADDRESSABLE
#define LANG_HOOKS_MARK_ADDRESSABLE c_mark_addressable
#undef LANG_HOOKS_PARSE_FILE
#define LANG_HOOKS_PARSE_FILE c_common_parse_file
#undef LANG_HOOKS_TRUTHVALUE_CONVERSION
#define LANG_HOOKS_TRUTHVALUE_CONVERSION c_common_truthvalue_conversion
#undef LANG_HOOKS_FINISH_INCOMPLETE_DECL
#define LANG_HOOKS_FINISH_INCOMPLETE_DECL c_finish_incomplete_decl
#undef LANG_HOOKS_UNSAFE_FOR_REEVAL
#define LANG_HOOKS_UNSAFE_FOR_REEVAL c_common_unsafe_for_reeval
#undef LANG_HOOKS_STATICP
#define LANG_HOOKS_STATICP c_staticp
#undef LANG_HOOKS_SET_DECL_ASSEMBLER_NAME
#define LANG_HOOKS_SET_DECL_ASSEMBLER_NAME c_static_assembler_name
#undef LANG_HOOKS_NO_BODY_BLOCKS
#define LANG_HOOKS_NO_BODY_BLOCKS true
#undef LANG_HOOKS_WARN_UNUSED_GLOBAL_DECL
#define LANG_HOOKS_WARN_UNUSED_GLOBAL_DECL c_warn_unused_global_decl
#undef LANG_HOOKS_PRINT_IDENTIFIER
#define LANG_HOOKS_PRINT_IDENTIFIER c_print_identifier
#undef LANG_HOOKS_FUNCTION_ENTER_NESTED
#define LANG_HOOKS_FUNCTION_ENTER_NESTED c_push_function_context
#undef LANG_HOOKS_FUNCTION_LEAVE_NESTED
#define LANG_HOOKS_FUNCTION_LEAVE_NESTED c_pop_function_context
#undef LANG_HOOKS_DUP_LANG_SPECIFIC_DECL
#define LANG_HOOKS_DUP_LANG_SPECIFIC_DECL c_dup_lang_specific_decl

#undef LANG_HOOKS_RTL_EXPAND_STMT
#define LANG_HOOKS_RTL_EXPAND_STMT expand_stmt_toplev

/* Attribute hooks.  */
#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE c_common_attribute_table
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE c_common_format_attribute_table

#undef LANG_HOOKS_TREE_INLINING_WALK_SUBTREES
#define LANG_HOOKS_TREE_INLINING_WALK_SUBTREES \
  c_walk_subtrees
#undef LANG_HOOKS_TREE_INLINING_CANNOT_INLINE_TREE_FN
#define LANG_HOOKS_TREE_INLINING_CANNOT_INLINE_TREE_FN \
  c_cannot_inline_tree_fn
#undef LANG_HOOKS_TREE_INLINING_DISREGARD_INLINE_LIMITS
#define LANG_HOOKS_TREE_INLINING_DISREGARD_INLINE_LIMITS \
  c_disregard_inline_limits
#undef LANG_HOOKS_TREE_INLINING_TREE_CHAIN_MATTERS_P
#define LANG_HOOKS_TREE_INLINING_TREE_CHAIN_MATTERS_P \
  c_tree_chain_matters_p
#undef LANG_HOOKS_TREE_INLINING_ANON_AGGR_TYPE_P
#define LANG_HOOKS_TREE_INLINING_ANON_AGGR_TYPE_P \
  anon_aggr_type_p
#undef LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING
#define LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING \
  c_convert_parm_for_inlining
#undef LANG_HOOKS_TREE_INLINING_ESTIMATE_NUM_INSNS
#define LANG_HOOKS_TREE_INLINING_ESTIMATE_NUM_INSNS c_estimate_num_insns
#undef LANG_HOOKS_TREE_DUMP_DUMP_TREE_FN
#define LANG_HOOKS_TREE_DUMP_DUMP_TREE_FN c_dump_tree

#undef LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION c_expand_body

#undef LANG_HOOKS_TYPE_FOR_MODE
#define LANG_HOOKS_TYPE_FOR_MODE c_common_type_for_mode
#undef LANG_HOOKS_TYPE_FOR_SIZE
#define LANG_HOOKS_TYPE_FOR_SIZE c_common_type_for_size
#undef LANG_HOOKS_SIGNED_TYPE
#define LANG_HOOKS_SIGNED_TYPE c_common_signed_type
#undef LANG_HOOKS_UNSIGNED_TYPE
#define LANG_HOOKS_UNSIGNED_TYPE c_common_unsigned_type
#undef LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE
#define LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE c_common_signed_or_unsigned_type
#undef LANG_HOOKS_INCOMPLETE_TYPE_ERROR
#define LANG_HOOKS_INCOMPLETE_TYPE_ERROR c_incomplete_type_error
#undef LANG_HOOKS_TYPE_PROMOTES_TO
#define LANG_HOOKS_TYPE_PROMOTES_TO c_type_promotes_to

#undef LANG_HOOKS_WRITE_GLOBALS
#define LANG_HOOKS_WRITE_GLOBALS c_write_global_declarations

/* Hooks for tree gimplification.  */
#undef LANG_HOOKS_GIMPLIFY_EXPR
#define LANG_HOOKS_GIMPLIFY_EXPR c_gimplify_expr

/* ### When changing hooks, consider if ObjC needs changing too!! ### */

/* Each front end provides its own.  */
const struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

/* Tree code classes.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

const char tree_code_type[] = {
#include "tree.def"
  'x',
#include "c-common.def"
};
#undef DEFTREECODE

/* Table indexed by tree code giving number of expression
   operands beyond the fixed part of the node structure.
   Not used for types or decls.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

const unsigned char tree_code_length[] = {
#include "tree.def"
  0,
#include "c-common.def"
};
#undef DEFTREECODE

/* Names of tree components.
   Used for printing out the tree and error messages.  */
#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

const char *const tree_code_name[] = {
#include "tree.def"
  "@@dummy",
#include "c-common.def"
};
#undef DEFTREECODE

/* Used by c-lex.c, but only for objc.  */

tree
lookup_interface (tree arg ATTRIBUTE_UNUSED)
{
  return 0;
}

tree
is_class_name (tree arg ATTRIBUTE_UNUSED)
{
  return 0;
}

tree
objc_is_id (tree arg ATTRIBUTE_UNUSED)
{
  return 0;
}

void
objc_check_decl (tree decl ATTRIBUTE_UNUSED)
{
}

int
objc_comptypes (tree lhs ATTRIBUTE_UNUSED, tree rhs ATTRIBUTE_UNUSED,
		int reflexive ATTRIBUTE_UNUSED)
{
  return -1;
}

tree
objc_message_selector (void)
{
  return 0;
}

/* Used by c-typeck.c (build_external_ref), but only for objc.  */

tree
lookup_objc_ivar (tree id ATTRIBUTE_UNUSED)
{
  return 0;
}

void
finish_file (void)
{
  c_objc_common_finish_file ();
}

static void
c_initialize_diagnostics (diagnostic_context *context)
{
  pretty_printer *base = context->printer;
  c_pretty_printer pp = xmalloc (sizeof *pp);
  memcpy (pp_base (pp), base, sizeof (pretty_printer));
  pp_c_pretty_printer_init (pp);
  context->printer = (pretty_printer *) pp;

  /* It is safe to free this object because it was previously malloc()'d.  */
  free (base);
}


#include "gtype-c.h"
