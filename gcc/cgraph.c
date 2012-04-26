/* Callgraph handling code.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
   2011, 2012 Free Software Foundation, Inc.
   Contributed by Jan Hubicka

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

/*  This file contains basic routines manipulating call graph

    The call-graph is a data structure designed for intra-procedural optimization.
    It represents a multi-graph where nodes are functions and edges are call sites. */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "tree-inline.h"
#include "langhooks.h"
#include "hashtab.h"
#include "toplev.h"
#include "flags.h"
#include "ggc.h"
#include "debug.h"
#include "target.h"
#include "basic-block.h"
#include "cgraph.h"
#include "output.h"
#include "intl.h"
#include "gimple.h"
#include "tree-dump.h"
#include "tree-flow.h"
#include "value-prof.h"
#include "except.h"
#include "diagnostic-core.h"
#include "rtl.h"
#include "ipa-utils.h"
#include "lto-streamer.h"
#include "ipa-inline.h"
#include "cfgloop.h"
#include "gimple-pretty-print.h"

const char * const ld_plugin_symbol_resolution_names[]=
{
  "",
  "undef",
  "prevailing_def",
  "prevailing_def_ironly",
  "preempted_reg",
  "preempted_ir",
  "resolved_ir",
  "resolved_exec",
  "resolved_dyn",
  "prevailing_def_ironly_exp"
};

static void cgraph_node_remove_callers (struct cgraph_node *node);
static inline void cgraph_edge_remove_caller (struct cgraph_edge *e);
static inline void cgraph_edge_remove_callee (struct cgraph_edge *e);

/* Queue of cgraph nodes scheduled to be lowered.  */
symtab_node x_cgraph_nodes_queue;
#define cgraph_nodes_queue ((struct cgraph_node *)x_cgraph_nodes_queue)

/* Number of nodes in existence.  */
int cgraph_n_nodes;

/* Maximal uid used in cgraph nodes.  */
int cgraph_max_uid;

/* Maximal uid used in cgraph edges.  */
int cgraph_edge_max_uid;

/* Set when whole unit has been analyzed so we can access global info.  */
bool cgraph_global_info_ready = false;

/* What state callgraph is in right now.  */
enum cgraph_state cgraph_state = CGRAPH_STATE_PARSING;

/* Set when the cgraph is fully build and the basic flags are computed.  */
bool cgraph_function_flags_ready = false;

/* Linked list of cgraph asm nodes.  */
struct cgraph_asm_node *cgraph_asm_nodes;

/* Last node in cgraph_asm_nodes.  */
static GTY(()) struct cgraph_asm_node *cgraph_asm_last_node;

/* List of hooks triggered on cgraph_edge events.  */
struct cgraph_edge_hook_list {
  cgraph_edge_hook hook;
  void *data;
  struct cgraph_edge_hook_list *next;
};

/* List of hooks triggered on cgraph_node events.  */
struct cgraph_node_hook_list {
  cgraph_node_hook hook;
  void *data;
  struct cgraph_node_hook_list *next;
};

/* List of hooks triggered on events involving two cgraph_edges.  */
struct cgraph_2edge_hook_list {
  cgraph_2edge_hook hook;
  void *data;
  struct cgraph_2edge_hook_list *next;
};

/* List of hooks triggered on events involving two cgraph_nodes.  */
struct cgraph_2node_hook_list {
  cgraph_2node_hook hook;
  void *data;
  struct cgraph_2node_hook_list *next;
};

/* List of hooks triggered when an edge is removed.  */
struct cgraph_edge_hook_list *first_cgraph_edge_removal_hook;
/* List of hooks triggered when a node is removed.  */
struct cgraph_node_hook_list *first_cgraph_node_removal_hook;
/* List of hooks triggered when an edge is duplicated.  */
struct cgraph_2edge_hook_list *first_cgraph_edge_duplicated_hook;
/* List of hooks triggered when a node is duplicated.  */
struct cgraph_2node_hook_list *first_cgraph_node_duplicated_hook;
/* List of hooks triggered when an function is inserted.  */
struct cgraph_node_hook_list *first_cgraph_function_insertion_hook;

/* Head of a linked list of unused (freed) call graph nodes.
   Do not GTY((delete)) this list so UIDs gets reliably recycled.  */
static GTY(()) struct cgraph_node *free_nodes;
/* Head of a linked list of unused (freed) call graph edges.
   Do not GTY((delete)) this list so UIDs gets reliably recycled.  */
static GTY(()) struct cgraph_edge *free_edges;

/* Did procss_same_body_aliases run?  */
bool same_body_aliases_done;

/* Macros to access the next item in the list of free cgraph nodes and
   edges. */
#define NEXT_FREE_NODE(NODE) cgraph ((NODE)->symbol.next)
#define SET_NEXT_FREE_NODE(NODE,NODE2) ((NODE))->symbol.next = (symtab_node)NODE2
#define NEXT_FREE_EDGE(EDGE) (EDGE)->prev_caller

/* Register HOOK to be called with DATA on each removed edge.  */
struct cgraph_edge_hook_list *
cgraph_add_edge_removal_hook (cgraph_edge_hook hook, void *data)
{
  struct cgraph_edge_hook_list *entry;
  struct cgraph_edge_hook_list **ptr = &first_cgraph_edge_removal_hook;

  entry = (struct cgraph_edge_hook_list *) xmalloc (sizeof (*entry));
  entry->hook = hook;
  entry->data = data;
  entry->next = NULL;
  while (*ptr)
    ptr = &(*ptr)->next;
  *ptr = entry;
  return entry;
}

/* Remove ENTRY from the list of hooks called on removing edges.  */
void
cgraph_remove_edge_removal_hook (struct cgraph_edge_hook_list *entry)
{
  struct cgraph_edge_hook_list **ptr = &first_cgraph_edge_removal_hook;

  while (*ptr != entry)
    ptr = &(*ptr)->next;
  *ptr = entry->next;
  free (entry);
}

/* Call all edge removal hooks.  */
static void
cgraph_call_edge_removal_hooks (struct cgraph_edge *e)
{
  struct cgraph_edge_hook_list *entry = first_cgraph_edge_removal_hook;
  while (entry)
  {
    entry->hook (e, entry->data);
    entry = entry->next;
  }
}

/* Register HOOK to be called with DATA on each removed node.  */
struct cgraph_node_hook_list *
cgraph_add_node_removal_hook (cgraph_node_hook hook, void *data)
{
  struct cgraph_node_hook_list *entry;
  struct cgraph_node_hook_list **ptr = &first_cgraph_node_removal_hook;

  entry = (struct cgraph_node_hook_list *) xmalloc (sizeof (*entry));
  entry->hook = hook;
  entry->data = data;
  entry->next = NULL;
  while (*ptr)
    ptr = &(*ptr)->next;
  *ptr = entry;
  return entry;
}

/* Remove ENTRY from the list of hooks called on removing nodes.  */
void
cgraph_remove_node_removal_hook (struct cgraph_node_hook_list *entry)
{
  struct cgraph_node_hook_list **ptr = &first_cgraph_node_removal_hook;

  while (*ptr != entry)
    ptr = &(*ptr)->next;
  *ptr = entry->next;
  free (entry);
}

/* Call all node removal hooks.  */
static void
cgraph_call_node_removal_hooks (struct cgraph_node *node)
{
  struct cgraph_node_hook_list *entry = first_cgraph_node_removal_hook;
  while (entry)
  {
    entry->hook (node, entry->data);
    entry = entry->next;
  }
}

/* Register HOOK to be called with DATA on each inserted node.  */
struct cgraph_node_hook_list *
cgraph_add_function_insertion_hook (cgraph_node_hook hook, void *data)
{
  struct cgraph_node_hook_list *entry;
  struct cgraph_node_hook_list **ptr = &first_cgraph_function_insertion_hook;

  entry = (struct cgraph_node_hook_list *) xmalloc (sizeof (*entry));
  entry->hook = hook;
  entry->data = data;
  entry->next = NULL;
  while (*ptr)
    ptr = &(*ptr)->next;
  *ptr = entry;
  return entry;
}

/* Remove ENTRY from the list of hooks called on inserted nodes.  */
void
cgraph_remove_function_insertion_hook (struct cgraph_node_hook_list *entry)
{
  struct cgraph_node_hook_list **ptr = &first_cgraph_function_insertion_hook;

  while (*ptr != entry)
    ptr = &(*ptr)->next;
  *ptr = entry->next;
  free (entry);
}

/* Call all node insertion hooks.  */
void
cgraph_call_function_insertion_hooks (struct cgraph_node *node)
{
  struct cgraph_node_hook_list *entry = first_cgraph_function_insertion_hook;
  while (entry)
  {
    entry->hook (node, entry->data);
    entry = entry->next;
  }
}

/* Register HOOK to be called with DATA on each duplicated edge.  */
struct cgraph_2edge_hook_list *
cgraph_add_edge_duplication_hook (cgraph_2edge_hook hook, void *data)
{
  struct cgraph_2edge_hook_list *entry;
  struct cgraph_2edge_hook_list **ptr = &first_cgraph_edge_duplicated_hook;

  entry = (struct cgraph_2edge_hook_list *) xmalloc (sizeof (*entry));
  entry->hook = hook;
  entry->data = data;
  entry->next = NULL;
  while (*ptr)
    ptr = &(*ptr)->next;
  *ptr = entry;
  return entry;
}

/* Remove ENTRY from the list of hooks called on duplicating edges.  */
void
cgraph_remove_edge_duplication_hook (struct cgraph_2edge_hook_list *entry)
{
  struct cgraph_2edge_hook_list **ptr = &first_cgraph_edge_duplicated_hook;

  while (*ptr != entry)
    ptr = &(*ptr)->next;
  *ptr = entry->next;
  free (entry);
}

/* Call all edge duplication hooks.  */
static void
cgraph_call_edge_duplication_hooks (struct cgraph_edge *cs1,
				    struct cgraph_edge *cs2)
{
  struct cgraph_2edge_hook_list *entry = first_cgraph_edge_duplicated_hook;
  while (entry)
  {
    entry->hook (cs1, cs2, entry->data);
    entry = entry->next;
  }
}

/* Register HOOK to be called with DATA on each duplicated node.  */
struct cgraph_2node_hook_list *
cgraph_add_node_duplication_hook (cgraph_2node_hook hook, void *data)
{
  struct cgraph_2node_hook_list *entry;
  struct cgraph_2node_hook_list **ptr = &first_cgraph_node_duplicated_hook;

  entry = (struct cgraph_2node_hook_list *) xmalloc (sizeof (*entry));
  entry->hook = hook;
  entry->data = data;
  entry->next = NULL;
  while (*ptr)
    ptr = &(*ptr)->next;
  *ptr = entry;
  return entry;
}

/* Remove ENTRY from the list of hooks called on duplicating nodes.  */
void
cgraph_remove_node_duplication_hook (struct cgraph_2node_hook_list *entry)
{
  struct cgraph_2node_hook_list **ptr = &first_cgraph_node_duplicated_hook;

  while (*ptr != entry)
    ptr = &(*ptr)->next;
  *ptr = entry->next;
  free (entry);
}

/* Call all node duplication hooks.  */
void
cgraph_call_node_duplication_hooks (struct cgraph_node *node1,
				    struct cgraph_node *node2)
{
  struct cgraph_2node_hook_list *entry = first_cgraph_node_duplicated_hook;
  while (entry)
  {
    entry->hook (node1, node2, entry->data);
    entry = entry->next;
  }
}

/* Allocate new callgraph node.  */

static inline struct cgraph_node *
cgraph_allocate_node (void)
{
  struct cgraph_node *node;

  if (free_nodes)
    {
      node = free_nodes;
      free_nodes = NEXT_FREE_NODE (node);
    }
  else
    {
      node = ggc_alloc_cleared_cgraph_node ();
      node->uid = cgraph_max_uid++;
    }

  return node;
}

/* Allocate new callgraph node and insert it into basic data structures.  */

static struct cgraph_node *
cgraph_create_node_1 (void)
{
  struct cgraph_node *node = cgraph_allocate_node ();

  node->symbol.type = SYMTAB_FUNCTION;
  node->frequency = NODE_FREQUENCY_NORMAL;
  node->count_materialization_scale = REG_BR_PROB_BASE;
  cgraph_n_nodes++;
  return node;
}

