/* Process declarations and variables for C compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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

/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "intl.h"
#include "tree.h"
#include "tree-inline.h"
#include "rtl.h"
#include "flags.h"
#include "function.h"
#include "output.h"
#include "expr.h"
#include "c-tree.h"
#include "toplev.h"
#include "ggc.h"
#include "tm_p.h"
#include "cpplib.h"
#include "target.h"
#include "debug.h"
#include "opts.h"
#include "timevar.h"
#include "c-common.h"
#include "c-pragma.h"
#include "langhooks.h"
#include "tree-mudflap.h"
#include "tree-gimple.h"
#include "diagnostic.h"
#include "tree-dump.h"
#include "cgraph.h"
#include "hashtab.h"
#include "libfuncs.h"
#include "except.h"
#include "langhooks-def.h"

/* APPLE LOCAL begin new tree dump */
#include "dmp-tree.h"
extern int c_dump_tree_p (FILE *, const char *, tree, int);
extern lang_dump_tree_p_t c_prev_lang_dump_tree_p;
/* APPLE LOCAL end new tree dump */

/* In grokdeclarator, distinguish syntactic contexts of declarators.  */
enum decl_context
{ NORMAL,			/* Ordinary declaration */
  FUNCDEF,			/* Function definition */
  PARM,				/* Declaration of parm before function body */
  FIELD,			/* Declaration inside struct or union */
  TYPENAME};			/* Typename (inside cast or sizeof)  */


/* Nonzero if we have seen an invalid cross reference
   to a struct, union, or enum, but not yet printed the message.  */
tree pending_invalid_xref;

/* File and line to appear in the eventual error message.  */
location_t pending_invalid_xref_location;

/* True means we've initialized exception handling.  */
bool c_eh_initialized_p;

/* While defining an enum type, this is 1 plus the last enumerator
   constant value.  Note that will do not have to save this or `enum_overflow'
   around nested function definition since such a definition could only
   occur in an enum value expression and we don't use these variables in
   that case.  */

static tree enum_next_value;

/* Nonzero means that there was overflow computing enum_next_value.  */

static int enum_overflow;

/* These #defines are for clarity in working with the information block
   returned by get_parm_info.  */
#define ARG_INFO_PARMS(args)  TREE_PURPOSE(args)
#define ARG_INFO_TAGS(args)   TREE_VALUE(args)
#define ARG_INFO_TYPES(args)  TREE_CHAIN(args)
#define ARG_INFO_OTHERS(args) TREE_TYPE(args)

/* The file and line that the prototype came from if this is an
   old-style definition; used for diagnostics in
   store_parm_decls_oldstyle.  */

static location_t current_function_prototype_locus;

/* The current statement tree.  */

static GTY(()) struct stmt_tree_s c_stmt_tree;

/* State saving variables.  */
tree c_break_label;
tree c_cont_label;

/* Linked list of TRANSLATION_UNIT_DECLS for the translation units
   included in this invocation.  Note that the current translation
   unit is not included in this list.  */

static GTY(()) tree all_translation_units;

/* A list of decls to be made automatically visible in each file scope.  */
static GTY(()) tree visible_builtins;

/* Set to 0 at beginning of a function definition, set to 1 if
   a return statement that specifies a return value is seen.  */

int current_function_returns_value;

/* Set to 0 at beginning of a function definition, set to 1 if
   a return statement with no argument is seen.  */

int current_function_returns_null;

/* Set to 0 at beginning of a function definition, set to 1 if
   a call to a noreturn function is seen.  */

int current_function_returns_abnormally;

/* Set to nonzero by `grokdeclarator' for a function
   whose return type is defaulted, if warnings for this are desired.  */

static int warn_about_return_type;

/* Nonzero when starting a function declared `extern inline'.  */

static int current_extern_inline;

/* True means global_bindings_p should return false even if the scope stack
   says we are in file scope.  */
bool c_override_global_bindings_to_false;


/* Each c_binding structure describes one binding of an identifier to
   a decl.  All the decls in a scope - irrespective of namespace - are
   chained together by the ->prev field, which (as the name implies)
   runs in reverse order.  All the decls in a given namespace bound to
   a given identifier are chained by the ->shadowed field, which runs
   from inner to outer scopes.

   The ->decl field usually points to a DECL node, but there are two
   exceptions.  In the namespace of type tags, the bound entity is a
   RECORD_TYPE, UNION_TYPE, or ENUMERAL_TYPE node.  If an undeclared
   identifier is encountered, it is bound to error_mark_node to
   suppress further errors about that identifier in the current
   function.

   The depth field is copied from the scope structure that holds this
   decl.  It is used to preserve the proper ordering of the ->shadowed
   field (see bind()) and also for a handful of special-case checks.
   Finally, the invisible bit is true for a decl which should be
   ignored for purposes of normal name lookup, and the nested bit is
   true for a decl that's been bound a second time in an inner scope;
   in all such cases, the binding in the outer scope will have its
   invisible bit true.  */

struct c_binding GTY((chain_next ("%h.prev")))
{
  tree decl;			/* the decl bound */
  tree id;			/* the identifier it's bound to */
  struct c_binding *prev;	/* the previous decl in this scope */
  struct c_binding *shadowed;	/* the innermost decl shadowed by this one */
  unsigned int depth : 28;      /* depth of this scope */
  BOOL_BITFIELD invisible : 1;  /* normal lookup should ignore this binding */
  BOOL_BITFIELD nested : 1;     /* do not set DECL_CONTEXT when popping */
  /* two free bits */
};
#define B_IN_SCOPE(b1, b2) ((b1)->depth == (b2)->depth)
#define B_IN_CURRENT_SCOPE(b) ((b)->depth == current_scope->depth)
#define B_IN_FILE_SCOPE(b) ((b)->depth == 1 /*file_scope->depth*/)
#define B_IN_EXTERNAL_SCOPE(b) ((b)->depth == 0 /*external_scope->depth*/)

#define I_SYMBOL_BINDING(node) \
  (((struct lang_identifier *)IDENTIFIER_NODE_CHECK(node))->symbol_binding)
#define I_SYMBOL_DECL(node) \
 (I_SYMBOL_BINDING(node) ? I_SYMBOL_BINDING(node)->decl : 0)

#define I_TAG_BINDING(node) \
  (((struct lang_identifier *)IDENTIFIER_NODE_CHECK(node))->tag_binding)
#define I_TAG_DECL(node) \
 (I_TAG_BINDING(node) ? I_TAG_BINDING(node)->decl : 0)

#define I_LABEL_BINDING(node) \
  (((struct lang_identifier *)IDENTIFIER_NODE_CHECK(node))->label_binding)
#define I_LABEL_DECL(node) \
 (I_LABEL_BINDING(node) ? I_LABEL_BINDING(node)->decl : 0)

/* Each C symbol points to three linked lists of c_binding structures.
   These describe the values of the identifier in the three different
   namespaces defined by the language.  */

struct lang_identifier GTY(())
{
  struct c_common_identifier common_id;
  struct c_binding *symbol_binding; /* vars, funcs, constants, typedefs */
  struct c_binding *tag_binding;    /* struct/union/enum tags */
  struct c_binding *label_binding;  /* labels */
};

/* Validate c-lang.c's assumptions.  */
extern char C_SIZEOF_STRUCT_LANG_IDENTIFIER_isnt_accurate
[(sizeof(struct lang_identifier) == C_SIZEOF_STRUCT_LANG_IDENTIFIER) ? 1 : -1];

/* The resulting tree type.  */

union lang_tree_node
  GTY((desc ("TREE_CODE (&%h.generic) == IDENTIFIER_NODE"),
       chain_next ("TREE_CODE (&%h.generic) == INTEGER_TYPE ? (union lang_tree_node *)TYPE_NEXT_VARIANT (&%h.generic) : (union lang_tree_node *)TREE_CHAIN (&%h.generic)")))
{
  union tree_node GTY ((tag ("0"),
			desc ("tree_node_structure (&%h)")))
    generic;
  struct lang_identifier GTY ((tag ("1"))) identifier;
};

/* Each c_scope structure describes the complete contents of one
   scope.  Four scopes are distinguished specially: the innermost or
   current scope, the innermost function scope, the file scope (always
   the second to outermost) and the outermost or external scope.

   Most declarations are recorded in the current scope.

   All normal label declarations are recorded in the innermost
   function scope, as are bindings of undeclared identifiers to
   error_mark_node.  (GCC permits nested functions as an extension,
   hence the 'innermost' qualifier.)  Explicitly declared labels
   (using the __label__ extension) appear in the current scope.

   Being in the file scope (current_scope == file_scope) causes
   special behavior in several places below.  Also, under some
   conditions the Objective-C front end records declarations in the
   file scope even though that isn't the current scope.

   All declarations with external linkage are recorded in the external
   scope, even if they aren't visible there; this models the fact that
   such declarations are visible to the entire program, and (with a
   bit of cleverness, see pushdecl) allows diagnosis of some violations
   of C99 6.2.2p7 and 6.2.7p2:

     If, within the same translation unit, the same identifier appears
     with both internal and external linkage, the behavior is
     undefined.

     All declarations that refer to the same object or function shall
     have compatible type; otherwise, the behavior is undefined.

   Initially only the built-in declarations, which describe compiler
   intrinsic functions plus a subset of the standard library, are in
   this scope.

   The order of the blocks list matters, and it is frequently appended
   to.  To avoid having to walk all the way to the end of the list on
   each insertion, or reverse the list later, we maintain a pointer to
   the last list entry.  (FIXME: It should be feasible to use a reversed
   list here.)

   The bindings list is strictly in reverse order of declarations;
   pop_scope relies on this.  */


struct c_scope GTY((chain_next ("%h.outer")))
{
  /* The scope containing this one.  */
  struct c_scope *outer;

  /* The next outermost function scope.  */
  struct c_scope *outer_function;

  /* All bindings in this scope.  */
  struct c_binding *bindings;

  /* For each scope (except the global one), a chain of BLOCK nodes
     for all the scopes that were entered and exited one level down.  */
  tree blocks;
  tree blocks_last;

  /* The depth of this scope.  Used to keep the ->shadowed chain of
     bindings sorted innermost to outermost.  */
  unsigned int depth : 28;

  /* True if we are currently filling this scope with parameter
     declarations.  */
  BOOL_BITFIELD parm_flag : 1;

  /* True if we already complained about forward parameter decls
     in this scope.  This prevents double warnings on
     foo (int a; int b; ...)  */
  BOOL_BITFIELD warned_forward_parm_decls : 1;

  /* True if this is the outermost block scope of a function body.
     This scope contains the parameters, the local variables declared
     in the outermost block, and all the labels (except those in
     nested functions, or declared at block scope with __label__).  */
  BOOL_BITFIELD function_body : 1;

  /* True means make a BLOCK for this scope no matter what.  */
  BOOL_BITFIELD keep : 1;
};

/* The scope currently in effect.  */

static GTY(()) struct c_scope *current_scope;

/* The innermost function scope.  Ordinary (not explicitly declared)
   labels, bindings to error_mark_node, and the lazily-created
   bindings of __func__ and its friends get this scope.  */

static GTY(()) struct c_scope *current_function_scope;

/* The C file scope.  This is reset for each input translation unit.  */

static GTY(()) struct c_scope *file_scope;

/* The outermost scope.  This is used for all declarations with
   external linkage, and only these, hence the name.  */

static GTY(()) struct c_scope *external_scope;

/* A chain of c_scope structures awaiting reuse.  */

static GTY((deletable)) struct c_scope *scope_freelist;

/* A chain of c_binding structures awaiting reuse.  */

static GTY((deletable)) struct c_binding *binding_freelist;

/* Append VAR to LIST in scope SCOPE.  */
#define SCOPE_LIST_APPEND(scope, list, decl) do {	\
  struct c_scope *s_ = (scope);				\
  tree d_ = (decl);					\
  if (s_->list##_last)					\
    TREE_CHAIN (s_->list##_last) = d_;			\
  else							\
    s_->list = d_;					\
  s_->list##_last = d_;					\
} while (0)

/* Concatenate FROM in scope FSCOPE onto TO in scope TSCOPE.  */
#define SCOPE_LIST_CONCAT(tscope, to, fscope, from) do {	\
  struct c_scope *t_ = (tscope);				\
  struct c_scope *f_ = (fscope);				\
  if (t_->to##_last)						\
    TREE_CHAIN (t_->to##_last) = f_->from;			\
  else								\
    t_->to = f_->from;						\
  t_->to##_last = f_->from##_last;				\
} while (0)

/* True means unconditionally make a BLOCK for the next scope pushed.  */

static bool keep_next_level_flag;

/* True means the next call to push_scope will be the outermost scope
   of a function body, so do not push a new scope, merely cease
   expecting parameter decls.  */

static bool next_is_function_body;

/* Functions called automatically at the beginning and end of execution.  */

static GTY(()) tree static_ctors;
static GTY(()) tree static_dtors;

/* Forward declarations.  */
static tree lookup_name_in_scope (tree, struct c_scope *);
static tree c_make_fname_decl (tree, int);
static tree grokdeclarator (tree, tree, enum decl_context, bool, tree *);
static tree grokparms (tree, bool);
static void layout_array_type (tree);
/* APPLE LOCAL begin loop transposition */
static void loop_transpose (tree);
/* APPLE LOCAL begin MERGE HACK Disable loop transposition */
#if (0)
static tree perform_loop_transpose (tree *, int *, void *);
static tree tree_contains_1 (tree *, int *, void *);
static bool tree_contains (tree, tree);
static tree should_transpose_for_loops_1 (tree *, int *, void *);
static bool should_transpose_for_loops (tree, tree, tree, tree*);
static tree find_tree_with_code_1 (tree *, int *, void *);
static tree find_tree_with_code (tree, enum tree_code);
static tree find_pointer (tree);
#endif
/* APPLE LOCAL end MERGE HACK Disable loop transposition */
/* APPLE LOCAL end loop transposition */

/* States indicating how grokdeclarator() should handle declspecs marked
   with __attribute__((deprecated)).  An object declared as
   __attribute__((deprecated)) suppresses warnings of uses of other
   deprecated items.  */
/* APPLE LOCAL begin "unavailable" attribute (radar 2809697) */
/* Also add an __attribute__((unavailable)).  An object declared as
   __attribute__((unavailable)) suppresses any reports of being
   declared with unavailable or deprecated items.  */
/* APPLE LOCAL end */

enum deprecated_states {
  DEPRECATED_NORMAL,
  DEPRECATED_SUPPRESS
  /* APPLE LOCAL "unavailable" attribute (radar 2809697) */
  , DEPRECATED_UNAVAILABLE_SUPPRESS
};

static enum deprecated_states deprecated_state = DEPRECATED_NORMAL;

void
c_print_identifier (FILE *file, tree node, int indent)
{
  print_node (file, "symbol", I_SYMBOL_DECL (node), indent + 4);
  print_node (file, "tag", I_TAG_DECL (node), indent + 4);
  print_node (file, "label", I_LABEL_DECL (node), indent + 4);
  if (C_IS_RESERVED_WORD (node))
    {
      tree rid = ridpointers[C_RID_CODE (node)];
      indent_to (file, indent + 4);
      fprintf (file, "rid " HOST_PTR_PRINTF " \"%s\"",
	       (void *) rid, IDENTIFIER_POINTER (rid));
    }
}

/* Establish a binding between NAME, an IDENTIFIER_NODE, and DECL,
   which may be any of several kinds of DECL or TYPE or error_mark_node,
   in the scope SCOPE.  */
static void
bind (tree name, tree decl, struct c_scope *scope, bool invisible, bool nested)
{
  struct c_binding *b, **here;

  if (binding_freelist)
    {
      b = binding_freelist;
      binding_freelist = b->prev;
    }
  else
    b = GGC_NEW (struct c_binding);

  b->shadowed = 0;
  b->decl = decl;
  b->id = name;
  b->depth = scope->depth;
  b->invisible = invisible;
  b->nested = nested;

  b->prev = scope->bindings;
  scope->bindings = b;

  if (!name)
    return;

  switch (TREE_CODE (decl))
    {
    case LABEL_DECL:     here = &I_LABEL_BINDING (name);   break;
    case ENUMERAL_TYPE:
    case UNION_TYPE:
    case RECORD_TYPE:    here = &I_TAG_BINDING (name);     break;
    case VAR_DECL:
    case FUNCTION_DECL:
    case TYPE_DECL:
    case CONST_DECL:
    case PARM_DECL:
    case ERROR_MARK:     here = &I_SYMBOL_BINDING (name);  break;

    default:
      abort ();
    }

  /* Locate the appropriate place in the chain of shadowed decls
     to insert this binding.  Normally, scope == current_scope and
     this does nothing.  */
  while (*here && (*here)->depth > scope->depth)
    here = &(*here)->shadowed;

  b->shadowed = *here;
  *here = b;
}

/* Clear the binding structure B, stick it on the binding_freelist,
   and return the former value of b->prev.  This is used by pop_scope
   and get_parm_info to iterate destructively over all the bindings
   from a given scope.  */
static struct c_binding *
free_binding_and_advance (struct c_binding *b)
{
  struct c_binding *prev = b->prev;

  memset (b, 0, sizeof (struct c_binding));
  b->prev = binding_freelist;
  binding_freelist = b;

  return prev;
}


/* Hook called at end of compilation to assume 1 elt
   for a file-scope tentative array defn that wasn't complete before.  */

void
c_finish_incomplete_decl (tree decl)
{
  if (TREE_CODE (decl) == VAR_DECL)
    {
      tree type = TREE_TYPE (decl);
      if (type != error_mark_node
	  && TREE_CODE (type) == ARRAY_TYPE
	  && ! DECL_EXTERNAL (decl)
	  && TYPE_DOMAIN (type) == 0)
	{
	  warning ("%Jarray '%D' assumed to have one element", decl, decl);

	  complete_array_type (type, NULL_TREE, 1);

	  layout_decl (decl, 0);
	}
    }
}

/* The Objective-C front-end often needs to determine the current scope.  */

void *
get_current_scope (void)
{
  return current_scope;
}

/* The following function is used only by Objective-C.  It needs to live here
   because it accesses the innards of c_scope.  */

void
objc_mark_locals_volatile (void *enclosing_blk)
{
  struct c_scope *scope;
  struct c_binding *b;

  for (scope = current_scope;
       scope && scope != enclosing_blk;
       scope = scope->outer)
    {
      for (b = scope->bindings; b; b = b->prev)
	{
	  if (TREE_CODE (b->decl) == VAR_DECL
	      || TREE_CODE (b->decl) == PARM_DECL)
	    {
	      C_DECL_REGISTER (b->decl) = 0;
	      DECL_REGISTER (b->decl) = 0;
	      TREE_THIS_VOLATILE (b->decl) = 1;
	    }
	}

      /* Do not climb up past the current function.  */
      if (scope->function_body)
	break;
    }
}

/* Nonzero if we are currently in file scope.  */

int
global_bindings_p (void)
{
  return current_scope == file_scope && !c_override_global_bindings_to_false;
}

void
keep_next_level (void)
{
  keep_next_level_flag = true;
}

/* Identify this scope as currently being filled with parameters.  */

void
declare_parm_level (void)
{
  current_scope->parm_flag = true;
}

void
push_scope (void)
{
  if (next_is_function_body)
    {
      /* This is the transition from the parameters to the top level
	 of the function body.  These are the same scope
	 (C99 6.2.1p4,6) so we do not push another scope structure.
	 next_is_function_body is set only by store_parm_decls, which
	 in turn is called when and only when we are about to
	 encounter the opening curly brace for the function body.

	 The outermost block of a function always gets a BLOCK node,
	 because the debugging output routines expect that each
	 function has at least one BLOCK.  */
      current_scope->parm_flag         = false;
      current_scope->function_body     = true;
      current_scope->keep              = true;
      current_scope->outer_function    = current_function_scope;
      current_function_scope           = current_scope;

      keep_next_level_flag = false;
      next_is_function_body = false;
    }
  else
    {
      struct c_scope *scope;
      if (scope_freelist)
	{
	  scope = scope_freelist;
	  scope_freelist = scope->outer;
	}
      else
	scope = GGC_CNEW (struct c_scope);

      scope->keep          = keep_next_level_flag;
      scope->outer         = current_scope;
      scope->depth	   = current_scope ? (current_scope->depth + 1) : 0;

      /* Check for scope depth overflow.  Unlikely (2^28 == 268,435,456) but
	 possible.  */
      if (current_scope && scope->depth == 0)
	{
	  scope->depth--;
	  sorry ("GCC supports only %u nested scopes\n", scope->depth);
	}

      current_scope        = scope;
      keep_next_level_flag = false;
    }
}

/* Set the TYPE_CONTEXT of all of TYPE's variants to CONTEXT.  */

static void
set_type_context (tree type, tree context)
{
  for (type = TYPE_MAIN_VARIANT (type); type;
       type = TYPE_NEXT_VARIANT (type))
    TYPE_CONTEXT (type) = context;
}

/* Exit a scope.  Restore the state of the identifier-decl mappings
   that were in effect when this scope was entered.  Return a BLOCK
   node containing all the DECLs in this scope that are of interest
   to debug info generation.  */

tree
pop_scope (void)
{
  struct c_scope *scope = current_scope;
  tree block, context, p;
  struct c_binding *b;

  bool functionbody = scope->function_body;
  bool keep = functionbody || scope->keep || scope->bindings;

  /* If appropriate, create a BLOCK to record the decls for the life
     of this function.  */
  block = 0;
  if (keep)
    {
      block = make_node (BLOCK);
      BLOCK_SUBBLOCKS (block) = scope->blocks;
      TREE_USED (block) = 1;

      /* In each subblock, record that this is its superior.  */
      for (p = scope->blocks; p; p = TREE_CHAIN (p))
	BLOCK_SUPERCONTEXT (p) = block;

      BLOCK_VARS (block) = 0;
    }

  /* The TYPE_CONTEXTs for all of the tagged types belonging to this
     scope must be set so that they point to the appropriate
     construct, i.e.  either to the current FUNCTION_DECL node, or
     else to the BLOCK node we just constructed.

     Note that for tagged types whose scope is just the formal
     parameter list for some function type specification, we can't
     properly set their TYPE_CONTEXTs here, because we don't have a
     pointer to the appropriate FUNCTION_TYPE node readily available
     to us.  For those cases, the TYPE_CONTEXTs of the relevant tagged
     type nodes get set in `grokdeclarator' as soon as we have created
     the FUNCTION_TYPE node which will represent the "scope" for these
     "parameter list local" tagged types.  */
  if (scope->function_body)
    context = current_function_decl;
  else if (scope == file_scope)
    {
      tree file_decl = build_decl (TRANSLATION_UNIT_DECL, 0, 0);
      TREE_CHAIN (file_decl) = all_translation_units;
      all_translation_units = file_decl;
      context = file_decl;
    }
  else
    context = block;

  /* Clear all bindings in this scope.  */
  for (b = scope->bindings; b; b = free_binding_and_advance (b))
    {
      p = b->decl;
      switch (TREE_CODE (p))
	{
	case LABEL_DECL:
	  /* Warnings for unused labels, errors for undefined labels.  */
	  if (TREE_USED (p) && !DECL_INITIAL (p))
	    {
	      error ("%Jlabel `%D' used but not defined", p, p);
	      DECL_INITIAL (p) = error_mark_node;
	    }
	  else if (!TREE_USED (p) && warn_unused_label)
	    {
	      if (DECL_INITIAL (p))
		warning ("%Jlabel `%D' defined but not used", p, p);
	      else
		warning ("%Jlabel `%D' declared but not defined", p, p);
	    }
	  /* Labels go in BLOCK_VARS.  */
	  TREE_CHAIN (p) = BLOCK_VARS (block);
	  BLOCK_VARS (block) = p;

#ifdef ENABLE_CHECKING
	  if (I_LABEL_BINDING (b->id) != b) abort ();
#endif
 	  I_LABEL_BINDING (b->id) = b->shadowed;
 	  break;

	case ENUMERAL_TYPE:
	case UNION_TYPE:
	case RECORD_TYPE:
	  set_type_context (p, context);

	  /* Types may not have tag-names, in which case the type
	     appears in the bindings list with b->id NULL.  */
	  if (b->id)
	    {
#ifdef ENABLE_CHECKING
	      if (I_TAG_BINDING (b->id) != b) abort ();
#endif
	      I_TAG_BINDING (b->id) = b->shadowed;
	    }
  	  break;

	case FUNCTION_DECL:
	  /* Propagate TREE_ADDRESSABLE from nested functions to their
	     containing functions.  */
	  if (! TREE_ASM_WRITTEN (p)
	      && DECL_INITIAL (p) != 0
	      && TREE_ADDRESSABLE (p)
	      && DECL_ABSTRACT_ORIGIN (p) != 0
	      && DECL_ABSTRACT_ORIGIN (p) != p)
	    TREE_ADDRESSABLE (DECL_ABSTRACT_ORIGIN (p)) = 1;
	  goto common_symbol;

	case VAR_DECL:
	  /* Warnings for unused variables.  */
	  if (warn_unused_variable
	      && !TREE_USED (p)
	      && !DECL_IN_SYSTEM_HEADER (p)
	      && DECL_NAME (p)
	      && !DECL_ARTIFICIAL (p)
	      && (scope != file_scope
		  || (TREE_STATIC (p) && !TREE_PUBLIC (p)
		      && !TREE_THIS_VOLATILE (p)))
	      && scope != external_scope)
	    warning ("%Junused variable `%D'", p, p);

	  /* Fall through.  */
	case TYPE_DECL:
	case CONST_DECL:
	common_symbol:
	  /* All of these go in BLOCK_VARS, but only if this is the
	     binding in the home scope.  */
	  if (!b->nested)
	    {
	      TREE_CHAIN (p) = BLOCK_VARS (block);
	      BLOCK_VARS (block) = p;
	    }
	  /* If this is the file scope, and we are processing more
	     than one translation unit in this compilation, set
	     DECL_CONTEXT of each decl to the TRANSLATION_UNIT_DECL.
	     This makes same_translation_unit_p work, and causes
	     static declarations to be given disambiguating suffixes.  */
	  if (scope == file_scope && num_in_fnames > 1)
	    {
	      DECL_CONTEXT (p) = context;
	      if (TREE_CODE (p) == TYPE_DECL)
		set_type_context (TREE_TYPE (p), context);
	    }

	  /* Fall through.  */
	  /* Parameters go in DECL_ARGUMENTS, not BLOCK_VARS, and have
	     already been put there by store_parm_decls.  Unused-
	     parameter warnings are handled by function.c.
	     error_mark_node obviously does not go in BLOCK_VARS and
	     does not get unused-variable warnings.  */
	case PARM_DECL:
	case ERROR_MARK:
	  /* It is possible for a decl not to have a name.  We get
	     here with b->id NULL in this case.  */
	  if (b->id)
	    {
#ifdef ENABLE_CHECKING
	      if (I_SYMBOL_BINDING (b->id) != b) abort ();
#endif
	      I_SYMBOL_BINDING (b->id) = b->shadowed;
	    }
	  break;

	default:
	  abort ();
	}
    }


  /* Dispose of the block that we just made inside some higher level.  */
  if ((scope->function_body || scope == file_scope) && context)
    {
      DECL_INITIAL (context) = block;
      BLOCK_SUPERCONTEXT (block) = context;
    }
  else if (scope->outer)
    {
      if (block)
	SCOPE_LIST_APPEND (scope->outer, blocks, block);
      /* If we did not make a block for the scope just exited, any
	 blocks made for inner scopes must be carried forward so they
	 will later become subblocks of something else.  */
      else if (scope->blocks)
	SCOPE_LIST_CONCAT (scope->outer, blocks, scope, blocks);
    }

  /* Pop the current scope, and free the structure for reuse.  */
  current_scope = scope->outer;
  if (scope->function_body)
    current_function_scope = scope->outer_function;

  memset (scope, 0, sizeof (struct c_scope));
  scope->outer = scope_freelist;
  scope_freelist = scope;

  return block;
}

void
push_file_scope (void)
{
  tree decl;

  if (file_scope)
    return;

  push_scope ();
  file_scope = current_scope;

  start_fname_decls ();

  for (decl = visible_builtins; decl; decl = TREE_CHAIN (decl))
    bind (DECL_NAME (decl), decl, file_scope,
	  /*invisible=*/false, /*nested=*/true);
}

void
pop_file_scope (void)
{
  /* In case there were missing closebraces, get us back to the global
     binding level.  */
  while (current_scope != file_scope)
    pop_scope ();

  /* __FUNCTION__ is defined at file scope ("").  This
     call may not be necessary as my tests indicate it
     still works without it.  */
  finish_fname_decls ();

  /* This is the point to write out a PCH if we're doing that.
     In that case we do not want to do anything else.  */
  if (pch_file)
    {
      c_common_write_pch ();
      return;
    }

  /* Pop off the file scope and close this translation unit.  */
  pop_scope ();
  file_scope = 0;
  cgraph_finalize_compilation_unit ();
}

/* Insert BLOCK at the end of the list of subblocks of the current
   scope.  This is used when a BIND_EXPR is expanded, to handle the
   BLOCK node inside the BIND_EXPR.  */

void
insert_block (tree block)
{
  TREE_USED (block) = 1;
  SCOPE_LIST_APPEND (current_scope, blocks, block);
}

/* Push a definition or a declaration of struct, union or enum tag "name".
   "type" should be the type node.
   We assume that the tag "name" is not already defined.

   Note that the definition may really be just a forward reference.
   In that case, the TYPE_SIZE will be zero.  */

static void
pushtag (tree name, tree type)
{
  /* Record the identifier as the type's name if it has none.  */
  if (name && !TYPE_NAME (type))
    TYPE_NAME (type) = name;
  bind (name, type, current_scope, /*invisible=*/false, /*nested=*/false);

  /* Create a fake NULL-named TYPE_DECL node whose TREE_TYPE will be the
     tagged type we just added to the current scope.  This fake
     NULL-named TYPE_DECL node helps dwarfout.c to know when it needs
     to output a representation of a tagged type, and it also gives
     us a convenient place to record the "scope start" address for the
     tagged type.  */

  TYPE_STUB_DECL (type) = pushdecl (build_decl (TYPE_DECL, NULL_TREE, type));

  /* An approximation for now, so we can tell this is a function-scope tag.
     This will be updated in pop_scope.  */
  TYPE_CONTEXT (type) = DECL_CONTEXT (TYPE_STUB_DECL (type));
}

/* Subroutine of compare_decls.  Allow harmless mismatches in return
   and argument types provided that the type modes match.  This function
   return a unified type given a suitable match, and 0 otherwise.  */

static tree
match_builtin_function_types (tree newtype, tree oldtype)
{
  tree newrettype, oldrettype;
  tree newargs, oldargs;
  tree trytype, tryargs;

  /* Accept the return type of the new declaration if same modes.  */
  oldrettype = TREE_TYPE (oldtype);
  newrettype = TREE_TYPE (newtype);

  if (TYPE_MODE (oldrettype) != TYPE_MODE (newrettype))
    return 0;

  oldargs = TYPE_ARG_TYPES (oldtype);
  newargs = TYPE_ARG_TYPES (newtype);
  tryargs = newargs;

  while (oldargs || newargs)
    {
      if (! oldargs
	  || ! newargs
	  || ! TREE_VALUE (oldargs)
	  || ! TREE_VALUE (newargs)
	  || TYPE_MODE (TREE_VALUE (oldargs))
	     != TYPE_MODE (TREE_VALUE (newargs)))
	return 0;

      oldargs = TREE_CHAIN (oldargs);
      newargs = TREE_CHAIN (newargs);
    }

  trytype = build_function_type (newrettype, tryargs);
  return build_type_attribute_variant (trytype, TYPE_ATTRIBUTES (oldtype));
}

/* Subroutine of diagnose_mismatched_decls.  Check for function type
   mismatch involving an empty arglist vs a nonempty one and give clearer
   diagnostics.  */
static void
diagnose_arglist_conflict (tree newdecl, tree olddecl,
			   tree newtype, tree oldtype)
{
  tree t;

  if (TREE_CODE (olddecl) != FUNCTION_DECL
      || !comptypes (TREE_TYPE (oldtype), TREE_TYPE (newtype))
      || !((TYPE_ARG_TYPES (oldtype) == 0 && DECL_INITIAL (olddecl) == 0)
	   ||
	   (TYPE_ARG_TYPES (newtype) == 0 && DECL_INITIAL (newdecl) == 0)))
    return;

  t = TYPE_ARG_TYPES (oldtype);
  if (t == 0)
    t = TYPE_ARG_TYPES (newtype);
  for (; t; t = TREE_CHAIN (t))
    {
      tree type = TREE_VALUE (t);

      if (TREE_CHAIN (t) == 0
	  && TYPE_MAIN_VARIANT (type) != void_type_node)
	{
	  inform ("a parameter list with an ellipsis can't match "
		  "an empty parameter name list declaration");
	  break;
	}

      if (c_type_promotes_to (type) != type)
	{
	  inform ("an argument type that has a default promotion can't match "
		  "an empty parameter name list declaration");
	  break;
	}
    }
}

/* Another subroutine of diagnose_mismatched_decls.  OLDDECL is an
   old-style function definition, NEWDECL is a prototype declaration.
   Diagnose inconsistencies in the argument list.  Returns TRUE if
   the prototype is compatible, FALSE if not.  */
static bool
validate_proto_after_old_defn (tree newdecl, tree newtype, tree oldtype)
{
  tree newargs, oldargs;
  int i;

  /* ??? Elsewhere TYPE_MAIN_VARIANT is not used in this context.  */
#define END_OF_ARGLIST(t) (TYPE_MAIN_VARIANT (t) == void_type_node)

  oldargs = TYPE_ACTUAL_ARG_TYPES (oldtype);
  newargs = TYPE_ARG_TYPES (newtype);
  i = 1;

  for (;;)
    {
      tree oldargtype = TREE_VALUE (oldargs);
      tree newargtype = TREE_VALUE (newargs);

      if (END_OF_ARGLIST (oldargtype) && END_OF_ARGLIST (newargtype))
	break;

      /* Reaching the end of just one list means the two decls don't
	 agree on the number of arguments.  */
      if (END_OF_ARGLIST (oldargtype))
	{
	  error ("%Jprototype for '%D' declares more arguments "
		 "than previous old-style definition", newdecl, newdecl);
	  return false;
	}
      else if (END_OF_ARGLIST (newargtype))
	{
	  error ("%Jprototype for '%D' declares fewer arguments "
		 "than previous old-style definition", newdecl, newdecl);
	  return false;
	}

      /* Type for passing arg must be consistent with that declared
	 for the arg.  */
      else if (! comptypes (oldargtype, newargtype))
	{
	  error ("%Jprototype for '%D' declares arg %d with incompatible type",
		 newdecl, newdecl, i);
	  return false;
	}

      oldargs = TREE_CHAIN (oldargs);
      newargs = TREE_CHAIN (newargs);
      i++;
    }

  /* If we get here, no errors were found, but do issue a warning
     for this poor-style construct.  */
  warning ("%Jprototype for '%D' follows non-prototype definition",
	   newdecl, newdecl);
  return true;
#undef END_OF_ARGLIST
}

/* Subroutine of diagnose_mismatched_decls.  Report the location of DECL,
   first in a pair of mismatched declarations, using the diagnostic
   function DIAG.  */
static void
locate_old_decl (tree decl, void (*diag)(const char *, ...))
{
  if (TREE_CODE (decl) == FUNCTION_DECL && DECL_BUILT_IN (decl))
    ;
  else if (DECL_INITIAL (decl))
    diag (N_("%Jprevious definition of '%D' was here"), decl, decl);
  else if (C_DECL_IMPLICIT (decl))
    diag (N_("%Jprevious implicit declaration of '%D' was here"), decl, decl);
  else
    diag (N_("%Jprevious declaration of '%D' was here"), decl, decl);
}

/* Subroutine of duplicate_decls.  Compare NEWDECL to OLDDECL.
   Returns true if the caller should proceed to merge the two, false
   if OLDDECL should simply be discarded.  As a side effect, issues
   all necessary diagnostics for invalid or poor-style combinations.
   If it returns true, writes the types of NEWDECL and OLDDECL to
   *NEWTYPEP and *OLDTYPEP - these may have been adjusted from
   TREE_TYPE (NEWDECL, OLDDECL) respectively.  */

static bool
diagnose_mismatched_decls (tree newdecl, tree olddecl,
			   tree *newtypep, tree *oldtypep)
{
  tree newtype, oldtype;
  bool pedwarned = false;
  bool warned = false;

  /* If we have error_mark_node for either decl or type, just discard
     the previous decl - we're in an error cascade already.  */
  if (olddecl == error_mark_node || newdecl == error_mark_node)
    return false;
  *oldtypep = oldtype = TREE_TYPE (olddecl);
  *newtypep = newtype = TREE_TYPE (newdecl);
  if (oldtype == error_mark_node || newtype == error_mark_node)
    return false;

  /* Two different categories of symbol altogether.  This is an error
     unless OLDDECL is a builtin.  OLDDECL will be discarded in any case.  */
  if (TREE_CODE (olddecl) != TREE_CODE (newdecl))
    {
      if (!(TREE_CODE (olddecl) == FUNCTION_DECL
	    && DECL_BUILT_IN (olddecl)
	    && !C_DECL_DECLARED_BUILTIN (olddecl)))
	{
	  error ("%J'%D' redeclared as different kind of symbol",
		 newdecl, newdecl);
	  locate_old_decl (olddecl, error);
	}
      else if (TREE_PUBLIC (newdecl))
	warning ("%Jbuilt-in function '%D' declared as non-function",
		 newdecl, newdecl);
      else if (warn_shadow)
	warning ("%Jdeclaration of '%D' shadows a built-in function",
		 newdecl, newdecl);
      return false;
    }

  if (!comptypes (oldtype, newtype))
    {
      if (TREE_CODE (olddecl) == FUNCTION_DECL
	  && DECL_BUILT_IN (olddecl) && !C_DECL_DECLARED_BUILTIN (olddecl))
	{
	  /* Accept harmless mismatch in function types.
	     This is for the ffs and fprintf builtins.  */
	  tree trytype = match_builtin_function_types (newtype, oldtype);

	  if (trytype && comptypes (newtype, trytype))
	    *oldtypep = oldtype = trytype;
	  else
	    {
	      /* If types don't match for a built-in, throw away the
		 built-in.  No point in calling locate_old_decl here, it
		 won't print anything.  */
	      warning ("%Jconflicting types for built-in function '%D'",
		       newdecl, newdecl);
	      return false;
	    }
	}
      else if (TREE_CODE (olddecl) == FUNCTION_DECL
	       && DECL_IS_BUILTIN (olddecl))
	{
	  /* A conflicting function declaration for a predeclared
	     function that isn't actually built in.  Objective C uses
	     these.  The new declaration silently overrides everything
	     but the volatility (i.e. noreturn) indication.  See also
	     below.  FIXME: Make Objective C use normal builtins.  */
	  TREE_THIS_VOLATILE (newdecl) |= TREE_THIS_VOLATILE (olddecl);
	  return false;
	}
      /* Permit void foo (...) to match int foo (...) if the latter is
	 the definition and implicit int was used.  See
	 c-torture/compile/920625-2.c.  */
      else if (TREE_CODE (newdecl) == FUNCTION_DECL && DECL_INITIAL (newdecl)
	       && TYPE_MAIN_VARIANT (TREE_TYPE (oldtype)) == void_type_node
	       && TYPE_MAIN_VARIANT (TREE_TYPE (newtype)) == integer_type_node
	       && C_FUNCTION_IMPLICIT_INT (newdecl))
	{
	  pedwarn ("%Jconflicting types for '%D'", newdecl, newdecl);
	  /* Make sure we keep void as the return type.  */
	  TREE_TYPE (newdecl) = *newtypep = newtype = oldtype;
	  C_FUNCTION_IMPLICIT_INT (newdecl) = 0;
	  pedwarned = true;
	}
      else
	{
	  /* APPLE LOCAL begin disable typechecking for SPEC --dbj */
	  /* Accept incompatible function declarations, and incompatible
	     global variables provided they are in different files. 
	     (Would be nice to check this for functions also, but
	     context is not set for them.)  */
	  void (*err) (const char*, ...);
	  if (disable_typechecking_for_spec_flag 
	      && TREE_PUBLIC (olddecl)
	      && TREE_PUBLIC (newdecl)
	      && ((TREE_CODE (olddecl) == VAR_DECL 
	            && TREE_CODE (newdecl) == VAR_DECL
		    && !same_translation_unit_p (olddecl, newdecl))
	          || (TREE_CODE (olddecl) == FUNCTION_DECL
		      && TREE_CODE (newdecl) == FUNCTION_DECL)))
	    err = warning;
	  else
	    err = error;
	  if (TYPE_QUALS (newtype) != TYPE_QUALS (oldtype))
	    err ("%J conflicting type qualifiers for '%D'", newdecl, newdecl);
	  else
	    err ("%Jconflicting types for '%D'", newdecl, newdecl);
	  diagnose_arglist_conflict (newdecl, olddecl, newtype, oldtype);
	  locate_old_decl (olddecl, *err);
	  /* In the case where we're being lenient, two trees will
	     continue to exist, which represent the same variable.
	     These are currently never used in the same function, but
	     watch out for inlining.  */
	  if (err == warning
	      && TREE_CODE (olddecl) == VAR_DECL)
	    {
	      HOST_WIDE_INT newalias;
	      /* To prevent aliasing problems, 
	         make both of them have the same alias class right now.  */
	      if (TYPE_ALIAS_SET_KNOWN_P (oldtype))
		{
		  newalias = TYPE_ALIAS_SET (oldtype);
		  if (TYPE_ALIAS_SET_KNOWN_P (newtype)
		      && newalias != TYPE_ALIAS_SET (oldtype))
		    internal_error ("%Jalias set conflict for '%D'", 
				newdecl, newdecl);
		}
	      else if (TYPE_ALIAS_SET_KNOWN_P (newtype))
		newalias = TYPE_ALIAS_SET (newtype);
	      else
		newalias = new_alias_set ();
	      TYPE_ALIAS_SET (oldtype) = newalias;
	      TYPE_ALIAS_SET (newtype) = newalias;
	      /* Set a marker bit so that only one of them gets emitted
		 to the output file.  I think this only matters if the 
		 sizes are different, in which case we must emit the larger.
		 If a size is unknown, we can mark that one.  */
	      if (!DECL_SIZE (newdecl))
		DECL_DUPLICATE_DECL (newdecl) = 1;
	      else if (!DECL_SIZE (olddecl))
		DECL_DUPLICATE_DECL (olddecl) = 1;
	      else if (tree_int_cst_lt (DECL_SIZE (newdecl), 
					DECL_SIZE (olddecl)))
		DECL_DUPLICATE_DECL (newdecl) = 1;
	      else
		DECL_DUPLICATE_DECL (olddecl) = 1;
	    }
	  /* APPLE LOCAL end disable typechecking for SPEC --dbj */
	  return false;
	}
    }

  /* Redeclaration of a type is a constraint violation (6.7.2.3p1),
     but silently ignore the redeclaration if either is in a system
     header.  (Conflicting redeclarations were handled above.)  */
  if (TREE_CODE (newdecl) == TYPE_DECL)
    {
      if (DECL_IN_SYSTEM_HEADER (newdecl) || DECL_IN_SYSTEM_HEADER (olddecl))
	return true;  /* Allow OLDDECL to continue in use.  */

      error ("%Jredefinition of typedef '%D'", newdecl, newdecl);
      locate_old_decl (olddecl, error);
      return false;
    }

  /* Function declarations can either be 'static' or 'extern' (no
     qualifier is equivalent to 'extern' - C99 6.2.2p5) and therefore
     can never conflict with each other on account of linkage (6.2.2p4).
     Multiple definitions are not allowed (6.9p3,5) but GCC permits
     two definitions if one is 'extern inline' and one is not.  The non-
     extern-inline definition supersedes the extern-inline definition.  */
  else if (TREE_CODE (newdecl) == FUNCTION_DECL)
    {
      /* If you declare a built-in function name as static, or
	 define the built-in with an old-style definition (so we
	 can't validate the argument list) the built-in definition is
	 overridden, but optionally warn this was a bad choice of name.  */
      if (DECL_BUILT_IN (olddecl)
	  && !C_DECL_DECLARED_BUILTIN (olddecl)
	  && (!TREE_PUBLIC (newdecl)
	      || (DECL_INITIAL (newdecl)
		  && !TYPE_ARG_TYPES (TREE_TYPE (newdecl)))))
	{
	  if (warn_shadow)
	    warning ("%Jdeclaration of '%D' shadows a built-in function",
		     newdecl, newdecl);
	  /* Discard the old built-in function.  */
	  return false;
	}

      if (DECL_INITIAL (newdecl))
	{
	  if (DECL_INITIAL (olddecl)
	      && !(DECL_DECLARED_INLINE_P (olddecl)
		   && DECL_EXTERNAL (olddecl)
		   && !(DECL_DECLARED_INLINE_P (newdecl)
			&& DECL_EXTERNAL (newdecl)
	    		&& same_translation_unit_p (olddecl, newdecl))))
	    {
	      error ("%Jredefinition of '%D'", newdecl, newdecl);
	      locate_old_decl (olddecl, error);
	      return false;
	    }
	}
      /* If we have a prototype after an old-style function definition,
	 the argument types must be checked specially.  */
      else if (DECL_INITIAL (olddecl)
	       && !TYPE_ARG_TYPES (oldtype) && TYPE_ARG_TYPES (newtype)
	       && TYPE_ACTUAL_ARG_TYPES (oldtype)
	       && !validate_proto_after_old_defn (newdecl, newtype, oldtype))
	{
	  locate_old_decl (olddecl, error);
	  return false;
	}
      /* A non-static declaration (even an "extern") followed by a
	 static declaration is undefined behavior per C99 6.2.2p3-5,7.
	 The same is true for a static forward declaration at block
	 scope followed by a non-static declaration/definition at file
	 scope.  Static followed by non-static at the same scope is
	 not undefined behavior, and is the most convenient way to get
	 some effects (see e.g.  what unwind-dw2-fde-glibc.c does to
	 the definition of _Unwind_Find_FDE in unwind-dw2-fde.c), but
	 we do diagnose it if -Wtraditional. */
      if (TREE_PUBLIC (olddecl) && !TREE_PUBLIC (newdecl))
	{
	  /* Two exceptions to the rule.  If olddecl is an extern
	     inline, or a predeclared function that isn't actually
	     built in, newdecl silently overrides olddecl.  The latter
	     occur only in Objective C; see also above.  (FIXME: Make
	     Objective C use normal builtins.)  */
	  if (!DECL_IS_BUILTIN (olddecl)
	      && !(DECL_EXTERNAL (olddecl)
		   && DECL_DECLARED_INLINE_P (olddecl)))
	    {
	      error ("%Jstatic declaration of '%D' follows "
		     "non-static declaration", newdecl, newdecl);
	      locate_old_decl (olddecl, error);
	    }
	  return false;
	}
      else if (TREE_PUBLIC (newdecl) && !TREE_PUBLIC (olddecl))
	{
	  if (DECL_CONTEXT (olddecl))
	    {
	      error ("%Jnon-static declaration of '%D' follows "
		     "static declaration", newdecl, newdecl);
	      locate_old_decl (olddecl, error);
	      return false;
	    }
	  else if (warn_traditional)
	    {
	      warning ("%Jnon-static declaration of '%D' follows "
		       "static declaration", newdecl, newdecl);
	      warned = true;
	    }
	}
    }
  else if (TREE_CODE (newdecl) == VAR_DECL)
    {
      /* Only variables can be thread-local, and all declarations must
	 agree on this property.  */
      if (DECL_THREAD_LOCAL (newdecl) != DECL_THREAD_LOCAL (olddecl))
	{
	  if (DECL_THREAD_LOCAL (newdecl))
	    error ("%Jthread-local declaration of '%D' follows "
		   "non-thread-local declaration", newdecl, newdecl);
	  else
	    error ("%Jnon-thread-local declaration of '%D' follows "
		   "thread-local declaration", newdecl, newdecl);

	  locate_old_decl (olddecl, error);
	  return false;
	}

      /* Multiple initialized definitions are not allowed (6.9p3,5).  */
      if (DECL_INITIAL (newdecl) && DECL_INITIAL (olddecl))
	{
	  error ("%Jredefinition of '%D'", newdecl, newdecl);
	  locate_old_decl (olddecl, error);
	  return false;
	}

      /* Objects declared at file scope: if the first declaration had
	 external linkage (even if it was an external reference) the
	 second must have external linkage as well, or the behavior is
	 undefined.  If the first declaration had internal linkage, then
	 the second must too, or else be an external reference (in which
	 case the composite declaration still has internal linkage).
	 As for function declarations, we warn about the static-then-
	 extern case only for -Wtraditional.  See generally 6.2.2p3-5,7.  */
      if (DECL_FILE_SCOPE_P (newdecl)
	  && TREE_PUBLIC (newdecl) != TREE_PUBLIC (olddecl))
	{
	  if (DECL_EXTERNAL (newdecl))
	    {
	      if (!DECL_FILE_SCOPE_P (olddecl))
		{
		  error ("%Jextern declaration of %qD follows "
			 "declaration with no linkage", newdecl, newdecl);
		  locate_old_decl (olddecl, error);
		  return false;
		}
	      else if (warn_traditional)
		{
		  warning ("%Jnon-static declaration of '%D' follows "
			   "static declaration", newdecl, newdecl);
		  warned = true;
		}
	    }
	  else
	    {
	      if (TREE_PUBLIC (newdecl))
		error ("%Jnon-static declaration of '%D' follows "
		       "static declaration", newdecl, newdecl);
	      else
		error ("%Jstatic declaration of '%D' follows "
		       "non-static declaration", newdecl, newdecl);

	      locate_old_decl (olddecl, error);
	      return false;
	    }
	}
      /* Two objects with the same name declared at the same block
	 scope must both be external references (6.7p3).  */
      else if (!DECL_FILE_SCOPE_P (newdecl))
	{
	  if (DECL_EXTERNAL (newdecl))
	    abort ();
	  else if (DECL_EXTERNAL (olddecl))
	    error ("%Jdeclaration of '%D' with no linkage follows "
		   "extern declaration", newdecl, newdecl);
	  else
	    error ("%Jredeclaration of '%D' with no linkage",
		   newdecl, newdecl);

	  locate_old_decl (olddecl, error);
	  return false;
	}
    }

  /* warnings */
  /* All decls must agree on a visibility.  */
  if (DECL_VISIBILITY_SPECIFIED (newdecl) && DECL_VISIBILITY_SPECIFIED (olddecl)
      && DECL_VISIBILITY (newdecl) != DECL_VISIBILITY (olddecl))
    {
      warning ("%Jredeclaration of '%D' with different visibility "
	       "(old visibility preserved)", newdecl, newdecl);
      warned = true;
    }

  if (TREE_CODE (newdecl) == FUNCTION_DECL)
    {
      /* Diagnose inline __attribute__ ((noinline)) which is silly.  */
      if (DECL_DECLARED_INLINE_P (newdecl)
	  && lookup_attribute ("noinline", DECL_ATTRIBUTES (olddecl)))
	{
	  warning ("%Jinline declaration of '%D' follows "
		   "declaration with attribute noinline", newdecl, newdecl);
	  warned = true;
	}
      else if (DECL_DECLARED_INLINE_P (olddecl)
	       && lookup_attribute ("noinline", DECL_ATTRIBUTES (newdecl)))
	{
	  warning ("%Jdeclaration of '%D' with attribute noinline follows "
		   "inline declaration ", newdecl, newdecl);
	  warned = true;
	}

      /* Inline declaration after use or definition.
	 ??? Should we still warn about this now we have unit-at-a-time
	 mode and can get it right?
	 Definitely don't complain if the decls are in different translation
	 units.  */
      if (DECL_DECLARED_INLINE_P (newdecl) && !DECL_DECLARED_INLINE_P (olddecl)
	  && same_translation_unit_p (olddecl, newdecl))
	{
	  if (TREE_USED (olddecl))
	    {
	      warning ("%J'%D' declared inline after being called",
		       olddecl, olddecl);
	      warned = true;
	    }
	  else if (DECL_INITIAL (olddecl))
	    {
	      warning ("%J'%D' declared inline after its definition",
		       olddecl, olddecl);
	      warned = true;
	    }
	}
    }
  else /* PARM_DECL, VAR_DECL */
    {
      /* Redeclaration of a parameter is a constraint violation (this is
	 not explicitly stated, but follows from C99 6.7p3 [no more than
	 one declaration of the same identifier with no linkage in the
	 same scope, except type tags] and 6.2.2p6 [parameters have no
	 linkage]).  We must check for a forward parameter declaration,
	 indicated by TREE_ASM_WRITTEN on the old declaration - this is
	 an extension, the mandatory diagnostic for which is handled by
	 mark_forward_parm_decls.  */

      if (TREE_CODE (newdecl) == PARM_DECL
	  && (!TREE_ASM_WRITTEN (olddecl) || TREE_ASM_WRITTEN (newdecl)))
	{
	  error ("%Jredefinition of parameter '%D'", newdecl, newdecl);
	  locate_old_decl (olddecl, error);
	  return false;
	}
    }

  /* Optional warning for completely redundant decls.  */
  if (!warned && !pedwarned
      && warn_redundant_decls
      /* Don't warn about a function declaration followed by a
	 definition.  */
      && !(TREE_CODE (newdecl) == FUNCTION_DECL
	   && DECL_INITIAL (newdecl) && !DECL_INITIAL (olddecl))
      /* Don't warn about redundant redeclarations of builtins. */
      && !(TREE_CODE (newdecl) == FUNCTION_DECL
	   && !DECL_BUILT_IN (newdecl)
	   && DECL_BUILT_IN (olddecl)
	   && !C_DECL_DECLARED_BUILTIN (olddecl))
      /* Don't warn about an extern followed by a definition.  */
      && !(DECL_EXTERNAL (olddecl) && !DECL_EXTERNAL (newdecl))
      /* Don't warn about forward parameter decls.  */
      && !(TREE_CODE (newdecl) == PARM_DECL
	   && TREE_ASM_WRITTEN (olddecl) && !TREE_ASM_WRITTEN (newdecl)))
    {
      warning ("%Jredundant redeclaration of '%D'", newdecl, newdecl);
      warned = true;
    }

  /* Report location of previous decl/defn in a consistent manner.  */
  if (warned || pedwarned)
    locate_old_decl (olddecl, pedwarned ? pedwarn : warning);

  return true;
}

/* Subroutine of duplicate_decls.  NEWDECL has been found to be
   consistent with OLDDECL, but carries new information.  Merge the
   new information into OLDDECL.  This function issues no
   diagnostics.  */

static void
merge_decls (tree newdecl, tree olddecl, tree newtype, tree oldtype)
{
  int new_is_definition = (TREE_CODE (newdecl) == FUNCTION_DECL
			   && DECL_INITIAL (newdecl) != 0);

  /* For real parm decl following a forward decl, rechain the old decl
     in its new location and clear TREE_ASM_WRITTEN (it's not a
     forward decl anymore).  */
  if (TREE_CODE (newdecl) == PARM_DECL
      && TREE_ASM_WRITTEN (olddecl) && ! TREE_ASM_WRITTEN (newdecl))
    {
      struct c_binding *b, **here;

      for (here = &current_scope->bindings; *here; here = &(*here)->prev)
	if ((*here)->decl == olddecl)
	  goto found;
      abort ();

    found:
      b = *here;
      *here = b->prev;
      b->prev = current_scope->bindings;
      current_scope->bindings = b;

      TREE_ASM_WRITTEN (olddecl) = 0;
    }

  DECL_ATTRIBUTES (newdecl)
    = targetm.merge_decl_attributes (olddecl, newdecl);

  /* Merge the data types specified in the two decls.  */
  TREE_TYPE (newdecl)
    = TREE_TYPE (olddecl)
    = composite_type (newtype, oldtype);

  /* Lay the type out, unless already done.  */
  if (oldtype != TREE_TYPE (newdecl))
    {
      if (TREE_TYPE (newdecl) != error_mark_node)
	layout_type (TREE_TYPE (newdecl));
      if (TREE_CODE (newdecl) != FUNCTION_DECL
	  && TREE_CODE (newdecl) != TYPE_DECL
	  && TREE_CODE (newdecl) != CONST_DECL)
	layout_decl (newdecl, 0);
    }
  else
    {
      /* Since the type is OLDDECL's, make OLDDECL's size go with.  */
      DECL_SIZE (newdecl) = DECL_SIZE (olddecl);
      DECL_SIZE_UNIT (newdecl) = DECL_SIZE_UNIT (olddecl);
      DECL_MODE (newdecl) = DECL_MODE (olddecl);
      if (TREE_CODE (olddecl) != FUNCTION_DECL)
	if (DECL_ALIGN (olddecl) > DECL_ALIGN (newdecl))
	  {
	    DECL_ALIGN (newdecl) = DECL_ALIGN (olddecl);
	    DECL_USER_ALIGN (newdecl) |= DECL_ALIGN (olddecl);
	  }
    }

  /* Keep the old rtl since we can safely use it.  */
  COPY_DECL_RTL (olddecl, newdecl);

  /* Merge the type qualifiers.  */
  if (TREE_READONLY (newdecl))
    TREE_READONLY (olddecl) = 1;

  if (TREE_THIS_VOLATILE (newdecl))
    {
      TREE_THIS_VOLATILE (olddecl) = 1;
      if (TREE_CODE (newdecl) == VAR_DECL)
	make_var_volatile (newdecl);
    }

  /* Keep source location of definition rather than declaration.  */
  if (DECL_INITIAL (newdecl) == 0 && DECL_INITIAL (olddecl) != 0)
    DECL_SOURCE_LOCATION (newdecl) = DECL_SOURCE_LOCATION (olddecl);

  /* Merge the unused-warning information.  */
  if (DECL_IN_SYSTEM_HEADER (olddecl))
    DECL_IN_SYSTEM_HEADER (newdecl) = 1;
  else if (DECL_IN_SYSTEM_HEADER (newdecl))
    DECL_IN_SYSTEM_HEADER (olddecl) = 1;

  /* Merge the initialization information.  */
   if (DECL_INITIAL (newdecl) == 0)
    DECL_INITIAL (newdecl) = DECL_INITIAL (olddecl);

  /* Merge the section attribute.
     We want to issue an error if the sections conflict but that must be
     done later in decl_attributes since we are called before attributes
     are assigned.  */
  if (DECL_SECTION_NAME (newdecl) == NULL_TREE)
    DECL_SECTION_NAME (newdecl) = DECL_SECTION_NAME (olddecl);

  /* Copy the assembler name.
     Currently, it can only be defined in the prototype.  */
  COPY_DECL_ASSEMBLER_NAME (olddecl, newdecl);

  /* Use visibility of whichever declaration had it specified */
  if (DECL_VISIBILITY_SPECIFIED (olddecl))
    {
      DECL_VISIBILITY (newdecl) = DECL_VISIBILITY (olddecl);
      DECL_VISIBILITY_SPECIFIED (newdecl) = 1;
    }

  if (TREE_CODE (newdecl) == FUNCTION_DECL)
    {
      DECL_STATIC_CONSTRUCTOR(newdecl) |= DECL_STATIC_CONSTRUCTOR(olddecl);
      DECL_STATIC_DESTRUCTOR (newdecl) |= DECL_STATIC_DESTRUCTOR (olddecl);
      DECL_NO_LIMIT_STACK (newdecl) |= DECL_NO_LIMIT_STACK (olddecl);
      DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (newdecl)
	|= DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (olddecl);
      TREE_THIS_VOLATILE (newdecl) |= TREE_THIS_VOLATILE (olddecl);
      TREE_READONLY (newdecl) |= TREE_READONLY (olddecl);
      DECL_IS_MALLOC (newdecl) |= DECL_IS_MALLOC (olddecl);
      DECL_IS_PURE (newdecl) |= DECL_IS_PURE (olddecl);
    }

  /* Merge the storage class information.  */
  merge_weak (newdecl, olddecl);

  /* For functions, static overrides non-static.  */
  if (TREE_CODE (newdecl) == FUNCTION_DECL)
    {
      TREE_PUBLIC (newdecl) &= TREE_PUBLIC (olddecl);
      /* This is since we don't automatically
	 copy the attributes of NEWDECL into OLDDECL.  */
      TREE_PUBLIC (olddecl) = TREE_PUBLIC (newdecl);
      /* If this clears `static', clear it in the identifier too.  */
      if (! TREE_PUBLIC (olddecl))
	TREE_PUBLIC (DECL_NAME (olddecl)) = 0;
    }
  if (DECL_EXTERNAL (newdecl))
    {
      TREE_STATIC (newdecl) = TREE_STATIC (olddecl);
      DECL_EXTERNAL (newdecl) = DECL_EXTERNAL (olddecl);

      /* An extern decl does not override previous storage class.  */
      TREE_PUBLIC (newdecl) = TREE_PUBLIC (olddecl);
      if (! DECL_EXTERNAL (newdecl))
	{
	  DECL_CONTEXT (newdecl) = DECL_CONTEXT (olddecl);
	  DECL_COMMON (newdecl) = DECL_COMMON (olddecl);
	}
    }
  else
    {
      TREE_STATIC (olddecl) = TREE_STATIC (newdecl);
      TREE_PUBLIC (olddecl) = TREE_PUBLIC (newdecl);
    }

  if (TREE_CODE (newdecl) == FUNCTION_DECL)
    {
      /* If we're redefining a function previously defined as extern
	 inline, make sure we emit debug info for the inline before we
	 throw it away, in case it was inlined into a function that hasn't
	 been written out yet.  */
      if (new_is_definition && DECL_INITIAL (olddecl))
	{
	  if (TREE_USED (olddecl)
	      /* In unit-at-a-time mode we never inline re-defined extern
	         inline functions.  */
	      && !flag_unit_at_a_time
	      && cgraph_function_possibly_inlined_p (olddecl))
	    (*debug_hooks->outlining_inline_function) (olddecl);

	  /* The new defn must not be inline.  */
	  DECL_INLINE (newdecl) = 0;
	  DECL_UNINLINABLE (newdecl) = 1;
	}
      else
	{
	  /* If either decl says `inline', this fn is inline,
	     unless its definition was passed already.  */
	  if (DECL_DECLARED_INLINE_P (newdecl)
	      || DECL_DECLARED_INLINE_P (olddecl))
	    DECL_DECLARED_INLINE_P (newdecl) = 1;

	  DECL_UNINLINABLE (newdecl) = DECL_UNINLINABLE (olddecl)
	    = (DECL_UNINLINABLE (newdecl) || DECL_UNINLINABLE (olddecl));
	}

      if (DECL_BUILT_IN (olddecl))
	{
	  /* If redeclaring a builtin function, it stays built in.
	     But it gets tagged as having been declared.  */
	  DECL_BUILT_IN_CLASS (newdecl) = DECL_BUILT_IN_CLASS (olddecl);
	  DECL_FUNCTION_CODE (newdecl) = DECL_FUNCTION_CODE (olddecl);
	  C_DECL_DECLARED_BUILTIN (newdecl) = 1;
	}

      /* Also preserve various other info from the definition.  */
      if (! new_is_definition)
	{
	  DECL_RESULT (newdecl) = DECL_RESULT (olddecl);
	  DECL_INITIAL (newdecl) = DECL_INITIAL (olddecl);
	  DECL_STRUCT_FUNCTION (newdecl) = DECL_STRUCT_FUNCTION (olddecl);
	  DECL_SAVED_TREE (newdecl) = DECL_SAVED_TREE (olddecl);
	  DECL_ARGUMENTS (newdecl) = DECL_ARGUMENTS (olddecl);

	  /* Set DECL_INLINE on the declaration if we've got a body
	     from which to instantiate.  */
	  if (DECL_INLINE (olddecl) && ! DECL_UNINLINABLE (newdecl))
	    {
	      DECL_INLINE (newdecl) = 1;
	      DECL_ABSTRACT_ORIGIN (newdecl)
		= DECL_ABSTRACT_ORIGIN (olddecl);
	    }
	}
      else
	{
	  /* If a previous declaration said inline, mark the
	     definition as inlinable.  */
	  if (DECL_DECLARED_INLINE_P (newdecl)
	      && ! DECL_UNINLINABLE (newdecl))
	    DECL_INLINE (newdecl) = 1;
	}
    }

  /* Copy most of the decl-specific fields of NEWDECL into OLDDECL.
     But preserve OLDDECL's DECL_UID and DECL_CONTEXT.  */
  {
    unsigned olddecl_uid = DECL_UID (olddecl);
    tree olddecl_context = DECL_CONTEXT (olddecl);

    memcpy ((char *) olddecl + sizeof (struct tree_common),
	    (char *) newdecl + sizeof (struct tree_common),
	    sizeof (struct tree_decl) - sizeof (struct tree_common));
    DECL_UID (olddecl) = olddecl_uid;
    DECL_CONTEXT (olddecl) = olddecl_context;
  }

  /* If OLDDECL had its DECL_RTL instantiated, re-invoke make_decl_rtl
     so that encode_section_info has a chance to look at the new decl
     flags and attributes.  */
  if (DECL_RTL_SET_P (olddecl)
      && (TREE_CODE (olddecl) == FUNCTION_DECL
	  || (TREE_CODE (olddecl) == VAR_DECL
	      && TREE_STATIC (olddecl))))
    make_decl_rtl (olddecl);
}

/* Handle when a new declaration NEWDECL has the same name as an old
   one OLDDECL in the same binding contour.  Prints an error message
   if appropriate.

   If safely possible, alter OLDDECL to look like NEWDECL, and return
   true.  Otherwise, return false.  */

static bool
duplicate_decls (tree newdecl, tree olddecl)
{
  tree newtype = NULL, oldtype = NULL;

  if (!diagnose_mismatched_decls (newdecl, olddecl, &newtype, &oldtype))
    return false;

  merge_decls (newdecl, olddecl, newtype, oldtype);
  return true;
}


/* Check whether decl-node NEW_DECL shadows an existing declaration.  */
static void
warn_if_shadowing (tree new_decl)
{
  struct c_binding *b;

  /* Shadow warnings wanted?  */
  if (!warn_shadow
      /* No shadow warnings for internally generated vars.  */
      || DECL_IS_BUILTIN (new_decl)
      /* No shadow warnings for vars made for inlining.  */
      || DECL_FROM_INLINE (new_decl)
      /* Don't warn about the parm names in function declarator
	 within a function declarator.  It would be nice to avoid
	 warning in any function declarator in a declaration, as
	 opposed to a definition, but there is no way to tell
	 it's not a definition at this point.  */
      || (TREE_CODE (new_decl) == PARM_DECL && current_scope->outer->parm_flag))
    return;

  /* Is anything being shadowed?  Invisible decls do not count.  */
  for (b = I_SYMBOL_BINDING (DECL_NAME (new_decl)); b; b = b->shadowed)
    if (b->decl && b->decl != new_decl && !b->invisible)
      {
	tree old_decl = b->decl;

	if (TREE_CODE (old_decl) == PARM_DECL)
	  warning ("%Jdeclaration of '%D' shadows a parameter",
		   new_decl, new_decl);
	else if (DECL_FILE_SCOPE_P (old_decl))
	  warning ("%Jdeclaration of '%D' shadows a global declaration",
		   new_decl, new_decl);
	else if (TREE_CODE (old_decl) == FUNCTION_DECL
		 && DECL_BUILT_IN (old_decl))
	  warning ("%Jdeclaration of '%D' shadows a built-in function",
		   new_decl, new_decl);
	else
	  warning ("%Jdeclaration of '%D' shadows a previous local",
		   new_decl, new_decl);

	if (TREE_CODE (old_decl) != FUNCTION_DECL
	    || ! DECL_BUILT_IN (old_decl))
	  warning ("%Jshadowed declaration is here", old_decl);

	break;
      }
}


/* Subroutine of pushdecl.

   X is a TYPE_DECL for a typedef statement.  Create a brand new
   ..._TYPE node (which will be just a variant of the existing
   ..._TYPE node with identical properties) and then install X
   as the TYPE_NAME of this brand new (duplicate) ..._TYPE node.

   The whole point here is to end up with a situation where each
   and every ..._TYPE node the compiler creates will be uniquely
   associated with AT MOST one node representing a typedef name.
   This way, even though the compiler substitutes corresponding
   ..._TYPE nodes for TYPE_DECL (i.e. "typedef name") nodes very
   early on, later parts of the compiler can always do the reverse
   translation and get back the corresponding typedef name.  For
   example, given:

        typedef struct S MY_TYPE;
	MY_TYPE object;

   Later parts of the compiler might only know that `object' was of
   type `struct S' if it were not for code just below.  With this
   code however, later parts of the compiler see something like:

	struct S' == struct S
	typedef struct S' MY_TYPE;
	struct S' object;

    And they can then deduce (from the node for type struct S') that
    the original object declaration was:

		MY_TYPE object;

    Being able to do this is important for proper support of protoize,
    and also for generating precise symbolic debugging information
    which takes full account of the programmer's (typedef) vocabulary.

    Obviously, we don't want to generate a duplicate ..._TYPE node if
    the TYPE_DECL node that we are now processing really represents a
    standard built-in type.

    Since all standard types are effectively declared at line zero
    in the source file, we can easily check to see if we are working
    on a standard type by checking the current value of lineno.  */

static void
clone_underlying_type (tree x)
{
  if (DECL_IS_BUILTIN (x))
    {
      if (TYPE_NAME (TREE_TYPE (x)) == 0)
	TYPE_NAME (TREE_TYPE (x)) = x;
    }
  else if (TREE_TYPE (x) != error_mark_node
	   && DECL_ORIGINAL_TYPE (x) == NULL_TREE)
    {
      tree tt = TREE_TYPE (x);
      DECL_ORIGINAL_TYPE (x) = tt;
      tt = build_type_copy (tt);
      TYPE_NAME (tt) = x;
      TREE_USED (tt) = TREE_USED (x);
      TREE_TYPE (x) = tt;
    }
}

/* Record a decl-node X as belonging to the current lexical scope.
   Check for errors (such as an incompatible declaration for the same
   name already seen in the same scope).

   Returns either X or an old decl for the same name.
   If an old decl is returned, it may have been smashed
   to agree with what X says.  */

tree
pushdecl (tree x)
{
  tree name = DECL_NAME (x);
  struct c_scope *scope = current_scope;
  struct c_binding *b;
  bool nested = false;

  /* Functions need the lang_decl data.  */
  if (TREE_CODE (x) == FUNCTION_DECL && ! DECL_LANG_SPECIFIC (x))
    DECL_LANG_SPECIFIC (x) = GGC_CNEW (struct lang_decl);

  /* Must set DECL_CONTEXT for everything not at file scope or
     DECL_FILE_SCOPE_P won't work.  Local externs don't count
     unless they have initializers (which generate code).  */
  if (current_function_decl
      && ((TREE_CODE (x) != FUNCTION_DECL && TREE_CODE (x) != VAR_DECL)
	  || DECL_INITIAL (x) || !DECL_EXTERNAL (x)))
    DECL_CONTEXT (x) = current_function_decl;

  /* Anonymous decls are just inserted in the scope.  */
  if (!name)
    {
      bind (name, x, scope, /*invisible=*/false, /*nested=*/false);
      return x;
    }

  /* First, see if there is another declaration with the same name in
     the current scope.  If there is, duplicate_decls may do all the
     work for us.  If duplicate_decls returns false, that indicates
     two incompatible decls in the same scope; we are to silently
     replace the old one (duplicate_decls has issued all appropriate
     diagnostics).  In particular, we should not consider possible
     duplicates in the external scope, or shadowing.  */
  b = I_SYMBOL_BINDING (name);
  if (b && B_IN_SCOPE (b, scope))
    {
      if (duplicate_decls (x, b->decl))
	return b->decl;
      else
	goto skip_external_and_shadow_checks;
    }

  /* All declarations with external linkage, and all external
     references, go in the external scope, no matter what scope is
     current.  However, the binding in that scope is ignored for
     purposes of normal name lookup.  A separate binding structure is
     created in the requested scope; this governs the normal
     visibility of the symbol.

     The binding in the externals scope is used exclusively for
     detecting duplicate declarations of the same object, no matter
     what scope they are in; this is what we do here.  (C99 6.2.7p2:
     All declarations that refer to the same object or function shall
     have compatible type; otherwise, the behavior is undefined.)  */
  if (DECL_EXTERNAL (x) || scope == file_scope)
    {
      if (warn_nested_externs
	  && scope != file_scope
	  && !DECL_IN_SYSTEM_HEADER (x))
	warning ("nested extern declaration of '%D'", x);

      while (b && !B_IN_EXTERNAL_SCOPE (b))
	b = b->shadowed;

      /* The point of the same_translation_unit_p check here is,
	 we want to detect a duplicate decl for a construct like
	 foo() { extern bar(); } ... static bar();  but not if
	 they are in different translation units.  In any case,
	 the static does not go in the externals scope.  */
      if (b
	  && (TREE_PUBLIC (x) || same_translation_unit_p (x, b->decl))
	  && duplicate_decls (x, b->decl))
	{
	  bind (name, b->decl, scope, /*invisible=*/false, /*nested=*/true);
	  return b->decl;
	}
      else if (TREE_PUBLIC (x))
	{
	  bind (name, x, external_scope, /*invisible=*/true, /*nested=*/false);
	  nested = true;
	}
    }
  /* Similarly, a declaration of a function with static linkage at
     block scope must be checked against any existing declaration
     of that function at file scope.  */
  else if (TREE_CODE (x) == FUNCTION_DECL && scope != file_scope
	   && !TREE_PUBLIC (x) && !DECL_INITIAL (x))
    {
      if (warn_nested_externs && !DECL_IN_SYSTEM_HEADER (x))
	warning ("nested static declaration of '%D'", x);

      while (b && !B_IN_FILE_SCOPE (b))
	b = b->shadowed;

      if (b && same_translation_unit_p (x, b->decl)
	  && duplicate_decls (x, b->decl))
	{
	  bind (name, b->decl, scope, /*invisible=*/false, /*nested=*/true);
	  return b->decl;
	}
      else
	{
	  bind (name, x, file_scope, /*invisible=*/true, /*nested=*/false);
	  nested = true;
	}
    }

  warn_if_shadowing (x);

 skip_external_and_shadow_checks:
  if (TREE_CODE (x) == TYPE_DECL)
    clone_underlying_type (x);

  bind (name, x, scope, /*invisible=*/false, nested);

  /* If x's type is incomplete because it's based on a
     structure or union which has not yet been fully declared,
     attach it to that structure or union type, so we can go
     back and complete the variable declaration later, if the
     structure or union gets fully declared.

     If the input is erroneous, we can have error_mark in the type
     slot (e.g. "f(void a, ...)") - that doesn't count as an
     incomplete type.  */
  if (TREE_TYPE (x) != error_mark_node
      && !COMPLETE_TYPE_P (TREE_TYPE (x)))
    {
      tree element = TREE_TYPE (x);

      while (TREE_CODE (element) == ARRAY_TYPE)
	element = TREE_TYPE (element);
      element = TYPE_MAIN_VARIANT (element);

      if ((TREE_CODE (element) == RECORD_TYPE
	   || TREE_CODE (element) == UNION_TYPE)
	  && (TREE_CODE (x) != TYPE_DECL
	      || TREE_CODE (TREE_TYPE (x)) == ARRAY_TYPE)
	  && !COMPLETE_TYPE_P (element))
	C_TYPE_INCOMPLETE_VARS (element)
	  = tree_cons (NULL_TREE, x, C_TYPE_INCOMPLETE_VARS (element));
    }
  return x;
}

/* Record X as belonging to file scope.
   This is used only internally by the Objective-C front end,
   and is limited to its needs.  duplicate_decls is not called;
   if there is any preexisting decl for this identifier, it is an ICE.  */

tree
pushdecl_top_level (tree x)
{
  tree name;
  bool nested = false;

  if (TREE_CODE (x) != VAR_DECL)
    abort ();

  name = DECL_NAME (x);

  if (I_SYMBOL_BINDING (name))
    abort ();

  if (TREE_PUBLIC (x))
    {
      bind (name, x, external_scope, /*invisible=*/true, /*nested=*/false);
      nested = true;
    }
  if (file_scope)
    bind (name, x, file_scope, /*invisible=*/false, nested);

  return x;
}

static void
implicit_decl_warning (tree id, tree olddecl)
{
  void (*diag) (const char *, ...);
  switch (mesg_implicit_function_declaration)
    {
    case 0: return;
    case 1: diag = warning; break;
    case 2: diag = error;   break;
    default: abort ();
    }

  diag (N_("implicit declaration of function '%E'"), id);
  if (olddecl)
    locate_old_decl (olddecl, diag);
}

/* Generate an implicit declaration for identifier FUNCTIONID as a
   function of type int ().  */

tree
implicitly_declare (tree functionid)
{
  tree decl = lookup_name_in_scope (functionid, external_scope);

  if (decl)
    {
      /* FIXME: Objective-C has weird not-really-builtin functions
	 which are supposed to be visible automatically.  They wind up
	 in the external scope because they're pushed before the file
	 scope gets created.  Catch this here and rebind them into the
	 file scope.  */
      if (!DECL_BUILT_IN (decl) && DECL_IS_BUILTIN (decl))
	{
	  bind (functionid, decl, file_scope,
		/*invisible=*/false, /*nested=*/true);
	  return decl;
	}
      else
	{
	  /* Implicit declaration of a function already declared
	     (somehow) in a different scope, or as a built-in.
	     If this is the first time this has happened, warn;
	     then recycle the old declaration.  */
	  if (!C_DECL_IMPLICIT (decl))
	    {
	      implicit_decl_warning (functionid, decl);
	      C_DECL_IMPLICIT (decl) = 1;
	    }
	  /* APPLE LOCAL disable typechecking for SPEC --dbj */
	  if (DECL_BUILT_IN (decl) || disable_typechecking_for_spec_flag)
	    {
	      if (!comptypes (default_function_type, TREE_TYPE (decl)))
		{
		  warning ("incompatible implicit declaration of built-in"
			   " function %qD", decl);
		}
	    }
	  else
	    {
	      if (!comptypes (default_function_type, TREE_TYPE (decl)))
		{
		  error ("incompatible implicit declaration of function %qD",
			 decl);
		  locate_old_decl (decl, error);
		}
	    }
	  bind (functionid, decl, current_scope,
		/*invisible=*/false, /*nested=*/true);
	  return decl;
	}
    }

  /* Not seen before.  */
  decl = build_decl (FUNCTION_DECL, functionid, default_function_type);
  DECL_EXTERNAL (decl) = 1;
  TREE_PUBLIC (decl) = 1;
  C_DECL_IMPLICIT (decl) = 1;
  implicit_decl_warning (functionid, 0);

  /* C89 says implicit declarations are in the innermost block.
     So we record the decl in the standard fashion.  */
  decl = pushdecl (decl);

  /* No need to call objc_check_decl here - it's a function type.  */
  rest_of_decl_compilation (decl, 0, 0);

  /* Write a record describing this implicit function declaration
     to the prototypes file (if requested).  */
  gen_aux_info_record (decl, 0, 1, 0);

  /* Possibly apply some default attributes to this implicit declaration.  */
  decl_attributes (&decl, NULL_TREE, 0);

  return decl;
}

/* Issue an error message for a reference to an undeclared variable
   ID, including a reference to a builtin outside of function-call
   context.  Establish a binding of the identifier to error_mark_node
   in an appropriate scope, which will suppress further errors for the
   same identifier.  */
void
undeclared_variable (tree id)
{
  static bool already = false;
  struct c_scope *scope;

  if (current_function_decl == 0)
    {
      error ("'%E' undeclared here (not in a function)", id);
      scope = current_scope;
    }
  else
    {
      error ("'%E' undeclared (first use in this function)", id);

      if (! already)
	{
	  error ("(Each undeclared identifier is reported only once");
	  error ("for each function it appears in.)");
	  already = true;
	}

      /* If we are parsing old-style parameter decls, current_function_decl
         will be nonnull but current_function_scope will be null.  */
      scope = current_function_scope ? current_function_scope : current_scope;
    }
  bind (id, error_mark_node, scope, /*invisible=*/false, /*nested=*/false);
}

/* Subroutine of lookup_label, declare_label, define_label: construct a
   LABEL_DECL with all the proper frills.  */

static tree
make_label (tree name, location_t location)
{
  tree label = build_decl (LABEL_DECL, name, void_type_node);

  DECL_CONTEXT (label) = current_function_decl;
  DECL_MODE (label) = VOIDmode;
  DECL_SOURCE_LOCATION (label) = location;

  return label;
}

/* Get the LABEL_DECL corresponding to identifier NAME as a label.
   Create one if none exists so far for the current function.
   This is called when a label is used in a goto expression or
   has its address taken.  */

tree
lookup_label (tree name)
{
  tree label;

  if (current_function_decl == 0)
    {
      error ("label %s referenced outside of any function",
	     IDENTIFIER_POINTER (name));
      return 0;
    }

  /* Use a label already defined or ref'd with this name, but not if
     it is inherited from a containing function and wasn't declared
     using __label__.  */
  label = I_LABEL_DECL (name);
  if (label && (DECL_CONTEXT (label) == current_function_decl
		|| C_DECLARED_LABEL_FLAG (label)))
    {
      /* If the label has only been declared, update its apparent
	 location to point here, for better diagnostics if it
	 turns out not to have been defined.  */
      if (!TREE_USED (label))
	DECL_SOURCE_LOCATION (label) = input_location;
      return label;
    }

  /* No label binding for that identifier; make one.  */
  label = make_label (name, input_location);

  /* Ordinary labels go in the current function scope.  */
  bind (name, label, current_function_scope,
	/*invisible=*/false, /*nested=*/false);
  return label;
}

/* Make a label named NAME in the current function, shadowing silently
   any that may be inherited from containing functions or containing
   scopes.  This is called for __label__ declarations.  */

tree
declare_label (tree name)
{
  struct c_binding *b = I_LABEL_BINDING (name);
  tree label;

  /* Check to make sure that the label hasn't already been declared
     at this scope */
  if (b && B_IN_CURRENT_SCOPE (b))
    {
      error ("duplicate label declaration `%s'", IDENTIFIER_POINTER (name));
      locate_old_decl (b->decl, error);

      /* Just use the previous declaration.  */
      return b->decl;
    }

  label = make_label (name, input_location);
  C_DECLARED_LABEL_FLAG (label) = 1;

  /* Declared labels go in the current scope.  */
  bind (name, label, current_scope,
	/*invisible=*/false, /*nested=*/false);
  return label;
}

/* Define a label, specifying the location in the source file.
   Return the LABEL_DECL node for the label, if the definition is valid.
   Otherwise return 0.  */

tree
define_label (location_t location, tree name)
{
  /* Find any preexisting label with this name.  It is an error
     if that label has already been defined in this function, or
     if there is a containing function with a declared label with
     the same name.  */
  tree label = I_LABEL_DECL (name);

  if (label
      && ((DECL_CONTEXT (label) == current_function_decl
	   && DECL_INITIAL (label) != 0)
	  || (DECL_CONTEXT (label) != current_function_decl
	      && C_DECLARED_LABEL_FLAG (label))))
    {
      error ("%Hduplicate label `%D'", &location, label);
      locate_old_decl (label, error);
      return 0;
    }
  else if (label && DECL_CONTEXT (label) == current_function_decl)
    {
      /* The label has been used or declared already in this function,
	 but not defined.  Update its location to point to this
	 definition.  */
      DECL_SOURCE_LOCATION (label) = location;
    }
  else
    {
      /* No label binding for that identifier; make one.  */
      label = make_label (name, location);

      /* Ordinary labels go in the current function scope.  */
      bind (name, label, current_function_scope,
	    /*invisible=*/false, /*nested=*/false);
    }

  if (warn_traditional && !in_system_header && lookup_name (name))
    warning ("%Htraditional C lacks a separate namespace for labels, "
             "identifier `%s' conflicts", &location,
	     IDENTIFIER_POINTER (name));

  /* Mark label as having been defined.  */
  DECL_INITIAL (label) = error_mark_node;
  return label;
}

/* Given NAME, an IDENTIFIER_NODE,
   return the structure (or union or enum) definition for that name.
   If THISLEVEL_ONLY is nonzero, searches only the current_scope.
   CODE says which kind of type the caller wants;
   it is RECORD_TYPE or UNION_TYPE or ENUMERAL_TYPE.
   If the wrong kind of type is found, an error is reported.  */

static tree
lookup_tag (enum tree_code code, tree name, int thislevel_only)
{
  struct c_binding *b = I_TAG_BINDING (name);
  int thislevel = 0;

  if (!b || !b->decl)
    return 0;

  /* We only care about whether it's in this level if
     thislevel_only was set or it might be a type clash.  */
  if (thislevel_only || TREE_CODE (b->decl) != code)
    {
      /* For our purposes, a tag in the external scope is the same as
	 a tag in the file scope.  (Primarily relevant to Objective-C
	 and its builtin structure tags, which get pushed before the
	 file scope is created.)  */
      if (B_IN_CURRENT_SCOPE (b)
	  || (current_scope == file_scope && B_IN_EXTERNAL_SCOPE (b)))
	thislevel = 1;
    }

  if (thislevel_only && !thislevel)
    return 0;

  if (TREE_CODE (b->decl) != code)
    {
      /* Definition isn't the kind we were looking for.  */
      pending_invalid_xref = name;
      pending_invalid_xref_location = input_location;

      /* If in the same binding level as a declaration as a tag
	 of a different type, this must not be allowed to
	 shadow that tag, so give the error immediately.
	 (For example, "struct foo; union foo;" is invalid.)  */
      if (thislevel)
	pending_xref_error ();
    }
  return b->decl;
}

/* Print an error message now
   for a recent invalid struct, union or enum cross reference.
   We don't print them immediately because they are not invalid
   when used in the `struct foo;' construct for shadowing.  */

void
pending_xref_error (void)
{
  if (pending_invalid_xref != 0)
    error ("%H`%s' defined as wrong kind of tag",
           &pending_invalid_xref_location,
           IDENTIFIER_POINTER (pending_invalid_xref));
  pending_invalid_xref = 0;
}


/* Look up NAME in the current scope and its superiors
   in the namespace of variables, functions and typedefs.
   Return a ..._DECL node of some kind representing its definition,
   or return 0 if it is undefined.  */

tree
lookup_name (tree name)
{
  struct c_binding *b = I_SYMBOL_BINDING (name);
  if (b && !b->invisible)
    return b->decl;
  return 0;
}

/* Similar to `lookup_name' but look only at the indicated scope.  */

static tree
lookup_name_in_scope (tree name, struct c_scope *scope)
{
  struct c_binding *b;

  for (b = I_SYMBOL_BINDING (name); b; b = b->shadowed)
    if (B_IN_SCOPE (b, scope))
      return b->decl;
  return 0;
}

/* Create the predefined scalar types of C,
   and some nodes representing standard constants (0, 1, (void *) 0).
   Initialize the global scope.
   Make definitions for built-in primitive functions.  */

void
c_init_decl_processing (void)
{
  tree endlink;
  tree ptr_ftype_void, ptr_ftype_ptr;
  location_t save_loc = input_location;

  /* Adds some ggc roots, and reserved words for c-parse.in.  */
  c_parse_init ();

  current_function_decl = 0;

  /* Make the externals scope.  */
  push_scope ();
  external_scope = current_scope;

  /* Declarations from c_common_nodes_and_builtins must not be associated
     with this input file, lest we get differences between using and not
     using preprocessed headers.  */
#ifdef USE_MAPPED_LOCATION
  input_location = BUILTINS_LOCATION;
#else
  input_location.file = "<built-in>";
  input_location.line = 0;
#endif

  build_common_tree_nodes (flag_signed_char);

  c_common_nodes_and_builtins ();

  /* In C, comparisons and TRUTH_* expressions have type int.  */
  truthvalue_type_node = integer_type_node;
  truthvalue_true_node = integer_one_node;
  truthvalue_false_node = integer_zero_node;

  /* Even in C99, which has a real boolean type.  */
  pushdecl (build_decl (TYPE_DECL, get_identifier ("_Bool"),
			boolean_type_node));

  endlink = void_list_node;
  ptr_ftype_void = build_function_type (ptr_type_node, endlink);
  ptr_ftype_ptr
    = build_function_type (ptr_type_node,
			   tree_cons (NULL_TREE, ptr_type_node, endlink));

  input_location = save_loc;

  pedantic_lvalues = true;

  make_fname_decl = c_make_fname_decl;
  start_fname_decls ();

  /* APPLE LOCAL begin new tree dump */
#if 0
  /* MERGE FIXME: 3468690 */
  /* For condensed tree dumps with debugger.  */
  c_prev_lang_dump_tree_p = set_dump_tree_p (c_dump_tree_p);
  SET_MAX_DMP_TREE_CODE(LAST_C_TREE_CODE);
#endif
  /* APPLE LOCAL end new tree dump */
}

/* Create the VAR_DECL for __FUNCTION__ etc. ID is the name to give the
   decl, NAME is the initialization string and TYPE_DEP indicates whether
   NAME depended on the type of the function.  As we don't yet implement
   delayed emission of static data, we mark the decl as emitted
   so it is not placed in the output.  Anything using it must therefore pull
   out the STRING_CST initializer directly.  FIXME.  */

static tree
c_make_fname_decl (tree id, int type_dep)
{
  const char *name = fname_as_string (type_dep);
  tree decl, type, init;
  size_t length = strlen (name);

  type =  build_array_type
          (build_qualified_type (char_type_node, TYPE_QUAL_CONST),
	   build_index_type (size_int (length)));

  decl = build_decl (VAR_DECL, id, type);

  TREE_STATIC (decl) = 1;
  TREE_READONLY (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;

  init = build_string (length + 1, name);
  free ((char *) name);
  TREE_TYPE (init) = type;
  DECL_INITIAL (decl) = init;

  TREE_USED (decl) = 1;

  if (current_function_decl)
    {
      DECL_CONTEXT (decl) = current_function_decl;
      bind (id, decl, current_function_scope,
	    /*invisible=*/false, /*nested=*/false);
    }

  finish_decl (decl, init, NULL_TREE);

  return decl;
}

/* Return a definition for a builtin function named NAME and whose data type
   is TYPE.  TYPE should be a function type with argument types.
   FUNCTION_CODE tells later passes how to compile calls to this function.
   See tree.h for its possible values.

   If LIBRARY_NAME is nonzero, use that for DECL_ASSEMBLER_NAME,
   the name to be called if we can't opencode the function.  If
   ATTRS is nonzero, use that for the function's attribute list.  */

tree
builtin_function (const char *name, tree type, int function_code,
		  enum built_in_class cl, const char *library_name,
		  tree attrs)
{
  tree id = get_identifier (name);
  tree decl = build_decl (FUNCTION_DECL, id, type);
  TREE_PUBLIC (decl) = 1;
  DECL_EXTERNAL (decl) = 1;
  DECL_LANG_SPECIFIC (decl) = GGC_CNEW (struct lang_decl);
  DECL_BUILT_IN_CLASS (decl) = cl;
  DECL_FUNCTION_CODE (decl) = function_code;
  if (library_name)
    SET_DECL_ASSEMBLER_NAME (decl, get_identifier (library_name));

  /* Should never be called on a symbol with a preexisting meaning.  */
  if (I_SYMBOL_BINDING (id))
    abort ();

  bind (id, decl, external_scope, /*invisible=*/true, /*nested=*/false);

  /* Builtins in the implementation namespace are made visible without
     needing to be explicitly declared.  See push_file_scope.  */
  if (name[0] == '_' && (name[1] == '_' || ISUPPER (name[1])))
    {
      TREE_CHAIN (decl) = visible_builtins;
      visible_builtins = decl;
    }

  /* Possibly apply some default attributes to this built-in function.  */
  if (attrs)
    decl_attributes (&decl, attrs, ATTR_FLAG_BUILT_IN);
  else
    decl_attributes (&decl, NULL_TREE, 0);

  return decl;
}

/* Called when a declaration is seen that contains no names to declare.
   If its type is a reference to a structure, union or enum inherited
   from a containing scope, shadow that tag name for the current scope
   with a forward reference.
   If its type defines a new named structure or union
   or defines an enum, it is valid but we need not do anything here.
   Otherwise, it is an error.  */

void
shadow_tag (tree declspecs)
{
  shadow_tag_warned (declspecs, 0);
}

/* WARNED is 1 if we have done a pedwarn, 2 if we have done a warning,
   but no pedwarn.  */
void
shadow_tag_warned (tree declspecs, int warned)
{
  int found_tag = 0;
  tree link;
  tree specs, attrs;

  pending_invalid_xref = 0;

  /* Remove the attributes from declspecs, since they will confuse the
     following code.  */
  split_specs_attrs (declspecs, &specs, &attrs);

  for (link = specs; link; link = TREE_CHAIN (link))
    {
      tree value = TREE_VALUE (link);
      enum tree_code code = TREE_CODE (value);

      if (code == RECORD_TYPE || code == UNION_TYPE || code == ENUMERAL_TYPE)
	/* Used to test also that TYPE_SIZE (value) != 0.
	   That caused warning for `struct foo;' at top level in the file.  */
	{
	  tree name = TYPE_NAME (value);
	  tree t;

	  found_tag++;

	  if (name == 0)
	    {
	      if (warned != 1 && code != ENUMERAL_TYPE)
		/* Empty unnamed enum OK */
		{
		  pedwarn ("unnamed struct/union that defines no instances");
		  warned = 1;
		}
	    }
	  else
	    {
	      t = lookup_tag (code, name, 1);

	      if (t == 0)
		{
		  t = make_node (code);
		  pushtag (name, t);
		}
	    }
	}
      else
	{
	  if (!warned && ! in_system_header)
	    {
	      warning ("useless keyword or type name in empty declaration");
	      warned = 2;
	    }
	}
    }

  if (found_tag > 1)
    error ("two types specified in one empty declaration");

  if (warned != 1)
    {
      if (found_tag == 0)
	pedwarn ("empty declaration");
    }
}

/* Construct an array declarator.  EXPR is the expression inside [], or
   NULL_TREE.  QUALS are the type qualifiers inside the [] (to be applied
   to the pointer to which a parameter array is converted).  STATIC_P is
   true if "static" is inside the [], false otherwise.  VLA_UNSPEC_P
   is true if the array is [*], a VLA of unspecified length which is
   nevertheless a complete type (not currently implemented by GCC),
   false otherwise.  The declarator is constructed as an ARRAY_REF
   (to be decoded by grokdeclarator), whose operand 0 is what's on the
   left of the [] (filled by in set_array_declarator_inner) and operand 1
   is the expression inside; whose TREE_TYPE is the type qualifiers and
   which has TREE_STATIC set if "static" is used.  */

tree
build_array_declarator (tree expr, tree quals, bool static_p,
			bool vla_unspec_p)
{
  tree decl;
  decl = build_nt (ARRAY_REF, NULL_TREE, expr, NULL_TREE, NULL_TREE);
  TREE_TYPE (decl) = quals;
  TREE_STATIC (decl) = (static_p ? 1 : 0);
  if (pedantic && !flag_isoc99)
    {
      if (static_p || quals != NULL_TREE)
	pedwarn ("ISO C90 does not support `static' or type qualifiers in parameter array declarators");
      if (vla_unspec_p)
	pedwarn ("ISO C90 does not support `[*]' array declarators");
    }
  if (vla_unspec_p)
    warning ("GCC does not yet properly implement `[*]' array declarators");
  return decl;
}

/* Set the type of an array declarator.  DECL is the declarator, as
   constructed by build_array_declarator; TYPE is what appears on the left
   of the [] and goes in operand 0.  ABSTRACT_P is true if it is an
   abstract declarator, false otherwise; this is used to reject static and
   type qualifiers in abstract declarators, where they are not in the
   C99 grammar.  */

tree
set_array_declarator_inner (tree decl, tree type, bool abstract_p)
{
  TREE_OPERAND (decl, 0) = type;
  if (abstract_p && (TREE_TYPE (decl) != NULL_TREE || TREE_STATIC (decl)))
    error ("static or type qualifiers in abstract declarator");
  return decl;
}

/* Decode a "typename", such as "int **", returning a ..._TYPE node.  */

tree
groktypename (tree type_name)
{
  tree specs, attrs;

  if (TREE_CODE (type_name) != TREE_LIST)
    return type_name;

  split_specs_attrs (TREE_PURPOSE (type_name), &specs, &attrs);

  type_name = grokdeclarator (TREE_VALUE (type_name), specs, TYPENAME, false,
			     NULL);

  /* Apply attributes.  */
  decl_attributes (&type_name, attrs, 0);

  return type_name;
}

/* Return a PARM_DECL node for a given pair of specs and declarator.  */

tree
groktypename_in_parm_context (tree type_name)
{
  if (TREE_CODE (type_name) != TREE_LIST)
    return type_name;
  return grokdeclarator (TREE_VALUE (type_name),
			 TREE_PURPOSE (type_name),
			 PARM, false, NULL);
}

/* Decode a declarator in an ordinary declaration or data definition.
   This is called as soon as the type information and variable name
   have been parsed, before parsing the initializer if any.
   Here we create the ..._DECL node, fill in its type,
   and put it on the list of decls for the current context.
   The ..._DECL node is returned as the value.

   Exception: for arrays where the length is not specified,
   the type is left null, to be filled in by `finish_decl'.

   Function definitions do not come here; they go to start_function
   instead.  However, external and forward declarations of functions
   do go through here.  Structure field declarations are done by
   grokfield and not through here.  */

tree
start_decl (tree declarator, tree declspecs, bool initialized, tree attributes)
{
  tree decl;
  tree tem;

  /* An object declared as __attribute__((deprecated)) suppresses
     warnings of uses of other deprecated items.  */

  /* APPLE LOCAL begin "unavailable" attribute (radar 2809697) */
  /* An object declared as __attribute__((unavailable)) suppresses
     any reports of being declared with unavailable or deprecated
     items.  An object declared as __attribute__((deprecated))
     suppresses warnings of uses of other deprecated items.  */
#ifdef A_LESS_INEFFICENT_WAY /* which I really don't want to do!  */
  if (lookup_attribute ("deprecated", attributes))
    deprecated_state = DEPRECATED_SUPPRESS;
  else if (lookup_attribute ("unavailable", attributes))
    deprecated_state = DEPRECATED_UNAVAILABLE_SUPPRESS;
#else /* a more efficient way doing what lookup_attribute would do */
  tree a;

  for (a = attributes; a; a = TREE_CHAIN (a))
    {
      tree name = TREE_PURPOSE (a);
      if (TREE_CODE (name) == IDENTIFIER_NODE)
        if (is_attribute_p ("deprecated", name))
	  {
	    deprecated_state = DEPRECATED_SUPPRESS;
	    break;
	  }
        if (is_attribute_p ("unavailable", name))
	  {
	    deprecated_state = DEPRECATED_UNAVAILABLE_SUPPRESS;
	    break;
	  }
    }
#endif
  /* APPLE LOCAL end "unavailable" attribute (radar 2809697) */

  decl = grokdeclarator (declarator, declspecs,
			 NORMAL, initialized, NULL);

  deprecated_state = DEPRECATED_NORMAL;

  if (warn_main > 0 && TREE_CODE (decl) != FUNCTION_DECL
      && MAIN_NAME_P (DECL_NAME (decl)))
    warning ("%J'%D' is usually a function", decl, decl);

  if (initialized)
    /* Is it valid for this decl to have an initializer at all?
       If not, set INITIALIZED to zero, which will indirectly
       tell 'finish_decl' to ignore the initializer once it is parsed.  */
    switch (TREE_CODE (decl))
      {
      case TYPE_DECL:
	error ("typedef '%D' is initialized (use __typeof__ instead)", decl);
	initialized = 0;
	break;

      case FUNCTION_DECL:
	error ("function '%D' is initialized like a variable", decl);
	initialized = 0;
	break;

      case PARM_DECL:
	/* DECL_INITIAL in a PARM_DECL is really DECL_ARG_TYPE.  */
	error ("parameter '%D' is initialized", decl);
	initialized = 0;
	break;

      default:
	/* Don't allow initializations for incomplete types except for
	   arrays which might be completed by the initialization.  */

	/* This can happen if the array size is an undefined macro.
	   We already gave a warning, so we don't need another one.  */
	if (TREE_TYPE (decl) == error_mark_node)
	  initialized = 0;
	else if (COMPLETE_TYPE_P (TREE_TYPE (decl)))
	  {
	    /* A complete type is ok if size is fixed.  */

	    if (TREE_CODE (TYPE_SIZE (TREE_TYPE (decl))) != INTEGER_CST
		|| C_DECL_VARIABLE_SIZE (decl))
	      {
		error ("variable-sized object may not be initialized");
		initialized = 0;
	      }
	  }
	else if (TREE_CODE (TREE_TYPE (decl)) != ARRAY_TYPE)
	  {
	    error ("variable '%D' has initializer but incomplete type", decl);
	    initialized = 0;
	  }
	else if (!COMPLETE_TYPE_P (TREE_TYPE (TREE_TYPE (decl))))
	  {
	    error ("elements of array '%D' have incomplete type", decl);
	    initialized = 0;
	  }
      }

  if (initialized)
    {
      if (current_scope == file_scope)
	TREE_STATIC (decl) = 1;

      /* Tell 'pushdecl' this is an initialized decl
	 even though we don't yet have the initializer expression.
	 Also tell 'finish_decl' it may store the real initializer.  */
      DECL_INITIAL (decl) = error_mark_node;
    }

  /* If this is a function declaration, write a record describing it to the
     prototypes file (if requested).  */

  if (TREE_CODE (decl) == FUNCTION_DECL)
    gen_aux_info_record (decl, 0, 0, TYPE_ARG_TYPES (TREE_TYPE (decl)) != 0);

  /* ANSI specifies that a tentative definition which is not merged with
     a non-tentative definition behaves exactly like a definition with an
     initializer equal to zero.  (Section 3.7.2)

     -fno-common gives strict ANSI behavior, though this tends to break
     a large body of code that grew up without this rule.

     Thread-local variables are never common, since there's no entrenched
     body of code to break, and it allows more efficient variable references
     in the presence of dynamic linking.  */

  if (TREE_CODE (decl) == VAR_DECL
      && !initialized
      && TREE_PUBLIC (decl)
      && !DECL_THREAD_LOCAL (decl)
      && !flag_no_common)
    DECL_COMMON (decl) = 1;

  /* Set attributes here so if duplicate decl, will have proper attributes.  */
  decl_attributes (&decl, attributes, 0);

  if (TREE_CODE (decl) == FUNCTION_DECL
      && targetm.calls.promote_prototypes (TREE_TYPE (decl)))
    {
      tree ce = declarator;

      if (TREE_CODE (ce) == INDIRECT_REF)
	ce = TREE_OPERAND (declarator, 0);
      if (TREE_CODE (ce) == CALL_EXPR)
	{
	  tree args = TREE_PURPOSE (TREE_OPERAND (ce, 1));
	  for (; args; args = TREE_CHAIN (args))
	    {
	      tree type = TREE_TYPE (args);
	      if (type && INTEGRAL_TYPE_P (type)
		  && TYPE_PRECISION (type) < TYPE_PRECISION (integer_type_node))
		DECL_ARG_TYPE (args) = integer_type_node;
	    }
	}
    }

  if (TREE_CODE (decl) == FUNCTION_DECL
      && DECL_DECLARED_INLINE_P (decl)
      && DECL_UNINLINABLE (decl)
      && lookup_attribute ("noinline", DECL_ATTRIBUTES (decl)))
    warning ("%Jinline function '%D' given attribute noinline", decl, decl);

  /* Add this decl to the current scope.
     TEM may equal DECL or it may be a previous decl of the same name.  */
  tem = pushdecl (decl);

  if (initialized)
    DECL_EXTERNAL (tem) = 0;

  return tem;
}

/* Finish processing of a declaration;
   install its initial value.
   If the length of an array type is not known before,
   it must be determined now, from the initial value, or it is an error.  */

void
finish_decl (tree decl, tree init, tree asmspec_tree)
{
  tree type = TREE_TYPE (decl);
  int was_incomplete = (DECL_SIZE (decl) == 0);
  const char *asmspec = 0;

  /* If a name was specified, get the string.  */
  if (current_scope == file_scope)
    asmspec_tree = maybe_apply_renaming_pragma (decl, asmspec_tree);
  if (asmspec_tree)
    asmspec = TREE_STRING_POINTER (asmspec_tree);

  /* If `start_decl' didn't like having an initialization, ignore it now.  */
  if (init != 0 && DECL_INITIAL (decl) == 0)
    init = 0;

  /* Don't crash if parm is initialized.  */
  if (TREE_CODE (decl) == PARM_DECL)
    init = 0;

  if (init)
    store_init_value (decl, init);

  if (c_dialect_objc () && (TREE_CODE (decl) == VAR_DECL
			    || TREE_CODE (decl) == FUNCTION_DECL
			    || TREE_CODE (decl) == FIELD_DECL))
    objc_check_decl (decl);

  /* Deduce size of array from initialization, if not already known.  */
  if (TREE_CODE (type) == ARRAY_TYPE
      && TYPE_DOMAIN (type) == 0
      && TREE_CODE (decl) != TYPE_DECL)
    {
      int do_default
	= (TREE_STATIC (decl)
	   /* Even if pedantic, an external linkage array
	      may have incomplete type at first.  */
	   ? pedantic && !TREE_PUBLIC (decl)
	   : !DECL_EXTERNAL (decl));
      int failure
	= complete_array_type (type, DECL_INITIAL (decl), do_default);

      /* Get the completed type made by complete_array_type.  */
      type = TREE_TYPE (decl);

      if (failure == 1)
	error ("%Jinitializer fails to determine size of '%D'", decl, decl);

      else if (failure == 2)
	{
	  if (do_default)
	    error ("%Jarray size missing in '%D'", decl, decl);
	  /* If a `static' var's size isn't known,
	     make it extern as well as static, so it does not get
	     allocated.
	     If it is not `static', then do not mark extern;
	     finish_incomplete_decl will give it a default size
	     and it will get allocated.  */
	  else if (!pedantic && TREE_STATIC (decl) && ! TREE_PUBLIC (decl))
	    DECL_EXTERNAL (decl) = 1;
	}

      /* TYPE_MAX_VALUE is always one less than the number of elements
	 in the array, because we start counting at zero.  Therefore,
	 warn only if the value is less than zero.  */
      else if (pedantic && TYPE_DOMAIN (type) != 0
	       && tree_int_cst_sgn (TYPE_MAX_VALUE (TYPE_DOMAIN (type))) < 0)
	error ("%Jzero or negative size array '%D'", decl, decl);

      layout_decl (decl, 0);
    }

  if (TREE_CODE (decl) == VAR_DECL)
    {
      if (DECL_SIZE (decl) == 0 && TREE_TYPE (decl) != error_mark_node
	  && COMPLETE_TYPE_P (TREE_TYPE (decl)))
	layout_decl (decl, 0);

      if (DECL_SIZE (decl) == 0
	  /* Don't give an error if we already gave one earlier.  */
	  && TREE_TYPE (decl) != error_mark_node
	  && (TREE_STATIC (decl)
	      /* A static variable with an incomplete type
		 is an error if it is initialized.
		 Also if it is not file scope.
		 Otherwise, let it through, but if it is not `extern'
		 then it may cause an error message later.  */
	      ? (DECL_INITIAL (decl) != 0
		 || !DECL_FILE_SCOPE_P (decl))
	      /* An automatic variable with an incomplete type
		 is an error.  */
	      : !DECL_EXTERNAL (decl)))
	 {
	   error ("%Jstorage size of '%D' isn't known", decl, decl);
	   TREE_TYPE (decl) = error_mark_node;
	 }

      if ((DECL_EXTERNAL (decl) || TREE_STATIC (decl))
	  && DECL_SIZE (decl) != 0)
	{
	  if (TREE_CODE (DECL_SIZE (decl)) == INTEGER_CST)
	    constant_expression_warning (DECL_SIZE (decl));
	  else
	    error ("%Jstorage size of '%D' isn't constant", decl, decl);
	}

      if (TREE_USED (type))
	TREE_USED (decl) = 1;
    }

  /* If this is a function and an assembler name is specified, reset DECL_RTL
     so we can give it its new name.  Also, update built_in_decls if it
     was a normal built-in.  */
  if (TREE_CODE (decl) == FUNCTION_DECL && asmspec)
    {
      if (DECL_BUILT_IN_CLASS (decl) == BUILT_IN_NORMAL)
	{
	  tree builtin = built_in_decls [DECL_FUNCTION_CODE (decl)];
	  set_user_assembler_name (builtin, asmspec);
	   if (DECL_FUNCTION_CODE (decl) == BUILT_IN_MEMCPY)
	     init_block_move_fn (asmspec);
	   else if (DECL_FUNCTION_CODE (decl) == BUILT_IN_MEMSET)
	     init_block_clear_fn (asmspec);
	 }
      set_user_assembler_name (decl, asmspec);
    }

  /* If #pragma weak was used, mark the decl weak now.  */
  if (current_scope == file_scope)
    maybe_apply_pragma_weak (decl);

  /* If this is a variable definition, determine its ELF visibility.  */
  if (TREE_CODE (decl) == VAR_DECL 
      && TREE_STATIC (decl) 
      && !DECL_EXTERNAL (decl))
    c_determine_visibility (decl);

  /* Output the assembler code and/or RTL code for variables and functions,
     unless the type is an undefined structure or union.
     If not, it will get done when the type is completed.  */

  if (TREE_CODE (decl) == VAR_DECL || TREE_CODE (decl) == FUNCTION_DECL)
    {
      /* This is a no-op in c-lang.c or something real in objc-act.c.  */
      if (c_dialect_objc ())
	objc_check_decl (decl);

      if (asmspec) 
	{
	  /* If this is not a static variable, issue a warning.
	     It doesn't make any sense to give an ASMSPEC for an
	     ordinary, non-register local variable.  Historically,
	     GCC has accepted -- but ignored -- the ASMSPEC in
	     this case.  */
	  if (! DECL_FILE_SCOPE_P (decl)
	      && TREE_CODE (decl) == VAR_DECL
	      && !C_DECL_REGISTER (decl)
	      && !TREE_STATIC (decl))
	    warning ("%Jignoring asm-specifier for non-static local "
		     "variable '%D'", decl, decl);
	  else if (C_DECL_REGISTER (decl))
	    change_decl_assembler_name (decl, get_identifier (asmspec));
	  else
	    set_user_assembler_name (decl, asmspec);
	}
      
      if (DECL_FILE_SCOPE_P (decl))
	{
	  if (DECL_INITIAL (decl) == NULL_TREE
	      || DECL_INITIAL (decl) == error_mark_node)
	    /* Don't output anything
	       when a tentative file-scope definition is seen.
	       But at end of compilation, do output code for them.  */
	    DECL_DEFER_OUTPUT (decl) = 1;
	  rest_of_decl_compilation (decl, true, 0);
	}
      else
	{
	  /* In conjunction with an ASMSPEC, the `register'
	     keyword indicates that we should place the variable
	     in a particular register.  */
	  if (asmspec && C_DECL_REGISTER (decl))
	    {
	      DECL_HARD_REGISTER (decl) = 1;
	      /* This cannot be done for a structure with volatile
		 fields, on which DECL_REGISTER will have been
		 reset.  */
	      if (!DECL_REGISTER (decl))
		error ("cannot put object with volatile field into register");
	    }

	  if (TREE_CODE (decl) != FUNCTION_DECL)
	    {
	      /* If we're building a variable sized type, and we might be
		 reachable other than via the top of the current binding
		 level, then create a new BIND_EXPR so that we deallocate
		 the object at the right time.  */
	      /* Note that DECL_SIZE can be null due to errors.  */
	      if (DECL_SIZE (decl)
		  && !TREE_CONSTANT (DECL_SIZE (decl))
		  && STATEMENT_LIST_HAS_LABEL (cur_stmt_list))
		{
		  tree bind;
		  bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
		  TREE_SIDE_EFFECTS (bind) = 1;
		  add_stmt (bind);
		  BIND_EXPR_BODY (bind) = push_stmt_list ();
		}
	      add_stmt (build_stmt (DECL_EXPR, decl));
	    }
	}
  

      if (!DECL_FILE_SCOPE_P (decl))
	{
	  /* Recompute the RTL of a local array now
	     if it used to be an incomplete type.  */
	  if (was_incomplete
	      && ! TREE_STATIC (decl) && ! DECL_EXTERNAL (decl))
	    {
	      /* If we used it already as memory, it must stay in memory.  */
	      TREE_ADDRESSABLE (decl) = TREE_USED (decl);
	      /* If it's still incomplete now, no init will save it.  */
	      if (DECL_SIZE (decl) == 0)
		DECL_INITIAL (decl) = 0;
	    }
	}
    }

  /* If this was marked 'used', be sure it will be output.  */
  if (lookup_attribute ("used", DECL_ATTRIBUTES (decl)))
    mark_decl_referenced (decl);

  if (TREE_CODE (decl) == TYPE_DECL)
    {
      if (!DECL_FILE_SCOPE_P (decl)
	  && variably_modified_type_p (TREE_TYPE (decl), NULL_TREE))
	add_stmt (build_stmt (DECL_EXPR, decl));

      rest_of_decl_compilation (decl, DECL_FILE_SCOPE_P (decl), 0);
    }

  /* At the end of a declaration, throw away any variable type sizes
     of types defined inside that declaration.  There is no use
     computing them in the following function definition.  */
  if (current_scope == file_scope)
    get_pending_sizes ();

  /* Install a cleanup (aka destructor) if one was given.  */
  if (TREE_CODE (decl) == VAR_DECL && !TREE_STATIC (decl))
    {
      tree attr = lookup_attribute ("cleanup", DECL_ATTRIBUTES (decl));
      if (attr)
	{
	  tree cleanup_id = TREE_VALUE (TREE_VALUE (attr));
	  tree cleanup_decl = lookup_name (cleanup_id);
	  tree cleanup;

	  /* Build "cleanup(&decl)" for the destructor.  */
	  cleanup = build_unary_op (ADDR_EXPR, decl, 0);
	  cleanup = build_tree_list (NULL_TREE, cleanup);
	  cleanup = build_function_call (cleanup_decl, cleanup);

	  /* Don't warn about decl unused; the cleanup uses it.  */
	  TREE_USED (decl) = 1;
	  TREE_USED (cleanup_decl) = 1;

	  /* Initialize EH, if we've been told to do so.  */
	  if (flag_exceptions && !c_eh_initialized_p)
	    {
	      c_eh_initialized_p = true;
	      eh_personality_libfunc
		= init_one_libfunc (USING_SJLJ_EXCEPTIONS
				    ? "__gcc_personality_sj0"
				    : "__gcc_personality_v0");
	      using_eh_for_cleanups ();
	    }

	  push_cleanup (decl, cleanup, false);
	}
    }
}

/* Given a parsed parameter declaration, decode it into a PARM_DECL
   and push that on the current scope.  */

void
push_parm_decl (tree parm)
{
  tree decl;

  decl = grokdeclarator (TREE_VALUE (TREE_PURPOSE (parm)),
			 TREE_PURPOSE (TREE_PURPOSE (parm)),
			 PARM, false, NULL);
  decl_attributes (&decl, TREE_VALUE (parm), 0);

  decl = pushdecl (decl);

  finish_decl (decl, NULL_TREE, NULL_TREE);
}

/* Mark all the parameter declarations to date as forward decls.
   Also diagnose use of this extension.  */

void
mark_forward_parm_decls (void)
{
  struct c_binding *b;

  if (pedantic && !current_scope->warned_forward_parm_decls)
    {
      pedwarn ("ISO C forbids forward parameter declarations");
      current_scope->warned_forward_parm_decls = true;
    }

  for (b = current_scope->bindings; b; b = b->prev)
    if (TREE_CODE (b->decl) == PARM_DECL)
      TREE_ASM_WRITTEN (b->decl) = 1;
}

static GTY(()) int compound_literal_number;

/* Build a COMPOUND_LITERAL_EXPR.  TYPE is the type given in the compound
   literal, which may be an incomplete array type completed by the
   initializer; INIT is a CONSTRUCTOR that initializes the compound
   literal.  */

tree
build_compound_literal (tree type, tree init)
{
  /* We do not use start_decl here because we have a type, not a declarator;
     and do not use finish_decl because the decl should be stored inside
     the COMPOUND_LITERAL_EXPR rather than added elsewhere as a DECL_EXPR.  */
  tree decl = build_decl (VAR_DECL, NULL_TREE, type);
  tree complit;
  tree stmt;
  DECL_EXTERNAL (decl) = 0;
  TREE_PUBLIC (decl) = 0;
  TREE_STATIC (decl) = (current_scope == file_scope);
  DECL_CONTEXT (decl) = current_function_decl;
  TREE_USED (decl) = 1;
  TREE_TYPE (decl) = type;
  TREE_READONLY (decl) = TYPE_READONLY (type);
  store_init_value (decl, init);

  if (TREE_CODE (type) == ARRAY_TYPE && !COMPLETE_TYPE_P (type))
    {
      int failure = complete_array_type (type, DECL_INITIAL (decl), 1);
      if (failure)
	abort ();
    }

  type = TREE_TYPE (decl);
  if (type == error_mark_node || !COMPLETE_TYPE_P (type))
    return error_mark_node;

  stmt = build_stmt (DECL_EXPR, decl);
  complit = build1 (COMPOUND_LITERAL_EXPR, TREE_TYPE (decl), stmt);
  TREE_SIDE_EFFECTS (complit) = 1;

  layout_decl (decl, 0);

  if (TREE_STATIC (decl))
    {
      /* This decl needs a name for the assembler output.  We also need
	 a unique suffix to be added to the name.  */
      char *name;

      ASM_FORMAT_PRIVATE_NAME (name, "__compound_literal",
			       compound_literal_number);
      compound_literal_number++;
      DECL_NAME (decl) = get_identifier (name);
      DECL_DEFER_OUTPUT (decl) = 1;
      DECL_COMDAT (decl) = 1;
      DECL_ARTIFICIAL (decl) = 1;
      pushdecl (decl);
      rest_of_decl_compilation (decl, 1, 0);
    }

  return complit;
}

/* Make TYPE a complete type based on INITIAL_VALUE.
   Return 0 if successful, 1 if INITIAL_VALUE can't be deciphered,
   2 if there was no information (in which case assume 1 if DO_DEFAULT).  */

int
complete_array_type (tree type, tree initial_value, int do_default)
{
  tree maxindex = NULL_TREE;
  int value = 0;

  if (initial_value)
    {
      /* Note MAXINDEX  is really the maximum index,
	 one less than the size.  */
      if (TREE_CODE (initial_value) == STRING_CST)
	{
	  int eltsize
	    = int_size_in_bytes (TREE_TYPE (TREE_TYPE (initial_value)));
	  maxindex = build_int_cst (NULL_TREE,
				    (TREE_STRING_LENGTH (initial_value)
				     / eltsize) - 1, 0);
	}
      else if (TREE_CODE (initial_value) == CONSTRUCTOR)
	{
	  tree elts = CONSTRUCTOR_ELTS (initial_value);
	  maxindex = build_int_cst (NULL_TREE, -1, -1);
	  for (; elts; elts = TREE_CHAIN (elts))
	    {
	      if (TREE_PURPOSE (elts))
		maxindex = TREE_PURPOSE (elts);
	      else
		maxindex = fold (build2 (PLUS_EXPR, integer_type_node,
					 maxindex, integer_one_node));
	    }
	}
      else
	{
	  /* Make an error message unless that happened already.  */
	  if (initial_value != error_mark_node)
	    value = 1;

	  /* Prevent further error messages.  */
	  maxindex = build_int_cst (NULL_TREE, 0, 0);
	}
    }

  if (!maxindex)
    {
      if (do_default)
	maxindex = build_int_cst (NULL_TREE, 0, 0);
      value = 2;
    }

  if (maxindex)
    {
      TYPE_DOMAIN (type) = build_index_type (maxindex);
      if (!TREE_TYPE (maxindex))
	abort ();
    }

  /* Lay out the type now that we can get the real answer.  */

  layout_type (type);

  return value;
}

/* Determine whether TYPE is a structure with a flexible array member,
   or a union containing such a structure (possibly recursively).  */

static bool
flexible_array_type_p (tree type)
{
  tree x;
  switch (TREE_CODE (type))
    {
    case RECORD_TYPE:
      x = TYPE_FIELDS (type);
      if (x == NULL_TREE)
	return false;
      while (TREE_CHAIN (x) != NULL_TREE)
	x = TREE_CHAIN (x);
      if (TREE_CODE (TREE_TYPE (x)) == ARRAY_TYPE
	  && TYPE_SIZE (TREE_TYPE (x)) == NULL_TREE
	  && TYPE_DOMAIN (TREE_TYPE (x)) != NULL_TREE
	  && TYPE_MAX_VALUE (TYPE_DOMAIN (TREE_TYPE (x))) == NULL_TREE)
	return true;
      return false;
    case UNION_TYPE:
      for (x = TYPE_FIELDS (type); x != NULL_TREE; x = TREE_CHAIN (x))
	{
	  if (flexible_array_type_p (TREE_TYPE (x)))
	    return true;
	}
      return false;
    default:
    return false;
  }
}

/* Performs sanity checks on the TYPE and WIDTH of the bit-field NAME,
   replacing with appropriate values if they are invalid.  */
static void
check_bitfield_type_and_width (tree *type, tree *width, const char *orig_name)
{
  tree type_mv;
  unsigned int max_width;
  unsigned HOST_WIDE_INT w;
  const char *name = orig_name ? orig_name: _("<anonymous>");

  /* Necessary?  */
  STRIP_NOPS (*width);

  /* Detect and ignore out of range field width and process valid
     field widths.  */
  if (TREE_CODE (*width) != INTEGER_CST)
    {
      error ("bit-field `%s' width not an integer constant", name);
      *width = integer_one_node;
    }
  else
    {
      constant_expression_warning (*width);
      if (tree_int_cst_sgn (*width) < 0)
	{
	  error ("negative width in bit-field `%s'", name);
	  *width = integer_one_node;
	}
      else if (integer_zerop (*width) && orig_name)
	{
	  error ("zero width for bit-field `%s'", name);
	  *width = integer_one_node;
	}
    }

  /* Detect invalid bit-field type.  */
  if (TREE_CODE (*type) != INTEGER_TYPE
      && TREE_CODE (*type) != BOOLEAN_TYPE
      && TREE_CODE (*type) != ENUMERAL_TYPE)
    {
      error ("bit-field `%s' has invalid type", name);
      *type = unsigned_type_node;
    }

  type_mv = TYPE_MAIN_VARIANT (*type);
  if (pedantic
      && type_mv != integer_type_node
      && type_mv != unsigned_type_node
      && type_mv != boolean_type_node)
    pedwarn ("type of bit-field `%s' is a GCC extension", name);

  if (type_mv == boolean_type_node)
    max_width = CHAR_TYPE_SIZE;
  else
    max_width = TYPE_PRECISION (*type);

  if (0 < compare_tree_int (*width, max_width))
    {
      error ("width of `%s' exceeds its type", name);
      w = max_width;
      *width = build_int_cst (NULL_TREE, w, 0);
    }
  else
    w = tree_low_cst (*width, 1);

  if (TREE_CODE (*type) == ENUMERAL_TYPE)
    {
      struct lang_type *lt = TYPE_LANG_SPECIFIC (*type);
      if (!lt
          || w < min_precision (lt->enum_min, TYPE_UNSIGNED (*type))
	  || w < min_precision (lt->enum_max, TYPE_UNSIGNED (*type)))
	warning ("`%s' is narrower than values of its type", name);
    }
}

/* Given declspecs and a declarator,
   determine the name and type of the object declared
   and construct a ..._DECL node for it.
   (In one case we can return a ..._TYPE node instead.
    For invalid input we sometimes return 0.)

   DECLSPECS is a chain of tree_list nodes whose value fields
    are the storage classes and type specifiers.

   DECL_CONTEXT says which syntactic context this declaration is in:
     NORMAL for most contexts.  Make a VAR_DECL or FUNCTION_DECL or TYPE_DECL.
     FUNCDEF for a function definition.  Like NORMAL but a few different
      error messages in each case.  Return value may be zero meaning
      this definition is too screwy to try to parse.
     PARM for a parameter declaration (either within a function prototype
      or before a function body).  Make a PARM_DECL, or return void_type_node.
     TYPENAME if for a typename (in a cast or sizeof).
      Don't make a DECL node; just return the ..._TYPE node.
     FIELD for a struct or union field; make a FIELD_DECL.
   INITIALIZED is true if the decl has an initializer.
   WIDTH is non-NULL for bit-fields, and is a pointer to an INTEGER_CST node
   representing the width of the bit-field.

   In the TYPENAME case, DECLARATOR is really an absolute declarator.
   It may also be so in the PARM case, for a prototype where the
   argument type is specified but not the name.

   This function is where the complicated C meanings of `static'
   and `extern' are interpreted.  */

static tree
grokdeclarator (tree declarator, tree declspecs,
		enum decl_context decl_context, bool initialized, tree *width)
{
  int specbits = 0;
  /* APPLE LOCAL CW asm blocks */
  int cw_asm_specbit = 0;
  tree spec;
  tree type = NULL_TREE;
  int longlong = 0;
  int constp;
  int restrictp;
  int volatilep;
  int type_quals = TYPE_UNQUALIFIED;
  int inlinep;
  int explicit_int = 0;
  int explicit_char = 0;
  int defaulted_int = 0;
  tree typedef_decl = 0;
  const char *name, *orig_name;
  tree typedef_type = 0;
  int funcdef_flag = 0;
  enum tree_code innermost_code = ERROR_MARK;
  int size_varies = 0;
  tree decl_attr = NULL_TREE;
  tree array_ptr_quals = NULL_TREE;
  int array_parm_static = 0;
  tree returned_attrs = NULL_TREE;
  bool bitfield = width != NULL;
  tree element_type;
  tree arg_info = NULL_TREE;

  if (decl_context == FUNCDEF)
    funcdef_flag = 1, decl_context = NORMAL;

  /* Look inside a declarator for the name being declared
     and get it as a string, for an error message.  */
  {
    tree decl = declarator;
    name = 0;

    while (decl)
      switch (TREE_CODE (decl))
	{
	case ARRAY_REF:
	case INDIRECT_REF:
	case CALL_EXPR:
	  innermost_code = TREE_CODE (decl);
	  decl = TREE_OPERAND (decl, 0);
	  break;

	case TREE_LIST:
	  decl = TREE_VALUE (decl);
	  break;

	case IDENTIFIER_NODE:
	  name = IDENTIFIER_POINTER (decl);
	  decl = 0;
	  break;

	default:
	  abort ();
	}
    orig_name = name;
    if (name == 0)
      name = "type name";
  }

  /* A function definition's declarator must have the form of
     a function declarator.  */

  if (funcdef_flag && innermost_code != CALL_EXPR)
    return 0;

  /* If this looks like a function definition, make it one,
     even if it occurs where parms are expected.
     Then store_parm_decls will reject it and not use it as a parm.  */
  if (decl_context == NORMAL && !funcdef_flag && current_scope->parm_flag)
    decl_context = PARM;

  /* Look through the decl specs and record which ones appear.
     Some typespecs are defined as built-in typenames.
     Others, the ones that are modifiers of other types,
     are represented by bits in SPECBITS: set the bits for
     the modifiers that appear.  Storage class keywords are also in SPECBITS.

     If there is a typedef name or a type, store the type in TYPE.
     This includes builtin typedefs such as `int'.

     Set EXPLICIT_INT or EXPLICIT_CHAR if the type is `int' or `char'
     and did not come from a user typedef.

     Set LONGLONG if `long' is mentioned twice.  */

  for (spec = declspecs; spec; spec = TREE_CHAIN (spec))
    {
      tree id = TREE_VALUE (spec);

      /* APPLE LOCAL begin "unavailable" attribute (radar 2809697) */
      /* If the entire declaration is itself tagged as unavailable then
         suppress reports of unavailable/deprecated items.  If the
         entire declaration is tagged as only deprecated we still
         report unavailable uses.  */
      if (id && TREE_DEPRECATED (id))
        {
          if (TREE_UNAVAILABLE (id))
            {
              if (deprecated_state != DEPRECATED_UNAVAILABLE_SUPPRESS)
              	warn_unavailable_use (id);
            }
          else 
            {
              if (deprecated_state != DEPRECATED_SUPPRESS
                  && deprecated_state != DEPRECATED_UNAVAILABLE_SUPPRESS)
              	warn_deprecated_use (id);
           }
        }
      /* APPLE LOCAL end "unavailable" attribute (radar 2809697) */

      if (id == ridpointers[(int) RID_INT])
	explicit_int = 1;
      if (id == ridpointers[(int) RID_CHAR])
	explicit_char = 1;

      if (TREE_CODE (id) == IDENTIFIER_NODE && C_IS_RESERVED_WORD (id))
	{
	  enum rid i = C_RID_CODE (id);
	  /* APPLE LOCAL begin CW asm blocks */
	  /* Maybe remember that we saw an "asm".  Don't test
	     -fasm-blocks here because we want to be able to report an
	     error later.  */
	  if (i == RID_ASM)
	    {
	      ++cw_asm_specbit;
	      goto found;
	    }
	  /* APPLE LOCAL end CW asm blocks */
	  if ((int) i <= (int) RID_LAST_MODIFIER)
	    {
	      if (i == RID_LONG && (specbits & (1 << (int) RID_LONG)))
		{
		  if (longlong)
		    error ("`long long long' is too long for GCC");
		  else
		    {
		      if (pedantic && !flag_isoc99 && ! in_system_header
			  && warn_long_long)
			pedwarn ("ISO C90 does not support `long long'");
		      longlong = 1;
		    }
		}
	      else if (specbits & (1 << (int) i))
		{
		  if (i == RID_CONST || i == RID_VOLATILE || i == RID_RESTRICT)
		    {
		      if (pedantic && !flag_isoc99)
			pedwarn ("duplicate `%s'", IDENTIFIER_POINTER (id));
		    }
		  else
		    error ("duplicate `%s'", IDENTIFIER_POINTER (id));
		}

	      /* Diagnose "__thread extern".  Recall that this list
		 is in the reverse order seen in the text.  */
	      if (i == RID_THREAD
		  && (specbits & (1 << (int) RID_EXTERN
				  | 1 << (int) RID_STATIC)))
		{
		  if (specbits & 1 << (int) RID_EXTERN)
		    error ("`__thread' before `extern'");
		  else
		    error ("`__thread' before `static'");
		}

	      specbits |= 1 << (int) i;
	      goto found;
	    }
	}
      if (type)
	error ("two or more data types in declaration of `%s'", name);
      /* Actual typedefs come to us as TYPE_DECL nodes.  */
      else if (TREE_CODE (id) == TYPE_DECL)
	{
	  if (TREE_TYPE (id) == error_mark_node)
	    ; /* Allow the type to default to int to avoid cascading errors.  */
	  else
	    {
	      type = TREE_TYPE (id);
	      decl_attr = DECL_ATTRIBUTES (id);
	      typedef_decl = id;
	    }
	}
      /* Built-in types come as identifiers.  */
      else if (TREE_CODE (id) == IDENTIFIER_NODE)
	{
	  tree t = lookup_name (id);
	   if (!t || TREE_CODE (t) != TYPE_DECL)
	    error ("`%s' fails to be a typedef or built in type",
		   IDENTIFIER_POINTER (id));
	   else if (TREE_TYPE (t) == error_mark_node)
	    ;
	  else
	    {
	      type = TREE_TYPE (t);
	      typedef_decl = t;
	    }
	}
      else if (TREE_CODE (id) != ERROR_MARK)
	type = id;

    found:
      ;
    }

  typedef_type = type;
  if (type)
    size_varies = C_TYPE_VARIABLE_SIZE (type);

  /* No type at all: default to `int', and set DEFAULTED_INT
     because it was not a user-defined typedef.  */

  if (type == 0)
    {
      if ((! (specbits & ((1 << (int) RID_LONG) | (1 << (int) RID_SHORT)
			  | (1 << (int) RID_SIGNED)
			  | (1 << (int) RID_UNSIGNED)
			  | (1 << (int) RID_COMPLEX))))
	  /* Don't warn about typedef foo = bar.  */
	  && ! (specbits & (1 << (int) RID_TYPEDEF) && initialized)
	  && ! in_system_header)
	{
	  /* Issue a warning if this is an ISO C 99 program or if -Wreturn-type
	     and this is a function, or if -Wimplicit; prefer the former
	     warning since it is more explicit.  */
	  if ((warn_implicit_int || warn_return_type || flag_isoc99)
	      && funcdef_flag)
	    warn_about_return_type = 1;
	  else if (warn_implicit_int || flag_isoc99)
	    pedwarn_c99 ("type defaults to `int' in declaration of `%s'",
			 name);
	}

      defaulted_int = 1;
      type = integer_type_node;
    }

  /* Now process the modifiers that were specified
     and check for invalid combinations.  */

  /* Long double is a special combination.  */

  if ((specbits & 1 << (int) RID_LONG) && ! longlong
      && TYPE_MAIN_VARIANT (type) == double_type_node)
    {
      specbits &= ~(1 << (int) RID_LONG);
      type = long_double_type_node;
    }

  /* Check all other uses of type modifiers.  */

  if (specbits & ((1 << (int) RID_LONG) | (1 << (int) RID_SHORT)
		  | (1 << (int) RID_UNSIGNED) | (1 << (int) RID_SIGNED)))
    {
      int ok = 0;

      if ((specbits & 1 << (int) RID_LONG)
	  && (specbits & 1 << (int) RID_SHORT))
	error ("both long and short specified for `%s'", name);
      else if (((specbits & 1 << (int) RID_LONG)
		|| (specbits & 1 << (int) RID_SHORT))
	       && explicit_char)
	error ("long or short specified with char for `%s'", name);
      else if (((specbits & 1 << (int) RID_LONG)
		|| (specbits & 1 << (int) RID_SHORT))
	       && TREE_CODE (type) == REAL_TYPE)
	{
	  static int already = 0;

	  error ("long or short specified with floating type for `%s'", name);
	  if (! already && ! pedantic)
	    {
	      error ("the only valid combination is `long double'");
	      already = 1;
	    }
	}
      else if ((specbits & 1 << (int) RID_SIGNED)
	       && (specbits & 1 << (int) RID_UNSIGNED))
	error ("both signed and unsigned specified for `%s'", name);
      else if (TREE_CODE (type) != INTEGER_TYPE)
	error ("long, short, signed or unsigned invalid for `%s'", name);
      else
	{
	  ok = 1;
	  if (!explicit_int && !defaulted_int && !explicit_char)
	    {
	      error ("long, short, signed or unsigned used invalidly for `%s'",
		     name);
	      ok = 0;
	    }
	}

      /* Discard the type modifiers if they are invalid.  */
      if (! ok)
	{
	  specbits &= ~((1 << (int) RID_LONG) | (1 << (int) RID_SHORT)
			| (1 << (int) RID_UNSIGNED) | (1 << (int) RID_SIGNED));
	  longlong = 0;
	}
    }

  if ((specbits & (1 << (int) RID_COMPLEX))
      && TREE_CODE (type) != INTEGER_TYPE && TREE_CODE (type) != REAL_TYPE)
    {
      error ("complex invalid for `%s'", name);
      specbits &= ~(1 << (int) RID_COMPLEX);
    }

  /* Decide whether an integer type is signed or not.
     Optionally treat bit-fields as signed by default.  */
  if (specbits & 1 << (int) RID_UNSIGNED
      || (bitfield && ! flag_signed_bitfields
	  && (explicit_int || defaulted_int || explicit_char
	      /* A typedef for plain `int' without `signed'
		 can be controlled just like plain `int'.  */
	      || ! (typedef_decl != 0
		    && C_TYPEDEF_EXPLICITLY_SIGNED (typedef_decl)))
	  && TREE_CODE (type) != ENUMERAL_TYPE
	  && !(specbits & 1 << (int) RID_SIGNED)))
    {
      if (longlong)
	type = long_long_unsigned_type_node;
      else if (specbits & 1 << (int) RID_LONG)
	type = long_unsigned_type_node;
      else if (specbits & 1 << (int) RID_SHORT)
	type = short_unsigned_type_node;
      else if (type == char_type_node)
	type = unsigned_char_type_node;
      else if (typedef_decl)
	type = c_common_unsigned_type (type);
      else
	type = unsigned_type_node;
    }
  else if ((specbits & 1 << (int) RID_SIGNED)
	   && type == char_type_node)
    type = signed_char_type_node;
  else if (longlong)
    type = long_long_integer_type_node;
  else if (specbits & 1 << (int) RID_LONG)
    type = long_integer_type_node;
  else if (specbits & 1 << (int) RID_SHORT)
    type = short_integer_type_node;

  if (specbits & 1 << (int) RID_COMPLEX)
    {
      if (pedantic && !flag_isoc99)
	pedwarn ("ISO C90 does not support complex types");
      /* If we just have "complex", it is equivalent to
	 "complex double", but if any modifiers at all are specified it is
	 the complex form of TYPE.  E.g, "complex short" is
	 "complex short int".  */

      if (defaulted_int && ! longlong
	  && ! (specbits & ((1 << (int) RID_LONG) | (1 << (int) RID_SHORT)
			    | (1 << (int) RID_SIGNED)
			    | (1 << (int) RID_UNSIGNED))))
	{
	  if (pedantic)
	    pedwarn ("ISO C does not support plain `complex' meaning `double complex'");
	  type = complex_double_type_node;
	}
      else if (type == integer_type_node)
	{
	  if (pedantic)
	    pedwarn ("ISO C does not support complex integer types");
	  type = complex_integer_type_node;
	}
      else if (type == float_type_node)
	type = complex_float_type_node;
      else if (type == double_type_node)
	type = complex_double_type_node;
      else if (type == long_double_type_node)
	type = complex_long_double_type_node;
      else
	{
	  if (pedantic)
	    pedwarn ("ISO C does not support complex integer types");
	  type = build_complex_type (type);
	}
    }

  /* Check the type and width of a bit-field.  */
  if (bitfield)
    check_bitfield_type_and_width (&type, width, orig_name);

  /* Figure out the type qualifiers for the declaration.  There are
     two ways a declaration can become qualified.  One is something
     like `const int i' where the `const' is explicit.  Another is
     something like `typedef const int CI; CI i' where the type of the
     declaration contains the `const'.  A third possibility is that
     there is a type qualifier on the element type of a typedefed
     array type, in which case we should extract that qualifier so
     that c_apply_type_quals_to_decls receives the full list of
     qualifiers to work with (C90 is not entirely clear about whether
     duplicate qualifiers should be diagnosed in this case, but it
     seems most appropriate to do so).  */
  element_type = strip_array_types (type);
  constp = !! (specbits & 1 << (int) RID_CONST) + TYPE_READONLY (element_type);
  restrictp
    = !! (specbits & 1 << (int) RID_RESTRICT) + TYPE_RESTRICT (element_type);
  volatilep
    = !! (specbits & 1 << (int) RID_VOLATILE) + TYPE_VOLATILE (element_type);
  inlinep = !! (specbits & (1 << (int) RID_INLINE));
  if (pedantic && !flag_isoc99)
    {
      if (constp > 1)
	pedwarn ("duplicate `const'");
      if (restrictp > 1)
	pedwarn ("duplicate `restrict'");
      if (volatilep > 1)
	pedwarn ("duplicate `volatile'");
    }
  if (! flag_gen_aux_info && (TYPE_QUALS (type)))
    type = TYPE_MAIN_VARIANT (type);
  type_quals = ((constp ? TYPE_QUAL_CONST : 0)
		| (restrictp ? TYPE_QUAL_RESTRICT : 0)
		| (volatilep ? TYPE_QUAL_VOLATILE : 0));

  /* Warn if two storage classes are given. Default to `auto'.  */

  {
    int nclasses = 0;

    if (specbits & 1 << (int) RID_AUTO) nclasses++;
    if (specbits & 1 << (int) RID_STATIC) nclasses++;
    if (specbits & 1 << (int) RID_EXTERN) nclasses++;
    if (specbits & 1 << (int) RID_REGISTER) nclasses++;
    if (specbits & 1 << (int) RID_TYPEDEF) nclasses++;

    /* "static __thread" and "extern __thread" are allowed.  */
    if ((specbits & (1 << (int) RID_THREAD
		     | 1 << (int) RID_STATIC
		     | 1 << (int) RID_EXTERN)) == (1 << (int) RID_THREAD))
      nclasses++;
    /* APPLE LOCAL private extern */
    if (specbits & 1 << (int) RID_PRIVATE_EXTERN) nclasses++;

    /* Warn about storage classes that are invalid for certain
       kinds of declarations (parameters, typenames, etc.).  */

    if (nclasses > 1)
      error ("multiple storage classes in declaration of `%s'", name);
    else if (funcdef_flag
	     && (specbits
		 & ((1 << (int) RID_REGISTER)
		    | (1 << (int) RID_AUTO)
		    | (1 << (int) RID_TYPEDEF)
		    | (1 << (int) RID_THREAD))))
      {
	if (specbits & 1 << (int) RID_AUTO
	    && (pedantic || current_scope == file_scope))
	  pedwarn ("function definition declared `auto'");
	if (specbits & 1 << (int) RID_REGISTER)
	  error ("function definition declared `register'");
	if (specbits & 1 << (int) RID_TYPEDEF)
	  error ("function definition declared `typedef'");
	if (specbits & 1 << (int) RID_THREAD)
	  error ("function definition declared `__thread'");
	specbits &= ~((1 << (int) RID_TYPEDEF) | (1 << (int) RID_REGISTER)
		      | (1 << (int) RID_AUTO) | (1 << (int) RID_THREAD));
      }
    else if (decl_context != NORMAL && nclasses > 0)
      {
	if (decl_context == PARM && specbits & 1 << (int) RID_REGISTER)
	  ;
	else
	  {
	    switch (decl_context)
	      {
	      case FIELD:
		error ("storage class specified for structure field `%s'",
		       name);
		break;
	      case PARM:
		error ("storage class specified for parameter `%s'", name);
		break;
	      default:
		error ("storage class specified for typename");
		break;
	      }
	    specbits &= ~((1 << (int) RID_TYPEDEF) | (1 << (int) RID_REGISTER)
			  | (1 << (int) RID_AUTO) | (1 << (int) RID_STATIC)
			  | (1 << (int) RID_EXTERN) | (1 << (int) RID_THREAD));
	  }
      }
    else if (specbits & 1 << (int) RID_EXTERN && initialized && ! funcdef_flag)
      {
	/* `extern' with initialization is invalid if not at file scope.  */
	if (current_scope == file_scope)
	  warning ("`%s' initialized and declared `extern'", name);
	else
	  error ("`%s' has both `extern' and initializer", name);
      }
    else if (current_scope == file_scope)
      {
	if (specbits & 1 << (int) RID_AUTO)
	  error ("file-scope declaration of `%s' specifies `auto'", name);
      }
    else
      {
	if (specbits & 1 << (int) RID_EXTERN && funcdef_flag)
	  error ("nested function `%s' declared `extern'", name);
	else if ((specbits & (1 << (int) RID_THREAD
			       | 1 << (int) RID_EXTERN
			       | 1 << (int) RID_STATIC))
		 == (1 << (int) RID_THREAD))
	  {
	    error ("function-scope `%s' implicitly auto and declared `__thread'",
		   name);
	    specbits &= ~(1 << (int) RID_THREAD);
	  }
      }
  }

  /* Now figure out the structure of the declarator proper.
     Descend through it, creating more complex types, until we reach
     the declared identifier (or NULL_TREE, in an absolute declarator).  */

  while (declarator && TREE_CODE (declarator) != IDENTIFIER_NODE)
    {
      if (type == error_mark_node)
	{
	  declarator = TREE_OPERAND (declarator, 0);
	  continue;
	}

      /* Each level of DECLARATOR is either an ARRAY_REF (for ...[..]),
	 an INDIRECT_REF (for *...),
	 a CALL_EXPR (for ...(...)),
	 a TREE_LIST (for nested attributes),
	 an identifier (for the name being declared)
	 or a null pointer (for the place in an absolute declarator
	 where the name was omitted).
	 For the last two cases, we have just exited the loop.

	 At this point, TYPE is the type of elements of an array,
	 or for a function to return, or for a pointer to point to.
	 After this sequence of ifs, TYPE is the type of the
	 array or function or pointer, and DECLARATOR has had its
	 outermost layer removed.  */

      if (array_ptr_quals != NULL_TREE || array_parm_static)
	{
	  /* Only the innermost declarator (making a parameter be of
	     array type which is converted to pointer type)
	     may have static or type qualifiers.  */
	  error ("static or type qualifiers in non-parameter array declarator");
	  array_ptr_quals = NULL_TREE;
	  array_parm_static = 0;
	}

      if (TREE_CODE (declarator) == TREE_LIST)
	{
	  /* We encode a declarator with embedded attributes using
	     a TREE_LIST.  */
	  tree attrs = TREE_PURPOSE (declarator);
	  tree inner_decl;
	  int attr_flags = 0;
	  declarator = TREE_VALUE (declarator);
	  inner_decl = declarator;
	  while (inner_decl != NULL_TREE
		 && TREE_CODE (inner_decl) == TREE_LIST)
	    inner_decl = TREE_VALUE (inner_decl);
	  if (inner_decl == NULL_TREE
	      || TREE_CODE (inner_decl) == IDENTIFIER_NODE)
	    attr_flags |= (int) ATTR_FLAG_DECL_NEXT;
	  else if (TREE_CODE (inner_decl) == CALL_EXPR)
	    attr_flags |= (int) ATTR_FLAG_FUNCTION_NEXT;
	  else if (TREE_CODE (inner_decl) == ARRAY_REF)
	    attr_flags |= (int) ATTR_FLAG_ARRAY_NEXT;
	  returned_attrs = decl_attributes (&type,
					    chainon (returned_attrs, attrs),
					    attr_flags);
	}
      else if (TREE_CODE (declarator) == ARRAY_REF)
	{
	  tree itype = NULL_TREE;
	  tree size = TREE_OPERAND (declarator, 1);
	  /* The index is a signed object `sizetype' bits wide.  */
	  tree index_type = c_common_signed_type (sizetype);

	  array_ptr_quals = TREE_TYPE (declarator);
	  array_parm_static = TREE_STATIC (declarator);

	  declarator = TREE_OPERAND (declarator, 0);

	  /* Check for some types that there cannot be arrays of.  */

	  if (VOID_TYPE_P (type))
	    {
	      error ("declaration of `%s' as array of voids", name);
	      type = error_mark_node;
	    }

	  if (TREE_CODE (type) == FUNCTION_TYPE)
	    {
	      error ("declaration of `%s' as array of functions", name);
	      type = error_mark_node;
	    }

	  if (pedantic && !in_system_header && flexible_array_type_p (type))
	    pedwarn ("invalid use of structure with flexible array member");

	  if (size == error_mark_node)
	    type = error_mark_node;

	  if (type == error_mark_node)
	    continue;

	  /* If size was specified, set ITYPE to a range-type for that size.
	     Otherwise, ITYPE remains null.  finish_decl may figure it out
	     from an initial value.  */

	  if (size)
	    {
	      /* Strip NON_LVALUE_EXPRs since we aren't using as an lvalue.  */
	      STRIP_TYPE_NOPS (size);

	      if (! INTEGRAL_TYPE_P (TREE_TYPE (size)))
		{
		  error ("size of array `%s' has non-integer type", name);
		  size = integer_one_node;
		}

	      if (pedantic && integer_zerop (size))
		pedwarn ("ISO C forbids zero-size array `%s'", name);

	      if (TREE_CODE (size) == INTEGER_CST)
		{
		  constant_expression_warning (size);
		  if (tree_int_cst_sgn (size) < 0)
		    {
		      error ("size of array `%s' is negative", name);
		      size = integer_one_node;
		    }
		}
	      else
		{
		  /* Make sure the array size remains visibly nonconstant
		     even if it is (eg) a const variable with known value.  */
		  size_varies = 1;

		  if (!flag_isoc99 && pedantic)
		    {
		      if (TREE_CONSTANT (size))
			pedwarn ("ISO C90 forbids array `%s' whose size can't be evaluated",
				 name);
		      else
			pedwarn ("ISO C90 forbids variable-size array `%s'",
				 name);
		    }
		}

	      if (integer_zerop (size))
		{
		  /* A zero-length array cannot be represented with an
		     unsigned index type, which is what we'll get with
		     build_index_type.  Create an open-ended range instead.  */
		  itype = build_range_type (sizetype, size, NULL_TREE);
		}
	      else
		{
		  /* Compute the maximum valid index, that is, size - 1.
		     Do the calculation in index_type, so that if it is
		     a variable the computations will be done in the
		     proper mode.  */
	          itype = fold (build2 (MINUS_EXPR, index_type,
					convert (index_type, size),
					convert (index_type, size_one_node)));

	          /* If that overflowed, the array is too big.
		     ??? While a size of INT_MAX+1 technically shouldn't
		     cause an overflow (because we subtract 1), the overflow
		     is recorded during the conversion to index_type, before
		     the subtraction.  Handling this case seems like an
		     unnecessary complication.  */
		  if (TREE_OVERFLOW (itype))
		    {
		      error ("size of array `%s' is too large", name);
		      type = error_mark_node;
		      continue;
		    }

		  if (size_varies)
		    itype = variable_size (itype);
		  itype = build_index_type (itype);
		}
	    }
	  else if (decl_context == FIELD)
	    {
	      if (pedantic && !flag_isoc99 && !in_system_header)
		pedwarn ("ISO C90 does not support flexible array members");

	      /* ISO C99 Flexible array members are effectively identical
		 to GCC's zero-length array extension.  */
	      itype = build_range_type (sizetype, size_zero_node, NULL_TREE);
	    }

	  /* If pedantic, complain about arrays of incomplete types.  */

	  if (pedantic && !COMPLETE_TYPE_P (type))
	    pedwarn ("array type has incomplete element type");

	  /* Build the array type itself, then merge any constancy or
	     volatility into the target type.  We must do it in this order
	     to ensure that the TYPE_MAIN_VARIANT field of the array type
	     is set correctly.  */

	  type = build_array_type (type, itype);
	  if (type_quals)
	    type = c_build_qualified_type (type, type_quals);

	  if (size_varies)
	    C_TYPE_VARIABLE_SIZE (type) = 1;

	  /* The GCC extension for zero-length arrays differs from
	     ISO flexible array members in that sizeof yields zero.  */
	  if (size && integer_zerop (size))
	    {
	      layout_type (type);
	      TYPE_SIZE (type) = bitsize_zero_node;
	      TYPE_SIZE_UNIT (type) = size_zero_node;
	    }
	  else if (declarator && TREE_CODE (declarator) == INDIRECT_REF)
	    /* We can never complete an array type which is the target of a
	       pointer, so go ahead and lay it out.  */
	    layout_type (type);

	  if (decl_context != PARM
	      && (array_ptr_quals != NULL_TREE || array_parm_static))
	    {
	      error ("static or type qualifiers in non-parameter array declarator");
	      array_ptr_quals = NULL_TREE;
	      array_parm_static = 0;
	    }
	}
      else if (TREE_CODE (declarator) == CALL_EXPR)
	{
	  /* Say it's a definition only for the declarator closest to
	     the identifier, apart possibly from some attributes.  */
	  bool really_funcdef = false;
	  tree arg_types;
	  if (funcdef_flag)
	    {
	      tree t = TREE_OPERAND (declarator, 0);
	      while (TREE_CODE (t) == TREE_LIST)
		t = TREE_VALUE (t);
	      really_funcdef = (TREE_CODE (t) == IDENTIFIER_NODE);
	    }

	  /* Declaring a function type.
	     Make sure we have a valid type for the function to return.  */
	  if (type == error_mark_node)
	    continue;

	  size_varies = 0;

	  /* Warn about some types functions can't return.  */

	  if (TREE_CODE (type) == FUNCTION_TYPE)
	    {
	      error ("`%s' declared as function returning a function", name);
	      type = integer_type_node;
	    }
	  if (TREE_CODE (type) == ARRAY_TYPE)
	    {
	      error ("`%s' declared as function returning an array", name);
	      type = integer_type_node;
	    }

	  /* Construct the function type and go to the next
	     inner layer of declarator.  */
	  arg_info = TREE_OPERAND (declarator, 1);
	  arg_types = grokparms (arg_info, really_funcdef);

	  /* Type qualifiers before the return type of the function
	     qualify the return type, not the function type.  */
	  if (type_quals)
	    {
	      /* Type qualifiers on a function return type are
		 normally permitted by the standard but have no
		 effect, so give a warning at -Wreturn-type.
		 Qualifiers on a void return type are banned on
		 function definitions in ISO C; GCC used to used them
		 for noreturn functions.  */
	      if (VOID_TYPE_P (type) && really_funcdef)
		pedwarn ("function definition has qualified void return type");
	      else if (warn_return_type)
		warning ("type qualifiers ignored on function return type");

	      type = c_build_qualified_type (type, type_quals);
	    }
	  type_quals = TYPE_UNQUALIFIED;

	  type = build_function_type (type, arg_types);
	  declarator = TREE_OPERAND (declarator, 0);

	  /* Set the TYPE_CONTEXTs for each tagged type which is local to
	     the formal parameter list of this FUNCTION_TYPE to point to
	     the FUNCTION_TYPE node itself.  */

	  {
	    tree link;

	    for (link = ARG_INFO_TAGS (arg_info);
		 link;
		 link = TREE_CHAIN (link))
	      TYPE_CONTEXT (TREE_VALUE (link)) = type;
	  }
	}
      else if (TREE_CODE (declarator) == INDIRECT_REF)
	{
	  /* Merge any constancy or volatility into the target type
	     for the pointer.  */

	  if (pedantic && TREE_CODE (type) == FUNCTION_TYPE
	      && type_quals)
	    pedwarn ("ISO C forbids qualified function types");
	  if (type_quals)
	    type = c_build_qualified_type (type, type_quals);
	  type_quals = TYPE_UNQUALIFIED;
	  size_varies = 0;

	  type = build_pointer_type (type);

	  /* Process a list of type modifier keywords
	     (such as const or volatile) that were given inside the `*'.  */

	  if (TREE_TYPE (declarator))
	    {
	      tree typemodlist;
	      int erred = 0;

	      constp = 0;
	      volatilep = 0;
	      restrictp = 0;
	      for (typemodlist = TREE_TYPE (declarator); typemodlist;
		   typemodlist = TREE_CHAIN (typemodlist))
		{
		  tree qualifier = TREE_VALUE (typemodlist);

		  if (C_IS_RESERVED_WORD (qualifier))
		    {
		      if (C_RID_CODE (qualifier) == RID_CONST)
			constp++;
		      else if (C_RID_CODE (qualifier) == RID_VOLATILE)
			volatilep++;
		      else if (C_RID_CODE (qualifier) == RID_RESTRICT)
			restrictp++;
		      else
			erred++;
		    }
		  else
		    erred++;
		}

	      if (erred)
		error ("invalid type modifier within pointer declarator");
	      if (pedantic && !flag_isoc99)
		{
		  if (constp > 1)
		    pedwarn ("duplicate `const'");
		  if (volatilep > 1)
		    pedwarn ("duplicate `volatile'");
		  if (restrictp > 1)
		    pedwarn ("duplicate `restrict'");
		}

	      type_quals = ((constp ? TYPE_QUAL_CONST : 0)
			    | (restrictp ? TYPE_QUAL_RESTRICT : 0)
			    | (volatilep ? TYPE_QUAL_VOLATILE : 0));
	    }

	  declarator = TREE_OPERAND (declarator, 0);
	}
      else
	abort ();

    }

  /* Now TYPE has the actual type.  */

  /* Did array size calculations overflow?  */

  if (TREE_CODE (type) == ARRAY_TYPE
      && COMPLETE_TYPE_P (type)
      && TREE_OVERFLOW (TYPE_SIZE (type)))
    {
      error ("size of array `%s' is too large", name);
      /* If we proceed with the array type as it is, we'll eventually
	 crash in tree_low_cst().  */
      type = error_mark_node;
    }

  /* If this is declaring a typedef name, return a TYPE_DECL.  */

  if (specbits & (1 << (int) RID_TYPEDEF))
    {
      tree decl;
      /* Note that the grammar rejects storage classes
	 in typenames, fields or parameters */
      if (pedantic && TREE_CODE (type) == FUNCTION_TYPE
	  && type_quals)
	pedwarn ("ISO C forbids qualified function types");
      if (type_quals)
	type = c_build_qualified_type (type, type_quals);
      decl = build_decl (TYPE_DECL, declarator, type);
      if ((specbits & (1 << (int) RID_SIGNED))
	  || (typedef_decl && C_TYPEDEF_EXPLICITLY_SIGNED (typedef_decl)))
	C_TYPEDEF_EXPLICITLY_SIGNED (decl) = 1;
      decl_attributes (&decl, returned_attrs, 0);
      return decl;
    }

  /* Detect the case of an array type of unspecified size
     which came, as such, direct from a typedef name.
     We must copy the type, so that each identifier gets
     a distinct type, so that each identifier's size can be
     controlled separately by its own initializer.  */

  if (type != 0 && typedef_type != 0
      && TREE_CODE (type) == ARRAY_TYPE && TYPE_DOMAIN (type) == 0
      && TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (typedef_type))
    {
      type = build_array_type (TREE_TYPE (type), 0);
      if (size_varies)
	C_TYPE_VARIABLE_SIZE (type) = 1;
    }

  /* If this is a type name (such as, in a cast or sizeof),
     compute the type and return it now.  */

  if (decl_context == TYPENAME)
    {
      /* Note that the grammar rejects storage classes
	 in typenames, fields or parameters */
      if (pedantic && TREE_CODE (type) == FUNCTION_TYPE
	  && type_quals)
	pedwarn ("ISO C forbids const or volatile function types");
      if (type_quals)
	type = c_build_qualified_type (type, type_quals);
      decl_attributes (&type, returned_attrs, 0);
      return type;
    }

  /* Aside from typedefs and type names (handle above),
     `void' at top level (not within pointer)
     is allowed only in public variables.
     We don't complain about parms either, but that is because
     a better error message can be made later.  */

  if (VOID_TYPE_P (type) && decl_context != PARM
      && ! ((decl_context != FIELD && TREE_CODE (type) != FUNCTION_TYPE)
	    && ((specbits & (1 << (int) RID_EXTERN))
		|| (current_scope == file_scope
		    && !(specbits
			 & ((1 << (int) RID_STATIC) | (1 << (int) RID_REGISTER)))))))
    {
      error ("variable or field `%s' declared void", name);
      type = integer_type_node;
    }

  /* Now create the decl, which may be a VAR_DECL, a PARM_DECL
     or a FUNCTION_DECL, depending on DECL_CONTEXT and TYPE.  */

  {
    tree decl;

    if (decl_context == PARM)
      {
	tree type_as_written;
	tree promoted_type;

	/* A parameter declared as an array of T is really a pointer to T.
	   One declared as a function is really a pointer to a function.  */

	if (TREE_CODE (type) == ARRAY_TYPE)
	  {
	    /* Transfer const-ness of array into that of type pointed to.  */
	    type = TREE_TYPE (type);
	    if (type_quals)
	      type = c_build_qualified_type (type, type_quals);
	    type = build_pointer_type (type);
	    type_quals = TYPE_UNQUALIFIED;
	    if (array_ptr_quals)
	      {
		tree new_ptr_quals, new_ptr_attrs;
		int erred = 0;
		split_specs_attrs (array_ptr_quals, &new_ptr_quals, &new_ptr_attrs);
		/* We don't yet implement attributes in this context.  */
		if (new_ptr_attrs != NULL_TREE)
		  warning ("attributes in parameter array declarator ignored");

		constp = 0;
		volatilep = 0;
		restrictp = 0;
		for (; new_ptr_quals; new_ptr_quals = TREE_CHAIN (new_ptr_quals))
		  {
		    tree qualifier = TREE_VALUE (new_ptr_quals);

		    if (C_IS_RESERVED_WORD (qualifier))
		      {
			if (C_RID_CODE (qualifier) == RID_CONST)
			  constp++;
			else if (C_RID_CODE (qualifier) == RID_VOLATILE)
			  volatilep++;
			else if (C_RID_CODE (qualifier) == RID_RESTRICT)
			  restrictp++;
			else
			  erred++;
		      }
		    else
		      erred++;
		  }

		if (erred)
		  error ("invalid type modifier within array declarator");

		type_quals = ((constp ? TYPE_QUAL_CONST : 0)
			      | (restrictp ? TYPE_QUAL_RESTRICT : 0)
			      | (volatilep ? TYPE_QUAL_VOLATILE : 0));
	      }
	    size_varies = 0;
	  }
	else if (TREE_CODE (type) == FUNCTION_TYPE)
	  {
	    if (pedantic && type_quals)
	      pedwarn ("ISO C forbids qualified function types");
	    if (type_quals)
	      type = c_build_qualified_type (type, type_quals);
	    type = build_pointer_type (type);
	    type_quals = TYPE_UNQUALIFIED;
	  }
	else if (type_quals)
	  type = c_build_qualified_type (type, type_quals);

	type_as_written = type;

	decl = build_decl (PARM_DECL, declarator, type);
	if (size_varies)
	  C_DECL_VARIABLE_SIZE (decl) = 1;

	/* Compute the type actually passed in the parmlist,
	   for the case where there is no prototype.
	   (For example, shorts and chars are passed as ints.)
	   When there is a prototype, this is overridden later.  */

	if (type == error_mark_node)
	  promoted_type = type;
	else
	  promoted_type = c_type_promotes_to (type);

	DECL_ARG_TYPE (decl) = promoted_type;
	DECL_ARG_TYPE_AS_WRITTEN (decl) = type_as_written;
      }
    else if (decl_context == FIELD)
      {
	/* Structure field.  It may not be a function.  */

	if (TREE_CODE (type) == FUNCTION_TYPE)
	  {
	    error ("field `%s' declared as a function", name);
	    type = build_pointer_type (type);
	  }
	else if (TREE_CODE (type) != ERROR_MARK
	         && !COMPLETE_OR_UNBOUND_ARRAY_TYPE_P (type))
	  {
	    error ("field `%s' has incomplete type", name);
	    type = error_mark_node;
	  }
	/* Move type qualifiers down to element of an array.  */
	if (TREE_CODE (type) == ARRAY_TYPE && type_quals)
	  type = build_array_type (c_build_qualified_type (TREE_TYPE (type),
							   type_quals),
				   TYPE_DOMAIN (type));
	decl = build_decl (FIELD_DECL, declarator, type);
	DECL_NONADDRESSABLE_P (decl) = bitfield;

	if (size_varies)
	  C_DECL_VARIABLE_SIZE (decl) = 1;
      }
    else if (TREE_CODE (type) == FUNCTION_TYPE)
      {
	if (specbits & (1 << (int) RID_REGISTER)
	    || specbits & (1 << (int) RID_THREAD))
	  error ("invalid storage class for function `%s'", name);
	else if (current_scope != file_scope)
	  {
	    /* Function declaration not at file scope.  Storage
	       classes other than `extern' are not allowed, C99
	       6.7.1p5, and `extern' makes no difference.  However,
	       GCC allows 'auto', perhaps with 'inline', to support
	       nested functions.  */
	    if (specbits & (1 << (int) RID_AUTO))
	      {
		if (pedantic)
		  pedwarn ("invalid storage class for function `%s'", name);
	      }
	    if (specbits & (1 << (int) RID_STATIC))
	      error ("invalid storage class for function `%s'", name);
	  }

	decl = build_decl (FUNCTION_DECL, declarator, type);
	decl = build_decl_attribute_variant (decl, decl_attr);

	DECL_LANG_SPECIFIC (decl) = GGC_CNEW (struct lang_decl);

	if (pedantic && type_quals && ! DECL_IN_SYSTEM_HEADER (decl))
	  pedwarn ("ISO C forbids qualified function types");

	/* GNU C interprets a volatile-qualified function type to indicate
	   that the function does not return.  */
	if ((type_quals & TYPE_QUAL_VOLATILE)
	    && !VOID_TYPE_P (TREE_TYPE (TREE_TYPE (decl))))
	  warning ("`noreturn' function returns non-void value");

	/* Every function declaration is an external reference
	   (DECL_EXTERNAL) except for those which are not at file
	   scope and are explicitly declared "auto".  This is
	   forbidden by standard C (C99 6.7.1p5) and is interpreted by
	   GCC to signify a forward declaration of a nested function.  */
	if ((specbits & (1 << RID_AUTO)) && current_scope != file_scope)
	  DECL_EXTERNAL (decl) = 0;
	else
	  DECL_EXTERNAL (decl) = 1;

	/* Record absence of global scope for `static' or `auto'.  */
	TREE_PUBLIC (decl)
	  = !(specbits & ((1 << (int) RID_STATIC) | (1 << (int) RID_AUTO)));

	/* For a function definition, record the argument information
	   block in DECL_ARGUMENTS where store_parm_decls will look
	   for it.  */
	if (funcdef_flag)
	  DECL_ARGUMENTS (decl) = arg_info;

	if (defaulted_int)
	  C_FUNCTION_IMPLICIT_INT (decl) = 1;

	/* APPLE LOCAL begin private extern */
        if (specbits & (1 << (int) RID_PRIVATE_EXTERN))
	  {
	    DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;
	    DECL_VISIBILITY_SPECIFIED (decl) = 1;
	  }
	/* APPLE LOCAL end private extern */

	/* Record presence of `inline', if it is reasonable.  */
	if (MAIN_NAME_P (declarator))
	  {
	    if (inlinep)
	      warning ("cannot inline function `main'");
	  }
	else if (inlinep)
	  {
	    /* Record that the function is declared `inline'.  */
	    DECL_DECLARED_INLINE_P (decl) = 1;

	    /* Do not mark bare declarations as DECL_INLINE.  Doing so
	       in the presence of multiple declarations can result in
	       the abstract origin pointing between the declarations,
	       which will confuse dwarf2out.  */
	    if (initialized)
	      {
		DECL_INLINE (decl) = 1;
		if (specbits & (1 << (int) RID_EXTERN))
		  current_extern_inline = 1;
	      }
	  }
	/* If -finline-functions, assume it can be inlined.  This does
	   two things: let the function be deferred until it is actually
	   needed, and let dwarf2 know that the function is inlinable.  */
	else if (flag_inline_trees == 2 && initialized)
	  DECL_INLINE (decl) = 1;
      }
    else
      {
	/* It's a variable.  */
	/* An uninitialized decl with `extern' is a reference.  */
	/* APPLE LOCAL private extern */
	int extern_ref = !initialized && (specbits & ((1 << (int) RID_EXTERN)
						      | (1 << (int) RID_PRIVATE_EXTERN)));

	/* Move type qualifiers down to element of an array.  */
	if (TREE_CODE (type) == ARRAY_TYPE && type_quals)
	  {
	    int saved_align = TYPE_ALIGN(type);
	    type = build_array_type (c_build_qualified_type (TREE_TYPE (type),
							     type_quals),
				     TYPE_DOMAIN (type));
	    TYPE_ALIGN (type) = saved_align;
	  }
	else if (type_quals)
	  type = c_build_qualified_type (type, type_quals);

	/* C99 6.2.2p7: It is invalid (compile-time undefined
	   behavior) to create an 'extern' declaration for a
	   variable if there is a global declaration that is
	   'static' and the global declaration is not visible.
	   (If the static declaration _is_ currently visible,
	   the 'extern' declaration is taken to refer to that decl.) */
	if (extern_ref && current_scope != file_scope)
	  {
	    tree global_decl  = identifier_global_value (declarator);
	    tree visible_decl = lookup_name (declarator);

	    if (global_decl
		&& global_decl != visible_decl
		&& TREE_CODE (global_decl) == VAR_DECL
		&& !TREE_PUBLIC (global_decl))
	      error ("variable previously declared 'static' redeclared "
		     "'extern'");
	  }

	decl = build_decl (VAR_DECL, declarator, type);
	if (size_varies)
	  C_DECL_VARIABLE_SIZE (decl) = 1;

	if (inlinep)
	  pedwarn ("%Jvariable '%D' declared `inline'", decl, decl);

	/* At file scope, an initialized extern declaration may follow
	   a static declaration.  In that case, DECL_EXTERNAL will be
	   reset later in start_decl.  */
	/* APPLE LOCAL private extern */
	DECL_EXTERNAL (decl) = !!(specbits & ((1 << (int) RID_EXTERN)
					      | (1 << (int) RID_PRIVATE_EXTERN)));

	/* APPLE LOCAL begin private extern */
	if (specbits & (1 << (int) RID_PRIVATE_EXTERN))
	  {
	    DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;
	    DECL_VISIBILITY_SPECIFIED (decl) = 1;
	  }
	/* APPLE LOCAL end private extern */

	/* At file scope, the presence of a `static' or `register' storage
	   class specifier, or the absence of all storage class specifiers
	   makes this declaration a definition (perhaps tentative).  Also,
	   the absence of both `static' and `register' makes it public.  */
	if (current_scope == file_scope)
	  {
	    TREE_PUBLIC (decl) = !(specbits & ((1 << (int) RID_STATIC)
					       | (1 << (int) RID_REGISTER)));
	    TREE_STATIC (decl) = !extern_ref;
	  }
	/* Not at file scope, only `static' makes a static definition.  */
	else
	  {
	    TREE_STATIC (decl) = (specbits & (1 << (int) RID_STATIC)) != 0;
	    TREE_PUBLIC (decl) = extern_ref;
	  }

	if (specbits & 1 << (int) RID_THREAD)
	  {
	    if (targetm.have_tls)
	      DECL_THREAD_LOCAL (decl) = 1;
	    else
	      /* A mere warning is sure to result in improper semantics
		 at runtime.  Don't bother to allow this to compile.  */
	      error ("thread-local storage not supported for this target");
	  }
      }

    /* Record `register' declaration for warnings on &
       and in case doing stupid register allocation.  */

    if (specbits & (1 << (int) RID_REGISTER))
      {
	C_DECL_REGISTER (decl) = 1;
	DECL_REGISTER (decl) = 1;
      }

    /* APPLE LOCAL begin CW asm blocks */
    if (cw_asm_specbit)
      {
	/* Record that this is a decl of a CW-style asm function.  */
	if (flag_cw_asm_blocks)
	  {
	    DECL_CW_ASM_FUNCTION (decl) = 1;
	    DECL_CW_ASM_NORETURN (decl) = 0;
	    DECL_CW_ASM_FRAME_SIZE (decl) = -2;
	  }
	else
	  error ("asm functions not enabled, use `-fasm-blocks'");
      }
    /* APPLE LOCAL end CW asm blocks */

    /* Record constancy and volatility.  */
    c_apply_type_quals_to_decl (type_quals, decl);

    /* If a type has volatile components, it should be stored in memory.
       Otherwise, the fact that those components are volatile
       will be ignored, and would even crash the compiler.  */
    if (C_TYPE_FIELDS_VOLATILE (TREE_TYPE (decl)))
      {
	/* It is not an error for a structure with volatile fields to
	   be declared register, but reset DECL_REGISTER since it
	   cannot actually go in a register.  */
	int was_reg = C_DECL_REGISTER (decl);
	C_DECL_REGISTER (decl) = 0;
	DECL_REGISTER (decl) = 0;
	c_mark_addressable (decl);
	C_DECL_REGISTER (decl) = was_reg;
      }

#ifdef ENABLE_CHECKING
  /* This is the earliest point at which we might know the assembler
     name of a variable.  Thus, if it's known before this, die horribly.  */
  if (DECL_ASSEMBLER_NAME_SET_P (decl))
    abort ();
#endif

    decl_attributes (&decl, returned_attrs, 0);

    return decl;
  }
}

/* Decode the parameter-list info for a function type or function definition.
   The argument is the value returned by `get_parm_info' (or made in parse.y
   if there is an identifier list instead of a parameter decl list).
   These two functions are separate because when a function returns
   or receives functions then each is called multiple times but the order
   of calls is different.  The last call to `grokparms' is always the one
   that contains the formal parameter names of a function definition.

   Return a list of arg types to use in the FUNCTION_TYPE for this function.

   FUNCDEF_FLAG is true for a function definition, false for
   a mere declaration.  A nonempty identifier-list gets an error message
   when FUNCDEF_FLAG is false.  */

static tree
grokparms (tree arg_info, bool funcdef_flag)
{
  tree arg_types = ARG_INFO_TYPES (arg_info);

  if (warn_strict_prototypes && arg_types == 0 && !funcdef_flag
      && !in_system_header)
    warning ("function declaration isn't a prototype");

  if (arg_types == error_mark_node)
    return 0;  /* don't set TYPE_ARG_TYPES in this case */

  else if (arg_types && TREE_CODE (TREE_VALUE (arg_types)) == IDENTIFIER_NODE)
    {
      if (! funcdef_flag)
	pedwarn ("parameter names (without types) in function declaration");

      ARG_INFO_PARMS (arg_info) = ARG_INFO_TYPES (arg_info);
      ARG_INFO_TYPES (arg_info) = 0;
      return 0;
    }
  else
    {
      tree parm, type, typelt;
      unsigned int parmno;

      /* If the arg types are incomplete in a declaration, they must
	 include undefined tags.  These tags can never be defined in
	 the scope of the declaration, so the types can never be
	 completed, and no call can be compiled successfully.  */

      for (parm = ARG_INFO_PARMS (arg_info), typelt = arg_types, parmno = 1;
	   parm;
	   parm = TREE_CHAIN (parm), typelt = TREE_CHAIN (typelt), parmno++)
	{
	  type = TREE_VALUE (typelt);
	  if (type == error_mark_node)
	    continue;

	  if (!COMPLETE_TYPE_P (type))
	    {
	      if (funcdef_flag)
		{
		  if (DECL_NAME (parm))
		    error ("%Jparameter %u ('%D') has incomplete type",
			   parm, parmno, parm);
		  else
		    error ("%Jparameter %u has incomplete type",
			   parm, parmno);

		  TREE_VALUE (typelt) = error_mark_node;
		  TREE_TYPE (parm) = error_mark_node;
		}
	      else
		{
		  if (DECL_NAME (parm))
		    warning ("%Jparameter %u ('%D') has incomplete type",
			     parm, parmno, parm);
		  else
		    warning ("%Jparameter %u has incomplete type",
			     parm, parmno);
		}
	    }
	}
      return arg_types;
    }
}

/* Take apart the current scope and return a tree_list node with info
   on a parameter list just parsed.  This tree_list node should be
   examined using the ARG_INFO_* macros, defined above:

     ARG_INFO_PARMS:  a list of parameter decls.
     ARG_INFO_TAGS:   a list of structure, union and enum tags defined.
     ARG_INFO_TYPES:  a list of argument types to go in the FUNCTION_TYPE.
     ARG_INFO_OTHERS: a list of non-parameter decls (notably enumeration
                      constants) defined with the parameters.

   This tree_list node is later fed to 'grokparms' and 'store_parm_decls'.

   ELLIPSIS being true means the argument list ended in '...' so don't
   append a sentinel (void_list_node) to the end of the type-list.  */

tree
get_parm_info (bool ellipsis)
{
  struct c_binding *b = current_scope->bindings;
  tree arg_info = make_node (TREE_LIST);
  tree parms    = 0;
  tree tags     = 0;
  tree types    = 0;
  tree others   = 0;

  static bool explained_incomplete_types = false;
  bool gave_void_only_once_err = false;

  /* The bindings in this scope must not get put into a block.
     We will take care of deleting the binding nodes.  */
  current_scope->bindings = 0;

  /* This function is only called if there was *something* on the
     parameter list.  */
#ifdef ENABLE_CHECKING
  if (b == 0)
    abort ();
#endif

  /* A parameter list consisting solely of 'void' indicates that the
     function takes no arguments.  But if the 'void' is qualified
     (by 'const' or 'volatile'), or has a storage class specifier
     ('register'), then the behavior is undefined; issue an error.
     Typedefs for 'void' are OK (see DR#157).  */
  if (b->prev == 0		            /* one binding */
      && TREE_CODE (b->decl) == PARM_DECL   /* which is a parameter */
      && !DECL_NAME (b->decl)               /* anonymous */
      && VOID_TYPE_P (TREE_TYPE (b->decl))) /* of void type */
    {
      if (TREE_THIS_VOLATILE (b->decl)
	  || TREE_READONLY (b->decl)
	  || C_DECL_REGISTER (b->decl))
	error ("'void' as only parameter may not be qualified");

      /* There cannot be an ellipsis.  */
      if (ellipsis)
	error ("'void' must be the only parameter");

      ARG_INFO_TYPES (arg_info) = void_list_node;
      return arg_info;
    }

  if (!ellipsis)
    types = void_list_node;

  /* Break up the bindings list into parms, tags, types, and others;
     apply sanity checks; purge the name-to-decl bindings.  */
  while (b)
    {
      tree decl = b->decl;
      tree type = TREE_TYPE (decl);
      const char *keyword;

      switch (TREE_CODE (decl))
	{
	case PARM_DECL:
	  if (b->id)
	    {
#ifdef ENABLE_CHECKING
	      if (I_SYMBOL_BINDING (b->id) != b) abort ();
#endif
	      I_SYMBOL_BINDING (b->id) = b->shadowed;
	    }

	  /* Check for forward decls that never got their actual decl.  */
	  if (TREE_ASM_WRITTEN (decl))
	    error ("%Jparameter '%D' has just a forward declaration",
		   decl, decl);
	  /* Check for (..., void, ...) and issue an error.  */
	  else if (VOID_TYPE_P (type) && !DECL_NAME (decl))
	    {
	      if (!gave_void_only_once_err)
		{
		  error ("'void' must be the only parameter");
		  gave_void_only_once_err = true;
		}
	    }
	  else
	    {
	      /* Valid parameter, add it to the list.  */
	      TREE_CHAIN (decl) = parms;
	      parms = decl;

	      /* Since there is a prototype, args are passed in their
		 declared types.  The back end may override this later.  */
	      DECL_ARG_TYPE (decl) = type;
	      types = tree_cons (0, type, types);
	    }
	  break;

	case ENUMERAL_TYPE: keyword = "enum"; goto tag;
	case UNION_TYPE:    keyword = "union"; goto tag;
	case RECORD_TYPE:   keyword = "struct"; goto tag;
	tag:
	  /* Types may not have tag-names, in which case the type
	     appears in the bindings list with b->id NULL.  */
	  if (b->id)
	    {
#ifdef ENABLE_CHECKING
	      if (I_TAG_BINDING (b->id) != b) abort ();
#endif
	      I_TAG_BINDING (b->id) = b->shadowed;
	    }

	  /* Warn about any struct, union or enum tags defined in a
	     parameter list.  The scope of such types is limited to
	     the parameter list, which is rarely if ever desirable
	     (it's impossible to call such a function with type-
	     correct arguments).  An anonymous union parm type is
	     meaningful as a GNU extension, so don't warn for that.  */
	  if (TREE_CODE (decl) != UNION_TYPE || b->id != 0)
	    {
	      if (b->id)
		/* The %s will be one of 'struct', 'union', or 'enum'.  */
		warning ("'%s %E' declared inside parameter list",
			 keyword, b->id);
	      else
		/* The %s will be one of 'struct', 'union', or 'enum'.  */
		warning ("anonymous %s declared inside parameter list",
			 keyword);

	      if (! explained_incomplete_types)
		{
		  warning ("its scope is only this definition or declaration,"
			   " which is probably not what you want");
		  explained_incomplete_types = true;
		}
	    }

	  tags = tree_cons (b->id, decl, tags);
	  break;

	case CONST_DECL:
	case TYPE_DECL:
	  /* CONST_DECLs appear here when we have an embedded enum,
	     and TYPE_DECLs appear here when we have an embedded struct
	     or union.  No warnings for this - we already warned about the
	     type itself.  */
	  TREE_CHAIN (decl) = others;
	  others = decl;
	  /* fall through */

	case ERROR_MARK:
	  /* error_mark_node appears here when we have an undeclared
	     variable.  Just throw it away.  */
	  if (b->id)
	    {
#ifdef ENABLE_CHECKING
	      if (I_SYMBOL_BINDING (b->id) != b) abort ();
#endif
	      I_SYMBOL_BINDING (b->id) = b->shadowed;
	    }
	  break;

	  /* Other things that might be encountered.  */
	case LABEL_DECL:
	case FUNCTION_DECL:
	case VAR_DECL:
	default:
	  abort ();
	}

      b = free_binding_and_advance (b);
    }

  ARG_INFO_PARMS  (arg_info) = parms;
  ARG_INFO_TAGS   (arg_info) = tags;
  ARG_INFO_TYPES  (arg_info) = types;
  ARG_INFO_OTHERS (arg_info) = others;
  return arg_info;
}

/* Get the struct, enum or union (CODE says which) with tag NAME.
   Define the tag as a forward-reference if it is not defined.  */

tree
xref_tag (enum tree_code code, tree name)
{
  /* If a cross reference is requested, look up the type
     already defined for this tag and return it.  */

  tree ref = lookup_tag (code, name, 0);
  /* If this is the right type of tag, return what we found.
     (This reference will be shadowed by shadow_tag later if appropriate.)
     If this is the wrong type of tag, do not return it.  If it was the
     wrong type in the same scope, we will have had an error
     message already; if in a different scope and declaring
     a name, pending_xref_error will give an error message; but if in a
     different scope and not declaring a name, this tag should
     shadow the previous declaration of a different type of tag, and
     this would not work properly if we return the reference found.
     (For example, with "struct foo" in an outer scope, "union foo;"
     must shadow that tag with a new one of union type.)  */
  if (ref && TREE_CODE (ref) == code)
    return ref;

  /* If no such tag is yet defined, create a forward-reference node
     and record it as the "definition".
     When a real declaration of this type is found,
     the forward-reference will be altered into a real type.  */

  ref = make_node (code);
  if (code == ENUMERAL_TYPE)
    {
      /* Give the type a default layout like unsigned int
	 to avoid crashing if it does not get defined.  */
      TYPE_MODE (ref) = TYPE_MODE (unsigned_type_node);
      TYPE_ALIGN (ref) = TYPE_ALIGN (unsigned_type_node);
      TYPE_USER_ALIGN (ref) = 0;
      TYPE_UNSIGNED (ref) = 1;
      TYPE_PRECISION (ref) = TYPE_PRECISION (unsigned_type_node);
      TYPE_MIN_VALUE (ref) = TYPE_MIN_VALUE (unsigned_type_node);
      TYPE_MAX_VALUE (ref) = TYPE_MAX_VALUE (unsigned_type_node);
    }

  pushtag (name, ref);

  return ref;
}

/* Make sure that the tag NAME is defined *in the current scope*
   at least as a forward reference.
   CODE says which kind of tag NAME ought to be.  */

tree
start_struct (enum tree_code code, tree name)
{
  /* If there is already a tag defined at this scope
     (as a forward reference), just return it.  */

  tree ref = 0;

  if (name != 0)
    ref = lookup_tag (code, name, 1);
  if (ref && TREE_CODE (ref) == code)
    {
      if (TYPE_FIELDS (ref))
        {
	  if (code == UNION_TYPE)
	    error ("redefinition of `union %s'", IDENTIFIER_POINTER (name));
          else
	    error ("redefinition of `struct %s'", IDENTIFIER_POINTER (name));
	}
    }
  else
    {
      /* Otherwise create a forward-reference just so the tag is in scope.  */

      ref = make_node (code);
      pushtag (name, ref);
    }

  C_TYPE_BEING_DEFINED (ref) = 1;
  TYPE_PACKED (ref) = flag_pack_struct;
  return ref;
}

/* Process the specs, declarator (NULL if omitted) and width (NULL if omitted)
   of a structure component, returning a FIELD_DECL node.
   WIDTH is non-NULL for bit-fields only, and is an INTEGER_CST node.

   This is done during the parsing of the struct declaration.
   The FIELD_DECL nodes are chained together and the lot of them
   are ultimately passed to `build_struct' to make the RECORD_TYPE node.  */

tree
grokfield (tree declarator, tree declspecs, tree width)
{
  tree value;

  if (declarator == NULL_TREE && width == NULL_TREE)
    {
      /* This is an unnamed decl.

	 If we have something of the form "union { list } ;" then this
	 is the anonymous union extension.  Similarly for struct.

	 If this is something of the form "struct foo;", then
	   If MS extensions are enabled, this is handled as an
	     anonymous struct.
	   Otherwise this is a forward declaration of a structure tag.

	 If this is something of the form "foo;" and foo is a TYPE_DECL, then
	   If MS extensions are enabled and foo names a structure, then
	     again this is an anonymous struct.
	   Otherwise this is an error.

	 Oh what a horrid tangled web we weave.  I wonder if MS consciously
	 took this from Plan 9 or if it was an accident of implementation
	 that took root before someone noticed the bug...  */

      tree type = TREE_VALUE (declspecs);

      if (flag_ms_extensions && TREE_CODE (type) == TYPE_DECL)
	type = TREE_TYPE (type);
      if (TREE_CODE (type) == RECORD_TYPE || TREE_CODE (type) == UNION_TYPE)
	{
	  if (flag_ms_extensions)
	    ; /* ok */
	  else if (flag_iso)
	    goto warn_unnamed_field;
	  else if (TYPE_NAME (type) == NULL)
	    ; /* ok */
	  else
	    goto warn_unnamed_field;
	}
      else
	{
	warn_unnamed_field:
	  warning ("declaration does not declare anything");
	  return NULL_TREE;
	}
    }

  value = grokdeclarator (declarator, declspecs, FIELD, false,
			  width ? &width : NULL);

  finish_decl (value, NULL_TREE, NULL_TREE);
  DECL_INITIAL (value) = width;

  return value;
}

/* Generate an error for any duplicate field names in FIELDLIST.  Munge
   the list such that this does not present a problem later.  */

static void
detect_field_duplicates (tree fieldlist)
{
  tree x, y;
  int timeout = 10;

  /* First, see if there are more than "a few" fields.
     This is trivially true if there are zero or one fields.  */
  if (!fieldlist)
    return;
  x = TREE_CHAIN (fieldlist);
  if (!x)
    return;
  do {
    timeout--;
    x = TREE_CHAIN (x);
  } while (timeout > 0 && x);

  /* If there were "few" fields, avoid the overhead of allocating
     a hash table.  Instead just do the nested traversal thing.  */
  if (timeout > 0)
    {
      for (x = TREE_CHAIN (fieldlist); x ; x = TREE_CHAIN (x))
	if (DECL_NAME (x))
	  {
	    for (y = fieldlist; y != x; y = TREE_CHAIN (y))
	      if (DECL_NAME (y) == DECL_NAME (x))
		{
		  error ("%Jduplicate member '%D'", x, x);
		  DECL_NAME (x) = NULL_TREE;
		}
	  }
    }
  else
    {
      htab_t htab = htab_create (37, htab_hash_pointer, htab_eq_pointer, NULL);
      void **slot;

      for (x = fieldlist; x ; x = TREE_CHAIN (x))
	if ((y = DECL_NAME (x)) != 0)
	  {
	    slot = htab_find_slot (htab, y, INSERT);
	    if (*slot)
	      {
		error ("%Jduplicate member '%D'", x, x);
		DECL_NAME (x) = NULL_TREE;
	      }
	    *slot = y;
	  }

      htab_delete (htab);
    }
}

/* Fill in the fields of a RECORD_TYPE or UNION_TYPE node, T.
   FIELDLIST is a chain of FIELD_DECL nodes for the fields.
   ATTRIBUTES are attributes to be applied to the structure.  */

tree
finish_struct (tree t, tree fieldlist, tree attributes)
{
  tree x;
  bool toplevel = file_scope == current_scope;
  int saw_named_field;

  /* If this type was previously laid out as a forward reference,
     make sure we lay it out again.  */

  TYPE_SIZE (t) = 0;

  decl_attributes (&t, attributes, (int) ATTR_FLAG_TYPE_IN_PLACE);

  if (pedantic)
    {
      for (x = fieldlist; x; x = TREE_CHAIN (x))
	if (DECL_NAME (x) != 0)
	  break;

      if (x == 0)
	pedwarn ("%s has no %s",
		 TREE_CODE (t) == UNION_TYPE ? _("union") : _("struct"),
		 fieldlist ? _("named members") : _("members"));
    }

  /* Install struct as DECL_CONTEXT of each field decl.
     Also process specified field sizes, found in the DECL_INITIAL,
     storing 0 there after the type has been changed to precision equal
     to its width, rather than the precision of the specified standard
     type.  (Correct layout requires the original type to have been preserved
     until now.)  */

  saw_named_field = 0;
  for (x = fieldlist; x; x = TREE_CHAIN (x))
    {
      DECL_CONTEXT (x) = t;
      DECL_PACKED (x) |= TYPE_PACKED (t);

      /* If any field is const, the structure type is pseudo-const.  */
      if (TREE_READONLY (x))
	C_TYPE_FIELDS_READONLY (t) = 1;
      else
	{
	  /* A field that is pseudo-const makes the structure likewise.  */
	  tree t1 = TREE_TYPE (x);
	  while (TREE_CODE (t1) == ARRAY_TYPE)
	    t1 = TREE_TYPE (t1);
	  if ((TREE_CODE (t1) == RECORD_TYPE || TREE_CODE (t1) == UNION_TYPE)
	      && C_TYPE_FIELDS_READONLY (t1))
	    C_TYPE_FIELDS_READONLY (t) = 1;
	}

      /* Any field that is volatile means variables of this type must be
	 treated in some ways as volatile.  */
      if (TREE_THIS_VOLATILE (x))
	C_TYPE_FIELDS_VOLATILE (t) = 1;

      /* Any field of nominal variable size implies structure is too.  */
      if (C_DECL_VARIABLE_SIZE (x))
	C_TYPE_VARIABLE_SIZE (t) = 1;

      /* Detect invalid nested redefinition.  */
      if (TREE_TYPE (x) == t)
	error ("nested redefinition of `%s'",
	       IDENTIFIER_POINTER (TYPE_NAME (t)));

      if (DECL_INITIAL (x))
	{
	  unsigned HOST_WIDE_INT width = tree_low_cst (DECL_INITIAL (x), 1);
	  DECL_SIZE (x) = bitsize_int (width);
	  DECL_BIT_FIELD (x) = 1;
	  SET_DECL_C_BIT_FIELD (x);
	}

      /* Detect flexible array member in an invalid context.  */
      if (TREE_CODE (TREE_TYPE (x)) == ARRAY_TYPE
	  && TYPE_SIZE (TREE_TYPE (x)) == NULL_TREE
	  && TYPE_DOMAIN (TREE_TYPE (x)) != NULL_TREE
	  && TYPE_MAX_VALUE (TYPE_DOMAIN (TREE_TYPE (x))) == NULL_TREE)
	{
	  if (TREE_CODE (t) == UNION_TYPE)
	    {
	      error ("%Jflexible array member in union", x);
	      TREE_TYPE (x) = error_mark_node;
	    }
	  else if (TREE_CHAIN (x) != NULL_TREE)
	    {
	      error ("%Jflexible array member not at end of struct", x);
	      TREE_TYPE (x) = error_mark_node;
	    }
	  else if (! saw_named_field)
	    {
	      error ("%Jflexible array member in otherwise empty struct", x);
	      TREE_TYPE (x) = error_mark_node;
	    }
	}

      if (pedantic && !in_system_header && TREE_CODE (t) == RECORD_TYPE
	  && flexible_array_type_p (TREE_TYPE (x)))
	pedwarn ("%Jinvalid use of structure with flexible array member", x);

      if (DECL_NAME (x))
	saw_named_field = 1;
    }

  detect_field_duplicates (fieldlist);

  /* Now we have the nearly final fieldlist.  Record it,
     then lay out the structure or union (including the fields).  */

  TYPE_FIELDS (t) = fieldlist;

  layout_type (t);

  /* Give bit-fields their proper types.  */
  {
    tree *fieldlistp = &fieldlist;
    while (*fieldlistp)
      if (TREE_CODE (*fieldlistp) == FIELD_DECL && DECL_INITIAL (*fieldlistp)
	  && TREE_TYPE (*fieldlistp) != error_mark_node)
	{
	  unsigned HOST_WIDE_INT width
	    = tree_low_cst (DECL_INITIAL (*fieldlistp), 1);
	  tree type = TREE_TYPE (*fieldlistp);
	  if (width != TYPE_PRECISION (type))
	    TREE_TYPE (*fieldlistp)
	      = build_nonstandard_integer_type (width, TYPE_UNSIGNED (type));
	  DECL_INITIAL (*fieldlistp) = 0;
	}
      else
	fieldlistp = &TREE_CHAIN (*fieldlistp);
  }

  /* Now we have the truly final field list.
     Store it in this type and in the variants.  */

  TYPE_FIELDS (t) = fieldlist;

  /* If there are lots of fields, sort so we can look through them fast.
     We arbitrarily consider 16 or more elts to be "a lot".  */

  {
    int len = 0;

    for (x = fieldlist; x; x = TREE_CHAIN (x))
      {
        if (len > 15 || DECL_NAME (x) == NULL)
          break;
        len += 1;
      }

    if (len > 15)
      {
        tree *field_array;
        struct lang_type *space;
        struct sorted_fields_type *space2;

        len += list_length (x);

        /* Use the same allocation policy here that make_node uses, to
          ensure that this lives as long as the rest of the struct decl.
          All decls in an inline function need to be saved.  */

        space = GGC_CNEW (struct lang_type);
        space2 = GGC_NEWVAR (struct sorted_fields_type,
			     sizeof (struct sorted_fields_type) + len * sizeof (tree));

        len = 0;
	space->s = space2;
	field_array = &space2->elts[0];
        for (x = fieldlist; x; x = TREE_CHAIN (x))
          {
            field_array[len++] = x;

            /* If there is anonymous struct or union, break out of the loop.  */
            if (DECL_NAME (x) == NULL)
              break;
          }
        /* Found no anonymous struct/union.  Add the TYPE_LANG_SPECIFIC.  */
        if (x == NULL)
          {
            TYPE_LANG_SPECIFIC (t) = space;
            TYPE_LANG_SPECIFIC (t)->s->len = len;
            field_array = TYPE_LANG_SPECIFIC (t)->s->elts;
            qsort (field_array, len, sizeof (tree), field_decl_cmp);
          }
      }
  }

  for (x = TYPE_MAIN_VARIANT (t); x; x = TYPE_NEXT_VARIANT (x))
    {
      TYPE_FIELDS (x) = TYPE_FIELDS (t);
      TYPE_LANG_SPECIFIC (x) = TYPE_LANG_SPECIFIC (t);
      TYPE_ALIGN (x) = TYPE_ALIGN (t);
      TYPE_USER_ALIGN (x) = TYPE_USER_ALIGN (t);
    }

  /* If this was supposed to be a transparent union, but we can't
     make it one, warn and turn off the flag.  */
  if (TREE_CODE (t) == UNION_TYPE
      && TYPE_TRANSPARENT_UNION (t)
      && TYPE_MODE (t) != DECL_MODE (TYPE_FIELDS (t)))
    {
      TYPE_TRANSPARENT_UNION (t) = 0;
      warning ("union cannot be made transparent");
    }

  /* If this structure or union completes the type of any previous
     variable declaration, lay it out and output its rtl.  */
  for (x = C_TYPE_INCOMPLETE_VARS (TYPE_MAIN_VARIANT (t));
       x;
       x = TREE_CHAIN (x))
    {
      tree decl = TREE_VALUE (x);
      if (TREE_CODE (TREE_TYPE (decl)) == ARRAY_TYPE)
	layout_array_type (TREE_TYPE (decl));
      if (TREE_CODE (decl) != TYPE_DECL)
	{
	  layout_decl (decl, 0);
	  if (c_dialect_objc ())
	    objc_check_decl (decl);
	  rest_of_decl_compilation (decl, toplevel, 0);
	  if (! toplevel)
	    expand_decl (decl);
	}
    }
  C_TYPE_INCOMPLETE_VARS (TYPE_MAIN_VARIANT (t)) = 0;

  /* Finish debugging output for this type.  */
  rest_of_type_compilation (t, toplevel);

  return t;
}

/* Lay out the type T, and its element type, and so on.  */

static void
layout_array_type (tree t)
{
  if (TREE_CODE (TREE_TYPE (t)) == ARRAY_TYPE)
    layout_array_type (TREE_TYPE (t));
  layout_type (t);
}

/* Begin compiling the definition of an enumeration type.
   NAME is its name (or null if anonymous).
   Returns the type object, as yet incomplete.
   Also records info about it so that build_enumerator
   may be used to declare the individual values as they are read.  */

tree
start_enum (tree name)
{
  tree enumtype = 0;

  /* If this is the real definition for a previous forward reference,
     fill in the contents in the same object that used to be the
     forward reference.  */

  if (name != 0)
    enumtype = lookup_tag (ENUMERAL_TYPE, name, 1);

  if (enumtype == 0 || TREE_CODE (enumtype) != ENUMERAL_TYPE)
    {
      enumtype = make_node (ENUMERAL_TYPE);
      pushtag (name, enumtype);
    }

  C_TYPE_BEING_DEFINED (enumtype) = 1;

  if (TYPE_VALUES (enumtype) != 0)
    {
      /* This enum is a named one that has been declared already.  */
      error ("redeclaration of `enum %s'", IDENTIFIER_POINTER (name));

      /* Completely replace its old definition.
	 The old enumerators remain defined, however.  */
      TYPE_VALUES (enumtype) = 0;
    }

  enum_next_value = integer_zero_node;
  enum_overflow = 0;

  if (flag_short_enums)
    TYPE_PACKED (enumtype) = 1;

  return enumtype;
}

/* After processing and defining all the values of an enumeration type,
   install their decls in the enumeration type and finish it off.
   ENUMTYPE is the type object, VALUES a list of decl-value pairs,
   and ATTRIBUTES are the specified attributes.
   Returns ENUMTYPE.  */

tree
finish_enum (tree enumtype, tree values, tree attributes)
{
  tree pair, tem;
  tree minnode = 0, maxnode = 0;
  int precision, unsign;
  bool toplevel = (file_scope == current_scope);
  struct lang_type *lt;

  decl_attributes (&enumtype, attributes, (int) ATTR_FLAG_TYPE_IN_PLACE);

  /* Calculate the maximum value of any enumerator in this type.  */

  if (values == error_mark_node)
    minnode = maxnode = integer_zero_node;
  else
    {
      minnode = maxnode = TREE_VALUE (values);
      for (pair = TREE_CHAIN (values); pair; pair = TREE_CHAIN (pair))
	{
	  tree value = TREE_VALUE (pair);
	  if (tree_int_cst_lt (maxnode, value))
	    maxnode = value;
	  if (tree_int_cst_lt (value, minnode))
	    minnode = value;
	}
    }

  /* Construct the final type of this enumeration.  It is the same
     as one of the integral types - the narrowest one that fits, except
     that normally we only go as narrow as int - and signed iff any of
     the values are negative.  */
  unsign = (tree_int_cst_sgn (minnode) >= 0);
  precision = MAX (min_precision (minnode, unsign),
		   min_precision (maxnode, unsign));
  if (TYPE_PACKED (enumtype) || precision > TYPE_PRECISION (integer_type_node))
    {
      tem = c_common_type_for_size (precision, unsign);
      if (tem == NULL)
	{
	  warning ("enumeration values exceed range of largest integer");
	  tem = long_long_integer_type_node;
	}
    }
  else
    tem = unsign ? unsigned_type_node : integer_type_node;

  TYPE_MIN_VALUE (enumtype) = TYPE_MIN_VALUE (tem);
  TYPE_MAX_VALUE (enumtype) = TYPE_MAX_VALUE (tem);
  TYPE_PRECISION (enumtype) = TYPE_PRECISION (tem);
  TYPE_UNSIGNED (enumtype) = TYPE_UNSIGNED (tem);
  TYPE_SIZE (enumtype) = 0;
  layout_type (enumtype);

  if (values != error_mark_node)
    {
      /* Change the type of the enumerators to be the enum type.  We
	 need to do this irrespective of the size of the enum, for
	 proper type checking.  Replace the DECL_INITIALs of the
	 enumerators, and the value slots of the list, with copies
	 that have the enum type; they cannot be modified in place
	 because they may be shared (e.g.  integer_zero_node) Finally,
	 change the purpose slots to point to the names of the decls.  */
      for (pair = values; pair; pair = TREE_CHAIN (pair))
	{
	  tree enu = TREE_PURPOSE (pair);
	  tree ini = DECL_INITIAL (enu);

	  TREE_TYPE (enu) = enumtype;

	  /* The ISO C Standard mandates enumerators to have type int,
	     even though the underlying type of an enum type is
	     unspecified.  Here we convert any enumerators that fit in
	     an int to type int, to avoid promotions to unsigned types
	     when comparing integers with enumerators that fit in the
	     int range.  When -pedantic is given, build_enumerator()
	     would have already taken care of those that don't fit.  */
	  if (int_fits_type_p (ini, integer_type_node))
	    tem = integer_type_node;
	  else
	    tem = enumtype;
	  ini = convert (tem, ini);

	  DECL_INITIAL (enu) = ini;
	  TREE_PURPOSE (pair) = DECL_NAME (enu);
	  TREE_VALUE (pair) = ini;
	}

      TYPE_VALUES (enumtype) = values;
    }

  /* Record the min/max values so that we can warn about bit-field
     enumerations that are too small for the values.  */
  lt = GGC_CNEW (struct lang_type);
  lt->enum_min = minnode;
  lt->enum_max = maxnode;
  TYPE_LANG_SPECIFIC (enumtype) = lt;

  /* Fix up all variant types of this enum type.  */
  for (tem = TYPE_MAIN_VARIANT (enumtype); tem; tem = TYPE_NEXT_VARIANT (tem))
    {
      if (tem == enumtype)
	continue;
      TYPE_VALUES (tem) = TYPE_VALUES (enumtype);
      TYPE_MIN_VALUE (tem) = TYPE_MIN_VALUE (enumtype);
      TYPE_MAX_VALUE (tem) = TYPE_MAX_VALUE (enumtype);
      TYPE_SIZE (tem) = TYPE_SIZE (enumtype);
      TYPE_SIZE_UNIT (tem) = TYPE_SIZE_UNIT (enumtype);
      TYPE_MODE (tem) = TYPE_MODE (enumtype);
      TYPE_PRECISION (tem) = TYPE_PRECISION (enumtype);
      TYPE_ALIGN (tem) = TYPE_ALIGN (enumtype);
      TYPE_USER_ALIGN (tem) = TYPE_USER_ALIGN (enumtype);
      TYPE_UNSIGNED (tem) = TYPE_UNSIGNED (enumtype);
      TYPE_LANG_SPECIFIC (tem) = TYPE_LANG_SPECIFIC (enumtype);
    }

  /* Finish debugging output for this type.  */
  rest_of_type_compilation (enumtype, toplevel);

  return enumtype;
}

/* Build and install a CONST_DECL for one value of the
   current enumeration type (one that was begun with start_enum).
   Return a tree-list containing the CONST_DECL and its value.
   Assignment of sequential values by default is handled here.  */

tree
build_enumerator (tree name, tree value)
{
  tree decl, type;

  /* Validate and default VALUE.  */

  /* Remove no-op casts from the value.  */
  if (value)
    STRIP_TYPE_NOPS (value);

  if (value != 0)
    {
      /* Don't issue more errors for error_mark_node (i.e. an
	 undeclared identifier) - just ignore the value expression.  */
      if (value == error_mark_node)
	value = 0;
      else if (TREE_CODE (value) != INTEGER_CST)
	{
	  error ("enumerator value for '%E' is not an integer constant", name);
	  value = 0;
	}
      else
	{
	  value = default_conversion (value);
	  constant_expression_warning (value);
	}
    }

  /* Default based on previous value.  */
  /* It should no longer be possible to have NON_LVALUE_EXPR
     in the default.  */
  if (value == 0)
    {
      value = enum_next_value;
      if (enum_overflow)
	error ("overflow in enumeration values");
    }

  if (pedantic && ! int_fits_type_p (value, integer_type_node))
    {
      pedwarn ("ISO C restricts enumerator values to range of `int'");
      /* XXX This causes -pedantic to change the meaning of the program.
	 Remove?  -zw 2004-03-15  */
      value = convert (integer_type_node, value);
    }

  /* Set basis for default for next value.  */
  enum_next_value = build_binary_op (PLUS_EXPR, value, integer_one_node, 0);
  enum_overflow = tree_int_cst_lt (enum_next_value, value);

  /* Now create a declaration for the enum value name.  */

  type = TREE_TYPE (value);
  type = c_common_type_for_size (MAX (TYPE_PRECISION (type),
				      TYPE_PRECISION (integer_type_node)),
				 (TYPE_PRECISION (type)
				  >= TYPE_PRECISION (integer_type_node)
				  && TYPE_UNSIGNED (type)));

  decl = build_decl (CONST_DECL, name, type);
  DECL_INITIAL (decl) = convert (type, value);
  pushdecl (decl);

  return tree_cons (decl, value, NULL_TREE);
}


/* Create the FUNCTION_DECL for a function definition.
   DECLSPECS, DECLARATOR and ATTRIBUTES are the parts of
   the declaration; they describe the function's name and the type it returns,
   but twisted together in a fashion that parallels the syntax of C.

   This function creates a binding context for the function body
   as well as setting up the FUNCTION_DECL in current_function_decl.

   Returns 1 on success.  If the DECLARATOR is not suitable for a function
   (it defines a datum instead), we return 0, which tells
   yyparse to report a parse error.  */

int
start_function (tree declspecs, tree declarator, tree attributes)
{
  tree decl1, old_decl;
  tree restype, resdecl;

  current_function_returns_value = 0;  /* Assume, until we see it does.  */
  current_function_returns_null = 0;
  current_function_returns_abnormally = 0;
  warn_about_return_type = 0;
  current_extern_inline = 0;
  c_switch_stack = NULL;

  /* Indicate no valid break/continue context by setting these variables
     to some non-null, non-label value.  We'll notice and emit the proper
     error message in c_finish_bc_stmt.  */
  c_break_label = c_cont_label = size_zero_node;

  decl1 = grokdeclarator (declarator, declspecs, FUNCDEF, true, NULL);

  /* If the declarator is not suitable for a function definition,
     cause a syntax error.  */
  if (decl1 == 0)
    return 0;

  /* APPLE LOCAL begin weak import (Radar 2809704) --ilr */
  decl_attributes (&decl1, attributes, (int)ATTR_FLAG_FUNCTION_DEF);
  /* APPLE LOCAL end weak import --ilr */

  if (DECL_DECLARED_INLINE_P (decl1)
      && DECL_UNINLINABLE (decl1)
      && lookup_attribute ("noinline", DECL_ATTRIBUTES (decl1)))
    warning ("%Jinline function '%D' given attribute noinline", decl1, decl1);

  announce_function (decl1);

  if (!COMPLETE_OR_VOID_TYPE_P (TREE_TYPE (TREE_TYPE (decl1))))
    {
      error ("return type is an incomplete type");
      /* Make it return void instead.  */
      TREE_TYPE (decl1)
	= build_function_type (void_type_node,
			       TYPE_ARG_TYPES (TREE_TYPE (decl1)));
    }

  if (warn_about_return_type)
    pedwarn_c99 ("return type defaults to `int'");

  /* Make the init_value nonzero so pushdecl knows this is not tentative.
     error_mark_node is replaced below (in pop_scope) with the BLOCK.  */
  DECL_INITIAL (decl1) = error_mark_node;

  /* If this definition isn't a prototype and we had a prototype declaration
     before, copy the arg type info from that prototype.
     But not if what we had before was a builtin function.  */
  old_decl = lookup_name_in_scope (DECL_NAME (decl1), current_scope);
  if (old_decl != 0 && TREE_CODE (TREE_TYPE (old_decl)) == FUNCTION_TYPE
      && !DECL_BUILT_IN (old_decl)
      && comptypes (TREE_TYPE (TREE_TYPE (decl1)),
		    TREE_TYPE (TREE_TYPE (old_decl)))
      && TYPE_ARG_TYPES (TREE_TYPE (decl1)) == 0)
    {
      TREE_TYPE (decl1) = composite_type (TREE_TYPE (old_decl),
					  TREE_TYPE (decl1));
      current_function_prototype_locus = DECL_SOURCE_LOCATION (old_decl);
    }

  /* Optionally warn of old-fashioned def with no previous prototype.  */
  if (warn_strict_prototypes
      && TYPE_ARG_TYPES (TREE_TYPE (decl1)) == 0
      && C_DECL_ISNT_PROTOTYPE (old_decl))
    warning ("function declaration isn't a prototype");
  /* Optionally warn of any global def with no previous prototype.  */
  else if (warn_missing_prototypes
	   && TREE_PUBLIC (decl1)
	   && ! MAIN_NAME_P (DECL_NAME (decl1))
	   && C_DECL_ISNT_PROTOTYPE (old_decl))
    warning ("%Jno previous prototype for '%D'", decl1, decl1);
  /* Optionally warn of any def with no previous prototype
     if the function has already been used.  */
  else if (warn_missing_prototypes
	   && old_decl != 0 && TREE_USED (old_decl)
	   && TYPE_ARG_TYPES (TREE_TYPE (old_decl)) == 0)
    warning ("%J'%D' was used with no prototype before its definition",
	     decl1, decl1);
  /* Optionally warn of any global def with no previous declaration.  */
  else if (warn_missing_declarations
	   && TREE_PUBLIC (decl1)
	   && old_decl == 0
	   && ! MAIN_NAME_P (DECL_NAME (decl1)))
    warning ("%Jno previous declaration for '%D'", decl1, decl1);
  /* Optionally warn of any def with no previous declaration
     if the function has already been used.  */
  else if (warn_missing_declarations
	   && old_decl != 0 && TREE_USED (old_decl)
	   && C_DECL_IMPLICIT (old_decl))
    warning ("%J`%D' was used with no declaration before its definition",
	     decl1, decl1);

  /* This is a definition, not a reference.
     So normally clear DECL_EXTERNAL.
     However, `extern inline' acts like a declaration
     except for defining how to inline.  So set DECL_EXTERNAL in that case.  */
  DECL_EXTERNAL (decl1) = current_extern_inline;

  /* This function exists in static storage.
     (This does not mean `static' in the C sense!)  */
  TREE_STATIC (decl1) = 1;

  /* A nested function is not global.  */
  if (current_function_decl != 0)
    TREE_PUBLIC (decl1) = 0;

#ifdef ENABLE_CHECKING
  /* This is the earliest point at which we might know the assembler
     name of the function.  Thus, if it's set before this, die horribly.  */
  if (DECL_ASSEMBLER_NAME_SET_P (decl1))
    abort ();
#endif

  /* If #pragma weak was used, mark the decl weak now.  */
  if (current_scope == file_scope)
    maybe_apply_pragma_weak (decl1);

  /* Warn for unlikely, improbable, or stupid declarations of `main'.  */
  if (warn_main > 0 && MAIN_NAME_P (DECL_NAME (decl1)))
    {
      tree args;
      int argct = 0;

      if (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (decl1)))
	  != integer_type_node)
	pedwarn ("%Jreturn type of '%D' is not `int'", decl1, decl1);

      for (args = TYPE_ARG_TYPES (TREE_TYPE (decl1)); args;
	   args = TREE_CHAIN (args))
	{
	  tree type = args ? TREE_VALUE (args) : 0;

	  if (type == void_type_node)
	    break;

	  ++argct;
	  switch (argct)
	    {
	    case 1:
	      if (TYPE_MAIN_VARIANT (type) != integer_type_node)
		pedwarn ("%Jfirst argument of '%D' should be `int'",
			 decl1, decl1);
	      break;

	    case 2:
	      if (TREE_CODE (type) != POINTER_TYPE
		  || TREE_CODE (TREE_TYPE (type)) != POINTER_TYPE
		  || (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (type)))
		      != char_type_node))
		pedwarn ("%Jsecond argument of '%D' should be 'char **'",
                         decl1, decl1);
	      break;

	    case 3:
	      if (TREE_CODE (type) != POINTER_TYPE
		  || TREE_CODE (TREE_TYPE (type)) != POINTER_TYPE
		  || (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (type)))
		      != char_type_node))
		pedwarn ("%Jthird argument of '%D' should probably be "
                         "'char **'", decl1, decl1);
	      break;
	    }
	}

      /* It is intentional that this message does not mention the third
	 argument because it's only mentioned in an appendix of the
	 standard.  */
      if (argct > 0 && (argct < 2 || argct > 3))
	pedwarn ("%J'%D' takes only zero or two arguments", decl1, decl1);

      if (! TREE_PUBLIC (decl1))
	pedwarn ("%J'%D' is normally a non-static function", decl1, decl1);
    }

  /* Record the decl so that the function name is defined.
     If we already have a decl for this name, and it is a FUNCTION_DECL,
     use the old decl.  */

  current_function_decl = pushdecl (decl1);

  push_scope ();
  declare_parm_level ();

  restype = TREE_TYPE (TREE_TYPE (current_function_decl));
  /* Promote the value to int before returning it.  */
  if (c_promoting_integer_type_p (restype))
    {
      /* It retains unsignedness if not really getting wider.  */
      if (TYPE_UNSIGNED (restype)
	  && (TYPE_PRECISION (restype)
		  == TYPE_PRECISION (integer_type_node)))
	restype = unsigned_type_node;
      else
	restype = integer_type_node;
    }

  resdecl = build_decl (RESULT_DECL, NULL_TREE, restype);
  DECL_ARTIFICIAL (resdecl) = 1;
  DECL_IGNORED_P (resdecl) = 1;
  DECL_RESULT (current_function_decl) = resdecl;

  /* APPLE LOCAL begin CW asm blocks */
  /* If this was a function declared as an assembly function, change
     the state to expect to see C decls, possibly followed by assembly
     code.  */
  if (DECL_CW_ASM_FUNCTION (current_function_decl))
    {
      cw_asm_state = cw_asm_decls;
      cw_asm_in_decl = 0;
    }
  /* APPLE LOCAL end CW asm blocks */
  start_fname_decls ();

  return 1;
}