/* Return cgraph node assigned to DECL.  Create new one when needed.  */

struct cgraph_node *
cgraph_create_node (tree decl)
{
  struct cgraph_node *node = cgraph_create_node_1 ();
  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);

  node->symbol.decl = decl;
  symtab_register_node ((symtab_node) node);

  if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
    {
      node->origin = cgraph_get_create_node (DECL_CONTEXT (decl));
      node->next_nested = node->origin->nested;
      node->origin->nested = node;
    }
  return node;
}

/* Try to find a call graph node for declaration DECL and if it does not exist,
   create it.  */

struct cgraph_node *
cgraph_get_create_node (tree decl)
{
  struct cgraph_node *node;

  node = cgraph_get_node (decl);
  if (node)
    return node;

  return cgraph_create_node (decl);
}

/* Mark ALIAS as an alias to DECL.  DECL_NODE is cgraph node representing
   the function body is associated with (not neccesarily cgraph_node (DECL).  */

struct cgraph_node *
cgraph_create_function_alias (tree alias, tree decl)
{
  struct cgraph_node *alias_node;

  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  gcc_assert (TREE_CODE (alias) == FUNCTION_DECL);
  alias_node = cgraph_get_create_node (alias);
  gcc_assert (!alias_node->local.finalized);
  alias_node->thunk.alias = decl;
  alias_node->local.finalized = true;
  alias_node->alias = 1;
  return alias_node;
}

/* Attempt to mark ALIAS as an alias to DECL.  Return alias node if successful
   and NULL otherwise.
   Same body aliases are output whenever the body of DECL is output,
   and cgraph_get_node (ALIAS) transparently returns cgraph_get_node (DECL).  */

struct cgraph_node *
cgraph_same_body_alias (struct cgraph_node *decl_node ATTRIBUTE_UNUSED, tree alias, tree decl)
{
  struct cgraph_node *n;
#ifndef ASM_OUTPUT_DEF
  /* If aliases aren't supported by the assembler, fail.  */
  return NULL;
#endif
  /* Langhooks can create same body aliases of symbols not defined.
     Those are useless. Drop them on the floor.  */
  if (cgraph_global_info_ready)
    return NULL;

  n = cgraph_create_function_alias (alias, decl);
  n->same_body_alias = true;
  if (same_body_aliases_done)
    ipa_record_reference ((symtab_node)n, (symtab_node)cgraph_get_node (decl),
			  IPA_REF_ALIAS, NULL);
  return n;
}

/* Add thunk alias into callgraph.  The alias declaration is ALIAS and it
   aliases DECL with an adjustments made into the first parameter.
   See comments in thunk_adjust for detail on the parameters.  */

struct cgraph_node *
cgraph_add_thunk (struct cgraph_node *decl_node ATTRIBUTE_UNUSED,
		  tree alias, tree decl ATTRIBUTE_UNUSED,
		  bool this_adjusting,
		  HOST_WIDE_INT fixed_offset, HOST_WIDE_INT virtual_value,
		  tree virtual_offset,
		  tree real_alias)
{
  struct cgraph_node *node;

  node = cgraph_get_node (alias);
  if (node)
    {
      gcc_assert (node->local.finalized);
      gcc_assert (!node->alias);
      gcc_assert (!node->thunk.thunk_p);
      cgraph_remove_node (node);
    }
  
  node = cgraph_create_node (alias);
  gcc_checking_assert (!virtual_offset
		       || double_int_equal_p
		            (tree_to_double_int (virtual_offset),
			     shwi_to_double_int (virtual_value)));
  node->thunk.fixed_offset = fixed_offset;
  node->thunk.this_adjusting = this_adjusting;
  node->thunk.virtual_value = virtual_value;
  node->thunk.virtual_offset_p = virtual_offset != NULL;
  node->thunk.alias = real_alias;
  node->thunk.thunk_p = true;
  node->local.finalized = true;

  return node;
}

/* Return the cgraph node that has ASMNAME for its DECL_ASSEMBLER_NAME.
   Return NULL if there's no such node.  */

struct cgraph_node *
cgraph_node_for_asm (tree asmname)
{
  symtab_node node = symtab_node_for_asm (asmname);

  /* We do not want to look at inline clones.  */
  for (node = symtab_node_for_asm (asmname); node; node = node->symbol.next_sharing_asm_name)
    if (symtab_function_p (node) && !cgraph(node)->global.inlined_to)
      return cgraph (node);
  return NULL;
}

/* Returns a hash value for X (which really is a die_struct).  */

static hashval_t
edge_hash (const void *x)
{
  return htab_hash_pointer (((const struct cgraph_edge *) x)->call_stmt);
}

/* Return nonzero if decl_id of die_struct X is the same as UID of decl *Y.  */

static int
edge_eq (const void *x, const void *y)
{
  return ((const struct cgraph_edge *) x)->call_stmt == y;
}

/* Add call graph edge E to call site hash of its caller.  */

static inline void
cgraph_add_edge_to_call_site_hash (struct cgraph_edge *e)
{
  void **slot;
  slot = htab_find_slot_with_hash (e->caller->call_site_hash,
				   e->call_stmt,
				   htab_hash_pointer (e->call_stmt),
				   INSERT);
  gcc_assert (!*slot);
  *slot = e;
}

/* Return the callgraph edge representing the GIMPLE_CALL statement
   CALL_STMT.  */

struct cgraph_edge *
cgraph_edge (struct cgraph_node *node, gimple call_stmt)
{
  struct cgraph_edge *e, *e2;
  int n = 0;

  if (node->call_site_hash)
    return (struct cgraph_edge *)
      htab_find_with_hash (node->call_site_hash, call_stmt,
      	                   htab_hash_pointer (call_stmt));

  /* This loop may turn out to be performance problem.  In such case adding
     hashtables into call nodes with very many edges is probably best
     solution.  It is not good idea to add pointer into CALL_EXPR itself
     because we want to make possible having multiple cgraph nodes representing
     different clones of the same body before the body is actually cloned.  */
  for (e = node->callees; e; e = e->next_callee)
    {
      if (e->call_stmt == call_stmt)
	break;
      n++;
    }

  if (!e)
    for (e = node->indirect_calls; e; e = e->next_callee)
      {
	if (e->call_stmt == call_stmt)
	  break;
	n++;
      }

  if (n > 100)
    {
      node->call_site_hash = htab_create_ggc (120, edge_hash, edge_eq, NULL);
      for (e2 = node->callees; e2; e2 = e2->next_callee)
	cgraph_add_edge_to_call_site_hash (e2);
      for (e2 = node->indirect_calls; e2; e2 = e2->next_callee)
	cgraph_add_edge_to_call_site_hash (e2);
    }

  return e;
}


/* Change field call_stmt of edge E to NEW_STMT.  */

void
cgraph_set_call_stmt (struct cgraph_edge *e, gimple new_stmt)
{
  tree decl;

  if (e->caller->call_site_hash)
    {
      htab_remove_elt_with_hash (e->caller->call_site_hash,
				 e->call_stmt,
				 htab_hash_pointer (e->call_stmt));
    }

  e->call_stmt = new_stmt;
  if (e->indirect_unknown_callee
      && (decl = gimple_call_fndecl (new_stmt)))
    {
      /* Constant propagation (and possibly also inlining?) can turn an
	 indirect call into a direct one.  */
      struct cgraph_node *new_callee = cgraph_get_node (decl);

      gcc_checking_assert (new_callee);
      cgraph_make_edge_direct (e, new_callee);
    }

  push_cfun (DECL_STRUCT_FUNCTION (e->caller->symbol.decl));
  e->can_throw_external = stmt_can_throw_external (new_stmt);
  pop_cfun ();
  if (e->caller->call_site_hash)
    cgraph_add_edge_to_call_site_hash (e);
}

/* Like cgraph_set_call_stmt but walk the clone tree and update all
   clones sharing the same function body.  */

void
cgraph_set_call_stmt_including_clones (struct cgraph_node *orig,
				       gimple old_stmt, gimple new_stmt)
{
  struct cgraph_node *node;
  struct cgraph_edge *edge = cgraph_edge (orig, old_stmt);

  if (edge)
    cgraph_set_call_stmt (edge, new_stmt);

  node = orig->clones;
  if (node)
    while (node != orig)
      {
	struct cgraph_edge *edge = cgraph_edge (node, old_stmt);
	if (edge)
	  cgraph_set_call_stmt (edge, new_stmt);
	if (node->clones)
	  node = node->clones;
	else if (node->next_sibling_clone)
	  node = node->next_sibling_clone;
	else
	  {
	    while (node != orig && !node->next_sibling_clone)
	      node = node->clone_of;
	    if (node != orig)
	      node = node->next_sibling_clone;
	  }
      }
}

/* Like cgraph_create_edge walk the clone tree and update all clones sharing
   same function body.  If clones already have edge for OLD_STMT; only
   update the edge same way as cgraph_set_call_stmt_including_clones does.

   TODO: COUNT and LOOP_DEPTH should be properly distributed based on relative
   frequencies of the clones.  */

void
cgraph_create_edge_including_clones (struct cgraph_node *orig,
				     struct cgraph_node *callee,
				     gimple old_stmt,
				     gimple stmt, gcov_type count,
				     int freq,
				     cgraph_inline_failed_t reason)
{
  struct cgraph_node *node;
  struct cgraph_edge *edge;

  if (!cgraph_edge (orig, stmt))
    {
      edge = cgraph_create_edge (orig, callee, stmt, count, freq);
      edge->inline_failed = reason;
    }

  node = orig->clones;
  if (node)
    while (node != orig)
      {
	struct cgraph_edge *edge = cgraph_edge (node, old_stmt);

        /* It is possible that clones already contain the edge while
	   master didn't.  Either we promoted indirect call into direct
	   call in the clone or we are processing clones of unreachable
	   master where edges has been removed.  */
	if (edge)
	  cgraph_set_call_stmt (edge, stmt);
	else if (!cgraph_edge (node, stmt))
	  {
	    edge = cgraph_create_edge (node, callee, stmt, count,
				       freq);
	    edge->inline_failed = reason;
	  }

	if (node->clones)
	  node = node->clones;
	else if (node->next_sibling_clone)
	  node = node->next_sibling_clone;
	else
	  {
	    while (node != orig && !node->next_sibling_clone)
	      node = node->clone_of;
	    if (node != orig)
	      node = node->next_sibling_clone;
	  }
      }
}

/* Allocate a cgraph_edge structure and fill it with data according to the
   parameters of which only CALLEE can be NULL (when creating an indirect call
   edge).  */

static struct cgraph_edge *
cgraph_create_edge_1 (struct cgraph_node *caller, struct cgraph_node *callee,
		       gimple call_stmt, gcov_type count, int freq)
{
  struct cgraph_edge *edge;

  /* LTO does not actually have access to the call_stmt since these
     have not been loaded yet.  */
  if (call_stmt)
    {
      /* This is a rather expensive check possibly triggering
	 construction of call stmt hashtable.  */
      gcc_checking_assert (!cgraph_edge (caller, call_stmt));

      gcc_assert (is_gimple_call (call_stmt));
    }

  if (free_edges)
    {
      edge = free_edges;
      free_edges = NEXT_FREE_EDGE (edge);
    }
  else
    {
      edge = ggc_alloc_cgraph_edge ();
      edge->uid = cgraph_edge_max_uid++;
    }

  edge->aux = NULL;
  edge->caller = caller;
  edge->callee = callee;
  edge->prev_caller = NULL;
  edge->next_caller = NULL;
  edge->prev_callee = NULL;
  edge->next_callee = NULL;

  edge->count = count;
  gcc_assert (count >= 0);
  edge->frequency = freq;
  gcc_assert (freq >= 0);
  gcc_assert (freq <= CGRAPH_FREQ_MAX);

  edge->call_stmt = call_stmt;
  push_cfun (DECL_STRUCT_FUNCTION (caller->symbol.decl));
  edge->can_throw_external
    = call_stmt ? stmt_can_throw_external (call_stmt) : false;
  pop_cfun ();
  if (call_stmt
      && callee && callee->symbol.decl
      && !gimple_check_call_matching_types (call_stmt, callee->symbol.decl))
    edge->call_stmt_cannot_inline_p = true;
  else
    edge->call_stmt_cannot_inline_p = false;
  if (call_stmt && caller->call_site_hash)
    cgraph_add_edge_to_call_site_hash (edge);

  edge->indirect_info = NULL;
  edge->indirect_inlining_edge = 0;

  return edge;
}