/* Subroutine of store_parm_decls which handles new-style function
   definitions (prototype format). The parms already have decls, so we
   need only record them as in effect and complain if any redundant
   old-style parm decls were written.  */
static void
store_parm_decls_newstyle (tree fndecl, tree arg_info)
{
  tree decl;
  tree parms  = ARG_INFO_PARMS  (arg_info);
  tree tags   = ARG_INFO_TAGS   (arg_info);
  tree others = ARG_INFO_OTHERS (arg_info);

  if (current_scope->bindings)
    {
      error ("%Jold-style parameter declarations in prototyped "
	     "function definition", fndecl);

      /* Get rid of the old-style declarations.  */
      pop_scope ();
      push_scope ();
    }
  /* Don't issue this warning for nested functions, and don't issue this
     warning if we got here because ARG_INFO_TYPES was error_mark_node
     (this happens when a function definition has just an ellipsis in
     its parameter list).  */
  else if (warn_traditional && !in_system_header && !current_function_scope
	   && ARG_INFO_TYPES (arg_info) != error_mark_node)
    warning ("%Jtraditional C rejects ISO C style function definitions",
	     fndecl);

  /* Now make all the parameter declarations visible in the function body.
     We can bypass most of the grunt work of pushdecl.  */
  for (decl = parms; decl; decl = TREE_CHAIN (decl))
    {
      DECL_CONTEXT (decl) = current_function_decl;
      if (DECL_NAME (decl))
	bind (DECL_NAME (decl), decl, current_scope,
	      /*invisible=*/false, /*nested=*/false);
      else
	error ("%Jparameter name omitted", decl);
    }

  /* Record the parameter list in the function declaration.  */
  DECL_ARGUMENTS (fndecl) = parms;

  /* Now make all the ancillary declarations visible, likewise.  */
  for (decl = others; decl; decl = TREE_CHAIN (decl))
    {
      DECL_CONTEXT (decl) = current_function_decl;
      if (DECL_NAME (decl))
	bind (DECL_NAME (decl), decl, current_scope,
	      /*invisible=*/false, /*nested=*/false);
    }

  /* And all the tag declarations.  */
  for (decl = tags; decl; decl = TREE_CHAIN (decl))
    if (TREE_PURPOSE (decl))
      bind (TREE_PURPOSE (decl), TREE_VALUE (decl), current_scope,
	    /*invisible=*/false, /*nested=*/false);
}

/* Subroutine of store_parm_decls which handles old-style function
   definitions (separate parameter list and declarations).  */

static void
store_parm_decls_oldstyle (tree fndecl, tree arg_info)
{
  struct c_binding *b;
  tree parm, decl, last;
  tree parmids = ARG_INFO_PARMS (arg_info);

  /* We use DECL_WEAK as a flag to show which parameters have been
     seen already, since it is not used on PARM_DECL.  */
#ifdef ENABLE_CHECKING
  for (b = current_scope->bindings; b; b = b->prev)
    if (TREE_CODE (b->decl) == PARM_DECL && DECL_WEAK (b->decl))
      abort ();
#endif

  if (warn_old_style_definition && !in_system_header)
    warning ("%Jold-style function definition", fndecl);

  /* Match each formal parameter name with its declaration.  Save each
     decl in the appropriate TREE_PURPOSE slot of the parmids chain.  */
  for (parm = parmids; parm; parm = TREE_CHAIN (parm))
    {
      if (TREE_VALUE (parm) == 0)
	{
	  error ("%Jparameter name missing from parameter list", fndecl);
	  TREE_PURPOSE (parm) = 0;
	  continue;
	}

      b = I_SYMBOL_BINDING (TREE_VALUE (parm));
      if (b && B_IN_CURRENT_SCOPE (b))
	{
	  decl = b->decl;
	  /* If we got something other than a PARM_DECL it is an error.  */
	  if (TREE_CODE (decl) != PARM_DECL)
	    error ("%J'%D' declared as a non-parameter", decl, decl);
	  /* If the declaration is already marked, we have a duplicate
	     name.  Complain and ignore the duplicate.  */
	  else if (DECL_WEAK (decl))
	    {
	      error ("%Jmultiple parameters named '%D'", decl, decl);
	      TREE_PURPOSE (parm) = 0;
	      continue;
	    }
	  /* If the declaration says "void", complain and turn it into
	     an int.  */
	  else if (VOID_TYPE_P (TREE_TYPE (decl)))
	    {
	      error ("%Jparameter '%D' declared with void type", decl, decl);
	      TREE_TYPE (decl) = integer_type_node;
	      DECL_ARG_TYPE (decl) = integer_type_node;
	      layout_decl (decl, 0);
	    }
	}
      /* If no declaration found, default to int.  */
      else
	{
	  decl = build_decl (PARM_DECL, TREE_VALUE (parm), integer_type_node);
	  DECL_ARG_TYPE (decl) = TREE_TYPE (decl);
	  DECL_SOURCE_LOCATION (decl) = DECL_SOURCE_LOCATION (fndecl);
	  pushdecl (decl);

	  if (flag_isoc99)
	    pedwarn ("%Jtype of '%D' defaults to 'int'", decl, decl);
	  else if (extra_warnings)
	    warning ("%Jtype of '%D' defaults to 'int'", decl, decl);
	}

      TREE_PURPOSE (parm) = decl;
      DECL_WEAK (decl) = 1;
    }

  /* Now examine the parms chain for incomplete declarations
     and declarations with no corresponding names.  */

  for (b = current_scope->bindings; b; b = b->prev)
    {
      parm = b->decl;
      if (TREE_CODE (parm) != PARM_DECL)
	continue;

      if (!COMPLETE_TYPE_P (TREE_TYPE (parm)))
	{
	  error ("%Jparameter '%D' has incomplete type", parm, parm);
	  TREE_TYPE (parm) = error_mark_node;
	}

      if (! DECL_WEAK (parm))
	{
	  error ("%Jdeclaration for parameter '%D' but no such parameter",
		 parm, parm);

	  /* Pretend the parameter was not missing.
	     This gets us to a standard state and minimizes
	     further error messages.  */
	  parmids = chainon (parmids, tree_cons (parm, 0, 0));
	}
    }

  /* Chain the declarations together in the order of the list of
     names.  Store that chain in the function decl, replacing the
     list of names.  Update the current scope to match.  */
  DECL_ARGUMENTS (fndecl) = 0;

  for (parm = parmids; parm; parm = TREE_CHAIN (parm))
    if (TREE_PURPOSE (parm))
      break;
  if (parm && TREE_PURPOSE (parm))
    {
      last = TREE_PURPOSE (parm);
      DECL_ARGUMENTS (fndecl) = last;
      DECL_WEAK (last) = 0;

      for (parm = TREE_CHAIN (parm); parm; parm = TREE_CHAIN (parm))
	if (TREE_PURPOSE (parm))
	  {
	    TREE_CHAIN (last) = TREE_PURPOSE (parm);
	    last = TREE_PURPOSE (parm);
	    DECL_WEAK (last) = 0;
	  }
      TREE_CHAIN (last) = 0;
    }

  /* If there was a previous prototype,
     set the DECL_ARG_TYPE of each argument according to
     the type previously specified, and report any mismatches.  */

  if (TYPE_ARG_TYPES (TREE_TYPE (fndecl)))
    {
      tree type;
      for (parm = DECL_ARGUMENTS (fndecl),
	     type = TYPE_ARG_TYPES (TREE_TYPE (fndecl));
	   parm || (type && (TYPE_MAIN_VARIANT (TREE_VALUE (type))
			     != void_type_node));
	   parm = TREE_CHAIN (parm), type = TREE_CHAIN (type))
	{
	  if (parm == 0 || type == 0
	      || TYPE_MAIN_VARIANT (TREE_VALUE (type)) == void_type_node)
	    {
	      error ("number of arguments doesn't match prototype");
	      error ("%Hprototype declaration",
		     &current_function_prototype_locus);
	      break;
	    }
	  /* Type for passing arg must be consistent with that
	     declared for the arg.  ISO C says we take the unqualified
	     type for parameters declared with qualified type.  */
	  if (! comptypes (TYPE_MAIN_VARIANT (DECL_ARG_TYPE (parm)),
			   TYPE_MAIN_VARIANT (TREE_VALUE (type))))
	    {
	      if (TYPE_MAIN_VARIANT (TREE_TYPE (parm))
		  == TYPE_MAIN_VARIANT (TREE_VALUE (type)))
		{
		  /* Adjust argument to match prototype.  E.g. a previous
		     `int foo(float);' prototype causes
		     `int foo(x) float x; {...}' to be treated like
		     `int foo(float x) {...}'.  This is particularly
		     useful for argument types like uid_t.  */
		  DECL_ARG_TYPE (parm) = TREE_TYPE (parm);

		  if (targetm.calls.promote_prototypes (TREE_TYPE (current_function_decl))
		      && INTEGRAL_TYPE_P (TREE_TYPE (parm))
		      && TYPE_PRECISION (TREE_TYPE (parm))
		      < TYPE_PRECISION (integer_type_node))
		    DECL_ARG_TYPE (parm) = integer_type_node;

		  if (pedantic)
		    {
		      pedwarn ("promoted argument '%D' "
			       "doesn't match prototype", parm);
		      pedwarn ("%Hprototype declaration",
			       &current_function_prototype_locus);
		    }
		}
	      else
		{
		  error ("argument '%D' doesn't match prototype", parm);
		  error ("%Hprototype declaration",
			 &current_function_prototype_locus);
		}
	    }
	}
      TYPE_ACTUAL_ARG_TYPES (TREE_TYPE (fndecl)) = 0;
    }

  /* Otherwise, create a prototype that would match.  */

  else
    {
      tree actual = 0, last = 0, type;

      for (parm = DECL_ARGUMENTS (fndecl); parm; parm = TREE_CHAIN (parm))
	{
	  type = tree_cons (NULL_TREE, DECL_ARG_TYPE (parm), NULL_TREE);
	  if (last)
	    TREE_CHAIN (last) = type;
	  else
	    actual = type;
	  last = type;
	}
      type = tree_cons (NULL_TREE, void_type_node, NULL_TREE);
      if (last)
	TREE_CHAIN (last) = type;
      else
	actual = type;

      /* We are going to assign a new value for the TYPE_ACTUAL_ARG_TYPES
	 of the type of this function, but we need to avoid having this
	 affect the types of other similarly-typed functions, so we must
	 first force the generation of an identical (but separate) type
	 node for the relevant function type.  The new node we create
	 will be a variant of the main variant of the original function
	 type.  */

      TREE_TYPE (fndecl) = build_type_copy (TREE_TYPE (fndecl));

      TYPE_ACTUAL_ARG_TYPES (TREE_TYPE (fndecl)) = actual;
    }
}