/* Create edge from CALLER to CALLEE in the cgraph.  */

struct cgraph_edge *
cgraph_create_edge (struct cgraph_node *caller, struct cgraph_node *callee,
		    gimple call_stmt, gcov_type count, int freq)
{
  struct cgraph_edge *edge = cgraph_create_edge_1 (caller, callee, call_stmt,
						   count, freq);

  edge->indirect_unknown_callee = 0;
  initialize_inline_failed (edge);

  edge->next_caller = callee->callers;
  if (callee->callers)
    callee->callers->prev_caller = edge;
  edge->next_callee = caller->callees;
  if (caller->callees)
    caller->callees->prev_callee = edge;
  caller->callees = edge;
  callee->callers = edge;

  return edge;
}

/* Allocate cgraph_indirect_call_info and set its fields to default values. */

struct cgraph_indirect_call_info *
cgraph_allocate_init_indirect_info (void)
{
  struct cgraph_indirect_call_info *ii;

  ii = ggc_alloc_cleared_cgraph_indirect_call_info ();
  ii->param_index = -1;
  return ii;
}

/* Create an indirect edge with a yet-undetermined callee where the call
   statement destination is a formal parameter of the caller with index
   PARAM_INDEX. */

struct cgraph_edge *
cgraph_create_indirect_edge (struct cgraph_node *caller, gimple call_stmt,
			     int ecf_flags,
			     gcov_type count, int freq)
{
  struct cgraph_edge *edge = cgraph_create_edge_1 (caller, NULL, call_stmt,
						   count, freq);

  edge->indirect_unknown_callee = 1;
  initialize_inline_failed (edge);

  edge->indirect_info = cgraph_allocate_init_indirect_info ();
  edge->indirect_info->ecf_flags = ecf_flags;

  edge->next_callee = caller->indirect_calls;
  if (caller->indirect_calls)
    caller->indirect_calls->prev_callee = edge;
  caller->indirect_calls = edge;

  return edge;
}

/* Remove the edge E from the list of the callers of the callee.  */

static inline void
cgraph_edge_remove_callee (struct cgraph_edge *e)
{
  gcc_assert (!e->indirect_unknown_callee);
  if (e->prev_caller)
    e->prev_caller->next_caller = e->next_caller;
  if (e->next_caller)
    e->next_caller->prev_caller = e->prev_caller;
  if (!e->prev_caller)
    e->callee->callers = e->next_caller;
}

/* Remove the edge E from the list of the callees of the caller.  */

static inline void
cgraph_edge_remove_caller (struct cgraph_edge *e)
{
  if (e->prev_callee)
    e->prev_callee->next_callee = e->next_callee;
  if (e->next_callee)
    e->next_callee->prev_callee = e->prev_callee;
  if (!e->prev_callee)
    {
      if (e->indirect_unknown_callee)
	e->caller->indirect_calls = e->next_callee;
      else
	e->caller->callees = e->next_callee;
    }
  if (e->caller->call_site_hash)
    htab_remove_elt_with_hash (e->caller->call_site_hash,
			       e->call_stmt,
	  		       htab_hash_pointer (e->call_stmt));
}

/* Put the edge onto the free list.  */

static void
cgraph_free_edge (struct cgraph_edge *e)
{
  int uid = e->uid;

  /* Clear out the edge so we do not dangle pointers.  */
  memset (e, 0, sizeof (*e));
  e->uid = uid;
  NEXT_FREE_EDGE (e) = free_edges;
  free_edges = e;
}

/* Remove the edge E in the cgraph.  */

void
cgraph_remove_edge (struct cgraph_edge *e)
{
  /* Call all edge removal hooks.  */
  cgraph_call_edge_removal_hooks (e);

  if (!e->indirect_unknown_callee)
    /* Remove from callers list of the callee.  */
    cgraph_edge_remove_callee (e);

  /* Remove from callees list of the callers.  */
  cgraph_edge_remove_caller (e);

  /* Put the edge onto the free list.  */
  cgraph_free_edge (e);
}

/* Set callee of call graph edge E and add it to the corresponding set of
   callers. */

static void
cgraph_set_edge_callee (struct cgraph_edge *e, struct cgraph_node *n)
{
  e->prev_caller = NULL;
  if (n->callers)
    n->callers->prev_caller = e;
  e->next_caller = n->callers;
  n->callers = e;
  e->callee = n;
}

/* Redirect callee of E to N.  The function does not update underlying
   call expression.  */

void
cgraph_redirect_edge_callee (struct cgraph_edge *e, struct cgraph_node *n)
{
  /* Remove from callers list of the current callee.  */
  cgraph_edge_remove_callee (e);

  /* Insert to callers list of the new callee.  */
  cgraph_set_edge_callee (e, n);
}

/* Make an indirect EDGE with an unknown callee an ordinary edge leading to
   CALLEE.  DELTA is an integer constant that is to be added to the this
   pointer (first parameter) to compensate for skipping a thunk adjustment.  */

void
cgraph_make_edge_direct (struct cgraph_edge *edge, struct cgraph_node *callee)
{
  edge->indirect_unknown_callee = 0;

  /* Get the edge out of the indirect edge list. */
  if (edge->prev_callee)
    edge->prev_callee->next_callee = edge->next_callee;
  if (edge->next_callee)
    edge->next_callee->prev_callee = edge->prev_callee;
  if (!edge->prev_callee)
    edge->caller->indirect_calls = edge->next_callee;

  /* Put it into the normal callee list */
  edge->prev_callee = NULL;
  edge->next_callee = edge->caller->callees;
  if (edge->caller->callees)
    edge->caller->callees->prev_callee = edge;
  edge->caller->callees = edge;

  /* Insert to callers list of the new callee.  */
  cgraph_set_edge_callee (edge, callee);

  if (edge->call_stmt)
    edge->call_stmt_cannot_inline_p
      = !gimple_check_call_matching_types (edge->call_stmt, callee->symbol.decl);

  /* We need to re-determine the inlining status of the edge.  */
  initialize_inline_failed (edge);
}


/* Update or remove the corresponding cgraph edge if a GIMPLE_CALL
   OLD_STMT changed into NEW_STMT.  OLD_CALL is gimple_call_fndecl
   of OLD_STMT if it was previously call statement.
   If NEW_STMT is NULL, the call has been dropped without any
   replacement.  */

static void
cgraph_update_edges_for_call_stmt_node (struct cgraph_node *node,
					gimple old_stmt, tree old_call,
					gimple new_stmt)
{
  tree new_call = (new_stmt && is_gimple_call (new_stmt))
		  ? gimple_call_fndecl (new_stmt) : 0;

  /* We are seeing indirect calls, then there is nothing to update.  */
  if (!new_call && !old_call)
    return;
  /* See if we turned indirect call into direct call or folded call to one builtin
     into different builtin.  */
  if (old_call != new_call)
    {
      struct cgraph_edge *e = cgraph_edge (node, old_stmt);
      struct cgraph_edge *ne = NULL;
      gcov_type count;
      int frequency;

      if (e)
	{
	  /* See if the edge is already there and has the correct callee.  It
	     might be so because of indirect inlining has already updated
	     it.  We also might've cloned and redirected the edge.  */
	  if (new_call && e->callee)
	    {
	      struct cgraph_node *callee = e->callee;
	      while (callee)
		{
		  if (callee->symbol.decl == new_call
		      || callee->former_clone_of == new_call)
		    return;
		  callee = callee->clone_of;
		}
	    }

	  /* Otherwise remove edge and create new one; we can't simply redirect
	     since function has changed, so inline plan and other information
	     attached to edge is invalid.  */
	  count = e->count;
	  frequency = e->frequency;
	  cgraph_remove_edge (e);
	}
      else if (new_call)
	{
	  /* We are seeing new direct call; compute profile info based on BB.  */
	  basic_block bb = gimple_bb (new_stmt);
	  count = bb->count;
	  frequency = compute_call_stmt_bb_frequency (current_function_decl,
						      bb);
	}

      if (new_call)
	{
	  ne = cgraph_create_edge (node, cgraph_get_create_node (new_call),
				   new_stmt, count, frequency);
	  gcc_assert (ne->inline_failed);
	}
    }
  /* We only updated the call stmt; update pointer in cgraph edge..  */
  else if (old_stmt != new_stmt)
    cgraph_set_call_stmt (cgraph_edge (node, old_stmt), new_stmt);
}

/* Update or remove the corresponding cgraph edge if a GIMPLE_CALL
   OLD_STMT changed into NEW_STMT.  OLD_DECL is gimple_call_fndecl
   of OLD_STMT before it was updated (updating can happen inplace).  */

void
cgraph_update_edges_for_call_stmt (gimple old_stmt, tree old_decl, gimple new_stmt)
{
  struct cgraph_node *orig = cgraph_get_node (cfun->decl);
  struct cgraph_node *node;

  gcc_checking_assert (orig);
  cgraph_update_edges_for_call_stmt_node (orig, old_stmt, old_decl, new_stmt);
  if (orig->clones)
    for (node = orig->clones; node != orig;)
      {
        cgraph_update_edges_for_call_stmt_node (node, old_stmt, old_decl, new_stmt);
	if (node->clones)
	  node = node->clones;
	else if (node->next_sibling_clone)
	  node = node->next_sibling_clone;
	else
	  {
	    while (node != orig && !node->next_sibling_clone)
	      node = node->clone_of;
	    if (node != orig)
	      node = node->next_sibling_clone;
	  }
      }
}


/* Remove all callees from the node.  */

void
cgraph_node_remove_callees (struct cgraph_node *node)
{
  struct cgraph_edge *e, *f;

  /* It is sufficient to remove the edges from the lists of callers of
     the callees.  The callee list of the node can be zapped with one
     assignment.  */
  for (e = node->callees; e; e = f)
    {
      f = e->next_callee;
      cgraph_call_edge_removal_hooks (e);
      if (!e->indirect_unknown_callee)
	cgraph_edge_remove_callee (e);
      cgraph_free_edge (e);
    }
  for (e = node->indirect_calls; e; e = f)
    {
      f = e->next_callee;
      cgraph_call_edge_removal_hooks (e);
      if (!e->indirect_unknown_callee)
	cgraph_edge_remove_callee (e);
      cgraph_free_edge (e);
    }
  node->indirect_calls = NULL;
  node->callees = NULL;
  if (node->call_site_hash)
    {
      htab_delete (node->call_site_hash);
      node->call_site_hash = NULL;
    }
}

/* Remove all callers from the node.  */

static void
cgraph_node_remove_callers (struct cgraph_node *node)
{
  struct cgraph_edge *e, *f;

  /* It is sufficient to remove the edges from the lists of callees of
     the callers.  The caller list of the node can be zapped with one
     assignment.  */
  for (e = node->callers; e; e = f)
    {
      f = e->next_caller;
      cgraph_call_edge_removal_hooks (e);
      cgraph_edge_remove_caller (e);
      cgraph_free_edge (e);
    }
  node->callers = NULL;
}

/* Release memory used to represent body of function NODE.  */

void
cgraph_release_function_body (struct cgraph_node *node)
{
  if (DECL_STRUCT_FUNCTION (node->symbol.decl))
    {
      tree old_decl = current_function_decl;
      push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));
      if (cfun->cfg
	  && current_loops)
	{
	  cfun->curr_properties &= ~PROP_loops;
	  loop_optimizer_finalize ();
	}
      if (cfun->gimple_df)
	{
	  current_function_decl = node->symbol.decl;
	  delete_tree_ssa ();
	  delete_tree_cfg_annotations ();
	  cfun->eh = NULL;
	  current_function_decl = old_decl;
	}
      if (cfun->cfg)
	{
	  gcc_assert (dom_computed[0] == DOM_NONE);
	  gcc_assert (dom_computed[1] == DOM_NONE);
	  clear_edges ();
	}
      if (cfun->value_histograms)
	free_histograms ();
      pop_cfun();
      gimple_set_body (node->symbol.decl, NULL);
      VEC_free (ipa_opt_pass, heap,
      		node->ipa_transforms_to_apply);
      /* Struct function hangs a lot of data that would leak if we didn't
         removed all pointers to it.   */
      ggc_free (DECL_STRUCT_FUNCTION (node->symbol.decl));
      DECL_STRUCT_FUNCTION (node->symbol.decl) = NULL;
    }
  DECL_SAVED_TREE (node->symbol.decl) = NULL;
  /* If the node is abstract and needed, then do not clear DECL_INITIAL
     of its associated function function declaration because it's
     needed to emit debug info later.  */
  if (!node->abstract_and_needed)
    DECL_INITIAL (node->symbol.decl) = error_mark_node;
}