/* Store the parameter declarations into the current function declaration.
   This is called after parsing the parameter declarations, before
   digesting the body of the function.

   For an old-style definition, construct a prototype out of the old-style
   parameter declarations and inject it into the function's type.  */

void
store_parm_decls (void)
{
  tree fndecl = current_function_decl;

  /* The argument information block for FNDECL.  */
  tree arg_info = DECL_ARGUMENTS (fndecl);

  /* True if this definition is written with a prototype.  Note:
     despite C99 6.7.5.3p14, we can *not* treat an empty argument
     list in a function definition as equivalent to (void) -- an
     empty argument list specifies the function has no parameters,
     but only (void) sets up a prototype for future calls.  */
  bool proto = ARG_INFO_TYPES (arg_info) != 0;

  if (proto)
    store_parm_decls_newstyle (fndecl, arg_info);
  else
    store_parm_decls_oldstyle (fndecl, arg_info);

  /* The next call to push_scope will be a function body.  */

  next_is_function_body = true;

  /* Write a record describing this function definition to the prototypes
     file (if requested).  */

  gen_aux_info_record (fndecl, 1, 0, proto);

  /* Initialize the RTL code for the function.  */
  allocate_struct_function (fndecl);

  /* Begin the statement tree for this function.  */
  DECL_SAVED_TREE (fndecl) = push_stmt_list ();

  /* ??? Insert the contents of the pending sizes list into the function
     to be evaluated.  This just changes mis-behaviour until assign_parms
     phase ordering problems are resolved.  */
  {
    tree t;
    for (t = nreverse (get_pending_sizes ()); t ; t = TREE_CHAIN (t))
      add_stmt (TREE_VALUE (t));
  }

  /* Even though we're inside a function body, we still don't want to
     call expand_expr to calculate the size of a variable-sized array.
     We haven't necessarily assigned RTL to all variables yet, so it's
     not safe to try to expand expressions involving them.  */
  cfun->x_dont_save_pending_sizes_p = 1;
}

/* Give FNDECL and all its nested functions to cgraph for compilation.  */

static void
c_finalize (tree fndecl)
{
  struct cgraph_node *cgn;

  /* Handle attribute((warn_unused_result)).  Relies on gimple input.  */
  c_warn_unused_result (&DECL_SAVED_TREE (fndecl));

  /* ??? Objc emits functions after finalizing the compilation unit.
     This should be cleaned up later and this conditional removed.  */
  if (cgraph_global_info_ready)
    {
      c_expand_body (fndecl);
      return;
    }

  /* Finalize all nested functions now.  */
  cgn = cgraph_node (fndecl);
  for (cgn = cgn->nested; cgn ; cgn = cgn->next_nested)
    c_finalize (cgn->decl);

  cgraph_finalize_function (fndecl, false);
}

/* Finish up a function declaration and compile that function
   all the way to assembler language output.  The free the storage
   for the function definition.

   This is called after parsing the body of the function definition.  */

void
finish_function (void)
{
  tree fndecl = current_function_decl;

  if (TREE_CODE (fndecl) == FUNCTION_DECL
      && targetm.calls.promote_prototypes (TREE_TYPE (fndecl)))
    {
      tree args = DECL_ARGUMENTS (fndecl);
      for (; args; args = TREE_CHAIN (args))
 	{
 	  tree type = TREE_TYPE (args);
 	  if (INTEGRAL_TYPE_P (type)
 	      && TYPE_PRECISION (type) < TYPE_PRECISION (integer_type_node))
 	    DECL_ARG_TYPE (args) = integer_type_node;
 	}
    }

  if (DECL_INITIAL (fndecl) && DECL_INITIAL (fndecl) != error_mark_node)
    BLOCK_SUPERCONTEXT (DECL_INITIAL (fndecl)) = fndecl;

  /* Must mark the RESULT_DECL as being in this function.  */

  if (DECL_RESULT (fndecl) && DECL_RESULT (fndecl) != error_mark_node)
    DECL_CONTEXT (DECL_RESULT (fndecl)) = fndecl;

  if (MAIN_NAME_P (DECL_NAME (fndecl)) && flag_hosted)
    {
      if (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (fndecl)))
	  != integer_type_node)
	{
	  /* If warn_main is 1 (-Wmain) or 2 (-Wall), we have already warned.
	     If warn_main is -1 (-Wno-main) we don't want to be warned.  */
	  if (!warn_main)
	    pedwarn ("%Jreturn type of '%D' is not `int'", fndecl, fndecl);
	}
      else
	{
	  if (flag_isoc99)
	    c_finish_return (integer_zero_node);
	}
    }

  /* Tie off the statement tree for this function.  */
  DECL_SAVED_TREE (fndecl) = pop_stmt_list (DECL_SAVED_TREE (fndecl));

  finish_fname_decls ();

  /* Complain if there's just no return statement.  */
  if (warn_return_type
      && TREE_CODE (TREE_TYPE (TREE_TYPE (fndecl))) != VOID_TYPE
      && !current_function_returns_value && !current_function_returns_null
      /* Don't complain if we abort.  */
      && !current_function_returns_abnormally
      /* Don't warn for main().  */
      && !MAIN_NAME_P (DECL_NAME (fndecl))
      /* Or if they didn't actually specify a return type.  */
      && !C_FUNCTION_IMPLICIT_INT (fndecl)
      /* Normally, with -Wreturn-type, flow will complain.  Unless we're an
	 inline function, as we might never be compiled separately.  */
      && DECL_INLINE (fndecl))
    warning ("no return statement in function returning non-void");

  /* With just -Wextra, complain only if function returns both with
     and without a value.  */
  if (extra_warnings
      && current_function_returns_value
      && current_function_returns_null)
    warning ("this function may return with or without a value");

  /* APPLE LOCAL begin loop transposition */
  /* Perform loop tranformations before doing inlining, but do not 
     do it if syntax only is requested. */
  if (!flag_syntax_only && flag_loop_transpose)
    loop_transpose(fndecl);
  /* APPLE LOCAL end loop transposition */

  /* Store the end of the function, so that we get good line number
     info for the epilogue.  */
  cfun->function_end_locus = input_location;

  /* If we don't have ctors/dtors sections, and this is a static
     constructor or destructor, it must be recorded now.  */
  if (DECL_STATIC_CONSTRUCTOR (fndecl)
      && !targetm.have_ctors_dtors)
    static_ctors = tree_cons (NULL_TREE, fndecl, static_ctors);
  if (DECL_STATIC_DESTRUCTOR (fndecl)
      && !targetm.have_ctors_dtors)
    static_dtors = tree_cons (NULL_TREE, fndecl, static_dtors);

  /* Finalize the ELF visibility for the function.  */
  c_determine_visibility (fndecl);

  /* Genericize before inlining.  Delay genericizing nested functions
     until their parent function is genericized.  Since finalizing
     requires GENERIC, delay that as well.  */

  if (DECL_INITIAL (fndecl) && DECL_INITIAL (fndecl) != error_mark_node)
    {
      if (!decl_function_context (fndecl))
        {
          c_genericize (fndecl);
	  lower_nested_functions (fndecl);
          c_finalize (fndecl);
        }
      else
        {
          /* Register this function with cgraph just far enough to get it
            added to our parent's nested function list.  Handy, since the
            C front end doesn't have such a list.  */
          (void) cgraph_node (fndecl);
        }
    }

  /* We're leaving the context of this function, so zap cfun.
     It's still in DECL_STRUCT_FUNCTION, and we'll restore it in
     tree_rest_of_compilation.  */
  cfun = NULL;
  current_function_decl = NULL;
}