/* NODE is being removed from symbol table; see if its entry can be replaced by
   other inline clone.  */
struct cgraph_node *
cgraph_find_replacement_node (struct cgraph_node *node)
{
  struct cgraph_node *next_inline_clone, *replacement;

  for (next_inline_clone = node->clones;
       next_inline_clone
       && next_inline_clone->symbol.decl != node->symbol.decl;
       next_inline_clone = next_inline_clone->next_sibling_clone)
    ;

  /* If there is inline clone of the node being removed, we need
     to put it into the position of removed node and reorganize all
     other clones to be based on it.  */
  if (next_inline_clone)
    {
      struct cgraph_node *n;
      struct cgraph_node *new_clones;

      replacement = next_inline_clone;

      /* Unlink inline clone from the list of clones of removed node.  */
      if (next_inline_clone->next_sibling_clone)
	next_inline_clone->next_sibling_clone->prev_sibling_clone
	  = next_inline_clone->prev_sibling_clone;
      if (next_inline_clone->prev_sibling_clone)
	{
	  gcc_assert (node->clones != next_inline_clone);
	  next_inline_clone->prev_sibling_clone->next_sibling_clone
	    = next_inline_clone->next_sibling_clone;
	}
      else
	{
	  gcc_assert (node->clones == next_inline_clone);
	  node->clones = next_inline_clone->next_sibling_clone;
	}

      new_clones = node->clones;
      node->clones = NULL;

      /* Copy clone info.  */
      next_inline_clone->clone = node->clone;

      /* Now place it into clone tree at same level at NODE.  */
      next_inline_clone->clone_of = node->clone_of;
      next_inline_clone->prev_sibling_clone = NULL;
      next_inline_clone->next_sibling_clone = NULL;
      if (node->clone_of)
	{
	  if (node->clone_of->clones)
	    node->clone_of->clones->prev_sibling_clone = next_inline_clone;
	  next_inline_clone->next_sibling_clone = node->clone_of->clones;
	  node->clone_of->clones = next_inline_clone;
	}

      /* Merge the clone list.  */
      if (new_clones)
	{
	  if (!next_inline_clone->clones)
	    next_inline_clone->clones = new_clones;
	  else
	    {
	      n = next_inline_clone->clones;
	      while (n->next_sibling_clone)
		n =  n->next_sibling_clone;
	      n->next_sibling_clone = new_clones;
	      new_clones->prev_sibling_clone = n;
	    }
	}

      /* Update clone_of pointers.  */
      n = new_clones;
      while (n)
	{
	  n->clone_of = next_inline_clone;
	  n = n->next_sibling_clone;
	}
      return replacement;
    }
  else
    return NULL;
}

/* Remove the node from cgraph.  */

void
cgraph_remove_node (struct cgraph_node *node)
{
  struct cgraph_node *n;
  int uid = node->uid;

  cgraph_call_node_removal_hooks (node);
  cgraph_node_remove_callers (node);
  cgraph_node_remove_callees (node);
  VEC_free (ipa_opt_pass, heap,
            node->ipa_transforms_to_apply);

  /* Incremental inlining access removed nodes stored in the postorder list.
     */
  node->symbol.force_output = false;
  for (n = node->nested; n; n = n->next_nested)
    n->origin = NULL;
  node->nested = NULL;
  if (node->origin)
    {
      struct cgraph_node **node2 = &node->origin->nested;

      while (*node2 != node)
	node2 = &(*node2)->next_nested;
      *node2 = node->next_nested;
    }
  symtab_unregister_node ((symtab_node)node);
  if (node->prev_sibling_clone)
    node->prev_sibling_clone->next_sibling_clone = node->next_sibling_clone;
  else if (node->clone_of)
    node->clone_of->clones = node->next_sibling_clone;
  if (node->next_sibling_clone)
    node->next_sibling_clone->prev_sibling_clone = node->prev_sibling_clone;
  if (node->clones)
    {
      struct cgraph_node *n, *next;

      if (node->clone_of)
        {
	  for (n = node->clones; n->next_sibling_clone; n = n->next_sibling_clone)
	    n->clone_of = node->clone_of;
	  n->clone_of = node->clone_of;
	  n->next_sibling_clone = node->clone_of->clones;
	  if (node->clone_of->clones)
	    node->clone_of->clones->prev_sibling_clone = n;
	  node->clone_of->clones = node->clones;
	}
      else
        {
	  /* We are removing node with clones.  this makes clones inconsistent,
	     but assume they will be removed subsequently and just keep clone
	     tree intact.  This can happen in unreachable function removal since
	     we remove unreachable functions in random order, not by bottom-up
	     walk of clone trees.  */
	  for (n = node->clones; n; n = next)
	    {
	       next = n->next_sibling_clone;
	       n->next_sibling_clone = NULL;
	       n->prev_sibling_clone = NULL;
	       n->clone_of = NULL;
	    }
	}
    }

  /* While all the clones are removed after being proceeded, the function
     itself is kept in the cgraph even after it is compiled.  Check whether
     we are done with this body and reclaim it proactively if this is the case.
     */
  n = cgraph_get_node (node->symbol.decl);
  if (!n
      || (!n->clones && !n->clone_of && !n->global.inlined_to
	  && (cgraph_global_info_ready
	      && (TREE_ASM_WRITTEN (n->symbol.decl)
		  || DECL_EXTERNAL (n->symbol.decl)
		  || n->symbol.in_other_partition))))
    cgraph_release_function_body (node);

  node->symbol.decl = NULL;
  if (node->call_site_hash)
    {
      htab_delete (node->call_site_hash);
      node->call_site_hash = NULL;
    }
  cgraph_n_nodes--;

  /* Clear out the node to NULL all pointers and add the node to the free
     list.  */
  memset (node, 0, sizeof(*node));
  node->symbol.type = SYMTAB_FUNCTION;
  node->uid = uid;
  SET_NEXT_FREE_NODE (node, free_nodes);
  free_nodes = node;
}

/* Add NEW_ to the same comdat group that OLD is in.  */

void
cgraph_add_to_same_comdat_group (struct cgraph_node *new_node,
				 struct cgraph_node *old_node)
{
  gcc_assert (DECL_ONE_ONLY (old_node->symbol.decl));
  gcc_assert (!new_node->symbol.same_comdat_group);
  gcc_assert (new_node != old_node);

  DECL_COMDAT_GROUP (new_node->symbol.decl) = DECL_COMDAT_GROUP (old_node->symbol.decl);
  new_node->symbol.same_comdat_group = (symtab_node)old_node;
  if (!old_node->symbol.same_comdat_group)
    old_node->symbol.same_comdat_group = (symtab_node)new_node;
  else
    {
      symtab_node n;
      for (n = old_node->symbol.same_comdat_group;
	   n->symbol.same_comdat_group != (symtab_node)old_node;
	   n = n->symbol.same_comdat_group)
	;
      n->symbol.same_comdat_group = (symtab_node)new_node;
    }
}

/* Remove the node from cgraph and all inline clones inlined into it.
   Skip however removal of FORBIDDEN_NODE and return true if it needs to be
   removed.  This allows to call the function from outer loop walking clone
   tree.  */

bool
cgraph_remove_node_and_inline_clones (struct cgraph_node *node, struct cgraph_node *forbidden_node)
{
  struct cgraph_edge *e, *next;
  bool found = false;

  if (node == forbidden_node)
    return true;
  for (e = node->callees; e; e = next)
    {
      next = e->next_callee;
      if (!e->inline_failed)
        found |= cgraph_remove_node_and_inline_clones (e->callee, forbidden_node);
    }
  cgraph_remove_node (node);
  return found;
}

/* Likewise indicate that a node is having address taken.  */

void
cgraph_mark_address_taken_node (struct cgraph_node *node)
{
  gcc_assert (!node->global.inlined_to);
  /* FIXME: address_taken flag is used both as a shortcut for testing whether
     IPA_REF_ADDR reference exists (and thus it should be set on node
     representing alias we take address of) and as a test whether address
     of the object was taken (and thus it should be set on node alias is
     referring to).  We should remove the first use and the remove the
     following set.  */
  node->symbol.address_taken = 1;
  node = cgraph_function_or_thunk_node (node, NULL);
  node->symbol.address_taken = 1;
}

/* Return local info for the compiled function.  */

struct cgraph_local_info *
cgraph_local_info (tree decl)
{
  struct cgraph_node *node;

  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  node = cgraph_get_node (decl);
  if (!node)
    return NULL;
  return &node->local;
}

/* Return local info for the compiled function.  */

struct cgraph_global_info *
cgraph_global_info (tree decl)
{
  struct cgraph_node *node;

  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL && cgraph_global_info_ready);
  node = cgraph_get_node (decl);
  if (!node)
    return NULL;
  return &node->global;
}

/* Return local info for the compiled function.  */

struct cgraph_rtl_info *
cgraph_rtl_info (tree decl)
{
  struct cgraph_node *node;

  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  node = cgraph_get_node (decl);
  if (!node
      || (decl != current_function_decl
	  && !TREE_ASM_WRITTEN (node->symbol.decl)))
    return NULL;
  return &node->rtl;
}

/* Return a string describing the failure REASON.  */

const char*
cgraph_inline_failed_string (cgraph_inline_failed_t reason)
{
#undef DEFCIFCODE
#define DEFCIFCODE(code, string)	string,

  static const char *cif_string_table[CIF_N_REASONS] = {
#include "cif-code.def"
  };

  /* Signedness of an enum type is implementation defined, so cast it
     to unsigned before testing. */
  gcc_assert ((unsigned) reason < CIF_N_REASONS);
  return cif_string_table[reason];
}

/* Names used to print out the availability enum.  */
const char * const cgraph_availability_names[] =
  {"unset", "not_available", "overwritable", "available", "local"};


/* Dump call graph node NODE to file F.  */

void
dump_cgraph_node (FILE *f, struct cgraph_node *node)
{
  struct cgraph_edge *edge;
  int indirect_calls_count = 0;

  dump_symtab_base (f, (symtab_node) node);

  if (node->global.inlined_to)
    fprintf (f, "  Function %s/%i is inline copy in %s/%i\n",
	     cgraph_node_name (node),
	     node->symbol.order,
	     cgraph_node_name (node->global.inlined_to),
	     node->global.inlined_to->symbol.order);
  if (node->clone_of)
    fprintf (f, "  Clone of %s/%i\n",
	     cgraph_node_asm_name (node->clone_of),
	     node->clone_of->symbol.order);
  if (cgraph_function_flags_ready)
    fprintf (f, "  Availability: %s\n",
	     cgraph_availability_names [cgraph_function_body_availability (node)]);

  fprintf (f, "  Function flags:");
  if (node->analyzed)
    fprintf (f, " analyzed");
  if (node->count)
    fprintf (f, " executed "HOST_WIDEST_INT_PRINT_DEC"x",
	     (HOST_WIDEST_INT)node->count);
  if (node->origin)
    fprintf (f, " nested in: %s", cgraph_node_asm_name (node->origin));
  if (gimple_has_body_p (node->symbol.decl))
    fprintf (f, " body");
  if (node->process)
    fprintf (f, " process");
  if (node->local.local)
    fprintf (f, " local");
  if (node->local.finalized)
    fprintf (f, " finalized");
  if (node->local.redefined_extern_inline)
    fprintf (f, " redefined_extern_inline");
  if (node->only_called_at_startup)
    fprintf (f, " only_called_at_startup");
  if (node->only_called_at_exit)
    fprintf (f, " only_called_at_exit");
  else if (node->alias)
    fprintf (f, " alias");
  if (node->tm_clone)
    fprintf (f, " tm_clone");

  fprintf (f, "\n");

  if (node->thunk.thunk_p)
    {
      fprintf (f, "  Thunk of %s (asm: %s) fixed offset %i virtual value %i has "
	       "virtual offset %i)\n",
	       lang_hooks.decl_printable_name (node->thunk.alias, 2),
	       IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (node->thunk.alias)),
	       (int)node->thunk.fixed_offset,
	       (int)node->thunk.virtual_value,
	       (int)node->thunk.virtual_offset_p);
    }
  if (node->alias && node->thunk.alias)
    {
      fprintf (f, "  Alias of %s",
	       lang_hooks.decl_printable_name (node->thunk.alias, 2));
      if (DECL_ASSEMBLER_NAME_SET_P (node->thunk.alias))
        fprintf (f, " (asm: %s)",
		 IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (node->thunk.alias)));
      fprintf (f, "\n");
    }
  
  fprintf (f, "  Called by: ");

  for (edge = node->callers; edge; edge = edge->next_caller)
    {
      fprintf (f, "%s/%i ", cgraph_node_asm_name (edge->caller),
	       edge->caller->symbol.order);
      if (edge->count)
	fprintf (f, "("HOST_WIDEST_INT_PRINT_DEC"x) ",
		 (HOST_WIDEST_INT)edge->count);
      if (edge->frequency)
	fprintf (f, "(%.2f per call) ",
		 edge->frequency / (double)CGRAPH_FREQ_BASE);
      if (!edge->inline_failed)
	fprintf(f, "(inlined) ");
      if (edge->indirect_inlining_edge)
	fprintf(f, "(indirect_inlining) ");
      if (edge->can_throw_external)
	fprintf(f, "(can throw external) ");
    }

  fprintf (f, "\n  Calls: ");
  for (edge = node->callees; edge; edge = edge->next_callee)
    {
      fprintf (f, "%s/%i ", cgraph_node_asm_name (edge->callee),
	       edge->callee->symbol.order);
      if (!edge->inline_failed)
	fprintf(f, "(inlined) ");
      if (edge->indirect_inlining_edge)
	fprintf(f, "(indirect_inlining) ");
      if (edge->count)
	fprintf (f, "("HOST_WIDEST_INT_PRINT_DEC"x) ",
		 (HOST_WIDEST_INT)edge->count);
      if (edge->frequency)
	fprintf (f, "(%.2f per call) ",
		 edge->frequency / (double)CGRAPH_FREQ_BASE);
      if (edge->can_throw_external)
	fprintf(f, "(can throw external) ");
    }
  fprintf (f, "\n");

  for (edge = node->indirect_calls; edge; edge = edge->next_callee)
    indirect_calls_count++;
  if (indirect_calls_count)
    fprintf (f, "  Has %i outgoing edges for indirect calls.\n",
	     indirect_calls_count);
}


/* Dump call graph node NODE to stderr.  */

DEBUG_FUNCTION void
debug_cgraph_node (struct cgraph_node *node)
{
  dump_cgraph_node (stderr, node);
}


/* Dump the callgraph to file F.  */

void
dump_cgraph (FILE *f)
{
  struct cgraph_node *node;

  fprintf (f, "callgraph:\n\n");
  FOR_EACH_FUNCTION (node)
    dump_cgraph_node (f, node);
}


/* Dump the call graph to stderr.  */

DEBUG_FUNCTION void
debug_cgraph (void)
{
  dump_cgraph (stderr);
}

/* Add a top-level asm statement to the list.  */

struct cgraph_asm_node *
cgraph_add_asm_node (tree asm_str)
{
  struct cgraph_asm_node *node;

  node = ggc_alloc_cleared_cgraph_asm_node ();
  node->asm_str = asm_str;
  node->order = symtab_order++;
  node->next = NULL;
  if (cgraph_asm_nodes == NULL)
    cgraph_asm_nodes = node;
  else
    cgraph_asm_last_node->next = node;
  cgraph_asm_last_node = node;
  return node;
}

/* Return true when the DECL can possibly be inlined.  */
bool
cgraph_function_possibly_inlined_p (tree decl)
{
  if (!cgraph_global_info_ready)
    return !DECL_UNINLINABLE (decl);
  return DECL_POSSIBLY_INLINED (decl);
}

/* Create clone of E in the node N represented by CALL_EXPR the callgraph.  */
struct cgraph_edge *
cgraph_clone_edge (struct cgraph_edge *e, struct cgraph_node *n,
		   gimple call_stmt, unsigned stmt_uid, gcov_type count_scale,
		   int freq_scale, bool update_original)
{
  struct cgraph_edge *new_edge;
  gcov_type count = e->count * count_scale / REG_BR_PROB_BASE;
  gcov_type freq;

  /* We do not want to ignore loop nest after frequency drops to 0.  */
  if (!freq_scale)
    freq_scale = 1;
  freq = e->frequency * (gcov_type) freq_scale / CGRAPH_FREQ_BASE;
  if (freq > CGRAPH_FREQ_MAX)
    freq = CGRAPH_FREQ_MAX;

  if (e->indirect_unknown_callee)
    {
      tree decl;

      if (call_stmt && (decl = gimple_call_fndecl (call_stmt)))
	{
	  struct cgraph_node *callee = cgraph_get_node (decl);
	  gcc_checking_assert (callee);
	  new_edge = cgraph_create_edge (n, callee, call_stmt, count, freq);
	}
      else
	{
	  new_edge = cgraph_create_indirect_edge (n, call_stmt,
						  e->indirect_info->ecf_flags,
						  count, freq);
	  *new_edge->indirect_info = *e->indirect_info;
	}
    }
  else
    {
      new_edge = cgraph_create_edge (n, e->callee, call_stmt, count, freq);
      if (e->indirect_info)
	{
	  new_edge->indirect_info
	    = ggc_alloc_cleared_cgraph_indirect_call_info ();
	  *new_edge->indirect_info = *e->indirect_info;
	}
    }

  new_edge->inline_failed = e->inline_failed;
  new_edge->indirect_inlining_edge = e->indirect_inlining_edge;
  new_edge->lto_stmt_uid = stmt_uid;
  /* Clone flags that depend on call_stmt availability manually.  */
  new_edge->can_throw_external = e->can_throw_external;
  new_edge->call_stmt_cannot_inline_p = e->call_stmt_cannot_inline_p;
  if (update_original)
    {
      e->count -= new_edge->count;
      if (e->count < 0)
	e->count = 0;
    }
  cgraph_call_edge_duplication_hooks (e, new_edge);
  return new_edge;
}


/* Create node representing clone of N executed COUNT times.  Decrease
   the execution counts from original node too.
   The new clone will have decl set to DECL that may or may not be the same
   as decl of N.

   When UPDATE_ORIGINAL is true, the counts are subtracted from the original
   function's profile to reflect the fact that part of execution is handled
   by node.  
   When CALL_DUPLICATOIN_HOOK is true, the ipa passes are acknowledged about
   the new clone. Otherwise the caller is responsible for doing so later.  */

struct cgraph_node *
cgraph_clone_node (struct cgraph_node *n, tree decl, gcov_type count, int freq,
		   bool update_original,
		   VEC(cgraph_edge_p,heap) *redirect_callers,
		   bool call_duplication_hook)
{
  struct cgraph_node *new_node = cgraph_create_node_1 ();
  struct cgraph_edge *e;
  gcov_type count_scale;
  unsigned i;

  new_node->symbol.decl = decl;
  symtab_register_node ((symtab_node)new_node);
  new_node->origin = n->origin;
  if (new_node->origin)
    {
      new_node->next_nested = new_node->origin->nested;
      new_node->origin->nested = new_node;
    }
  new_node->analyzed = n->analyzed;
  new_node->local = n->local;
  new_node->symbol.externally_visible = false;
  new_node->local.local = true;
  new_node->global = n->global;
  new_node->rtl = n->rtl;
  new_node->count = count;
  new_node->frequency = n->frequency;
  new_node->clone = n->clone;
  new_node->clone.tree_map = 0;
  if (n->count)
    {
      if (new_node->count > n->count)
        count_scale = REG_BR_PROB_BASE;
      else
        count_scale = new_node->count * REG_BR_PROB_BASE / n->count;
    }
  else
    count_scale = 0;
  if (update_original)
    {
      n->count -= count;
      if (n->count < 0)
	n->count = 0;
    }

  FOR_EACH_VEC_ELT (cgraph_edge_p, redirect_callers, i, e)
    {
      /* Redirect calls to the old version node to point to its new
	 version.  */
      cgraph_redirect_edge_callee (e, new_node);
    }


  for (e = n->callees;e; e=e->next_callee)
    cgraph_clone_edge (e, new_node, e->call_stmt, e->lto_stmt_uid,
		       count_scale, freq, update_original);

  for (e = n->indirect_calls; e; e = e->next_callee)
    cgraph_clone_edge (e, new_node, e->call_stmt, e->lto_stmt_uid,
		       count_scale, freq, update_original);
  ipa_clone_references ((symtab_node)new_node, &n->symbol.ref_list);

  new_node->next_sibling_clone = n->clones;
  if (n->clones)
    n->clones->prev_sibling_clone = new_node;
  n->clones = new_node;
  new_node->clone_of = n;

  if (call_duplication_hook)
    cgraph_call_node_duplication_hooks (n, new_node);
  return new_node;
}

/* Create a new name for clone of DECL, add SUFFIX.  Returns an identifier.  */

static GTY(()) unsigned int clone_fn_id_num;

tree
clone_function_name (tree decl, const char *suffix)
{
  tree name = DECL_ASSEMBLER_NAME (decl);
  size_t len = IDENTIFIER_LENGTH (name);
  char *tmp_name, *prefix;

  prefix = XALLOCAVEC (char, len + strlen (suffix) + 2);
  memcpy (prefix, IDENTIFIER_POINTER (name), len);
  strcpy (prefix + len + 1, suffix);
#ifndef NO_DOT_IN_LABEL
  prefix[len] = '.';
#elif !defined NO_DOLLAR_IN_LABEL
  prefix[len] = '$';
#else
  prefix[len] = '_';
#endif
  ASM_FORMAT_PRIVATE_NAME (tmp_name, prefix, clone_fn_id_num++);
  return get_identifier (tmp_name);
}

/* Create callgraph node clone with new declaration.  The actual body will
   be copied later at compilation stage.

   TODO: after merging in ipa-sra use function call notes instead of args_to_skip
   bitmap interface.
   */