/* Generate the RTL for the body of FNDECL.  */

void
c_expand_body (tree fndecl)
{

  if (!DECL_INITIAL (fndecl)
      || DECL_INITIAL (fndecl) == error_mark_node)
    return;

  tree_rest_of_compilation (fndecl, false);

  if (DECL_STATIC_CONSTRUCTOR (fndecl)
      && targetm.have_ctors_dtors)
    targetm.asm_out.constructor (XEXP (DECL_RTL (fndecl), 0),
                                 DEFAULT_INIT_PRIORITY);
  if (DECL_STATIC_DESTRUCTOR (fndecl)
      && targetm.have_ctors_dtors)
    targetm.asm_out.destructor (XEXP (DECL_RTL (fndecl), 0),
                                DEFAULT_INIT_PRIORITY);
}

/* Check the declarations given in a for-loop for satisfying the C99
   constraints.  */
void
check_for_loop_decls (void)
{
  struct c_binding *b;

  if (!flag_isoc99)
    {
      /* If we get here, declarations have been used in a for loop without
	 the C99 for loop scope.  This doesn't make much sense, so don't
	 allow it.  */
      error ("'for' loop initial declaration used outside C99 mode");
      return;
    }
  /* C99 subclause 6.8.5 paragraph 3:

       [#3]  The  declaration  part  of  a for statement shall only
       declare identifiers for objects having storage class auto or
       register.

     It isn't clear whether, in this sentence, "identifiers" binds to
     "shall only declare" or to "objects" - that is, whether all identifiers
     declared must be identifiers for objects, or whether the restriction
     only applies to those that are.  (A question on this in comp.std.c
     in November 2000 received no answer.)  We implement the strictest
     interpretation, to avoid creating an extension which later causes
     problems.  */

  for (b = current_scope->bindings; b; b = b->prev)
    {
      tree id = b->id;
      tree decl = b->decl;

      if (!id)
	continue;

      switch (TREE_CODE (decl))
	{
	case VAR_DECL:
	  if (TREE_STATIC (decl))
	    error ("%Jdeclaration of static variable '%D' in 'for' loop "
		   "initial declaration", decl, decl);
	  else if (DECL_EXTERNAL (decl))
	    error ("%Jdeclaration of 'extern' variable '%D' in 'for' loop "
		   "initial declaration", decl, decl);
	  break;

	case RECORD_TYPE:
	  error ("'struct %E' declared in 'for' loop initial declaration", id);
	  break;
	case UNION_TYPE:
	  error ("'union %E' declared in 'for' loop initial declaration", id);
	  break;
	case ENUMERAL_TYPE:
	  error ("'enum %E' declared in 'for' loop initial declaration", id);
	  break;
	default:
	  error ("%Jdeclaration of non-variable '%D' in 'for' loop "
		 "initial declaration", decl, decl);
	}
    }
}

/* Save and reinitialize the variables
   used during compilation of a C function.  */

void
c_push_function_context (struct function *f)
{
  struct language_function *p;
  p = GGC_NEW (struct language_function);
  f->language = p;

  p->base.x_stmt_tree = c_stmt_tree;
  p->x_break_label = c_break_label;
  p->x_cont_label = c_cont_label;
  p->x_switch_stack = c_switch_stack;
  p->returns_value = current_function_returns_value;
  p->returns_null = current_function_returns_null;
  p->returns_abnormally = current_function_returns_abnormally;
  p->warn_about_return_type = warn_about_return_type;
  p->extern_inline = current_extern_inline;
}

/* Restore the variables used during compilation of a C function.  */

void
c_pop_function_context (struct function *f)
{
  struct language_function *p = f->language;

  if (DECL_STRUCT_FUNCTION (current_function_decl) == 0
      && DECL_SAVED_TREE (current_function_decl) == NULL_TREE)
    {
      /* Stop pointing to the local nodes about to be freed.  */
      /* But DECL_INITIAL must remain nonzero so we know this
	 was an actual function definition.  */
      DECL_INITIAL (current_function_decl) = error_mark_node;
      DECL_ARGUMENTS (current_function_decl) = 0;
    }

  c_stmt_tree = p->base.x_stmt_tree;
  c_break_label = p->x_break_label;
  c_cont_label = p->x_cont_label;
  c_switch_stack = p->x_switch_stack;
  current_function_returns_value = p->returns_value;
  current_function_returns_null = p->returns_null;
  current_function_returns_abnormally = p->returns_abnormally;
  warn_about_return_type = p->warn_about_return_type;
  current_extern_inline = p->extern_inline;

  f->language = NULL;
}

/* Copy the DECL_LANG_SPECIFIC data associated with DECL.  */

void
c_dup_lang_specific_decl (tree decl)
{
  struct lang_decl *ld;

  if (!DECL_LANG_SPECIFIC (decl))
    return;

  ld = GGC_NEW (struct lang_decl);
  memcpy (ld, DECL_LANG_SPECIFIC (decl), sizeof (struct lang_decl));
  DECL_LANG_SPECIFIC (decl) = ld;
}

/* The functions below are required for functionality of doing
   function at once processing in the C front end. Currently these
   functions are not called from anywhere in the C front end, but as
   these changes continue, that will change.  */

/* Returns nonzero if the current statement is a full expression,
   i.e. temporaries created during that statement should be destroyed
   at the end of the statement.  */

int
stmts_are_full_exprs_p (void)
{
  return 0;
}

/* Returns the stmt_tree (if any) to which statements are currently
   being added.  If there is no active statement-tree, NULL is
   returned.  */

stmt_tree
current_stmt_tree (void)
{
  return &c_stmt_tree;
}

/* Nonzero if TYPE is an anonymous union or struct type.  Always 0 in
   C.  */

int
anon_aggr_type_p (tree ARG_UNUSED (node))
{
  return 0;
}

/* Dummy function in place of callback used by C++.  */

void
extract_interface_info (void)
{
}

/* Return the global value of T as a symbol.  */

tree
identifier_global_value	(tree t)
{
  struct c_binding *b;

  for (b = I_SYMBOL_BINDING (t); b; b = b->shadowed)
    if (B_IN_FILE_SCOPE (b) || B_IN_EXTERNAL_SCOPE (b))
      return b->decl;

  return 0;
}

/* Record a builtin type for C.  If NAME is non-NULL, it is the name used;
   otherwise the name is found in ridpointers from RID_INDEX.  */

void
record_builtin_type (enum rid rid_index, const char *name, tree type)
{
  tree id, decl;
  if (name == 0)
    id = ridpointers[(int) rid_index];
  else
    id = get_identifier (name);
  decl = build_decl (TYPE_DECL, id, type);
  pushdecl (decl);
  if (debug_hooks->type_decl)
    debug_hooks->type_decl (decl, false);
}

/* Build the void_list_node (void_type_node having been created).  */
tree
build_void_list_node (void)
{
  tree t = build_tree_list (NULL_TREE, void_type_node);
  return t;
}

/* Return a structure for a parameter with the given SPECS, ATTRS and
   DECLARATOR.  */

tree
build_c_parm (tree specs, tree attrs, tree declarator)
{
  return build_tree_list (build_tree_list (specs, declarator), attrs);
}

/* Return a declarator with nested attributes.  TARGET is the inner
   declarator to which these attributes apply.  ATTRS are the
   attributes.  */

tree
build_attrs_declarator (tree attrs, tree target)
{
  return tree_cons (attrs, target, NULL_TREE);
}

/* Return a declarator for a function with arguments specified by ARGS
   and return type specified by TARGET.  */

tree
build_function_declarator (tree args, tree target)
{
  return build_nt (CALL_EXPR, target, args, NULL_TREE);
}

/* Return something to represent absolute declarators containing a *.
   TARGET is the absolute declarator that the * contains.
   TYPE_QUALS_ATTRS is a list of modifiers such as const or volatile
   to apply to the pointer type, represented as identifiers, possible mixed
   with attributes.

   We return an INDIRECT_REF whose "contents" are TARGET (inside a TREE_LIST,
   if attributes are present) and whose type is the modifier list.  */

tree
make_pointer_declarator (tree type_quals_attrs, tree target)
{
  tree quals, attrs;
  tree itarget = target;
  split_specs_attrs (type_quals_attrs, &quals, &attrs);
  if (attrs != NULL_TREE)
    itarget = build_attrs_declarator (attrs, target);
  return build1 (INDIRECT_REF, quals, itarget);
}

/* Synthesize a function which calls all the global ctors or global
   dtors in this file.  This is only used for targets which do not
   support .ctors/.dtors sections.  FIXME: Migrate into cgraph.  */
static void
build_cdtor (int method_type, tree cdtors)
{
  tree body = 0;

  if (!cdtors)
    return;

  for (; cdtors; cdtors = TREE_CHAIN (cdtors))
    append_to_statement_list (build_function_call (TREE_VALUE (cdtors), 0),
			      &body);

  cgraph_build_static_cdtor (method_type, body, DEFAULT_INIT_PRIORITY);
}

/* Perform final processing on one file scope's declarations (or the
   external scope's declarations), GLOBALS.  */
static void
c_write_global_declarations_1 (tree globals)
{
  size_t len = list_length (globals);
  tree *vec = XNEWVEC (tree, len);
  size_t i;
  tree decl;

  /* Process the decls in the order they were written.  */
  for (i = 0, decl = globals; i < len; i++, decl = TREE_CHAIN (decl))
    vec[i] = decl;

  wrapup_global_declarations (vec, len);
  check_global_declarations (vec, len);

  free (vec);
}


void
c_write_global_declarations (void)
{
  tree ext_block, t;

  /* We don't want to do this if generating a PCH.  */
  if (pch_file)
    return;

  /* Don't waste time on further processing if -fsyntax-only or we've
     encountered errors.  */
  if (flag_syntax_only || errorcount || sorrycount || cpp_errors (parse_in))
    return;

  /* Close the external scope.  */
  ext_block = pop_scope ();
  external_scope = 0;
  if (current_scope)
    abort ();

  /* Process all file scopes in this compilation, and the external_scope,
     through wrapup_global_declarations and check_global_declarations.  */
  for (t = all_translation_units; t; t = TREE_CHAIN (t))
    c_write_global_declarations_1 (BLOCK_VARS (DECL_INITIAL (t)));
  c_write_global_declarations_1 (BLOCK_VARS (ext_block));

  /* Generate functions to call static constructors and destructors
     for targets that do not support .ctors/.dtors sections.  These
     functions have magic names which are detected by collect2.  */
  build_cdtor ('I', static_ctors); static_ctors = 0;
  build_cdtor ('D', static_dtors); static_dtors = 0;

  /* We're done parsing; proceed to optimize and emit assembly.
     FIXME: shouldn't be the front end's responsibility to call this.  */
  cgraph_optimize ();

  /* Presently this has to happen after cgraph_optimize.
     FIXME: shouldn't be the front end's responsibility to call this.  */
  if (flag_mudflap)
    mudflap_finish_file ();
}

/* Reset the parser's state in preparation for a new file.  */

#if 0
/* IMA MERGE. This module need be rewriten when IMA is working again! */ 

void
c_reset_state (void)
{
  tree link;
  tree file_scope_decl;

  /* Pop the global scope.  */
  if (current_scope != global_scope)
      current_scope = global_scope;
  file_scope_decl = current_file_decl;
  DECL_INITIAL (file_scope_decl) = poplevel (1, 0, 0);
  BLOCK_SUPERCONTEXT (DECL_INITIAL (file_scope_decl)) = file_scope_decl;
  truly_local_externals = NULL_TREE;

  /* Start a new global binding level.  */
  pushlevel (0);
  global_scope = current_scope;
  current_file_decl = build_decl (TRANSLATION_UNIT_DECL, NULL, NULL);
  TREE_CHAIN (current_file_decl) = file_scope_decl;

  /* Reintroduce the builtin declarations.  */
  for (link = first_builtin_decl;
       link != TREE_CHAIN (last_builtin_decl);
       link = TREE_CHAIN (link))
    pushdecl (copy_node (link));
}
#endif

/* APPLE LOCAL begin MERGE HACK loop transposition commented out b/c tree nodes changed */
#if (0)
/* APPLE LOCAL begin loop transposition (currently unsafe) */
/* This pass on trees is to transpose loops so that memory systems will 
   not be overtaxed.
   So it changes:
    for(i=0;i<size0;i++)
      for(j=0;j<size1;j++)
        a = a + pointer[j][i];
   into
    for(j=0;j<size1;j++)
      for(i=0;i<size0;i++)
	a = a + pointer[j][i];

   and
    for(i=0;i<size0;i++)
      {
        for(j=0;j<size1;j++)
          {
            a = a + pointer[j][i];
          }
        pointer[i][i] = b * pointer[i][i];
      }
   into
    for(j=0;j<size1;j++)
      {
        for(i=0;i<size0;i++)
          {
            a = a + pointer[j][i];
          }
      }
    for(j=0;j<size1;j++)
      {
        pointer[i][i] = b * pointer[i][i];
      }

   Note this is experimental because it does not always get it right,
   but works on SPEC 2000 and the bootstrap of gcc.
   Here is a case it miscompiles:

    struct {
	double unew[1782225];
    } COMMON;

    double swimneg_1()
    {
	double ucheck = 0;
	int i, j;
	for(i = 1; i <= 1334;i++) {
	    for(j = 1;j <= 1334;j++) {
		ucheck += COMMON.unew[(i-1) + 1335*(j-1) ];
	    }
	    COMMON.unew[i + 1335*(i)] *= 2;
	}
	return ucheck;
    }

    The loops are incorrectly transposed because it does not know 
    that the modification of
    COMMON.unew_[icheck_ + 1335*icheck_] (in the outer loop)
    needs to be done right after the inner loop. */

static tree
find_tree_with_code_1 (tree *tp, int *walk_subtrees ATTRIBUTE_UNUSED, 
		       void *data)
{
  if (*tp == NULL_TREE)
    return NULL_TREE;
  if (TREE_CODE (*tp) == *((enum tree_code *)data))
    return *tp;
  return NULL_TREE;
}

static tree find_tree_with_code (tree body, enum tree_code code)
{
  enum tree_code temp = code;
  return walk_tree_without_duplicates (&body, find_tree_with_code_1, (void *)&temp);
}

static tree
find_pointer (tree t)
{
  tree temp2 = find_tree_with_code (t, ARRAY_REF);
  if (temp2)
    return TREE_OPERAND (temp2, 0);
  temp2 = find_tree_with_code (t, INDIRECT_REF);
  if (temp2)
    {
      temp2 = TREE_OPERAND (temp2, 0);
      if (TREE_CODE (temp2) == PLUS_EXPR)
	{
	  temp2 = TREE_OPERAND (temp2, 0);
	  if (TREE_CODE (temp2) == PARM_DECL || TREE_CODE (temp2) == VAR_DECL)
	    return temp2;
	  return find_pointer (temp2);
	}
    }
 return NULL_TREE;
}

typedef struct should_transpose_for_loops_t
{
  tree inner_var;
  tree outer_var;
  bool doit;
  tree already_modified;
} should_transpose_for_loops_t;

/* If the transposition should be done, set data->doit to true and
   return NULL.  If it should not, set data->doit to false and 
   return *tp. */

static tree
should_transpose_for_loops_1 (tree *tp, int *walk_subtrees, void *data)
{
  tree assignment_to = *tp;
  should_transpose_for_loops_t *temp = (should_transpose_for_loops_t*)data;
  tree inner_var = temp->inner_var;
  tree outer_var = temp->outer_var;
  if (*tp == NULL_TREE)
    return NULL_TREE;
  /* We cannot do the transposition if any of these are in the loop. */
  if (TREE_CODE (*tp) == LABEL_DECL
      || TREE_CODE (*tp) == FOR_STMT || TREE_CODE (*tp) == DO_STMT
      || TREE_CODE (*tp) == WHILE_STMT
      || TREE_CODE (*tp) == BREAK_STMT || TREE_CODE (*tp) == CONTINUE_STMT 
      || TREE_CODE (*tp) == RETURN_EXPR)
    {
      temp->doit = false;
      return *tp;
    }
  if (TREE_CODE (assignment_to) == MODIFY_EXPR)
    {
      tree temp1;
      tree temp2 = find_pointer (TREE_OPERAND (assignment_to, 0));
      /* We cannot do the transposition because the pointer temp2 is modified 
         with a value dependent on itself.
         (Note this could be better if it is only dependent on a non-forward 
         loop dependent). */
      if (temp2 != NULL_TREE 
          && tree_contains (TREE_OPERAND (assignment_to, 1), temp2))
        {
          temp->doit = false;
          return *tp;
        }
      for (temp1 = temp->already_modified;
	   temp1 != NULL_TREE;
	   temp1 = TREE_CHAIN (temp1))
	{
          tree temp3 = TREE_VALUE(temp1);
          tree temp4 = TREE_OPERAND (assignment_to, 1);
          /* We cannot do the transposition because the pointer temp3 is 
             modified with a value dependent on itself or already has 
             been modified. */
          if (tree_contains (temp4, temp3)
              || (temp2 != NULL_TREE && temp3 == temp2))
            {
              temp->doit = false;
              return *tp;
            }
        }
      /* If it is non-null, add temp2 to the list of already modified 
	 pointers. */
      if(temp2 != NULL_TREE)
	temp->already_modified = 
	       tree_cons(NULL_TREE, temp2, temp->already_modified);
    }
  /* Check for pointer[inner][outer], pointer[inner*outersize+outer] and 
     array[inner][outer].  */
  if ((TREE_CODE (assignment_to) == INDIRECT_REF 
       && TREE_CODE (TREE_OPERAND (assignment_to, 0)) == PLUS_EXPR)
      || (TREE_CODE (assignment_to) == ARRAY_REF 
          && TREE_CODE (TREE_OPERAND (assignment_to, 1)) == PLUS_EXPR))
    {
      tree plus1_expr_assignment = TREE_OPERAND (assignment_to, 
                            TREE_CODE (assignment_to) == ARRAY_REF ? 1 : 0);
      tree side0 = TREE_OPERAND (plus1_expr_assignment, 0);
      tree side1 = TREE_OPERAND (plus1_expr_assignment, 1);
      STRIP_NOPS (side0);
      STRIP_NOPS (side1);
      /* This handles a[inner][outer].  */
      if ((TREE_CODE (side0) == INDIRECT_REF
            && tree_contains (side0, inner_var) 
            && !tree_contains (side0, outer_var) 
            && tree_contains (side1, outer_var)
            && !tree_contains (side1, inner_var))
          || (TREE_CODE (side1) == INDIRECT_REF
              && tree_contains (side1, inner_var) 
              && !tree_contains (side1, outer_var) 
              && tree_contains (side0, outer_var)
              && !tree_contains (side0, inner_var)))
        {
          *walk_subtrees = 0; /* already walked them */
          temp->doit = true;
          return NULL_TREE;
        } 
      else
        {
          tree side = NULL_TREE;
          /* Handle array[inner*size+outer+offset] and pointer[inner*size+outer]
             (FIXME need to handle array[inner*size+outer] 
             (and pointer[inner*size+outer+offset]?) )*/
          if (tree_contains (side0, inner_var) 
              && tree_contains (side0, outer_var))
            side = side0;
          else if (tree_contains (side1, inner_var) 
                   && tree_contains (side1, outer_var))
            side = side1;
          if (side && (TREE_CODE (side) == MULT_EXPR))
            {
              tree temp0 = TREE_OPERAND (side, 0);
              tree temp1 = TREE_OPERAND (side, 1);
              STRIP_NOPS (temp0);
              STRIP_NOPS (temp1);
              if (tree_contains (temp0, inner_var) 
                  && tree_contains (temp0, outer_var))
                side = temp0;
              else if (tree_contains (temp1, inner_var) 
                       && tree_contains (temp1, outer_var))
                side = temp1;
              else
                side = NULL_TREE;
            }
          if (side && (TREE_CODE (side) == PLUS_EXPR))
            {
              tree side10 = TREE_OPERAND (side, 0);
              tree side11 = TREE_OPERAND (side, 1);
              STRIP_NOPS (side10);
              STRIP_NOPS (side11);
              if ((TREE_CODE (side10) == MULT_EXPR
                    && tree_contains (side10, inner_var) 
                    && !tree_contains (side10, outer_var)
                    && tree_contains (side11, outer_var)
		    && !tree_contains (side11, inner_var))
                  || (TREE_CODE (side11) == MULT_EXPR
                      && tree_contains (side11, inner_var)  
                      && !tree_contains (side11, outer_var) 
                      && tree_contains (side10, outer_var)
                      && !tree_contains (side10, inner_var)))
                {
                  *walk_subtrees = 0; /* already walked them */
                  temp->doit = true;
                  return NULL;
                }
              else
                {
                  temp->doit = false;
                  return *tp;
                }
            }
        }
    }
  /* We cannot do the transposition if there is an assignment to the 
     outer_var or inner_var.  */
  if (TREE_CODE (assignment_to) == MODIFY_EXPR)
    {
      tree side1 = TREE_OPERAND (assignment_to, 1);
      STRIP_NOPS (side1);
      if (side1 == outer_var || side1 == inner_var)
        {
          temp->doit = false;
          return *tp;
        }
    }
  return NULL_TREE;
}

/* Return true if the loops should be interchanged based on body, inner
   variable and outer variable, and also set already_modified to the pointers
   that are modified during the loop.  */

static bool
should_transpose_for_loops (tree body, tree inner_var, tree outer_var, 
			    tree *already_modified)
{
  should_transpose_for_loops_t temp;
  temp.inner_var = inner_var;
  temp.outer_var = outer_var;
  temp.already_modified = *already_modified;
  temp.doit = false;
  if (walk_tree (&body, should_transpose_for_loops_1, &temp, NULL))
    return false;
  *already_modified = temp.already_modified;
  return temp.doit;
}

static tree
tree_contains_1 (tree *tp, int *walk_subtrees ATTRIBUTE_UNUSED, void *data)
{
  if (*tp == data)
    return data;
  return NULL_TREE;
}

static bool
tree_contains (tree body, tree x)
{
  return walk_tree_without_duplicates (&body, tree_contains_1, (void *)x)
       != NULL_TREE;
}

/* Look for two nested loops and transpose them if this is a good idea. 
   Currently limited to FOR statements in C.  */

static tree
perform_loop_transpose (tree *tp, int *walk_subtrees,
			void *data ATTRIBUTE_UNUSED)
{
  tree already_modified = NULL_TREE;
  tree outer_init, inner_init;
  tree outer_init_expr, inner_init_expr;
  tree outer_var, inner_var;
  tree inner_loop_body;
  tree newouter, newinner;
  tree outer_loop, inner_loop;
  tree before_inner_loop, right_before_inner_loop;
  
  if (*tp == NULL_TREE)
    return NULL_TREE;
  
  if (TREE_CODE (*tp) != FOR_STMT)
    return NULL;
  
  outer_loop = *tp;
  inner_loop = TREE_OPERAND (outer_loop, 3);
  before_inner_loop = NULL_TREE;
  right_before_inner_loop = NULL_TREE;
  
  /* If the loops contains a call or an if statement or is empty, 
     do not do the transposition.  */
  if (inner_loop == NULL_TREE 
      || find_tree_with_code (inner_loop, CALL_EXPR) != NULL_TREE)
    return NULL_TREE;
  
      
      right_before_inner_loop = previous;
    }
  
  /* If we do not have found the inner loop return right away.  */
  if (inner_loop == NULL_TREE || TREE_CODE (inner_loop) != FOR_STMT)
    return NULL;
  
  outer_init = TREE_OPERAND (outer_loop, 0);
  inner_init = TREE_OPERAND (inner_loop, 0);
  
  /* FIXME: does not handle C99/C++ style for init statements */
  if (outer_init == NULL_TREE || inner_init == NULL_TREE
      || TREE_CODE (outer_init) != EXPR_STMT 
      || TREE_CODE (inner_init) != EXPR_STMT)
    return NULL;
  
  outer_init_expr = TREE_OPERAND (outer_init, 0);
  inner_init_expr = TREE_OPERAND (inner_init, 0);
  
  if (outer_init_expr == NULL_TREE || inner_init_expr == NULL_TREE
      || TREE_CODE (inner_init_expr) != MODIFY_EXPR
      || TREE_CODE (outer_init_expr) != MODIFY_EXPR)
    return NULL;
  
  outer_var = TREE_OPERAND (outer_init_expr, 0);
  inner_var = TREE_OPERAND (inner_init_expr, 0);
  
  /* The inner_var should be independent of outer_var */
  if (tree_contains (TREE_OPERAND (inner_init_expr, 1), outer_var)
      || tree_contains (TREE_OPERAND (inner_loop, 1), outer_var)
      || tree_contains (TREE_OPERAND (inner_loop, 2), outer_var)
      /* The outer loop variable should be independent of 
	 inner_var also. */
      || tree_contains (TREE_OPERAND (outer_loop, 1), inner_var)
      || tree_contains (TREE_OPERAND (outer_loop, 2), inner_var))
    return NULL;
  
  inner_loop_body = TREE_OPERAND (inner_loop, 3);
  if (!should_transpose_for_loops (inner_loop_body,inner_var, outer_var,
				   &already_modified))
    return NULL;
  
  /* Is the outter loop's body a compound statement?  */
  if (TREE_CODE (TREE_OPERAND (outer_loop, 3)) == COMPOUND_STMT)
    {
      tree after_loop = TREE_CHAIN (inner_loop);
      tree find;
      tree allloops_stmt;
      tree outloopafter;
      tree outloopbefore;
      tree save_chain = NULL_TREE;
      
      allloops_stmt = build_stmt (COMPOUND_STMT, NULL_TREE);
      outloopbefore = build_stmt (FOR_STMT, outer_init,
				  TREE_OPERAND (outer_loop, 1),
				  TREE_OPERAND (outer_loop, 2), NULL_TREE);
      
      /* Use copies of the loop test expression (TREE_OPERAND #1) for
	 these, lest the tree-profiler mix the execution counts of two different
	 loops.  */
      
      outloopafter = build_stmt (FOR_STMT, outer_init,
				 copy_node (TREE_OPERAND (outer_loop, 1)),
				 TREE_OPERAND (outer_loop, 2), NULL_TREE);
      newinner = build_stmt (FOR_STMT, outer_init,
			     copy_node (TREE_OPERAND (outer_loop, 1)),
			     TREE_OPERAND (outer_loop, 2), inner_loop_body);
      newouter = build_stmt (FOR_STMT, inner_init, TREE_OPERAND (inner_loop, 1),
			     TREE_OPERAND (inner_loop, 2), newinner);
      
      /* This new compound statement has no scope. */
      COMPOUND_STMT_NO_SCOPE (allloops_stmt) = 1;
      
      /* Move to the next statement in the chain of 
	 before_inner_loop if it is a scope statement */
      if (before_inner_loop != NULL_TREE
	  && TREE_CODE (before_inner_loop) == SCOPE_STMT)
	{
	  if (right_before_inner_loop)
	    {
	      save_chain = TREE_CHAIN (right_before_inner_loop);
	      TREE_CHAIN (right_before_inner_loop) = NULL_TREE;
	    }
	  before_inner_loop = TREE_CHAIN (before_inner_loop);
	}
      
      /* Are there statements before the inner loop? */
      if (before_inner_loop != NULL_TREE)
	{
	  tree beforeloopbody = build_stmt (COMPOUND_STMT, NULL_TREE);
	  COMPOUND_STMT_NO_SCOPE (beforeloopbody) = 1;
	  beforeloopbody = build_stmt (COMPOUND_STMT, NULL_TREE);
	  COMPOUND_BODY (beforeloopbody) = before_inner_loop;
	  FOR_BODY (outloopbefore) = beforeloopbody;
	  
	  /* If the outer loop body depends on the inner variable we can't do
	     the transposition. */
	  if (tree_contains (outloopbefore, inner_var))
	    {
	      if (right_before_inner_loop)
		TREE_CHAIN (right_before_inner_loop) = save_chain;
	      return NULL_TREE;
	    }

	  for (find = already_modified; find != NULL_TREE;
	      find = TREE_CHAIN (find))
	    {
	      tree temp3 = TREE_VALUE(find);
	      if (tree_contains(outloopbefore, temp3))
		{
		  /* We cannot do the transposition because there is a reference
		     to something modified in the outer loop. */
		  if (right_before_inner_loop)
		    TREE_CHAIN (right_before_inner_loop) = save_chain;
		  return NULL_TREE;
		}
	    }
	  /* If the new before loop body is independent
	     of the outer variable, remove the loop 
	     and make the body the first statement in 
	     the chain of all the statements. */
	  if (!tree_contains (beforeloopbody, outer_var))
	    {
	      COMPOUND_BODY (allloops_stmt) = beforeloopbody;
	      TREE_CHAIN (beforeloopbody) = newouter;
	    } 
	  else
	    {
	      COMPOUND_BODY (allloops_stmt) = outloopbefore;
	      TREE_CHAIN (outloopbefore) = newouter;
	    }
	}
      else
	{
	  COMPOUND_BODY (allloops_stmt) = newouter;
	  outloopbefore = NULL_TREE;
	}
      
      if (after_loop != NULL_TREE && TREE_CHAIN (after_loop) == NULL_TREE)
	{
	  if (TREE_CODE (after_loop) != SCOPE_STMT)
	    FOR_BODY (outloopafter) = after_loop;
	  else
	    outloopafter = NULL_TREE;
	 } 
       else
	 {
	   tree afterloopbody = build_stmt (COMPOUND_STMT, NULL_TREE);
	   tree temp5;
	   tree saved_scope = NULL_TREE;
	   tree *save_scope_spot = NULL;
	   
	   COMPOUND_STMT_NO_SCOPE (afterloopbody) = 1;
	   COMPOUND_BODY (afterloopbody) = after_loop;
	   FOR_BODY (outloopafter) = afterloopbody;
	   
	   for (temp5 = after_loop; temp5 != NULL_TREE;
		temp5 = TREE_CHAIN (temp5))
	     if (TREE_CODE (TREE_CHAIN (temp5)) == SCOPE_STMT)
	       {
		 saved_scope = TREE_CHAIN (temp5);
		 save_scope_spot = &TREE_CHAIN (temp5);
		 TREE_CHAIN (temp5) = NULL_TREE;
	       }
	   
	   /* If the outer loop body depends on the inner
	      variable, we cannot do the transposition. */
	   if (tree_contains (afterloopbody, inner_var))
	     {
	       if (right_before_inner_loop)
		 TREE_CHAIN (right_before_inner_loop) = save_chain;
	       if (save_scope_spot)
		 *save_scope_spot = saved_scope;
	       return NULL_TREE;
	     }
	   
	   /* FIXME: need to check for the afterloopbody
	      containing a pointer that gets modified
	      before the inner loop has a chance to
	      read it. */
	   for (find = already_modified; find != NULL_TREE;
		find = TREE_CHAIN (find))
	     {
	       tree temp3 = TREE_VALUE(find);
	       /* If something references something that
		  is stored into we cannot do the
		  transposition. */
	       if (tree_contains(afterloopbody, temp3))
		 {
		   if (right_before_inner_loop)
		     TREE_CHAIN (right_before_inner_loop) = save_chain;
		   if (save_scope_spot)
		     *save_scope_spot = saved_scope;
		   return NULL_TREE;
		 }
	     }
	   
	   /* If the stuff after the inner_loop is not 
	      dependent on the loop variable pull it 
	      out of the loop. */
	   if (!tree_contains (afterloopbody, outer_var))
	     outloopafter = afterloopbody;
	}
      
      if (before_inner_loop != NULL_TREE
	  && TREE_CODE (before_inner_loop) == SCOPE_STMT
	  && right_before_inner_loop != NULL_TREE)
	TREE_CHAIN (right_before_inner_loop) = NULL_TREE;
      
      TREE_CHAIN (newouter) = outloopafter;
      if (outloopafter == NULL_TREE && outloopbefore == NULL_TREE)
	  allloops_stmt = newouter;
      TREE_CHAIN (allloops_stmt) = TREE_CHAIN (outer_loop);
      *walk_subtrees = 0;
      *tp = allloops_stmt;
      return NULL_TREE;
    }
  /* Do the transposition. */
  newinner = build_stmt (FOR_STMT, outer_init, TREE_OPERAND (outer_loop, 1),
			 TREE_OPERAND (outer_loop, 2),  inner_loop_body);
  newouter = build_stmt (FOR_STMT, inner_init,  TREE_OPERAND (inner_loop, 1),
			 TREE_OPERAND (inner_loop, 2),  newinner);
  TREE_CHAIN (newouter) = TREE_CHAIN (outer_loop);
  *tp = newouter;
  *walk_subtrees = 0;
  return NULL_TREE;
}


/* The main entry point for the transposition.  */
void
loop_transpose (tree fn)
{
  /*timevar_push (TV_LOOP_TRANSPOSE);*/
  walk_tree (&DECL_SAVED_TREE (fn), perform_loop_transpose, NULL, NULL);
  /*timevar_pop (TV_LOOP_TRANSPOSE);*/
}
/* APPLE LOCAL end loop transposition (currently unsafe) */
#endif
/* Fake version to get things compiling */
void
loop_transpose (tree fn) 
{
  fn = NULL;
}

/* APPLE LOCAL end MERGE HACK loop transposition commented out b/c tree nodes changed */

/* APPLE LOCAL begin CW asm blocks */
/* Look for a struct or union tag, but quietly; don't complain if neither
   is found, and don't autocreate. Used to identify struct/union tags
   mentioned in CW asm operands.  */
tree
lookup_struct_or_union_tag (tree typename)
{
  tree rslt = lookup_tag (RECORD_TYPE, typename, 0);

  pending_invalid_xref = 0;
  if (!rslt)
    rslt = lookup_tag (UNION_TYPE, typename, 0);
  pending_invalid_xref = 0;
  return rslt;
}
/* APPLE LOCAL end CW asm blocks */

#include "gt-c-decl.h"