struct cgraph_node *
cgraph_create_virtual_clone (struct cgraph_node *old_node,
			     VEC(cgraph_edge_p,heap) *redirect_callers,
			     VEC(ipa_replace_map_p,gc) *tree_map,
			     bitmap args_to_skip,
			     const char * suffix)
{
  tree old_decl = old_node->symbol.decl;
  struct cgraph_node *new_node = NULL;
  tree new_decl;
  size_t i;
  struct ipa_replace_map *map;

  if (!flag_wpa)
    gcc_checking_assert  (tree_versionable_function_p (old_decl));

  gcc_assert (old_node->local.can_change_signature || !args_to_skip);

  /* Make a new FUNCTION_DECL tree node */
  if (!args_to_skip)
    new_decl = copy_node (old_decl);
  else
    new_decl = build_function_decl_skip_args (old_decl, args_to_skip, false);
  DECL_STRUCT_FUNCTION (new_decl) = NULL;

  /* Generate a new name for the new version. */
  DECL_NAME (new_decl) = clone_function_name (old_decl, suffix);
  SET_DECL_ASSEMBLER_NAME (new_decl, DECL_NAME (new_decl));
  SET_DECL_RTL (new_decl, NULL);

  new_node = cgraph_clone_node (old_node, new_decl, old_node->count,
				CGRAPH_FREQ_BASE, false,
				redirect_callers, false);
  /* Update the properties.
     Make clone visible only within this translation unit.  Make sure
     that is not weak also.
     ??? We cannot use COMDAT linkage because there is no
     ABI support for this.  */
  DECL_EXTERNAL (new_node->symbol.decl) = 0;
  if (DECL_ONE_ONLY (old_decl))
    DECL_SECTION_NAME (new_node->symbol.decl) = NULL;
  DECL_COMDAT_GROUP (new_node->symbol.decl) = 0;
  TREE_PUBLIC (new_node->symbol.decl) = 0;
  DECL_COMDAT (new_node->symbol.decl) = 0;
  DECL_WEAK (new_node->symbol.decl) = 0;
  DECL_STATIC_CONSTRUCTOR (new_node->symbol.decl) = 0;
  DECL_STATIC_DESTRUCTOR (new_node->symbol.decl) = 0;
  new_node->clone.tree_map = tree_map;
  new_node->clone.args_to_skip = args_to_skip;
  FOR_EACH_VEC_ELT (ipa_replace_map_p, tree_map, i, map)
    {
      tree var = map->new_tree;
      symtab_node ref_node;

      STRIP_NOPS (var);
      if (TREE_CODE (var) != ADDR_EXPR)
	continue;
      var = get_base_var (var);
      if (!var)
	continue;
      if (TREE_CODE (var) != FUNCTION_DECL
	  && TREE_CODE (var) != VAR_DECL)
	continue;

      /* Record references of the future statement initializing the constant
	 argument.  */
      ref_node = symtab_get_node (var);
      gcc_checking_assert (ref_node);
      ipa_record_reference ((symtab_node)new_node, (symtab_node)ref_node,
			    IPA_REF_ADDR, NULL);
    }
  if (!args_to_skip)
    new_node->clone.combined_args_to_skip = old_node->clone.combined_args_to_skip;
  else if (old_node->clone.combined_args_to_skip)
    {
      int newi = 0, oldi = 0;
      tree arg;
      bitmap new_args_to_skip = BITMAP_GGC_ALLOC ();
      struct cgraph_node *orig_node;
      for (orig_node = old_node; orig_node->clone_of; orig_node = orig_node->clone_of)
        ;
      for (arg = DECL_ARGUMENTS (orig_node->symbol.decl);
	   arg; arg = DECL_CHAIN (arg), oldi++)
	{
	  if (bitmap_bit_p (old_node->clone.combined_args_to_skip, oldi))
	    {
	      bitmap_set_bit (new_args_to_skip, oldi);
	      continue;
	    }
	  if (bitmap_bit_p (args_to_skip, newi))
	    bitmap_set_bit (new_args_to_skip, oldi);
	  newi++;
	}
      new_node->clone.combined_args_to_skip = new_args_to_skip;
    }
  else
    new_node->clone.combined_args_to_skip = args_to_skip;
  new_node->symbol.externally_visible = 0;
  new_node->local.local = 1;
  new_node->lowered = true;

  cgraph_call_node_duplication_hooks (old_node, new_node);


  return new_node;
}

/* NODE is no longer nested function; update cgraph accordingly.  */
void
cgraph_unnest_node (struct cgraph_node *node)
{
  struct cgraph_node **node2 = &node->origin->nested;
  gcc_assert (node->origin);

  while (*node2 != node)
    node2 = &(*node2)->next_nested;
  *node2 = node->next_nested;
  node->origin = NULL;
}

/* Return function availability.  See cgraph.h for description of individual
   return values.  */
enum availability
cgraph_function_body_availability (struct cgraph_node *node)
{
  enum availability avail;
  gcc_assert (cgraph_function_flags_ready);
  if (!node->analyzed)
    avail = AVAIL_NOT_AVAILABLE;
  else if (node->local.local)
    avail = AVAIL_LOCAL;
  else if (!node->symbol.externally_visible)
    avail = AVAIL_AVAILABLE;
  /* Inline functions are safe to be analyzed even if their symbol can
     be overwritten at runtime.  It is not meaningful to enforce any sane
     behaviour on replacing inline function by different body.  */
  else if (DECL_DECLARED_INLINE_P (node->symbol.decl))
    avail = AVAIL_AVAILABLE;

  /* If the function can be overwritten, return OVERWRITABLE.  Take
     care at least of two notable extensions - the COMDAT functions
     used to share template instantiations in C++ (this is symmetric
     to code cp_cannot_inline_tree_fn and probably shall be shared and
     the inlinability hooks completely eliminated).

     ??? Does the C++ one definition rule allow us to always return
     AVAIL_AVAILABLE here?  That would be good reason to preserve this
     bit.  */

  else if (decl_replaceable_p (node->symbol.decl)
	   && !DECL_EXTERNAL (node->symbol.decl))
    avail = AVAIL_OVERWRITABLE;
  else avail = AVAIL_AVAILABLE;

  return avail;
}

/* Worker for cgraph_node_can_be_local_p.  */
static bool
cgraph_node_cannot_be_local_p_1 (struct cgraph_node *node,
				 void *data ATTRIBUTE_UNUSED)
{
  return !(!node->symbol.force_output
	   && ((DECL_COMDAT (node->symbol.decl)
		&& !node->symbol.same_comdat_group)
	       || !node->symbol.externally_visible));
}

/* Return true if NODE can be made local for API change.
   Extern inline functions and C++ COMDAT functions can be made local
   at the expense of possible code size growth if function is used in multiple
   compilation units.  */
bool
cgraph_node_can_be_local_p (struct cgraph_node *node)
{
  return (!node->symbol.address_taken
	  && !cgraph_for_node_and_aliases (node,
					   cgraph_node_cannot_be_local_p_1,
					   NULL, true));
}

/* Make DECL local.  FIXME: We shouldn't need to mess with rtl this early,
   but other code such as notice_global_symbol generates rtl.  */
void
cgraph_make_decl_local (tree decl)
{
  rtx rtl, symbol;

  if (TREE_CODE (decl) == VAR_DECL)
    DECL_COMMON (decl) = 0;
  else gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);

  if (DECL_ONE_ONLY (decl) || DECL_COMDAT (decl))
    {
      /* It is possible that we are linking against library defining same COMDAT
	 function.  To avoid conflict we need to rename our local name of the
	 function just in the case WHOPR partitioning decide to make it hidden
	 to avoid cross partition references.  */
      if (flag_wpa)
	{
	  const char *old_name;

	  old_name  = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
	  if (TREE_CODE (decl) == FUNCTION_DECL)
	    {
	      struct cgraph_node *node = cgraph_get_node (decl);
	      change_decl_assembler_name (decl,
					  clone_function_name (decl, "local"));
	      if (node->symbol.lto_file_data)
		lto_record_renamed_decl (node->symbol.lto_file_data,
					 old_name,
					 IDENTIFIER_POINTER
					   (DECL_ASSEMBLER_NAME (decl)));
	    }
	  else if (TREE_CODE (decl) == VAR_DECL)
	    {
	      struct varpool_node *vnode = varpool_get_node (decl);
	      /* change_decl_assembler_name will warn here on vtables because
		 C++ frontend still sets TREE_SYMBOL_REFERENCED on them.  */
	      SET_DECL_ASSEMBLER_NAME (decl,
				       clone_function_name (decl, "local"));
	      if (vnode->symbol.lto_file_data)
		lto_record_renamed_decl (vnode->symbol.lto_file_data,
					 old_name,
					 IDENTIFIER_POINTER
					   (DECL_ASSEMBLER_NAME (decl)));
	    }
	}
      DECL_SECTION_NAME (decl) = 0;
      DECL_COMDAT (decl) = 0;
    }
  DECL_COMDAT_GROUP (decl) = 0;
  DECL_WEAK (decl) = 0;
  DECL_EXTERNAL (decl) = 0;
  TREE_PUBLIC (decl) = 0;
  if (!DECL_RTL_SET_P (decl))
    return;

  /* Update rtl flags.  */
  make_decl_rtl (decl);

  rtl = DECL_RTL (decl);
  if (!MEM_P (rtl))
    return;

  symbol = XEXP (rtl, 0);
  if (GET_CODE (symbol) != SYMBOL_REF)
    return;

  SYMBOL_REF_WEAK (symbol) = DECL_WEAK (decl);
}

/* Call calback on NODE, thunks and aliases asociated to NODE. 
   When INCLUDE_OVERWRITABLE is false, overwritable aliases and thunks are
   skipped. */

bool
cgraph_for_node_thunks_and_aliases (struct cgraph_node *node,
			            bool (*callback) (struct cgraph_node *, void *),
			            void *data,
				    bool include_overwritable)
{
  struct cgraph_edge *e;
  int i;
  struct ipa_ref *ref;

  if (callback (node, data))
    return true;
  for (e = node->callers; e; e = e->next_caller)
    if (e->caller->thunk.thunk_p
	&& (include_overwritable
	    || cgraph_function_body_availability (e->caller) > AVAIL_OVERWRITABLE))
      if (cgraph_for_node_thunks_and_aliases (e->caller, callback, data,
					      include_overwritable))
	return true;
  for (i = 0; ipa_ref_list_referring_iterate (&node->symbol.ref_list, i, ref); i++)
    if (ref->use == IPA_REF_ALIAS)
      {
	struct cgraph_node *alias = ipa_ref_referring_node (ref);
	if (include_overwritable
	    || cgraph_function_body_availability (alias) > AVAIL_OVERWRITABLE)
	  if (cgraph_for_node_thunks_and_aliases (alias, callback, data,
						  include_overwritable))
	    return true;
      }
  return false;
}

/* Call calback on NODE and aliases asociated to NODE. 
   When INCLUDE_OVERWRITABLE is false, overwritable aliases and thunks are
   skipped. */

bool
cgraph_for_node_and_aliases (struct cgraph_node *node,
			     bool (*callback) (struct cgraph_node *, void *),
			     void *data,
			     bool include_overwritable)
{
  int i;
  struct ipa_ref *ref;

  if (callback (node, data))
    return true;
  for (i = 0; ipa_ref_list_referring_iterate (&node->symbol.ref_list, i, ref); i++)
    if (ref->use == IPA_REF_ALIAS)
      {
	struct cgraph_node *alias = ipa_ref_referring_node (ref);
	if (include_overwritable
	    || cgraph_function_body_availability (alias) > AVAIL_OVERWRITABLE)
          if (cgraph_for_node_and_aliases (alias, callback, data,
					   include_overwritable))
	    return true;
      }
  return false;
}

/* Worker to bring NODE local.  */

static bool
cgraph_make_node_local_1 (struct cgraph_node *node, void *data ATTRIBUTE_UNUSED)
{
  gcc_checking_assert (cgraph_node_can_be_local_p (node));
  if (DECL_COMDAT (node->symbol.decl) || DECL_EXTERNAL (node->symbol.decl))
    {
      cgraph_make_decl_local (node->symbol.decl);

      node->symbol.externally_visible = false;
      node->local.local = true;
      node->symbol.resolution = LDPR_PREVAILING_DEF_IRONLY;
      gcc_assert (cgraph_function_body_availability (node) == AVAIL_LOCAL);
    }
  return false;
}

/* Bring NODE local.  */

void
cgraph_make_node_local (struct cgraph_node *node)
{
  cgraph_for_node_thunks_and_aliases (node, cgraph_make_node_local_1,
				      NULL, true);
}

/* Worker to set nothrow flag.  */

static bool
cgraph_set_nothrow_flag_1 (struct cgraph_node *node, void *data)
{
  struct cgraph_edge *e;

  TREE_NOTHROW (node->symbol.decl) = data != NULL;

  if (data != NULL)
    for (e = node->callers; e; e = e->next_caller)
      e->can_throw_external = false;
  return false;
}

/* Set TREE_NOTHROW on NODE's decl and on aliases of NODE
   if any to NOTHROW.  */

void
cgraph_set_nothrow_flag (struct cgraph_node *node, bool nothrow)
{
  cgraph_for_node_thunks_and_aliases (node, cgraph_set_nothrow_flag_1,
			              (void *)(size_t)nothrow, false);
}

/* Worker to set const flag.  */

static bool
cgraph_set_const_flag_1 (struct cgraph_node *node, void *data)
{
  /* Static constructors and destructors without a side effect can be
     optimized out.  */
  if (data && !((size_t)data & 2))
    {
      if (DECL_STATIC_CONSTRUCTOR (node->symbol.decl))
	DECL_STATIC_CONSTRUCTOR (node->symbol.decl) = 0;
      if (DECL_STATIC_DESTRUCTOR (node->symbol.decl))
	DECL_STATIC_DESTRUCTOR (node->symbol.decl) = 0;
    }
  TREE_READONLY (node->symbol.decl) = data != NULL;
  DECL_LOOPING_CONST_OR_PURE_P (node->symbol.decl) = ((size_t)data & 2) != 0;
  return false;
}

/* Set TREE_READONLY on NODE's decl and on aliases of NODE
   if any to READONLY.  */

void
cgraph_set_const_flag (struct cgraph_node *node, bool readonly, bool looping)
{
  cgraph_for_node_thunks_and_aliases (node, cgraph_set_const_flag_1,
			              (void *)(size_t)(readonly + (int)looping * 2),
				      false);
}

/* Worker to set pure flag.  */

static bool
cgraph_set_pure_flag_1 (struct cgraph_node *node, void *data)
{
  /* Static pureructors and destructors without a side effect can be
     optimized out.  */
  if (data && !((size_t)data & 2))
    {
      if (DECL_STATIC_CONSTRUCTOR (node->symbol.decl))
	DECL_STATIC_CONSTRUCTOR (node->symbol.decl) = 0;
      if (DECL_STATIC_DESTRUCTOR (node->symbol.decl))
	DECL_STATIC_DESTRUCTOR (node->symbol.decl) = 0;
    }
  DECL_PURE_P (node->symbol.decl) = data != NULL;
  DECL_LOOPING_CONST_OR_PURE_P (node->symbol.decl) = ((size_t)data & 2) != 0;
  return false;
}

/* Set DECL_PURE_P on NODE's decl and on aliases of NODE
   if any to PURE.  */

void
cgraph_set_pure_flag (struct cgraph_node *node, bool pure, bool looping)
{
  cgraph_for_node_thunks_and_aliases (node, cgraph_set_pure_flag_1,
			              (void *)(size_t)(pure + (int)looping * 2),
				      false);
}

/* Data used by cgraph_propagate_frequency.  */

struct cgraph_propagate_frequency_data
{
  bool maybe_unlikely_executed;
  bool maybe_executed_once;
  bool only_called_at_startup;
  bool only_called_at_exit;
};

/* Worker for cgraph_propagate_frequency_1.  */

static bool
cgraph_propagate_frequency_1 (struct cgraph_node *node, void *data)
{
  struct cgraph_propagate_frequency_data *d;
  struct cgraph_edge *edge;

  d = (struct cgraph_propagate_frequency_data *)data;
  for (edge = node->callers;
       edge && (d->maybe_unlikely_executed || d->maybe_executed_once
	        || d->only_called_at_startup || d->only_called_at_exit);
       edge = edge->next_caller)
    {
      if (edge->caller != node)
	{
          d->only_called_at_startup &= edge->caller->only_called_at_startup;
	  /* It makes sense to put main() together with the static constructors.
	     It will be executed for sure, but rest of functions called from
	     main are definitely not at startup only.  */
	  if (MAIN_NAME_P (DECL_NAME (edge->caller->symbol.decl)))
	    d->only_called_at_startup = 0;
          d->only_called_at_exit &= edge->caller->only_called_at_exit;
	}
      if (!edge->frequency)
	continue;
      switch (edge->caller->frequency)
        {
	case NODE_FREQUENCY_UNLIKELY_EXECUTED:
	  break;
	case NODE_FREQUENCY_EXECUTED_ONCE:
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "  Called by %s that is executed once\n",
		     cgraph_node_name (edge->caller));
	  d->maybe_unlikely_executed = false;
	  if (inline_edge_summary (edge)->loop_depth)
	    {
	      d->maybe_executed_once = false;
	      if (dump_file && (dump_flags & TDF_DETAILS))
	        fprintf (dump_file, "  Called in loop\n");
	    }
	  break;
	case NODE_FREQUENCY_HOT:
	case NODE_FREQUENCY_NORMAL:
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "  Called by %s that is normal or hot\n",
		     cgraph_node_name (edge->caller));
	  d->maybe_unlikely_executed = false;
	  d->maybe_executed_once = false;
	  break;
	}
    }
  return edge != NULL;
}

/* See if the frequency of NODE can be updated based on frequencies of its
   callers.  */
bool
cgraph_propagate_frequency (struct cgraph_node *node)
{
  struct cgraph_propagate_frequency_data d = {true, true, true, true};
  bool changed = false;

  if (!node->local.local)
    return false;
  gcc_assert (node->analyzed);
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "Processing frequency %s\n", cgraph_node_name (node));

  cgraph_for_node_and_aliases (node, cgraph_propagate_frequency_1, &d, true);

  if ((d.only_called_at_startup && !d.only_called_at_exit)
      && !node->only_called_at_startup)
    {
       node->only_called_at_startup = true;
       if (dump_file)
         fprintf (dump_file, "Node %s promoted to only called at startup.\n",
		  cgraph_node_name (node));
       changed = true;
    }
  if ((d.only_called_at_exit && !d.only_called_at_startup)
      && !node->only_called_at_exit)
    {
       node->only_called_at_exit = true;
       if (dump_file)
         fprintf (dump_file, "Node %s promoted to only called at exit.\n",
		  cgraph_node_name (node));
       changed = true;
    }
  /* These come either from profile or user hints; never update them.  */
  if (node->frequency == NODE_FREQUENCY_HOT
      || node->frequency == NODE_FREQUENCY_UNLIKELY_EXECUTED)
    return changed;
  if (d.maybe_unlikely_executed)
    {
      node->frequency = NODE_FREQUENCY_UNLIKELY_EXECUTED;
      if (dump_file)
	fprintf (dump_file, "Node %s promoted to unlikely executed.\n",
		 cgraph_node_name (node));
      changed = true;
    }
  else if (d.maybe_executed_once && node->frequency != NODE_FREQUENCY_EXECUTED_ONCE)
    {
      node->frequency = NODE_FREQUENCY_EXECUTED_ONCE;
      if (dump_file)
	fprintf (dump_file, "Node %s promoted to executed once.\n",
		 cgraph_node_name (node));
      changed = true;
    }
  return changed;
}

/* Return true when NODE can not return or throw and thus
   it is safe to ignore its side effects for IPA analysis.  */

bool
cgraph_node_cannot_return (struct cgraph_node *node)
{
  int flags = flags_from_decl_or_type (node->symbol.decl);
  if (!flag_exceptions)
    return (flags & ECF_NORETURN) != 0;
  else
    return ((flags & (ECF_NORETURN | ECF_NOTHROW))
	     == (ECF_NORETURN | ECF_NOTHROW));
}

/* Return true when call of E can not lead to return from caller
   and thus it is safe to ignore its side effects for IPA analysis
   when computing side effects of the caller.
   FIXME: We could actually mark all edges that have no reaching
   patch to EXIT_BLOCK_PTR or throw to get better results.  */
bool
cgraph_edge_cannot_lead_to_return (struct cgraph_edge *e)
{
  if (cgraph_node_cannot_return (e->caller))
    return true;
  if (e->indirect_unknown_callee)
    {
      int flags = e->indirect_info->ecf_flags;
      if (!flag_exceptions)
	return (flags & ECF_NORETURN) != 0;
      else
	return ((flags & (ECF_NORETURN | ECF_NOTHROW))
		 == (ECF_NORETURN | ECF_NOTHROW));
    }
  else
    return cgraph_node_cannot_return (e->callee);
}

/* Return true when function NODE can be removed from callgraph
   if all direct calls are eliminated.  */

bool
cgraph_can_remove_if_no_direct_calls_and_refs_p (struct cgraph_node *node)
{
  gcc_assert (!node->global.inlined_to);
  /* Extern inlines can always go, we will use the external definition.  */
  if (DECL_EXTERNAL (node->symbol.decl))
    return true;
  /* When function is needed, we can not remove it.  */
  if (node->symbol.force_output || node->symbol.used_from_other_partition)
    return false;
  if (DECL_STATIC_CONSTRUCTOR (node->symbol.decl)
      || DECL_STATIC_DESTRUCTOR (node->symbol.decl))
    return false;
  /* Only COMDAT functions can be removed if externally visible.  */
  if (node->symbol.externally_visible
      && (!DECL_COMDAT (node->symbol.decl)
	  || cgraph_used_from_object_file_p (node)))
    return false;
  return true;
}

/* Worker for cgraph_can_remove_if_no_direct_calls_p.  */

static bool
nonremovable_p (struct cgraph_node *node, void *data ATTRIBUTE_UNUSED)
{
  return !cgraph_can_remove_if_no_direct_calls_and_refs_p (node);
}

/* Return true when function NODE and its aliases can be removed from callgraph
   if all direct calls are eliminated.  */

bool
cgraph_can_remove_if_no_direct_calls_p (struct cgraph_node *node)
{
  /* Extern inlines can always go, we will use the external definition.  */
  if (DECL_EXTERNAL (node->symbol.decl))
    return true;
  if (node->symbol.address_taken)
    return false;
  return !cgraph_for_node_and_aliases (node, nonremovable_p, NULL, true);
}

/* Worker for cgraph_can_remove_if_no_direct_calls_p.  */

static bool
used_from_object_file_p (struct cgraph_node *node, void *data ATTRIBUTE_UNUSED)
{
  return cgraph_used_from_object_file_p (node);
}

/* Return true when function NODE can be expected to be removed
   from program when direct calls in this compilation unit are removed.

   As a special case COMDAT functions are
   cgraph_can_remove_if_no_direct_calls_p while the are not
   cgraph_only_called_directly_p (it is possible they are called from other
   unit)

   This function behaves as cgraph_only_called_directly_p because eliminating
   all uses of COMDAT function does not make it necessarily disappear from
   the program unless we are compiling whole program or we do LTO.  In this
   case we know we win since dynamic linking will not really discard the
   linkonce section.  */

bool
cgraph_will_be_removed_from_program_if_no_direct_calls (struct cgraph_node *node)
{
  gcc_assert (!node->global.inlined_to);
  if (cgraph_for_node_and_aliases (node, used_from_object_file_p, NULL, true))
    return false;
  if (!in_lto_p && !flag_whole_program)
    return cgraph_only_called_directly_p (node);
  else
    {
       if (DECL_EXTERNAL (node->symbol.decl))
         return true;
      return cgraph_can_remove_if_no_direct_calls_p (node);
    }
}

/* Return true when RESOLUTION indicate that linker will use
   the symbol from non-LTO object files.  */

bool
resolution_used_from_other_file_p (enum ld_plugin_symbol_resolution resolution)
{
  return (resolution == LDPR_PREVAILING_DEF
          || resolution == LDPR_PREEMPTED_REG
          || resolution == LDPR_RESOLVED_EXEC
          || resolution == LDPR_RESOLVED_DYN);
}


/* Return true when NODE is known to be used from other (non-LTO) object file.
   Known only when doing LTO via linker plugin.  */

bool
cgraph_used_from_object_file_p (struct cgraph_node *node)
{
  gcc_assert (!node->global.inlined_to);
  if (!TREE_PUBLIC (node->symbol.decl) || DECL_EXTERNAL (node->symbol.decl))
    return false;
  if (resolution_used_from_other_file_p (node->symbol.resolution))
    return true;
  return false;
}

/* Worker for cgraph_only_called_directly_p.  */

static bool
cgraph_not_only_called_directly_p_1 (struct cgraph_node *node, void *data ATTRIBUTE_UNUSED)
{
  return !cgraph_only_called_directly_or_aliased_p (node);
}

/* Return true when function NODE and all its aliases are only called
   directly.
   i.e. it is not externally visible, address was not taken and
   it is not used in any other non-standard way.  */

bool
cgraph_only_called_directly_p (struct cgraph_node *node)
{
  gcc_assert (cgraph_function_or_thunk_node (node, NULL) == node);
  return !cgraph_for_node_and_aliases (node, cgraph_not_only_called_directly_p_1,
				       NULL, true);
}


/* Collect all callers of NODE.  Worker for collect_callers_of_node.  */

static bool
collect_callers_of_node_1 (struct cgraph_node *node, void *data)
{
  VEC (cgraph_edge_p, heap) ** redirect_callers = (VEC (cgraph_edge_p, heap) **)data;
  struct cgraph_edge *cs;
  enum availability avail;
  cgraph_function_or_thunk_node (node, &avail);

  if (avail > AVAIL_OVERWRITABLE)
    for (cs = node->callers; cs != NULL; cs = cs->next_caller)
      if (!cs->indirect_inlining_edge)
        VEC_safe_push (cgraph_edge_p, heap, *redirect_callers, cs);
  return false;
}

/* Collect all callers of NODE and its aliases that are known to lead to NODE
   (i.e. are not overwritable).  */

VEC (cgraph_edge_p, heap) *
collect_callers_of_node (struct cgraph_node *node)
{
  VEC (cgraph_edge_p, heap) * redirect_callers = NULL;
  cgraph_for_node_and_aliases (node, collect_callers_of_node_1,
			       &redirect_callers, false);
  return redirect_callers;
}

/* Return TRUE if NODE2 is equivalent to NODE or its clone.  */
static bool
clone_of_p (struct cgraph_node *node, struct cgraph_node *node2)
{
  node = cgraph_function_or_thunk_node (node, NULL);
  node2 = cgraph_function_or_thunk_node (node2, NULL);
  while (node != node2 && node2)
    node2 = node2->clone_of;
  return node2 != NULL;
}

/* Verify edge E count and frequency.  */

static bool
verify_edge_count_and_frequency (struct cgraph_edge *e)
{
  bool error_found = false;
  if (e->count < 0)
    {
      error ("caller edge count is negative");
      error_found = true;
    }
  if (e->frequency < 0)
    {
      error ("caller edge frequency is negative");
      error_found = true;
    }
  if (e->frequency > CGRAPH_FREQ_MAX)
    {
      error ("caller edge frequency is too large");
      error_found = true;
    }
  if (gimple_has_body_p (e->caller->symbol.decl)
      && !e->caller->global.inlined_to
      /* FIXME: Inline-analysis sets frequency to 0 when edge is optimized out.
	 Remove this once edges are actualy removed from the function at that time.  */
      && (e->frequency
	  || (inline_edge_summary_vec
	      && ((VEC_length(inline_edge_summary_t, inline_edge_summary_vec)
		  <= (unsigned) e->uid)
	          || !inline_edge_summary (e)->predicate)))
      && (e->frequency
	  != compute_call_stmt_bb_frequency (e->caller->symbol.decl,
					     gimple_bb (e->call_stmt))))
    {
      error ("caller edge frequency %i does not match BB frequency %i",
	     e->frequency,
	     compute_call_stmt_bb_frequency (e->caller->symbol.decl,
					     gimple_bb (e->call_stmt)));
      error_found = true;
    }
  return error_found;
}

/* Switch to THIS_CFUN if needed and print STMT to stderr.  */
static void
cgraph_debug_gimple_stmt (struct function *this_cfun, gimple stmt)
{
  /* debug_gimple_stmt needs correct cfun */
  if (cfun != this_cfun)
    set_cfun (this_cfun);
  debug_gimple_stmt (stmt);
}

/* Verify that call graph edge E corresponds to DECL from the associated
   statement.  Return true if the verification should fail.  */

static bool
verify_edge_corresponds_to_fndecl (struct cgraph_edge *e, tree decl)
{
  struct cgraph_node *node;

  if (!decl || e->callee->global.inlined_to)
    return false;
  node = cgraph_get_node (decl);

  /* We do not know if a node from a different partition is an alias or what it
     aliases and therefore cannot do the former_clone_of check reliably.  */
  if (!node || node->symbol.in_other_partition)
    return false;
  node = cgraph_function_or_thunk_node (node, NULL);

  if ((e->callee->former_clone_of != node->symbol.decl
       && (!node->same_body_alias
	   || e->callee->former_clone_of != node->thunk.alias))
      /* IPA-CP sometimes redirect edge to clone and then back to the former
	 function.  This ping-pong has to go, eventually.  */
      && (node != cgraph_function_or_thunk_node (e->callee, NULL))
      && !clone_of_p (node, e->callee)
      /* If decl is a same body alias of some other decl, allow e->callee to be
	 a clone of a clone of that other decl too.  */
      && (!node->same_body_alias
	  || !clone_of_p (cgraph_get_node (node->thunk.alias), e->callee)))
    return true;
  else
    return false;
}

/* Verify cgraph nodes of given cgraph node.  */
DEBUG_FUNCTION void
verify_cgraph_node (struct cgraph_node *node)
{
  struct cgraph_edge *e;
  struct function *this_cfun = DECL_STRUCT_FUNCTION (node->symbol.decl);
  basic_block this_block;
  gimple_stmt_iterator gsi;
  bool error_found = false;

  if (seen_error ())
    return;

  timevar_push (TV_CGRAPH_VERIFY);
  error_found |= verify_symtab_base ((symtab_node) node);
  for (e = node->callees; e; e = e->next_callee)
    if (e->aux)
      {
	error ("aux field set for edge %s->%s",
	       identifier_to_locale (cgraph_node_name (e->caller)),
	       identifier_to_locale (cgraph_node_name (e->callee)));
	error_found = true;
      }
  if (node->count < 0)
    {
      error ("execution count is negative");
      error_found = true;
    }
  if (node->global.inlined_to && node->symbol.externally_visible)
    {
      error ("externally visible inline clone");
      error_found = true;
    }
  if (node->global.inlined_to && node->symbol.address_taken)
    {
      error ("inline clone with address taken");
      error_found = true;
    }
  if (node->global.inlined_to && node->symbol.force_output)
    {
      error ("inline clone is forced to output");
      error_found = true;
    }
  for (e = node->indirect_calls; e; e = e->next_callee)
    {
      if (e->aux)
	{
	  error ("aux field set for indirect edge from %s",
		 identifier_to_locale (cgraph_node_name (e->caller)));
	  error_found = true;
	}
      if (!e->indirect_unknown_callee
	  || !e->indirect_info)
	{
	  error ("An indirect edge from %s is not marked as indirect or has "
		 "associated indirect_info, the corresponding statement is: ",
		 identifier_to_locale (cgraph_node_name (e->caller)));
	  cgraph_debug_gimple_stmt (this_cfun, e->call_stmt);
	  error_found = true;
	}
    }
  for (e = node->callers; e; e = e->next_caller)
    {
      if (verify_edge_count_and_frequency (e))
	error_found = true;
      if (!e->inline_failed)
	{
	  if (node->global.inlined_to
	      != (e->caller->global.inlined_to
		  ? e->caller->global.inlined_to : e->caller))
	    {
	      error ("inlined_to pointer is wrong");
	      error_found = true;
	    }
	  if (node->callers->next_caller)
	    {
	      error ("multiple inline callers");
	      error_found = true;
	    }
	}
      else
	if (node->global.inlined_to)
	  {
	    error ("inlined_to pointer set for noninline callers");
	    error_found = true;
	  }
    }
  for (e = node->indirect_calls; e; e = e->next_callee)
    if (verify_edge_count_and_frequency (e))
      error_found = true;
  if (!node->callers && node->global.inlined_to)
    {
      error ("inlined_to pointer is set but no predecessors found");
      error_found = true;
    }
  if (node->global.inlined_to == node)
    {
      error ("inlined_to pointer refers to itself");
      error_found = true;
    }

  if (node->clone_of)
    {
      struct cgraph_node *n;
      for (n = node->clone_of->clones; n; n = n->next_sibling_clone)
        if (n == node)
	  break;
      if (!n)
	{
	  error ("node has wrong clone_of");
	  error_found = true;
	}
    }
  if (node->clones)
    {
      struct cgraph_node *n;
      for (n = node->clones; n; n = n->next_sibling_clone)
        if (n->clone_of != node)
	  break;
      if (n)
	{
	  error ("node has wrong clone list");
	  error_found = true;
	}
    }
  if ((node->prev_sibling_clone || node->next_sibling_clone) && !node->clone_of)
    {
       error ("node is in clone list but it is not clone");
       error_found = true;
    }
  if (!node->prev_sibling_clone && node->clone_of && node->clone_of->clones != node)
    {
      error ("node has wrong prev_clone pointer");
      error_found = true;
    }
  if (node->prev_sibling_clone && node->prev_sibling_clone->next_sibling_clone != node)
    {
      error ("double linked list of clones corrupted");
      error_found = true;
    }

  if (node->analyzed && node->alias)
    {
      bool ref_found = false;
      int i;
      struct ipa_ref *ref;

      if (node->callees)
	{
	  error ("Alias has call edges");
          error_found = true;
	}
      for (i = 0; ipa_ref_list_reference_iterate (&node->symbol.ref_list,
						  i, ref); i++)
	if (ref->use != IPA_REF_ALIAS)
	  {
	    error ("Alias has non-alias reference");
	    error_found = true;
	  }
	else if (ref_found)
	  {
	    error ("Alias has more than one alias reference");
	    error_found = true;
	  }
	else
	  ref_found = true;
	if (!ref_found)
	  {
	    error ("Analyzed alias has no reference");
	    error_found = true;
	  }
    }
  if (node->analyzed && node->thunk.thunk_p)
    {
      if (!node->callees)
	{
	  error ("No edge out of thunk node");
          error_found = true;
	}
      else if (node->callees->next_callee)
	{
	  error ("More than one edge out of thunk node");
          error_found = true;
	}
      if (gimple_has_body_p (node->symbol.decl))
        {
	  error ("Thunk is not supposed to have body");
          error_found = true;
        }
    }
  else if (node->analyzed && gimple_has_body_p (node->symbol.decl)
           && !TREE_ASM_WRITTEN (node->symbol.decl)
           && (!DECL_EXTERNAL (node->symbol.decl) || node->global.inlined_to)
           && !flag_wpa)
    {
      if (this_cfun->cfg)
	{
	  /* The nodes we're interested in are never shared, so walk
	     the tree ignoring duplicates.  */
	  struct pointer_set_t *visited_nodes = pointer_set_create ();
	  /* Reach the trees by walking over the CFG, and note the
	     enclosing basic-blocks in the call edges.  */
	  FOR_EACH_BB_FN (this_block, this_cfun)
	    for (gsi = gsi_start_bb (this_block);
                 !gsi_end_p (gsi);
                 gsi_next (&gsi))
	      {
		gimple stmt = gsi_stmt (gsi);
		if (is_gimple_call (stmt))
		  {
		    struct cgraph_edge *e = cgraph_edge (node, stmt);
		    tree decl = gimple_call_fndecl (stmt);
		    if (e)
		      {
			if (e->aux)
			  {
			    error ("shared call_stmt:");
			    cgraph_debug_gimple_stmt (this_cfun, stmt);
			    error_found = true;
			  }
			if (!e->indirect_unknown_callee)
			  {
			    if (verify_edge_corresponds_to_fndecl (e, decl))
			      {
				error ("edge points to wrong declaration:");
				debug_tree (e->callee->symbol.decl);
				fprintf (stderr," Instead of:");
				debug_tree (decl);
				error_found = true;
			      }
			  }
			else if (decl)
			  {
			    error ("an indirect edge with unknown callee "
				   "corresponding to a call_stmt with "
				   "a known declaration:");
			    error_found = true;
			    cgraph_debug_gimple_stmt (this_cfun, e->call_stmt);
			  }
			e->aux = (void *)1;
		      }
		    else if (decl)
		      {
			error ("missing callgraph edge for call stmt:");
			cgraph_debug_gimple_stmt (this_cfun, stmt);
			error_found = true;
		      }
		  }
	      }
	  pointer_set_destroy (visited_nodes);
	}
      else
	/* No CFG available?!  */
	gcc_unreachable ();

      for (e = node->callees; e; e = e->next_callee)
	{
	  if (!e->aux)
	    {
	      error ("edge %s->%s has no corresponding call_stmt",
		     identifier_to_locale (cgraph_node_name (e->caller)),
		     identifier_to_locale (cgraph_node_name (e->callee)));
	      cgraph_debug_gimple_stmt (this_cfun, e->call_stmt);
	      error_found = true;
	    }
	  e->aux = 0;
	}
      for (e = node->indirect_calls; e; e = e->next_callee)
	{
	  if (!e->aux)
	    {
	      error ("an indirect edge from %s has no corresponding call_stmt",
		     identifier_to_locale (cgraph_node_name (e->caller)));
	      cgraph_debug_gimple_stmt (this_cfun, e->call_stmt);
	      error_found = true;
	    }
	  e->aux = 0;
	}
    }
  if (error_found)
    {
      dump_cgraph_node (stderr, node);
      internal_error ("verify_cgraph_node failed");
    }
  timevar_pop (TV_CGRAPH_VERIFY);
}

/* Verify whole cgraph structure.  */
DEBUG_FUNCTION void
verify_cgraph (void)
{
  struct cgraph_node *node;

  if (seen_error ())
    return;

  FOR_EACH_FUNCTION (node)
    verify_cgraph_node (node);
}
#include "gt-cgraph.h"
