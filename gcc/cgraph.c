/* Callgraph handling code.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.
   Contributed by Jan Hubicka

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

/*  This file contains basic routines manipulating call graph and variable pool
  
The callgraph:

    The call-graph is data structure designed for intra-procedural optimization
    but it is also used in non-unit-at-a-time compilation to allow easier code
    sharing.

    The call-graph consist of nodes and edges represented via linked lists.
    Each function (external or not) corresponds to the unique node (in
    contrast to tree DECL nodes where we can have multiple nodes for each
    function).

    The mapping from declarations to call-graph nodes is done using hash table
    based on DECL_ASSEMBLER_NAME, so it is essential for assembler name to
    not change once the declaration is inserted into the call-graph.
    The call-graph nodes are created lazily using cgraph_node function when
    called for unknown declaration.
    
    When built, there is one edge for each direct call.  It is possible that
    the reference will be later optimized out.  The call-graph is built
    conservatively in order to make conservative data flow analysis possible.

    The callgraph at the moment does not represent indirect calls or calls
    from other compilation unit.  Flag NEEDED is set for each node that may
    be accessed in such an invisible way and it shall be considered an
    entry point to the callgraph.

    Intraprocedural information:

      Callgraph is place to store data needed for intraprocedural optimization.
      All data structures are divided into three components: local_info that
      is produced while analyzing the function, global_info that is result
      of global walking of the callgraph on the end of compilation and
      rtl_info used by RTL backend to propagate data from already compiled
      functions to their callers.

    Inlining plans:

      The function inlining information is decided in advance and maintained
      in the callgraph as so called inline plan.
      For each inlined call, the callee's node is cloned to represent the
      new function copy produced by inliner.
      Each inlined call gets a unique corresponding clone node of the callee
      and the data structure is updated while inlining is performed, so
      the clones are eliminated and their callee edges redirected to the
      caller. 

      Each edge has "inline_failed" field.  When the field is set to NULL,
      the call will be inlined.  When it is non-NULL it contains a reason
      why inlining wasn't performed.


The varpool data structure:

    Varpool is used to maintain variables in similar manner as call-graph
    is used for functions.  Most of the API is symmetric replacing cgraph
    function prefix by cgraph_varpool  */


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
#include "varray.h"
#include "output.h"
#include "intl.h"

/* Hash table used to convert declarations into nodes.  */
static GTY((param_is (struct cgraph_node))) htab_t cgraph_hash;

/* The linked list of cgraph nodes.  */
struct cgraph_node *cgraph_nodes;

/* Queue of cgraph nodes scheduled to be lowered.  */
struct cgraph_node *cgraph_nodes_queue;

/* Number of nodes in existence.  */
int cgraph_n_nodes;

/* Maximal uid used in cgraph nodes.  */
int cgraph_max_uid;

/* Set when whole unit has been analyzed so we can access global info.  */
bool cgraph_global_info_ready = false;

/* Set when the cgraph is fully build and the basic flags are computed.  */
bool cgraph_function_flags_ready = false;

/* Hash table used to convert declarations into nodes.  */
static GTY((param_is (struct cgraph_varpool_node))) htab_t cgraph_varpool_hash;

/* Queue of cgraph nodes scheduled to be lowered and output.  */
struct cgraph_varpool_node *cgraph_varpool_nodes_queue, *cgraph_varpool_first_unanalyzed_node;


/* Number of nodes in existence.  */
int cgraph_varpool_n_nodes;

/* The linked list of cgraph varpool nodes.  */
static GTY(()) struct cgraph_varpool_node *cgraph_varpool_nodes;

/* End of the varpool queue.  Needs to be QTYed to work with PCH.  */
static GTY(()) struct cgraph_varpool_node *cgraph_varpool_last_needed_node;

static hashval_t hash_node (const void *);
static int eq_node (const void *, const void *);

/* Returns a hash code for P.  */

static hashval_t
hash_node (const void *p)
{
  const struct cgraph_node *n = p;
  return (hashval_t) DECL_UID (n->decl);
}

/* Returns nonzero if P1 and P2 are equal.  */

static int
eq_node (const void *p1, const void *p2)
{
  const struct cgraph_node *n1 = p1, *n2 = p2;
  return DECL_UID (n1->decl) == DECL_UID (n2->decl);
}

/* Allocate new callgraph node and insert it into basic data structures.  */
static struct cgraph_node *
cgraph_create_node (void)
{
  struct cgraph_node *node;

  node = ggc_alloc_cleared (sizeof (*node));
  node->next = cgraph_nodes;
  node->uid = cgraph_max_uid++;
  if (cgraph_nodes)
    cgraph_nodes->previous = node;
  node->previous = NULL;
  node->static_vars_info = NULL;
  node->global.estimated_growth = INT_MIN;
  cgraph_nodes = node;
  cgraph_n_nodes++;
  return node;
}

/* Return cgraph node assigned to DECL.  Create new one when needed.  */
struct cgraph_node *
cgraph_node (tree decl)
{
  struct cgraph_node key, *node, **slot;

  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);

  if (!cgraph_hash)
    cgraph_hash = htab_create_ggc (10, hash_node, eq_node, NULL);

  key.decl = decl;

  slot = (struct cgraph_node **) htab_find_slot (cgraph_hash, &key, INSERT);

  if (*slot)
    {
      node = *slot;
      if (!node->master_clone)
	node->master_clone = node;
      return node;
    }

  node = cgraph_create_node ();
  node->decl = decl;
  *slot = node;
  if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
    {
      node->origin = cgraph_node (DECL_CONTEXT (decl));
      node->next_nested = node->origin->nested;
      node->origin->nested = node;
      node->master_clone = node;
    }
  return node;
}

/* Compare ASMNAME with the DECL_ASSEMBLER_NAME of DECL.  */

static bool
decl_assembler_name_equal (tree decl, tree asmname)
{
  tree decl_asmname = DECL_ASSEMBLER_NAME (decl);

  if (decl_asmname == asmname)
    return true;

  /* If the target assembler name was set by the user, things are trickier.
     We have a leading '*' to begin with.  After that, it's arguable what
     is the correct thing to do with -fleading-underscore.  Arguably, we've
     historically been doing the wrong thing in assemble_alias by always
     printing the leading underscore.  Since we're not changing that, make
     sure user_label_prefix follows the '*' before matching.  */
  if (IDENTIFIER_POINTER (decl_asmname)[0] == '*')
    {
      const char *decl_str = IDENTIFIER_POINTER (decl_asmname) + 1;
      size_t ulp_len = strlen (user_label_prefix);

      if (ulp_len == 0)
	;
      else if (strncmp (decl_str, user_label_prefix, ulp_len) == 0)
	decl_str += ulp_len;
      else
	return false;

      return strcmp (decl_str, IDENTIFIER_POINTER (asmname)) == 0;
    }

  return false;
}


/* Return the cgraph node that has ASMNAME for its DECL_ASSEMBLER_NAME.
   Return NULL if there's no such node.  */

struct cgraph_node *
cgraph_node_for_asm (tree asmname)
{
  struct cgraph_node *node;

  for (node = cgraph_nodes; node ; node = node->next)
    if (decl_assembler_name_equal (node->decl, asmname))
      return node;

  return NULL;
}

/* Return callgraph edge representing CALL_EXPR.  */
struct cgraph_edge *
cgraph_edge (struct cgraph_node *node, tree call_expr)
{
  struct cgraph_edge *e;

  /* This loop may turn out to be performance problem.  In such case adding
     hashtables into call nodes with very many edges is probably best
     solution.  It is not good idea to add pointer into CALL_EXPR itself
     because we want to make possible having multiple cgraph nodes representing
     different clones of the same body before the body is actually cloned.  */
  for (e = node->callees; e; e= e->next_callee)
    if (e->call_expr == call_expr)
      break;
  return e;
}

/* Create edge from CALLER to CALLEE in the cgraph.  */

struct cgraph_edge *
cgraph_create_edge (struct cgraph_node *caller, struct cgraph_node *callee,
		    tree call_expr, gcov_type count, int nest)
{
  struct cgraph_edge *edge = ggc_alloc (sizeof (struct cgraph_edge));
#ifdef ENABLE_CHECKING
  struct cgraph_edge *e;

  for (e = caller->callees; e; e = e->next_callee)
    gcc_assert (e->call_expr != call_expr);
#endif

  gcc_assert (TREE_CODE (call_expr) == CALL_EXPR);

  if (!DECL_SAVED_TREE (callee->decl))
    edge->inline_failed = N_("function body not available");
  else if (callee->local.redefined_extern_inline)
    edge->inline_failed = N_("redefined extern inline functions are not "
			     "considered for inlining");
  else if (callee->local.inlinable)
    edge->inline_failed = N_("function not considered for inlining");
  else
    edge->inline_failed = N_("function not inlinable");

  edge->aux = NULL;

  edge->caller = caller;
  edge->callee = callee;
  edge->call_expr = call_expr;
  edge->next_caller = callee->callers;
  edge->next_callee = caller->callees;
  caller->callees = edge;
  callee->callers = edge;
  edge->count = count;
  edge->loop_nest = nest;
  return edge;
}

/* Remove the edge E the cgraph.  */

void
cgraph_remove_edge (struct cgraph_edge *e)
{
  struct cgraph_edge **edge, **edge2;

  for (edge = &e->callee->callers; *edge && *edge != e;
       edge = &((*edge)->next_caller))
    continue;
  gcc_assert (*edge);
  *edge = (*edge)->next_caller;
  for (edge2 = &e->caller->callees; *edge2 && *edge2 != e;
       edge2 = &(*edge2)->next_callee)
    continue;
  gcc_assert (*edge2);
  *edge2 = (*edge2)->next_callee;
}

/* Redirect callee of E to N.  The function does not update underlying
   call expression.  */

void
cgraph_redirect_edge_callee (struct cgraph_edge *e, struct cgraph_node *n)
{
  struct cgraph_edge **edge;

  for (edge = &e->callee->callers; *edge && *edge != e;
       edge = &((*edge)->next_caller))
    continue;
  gcc_assert (*edge);
  *edge = (*edge)->next_caller;
  e->callee = n;
  e->next_caller = n->callers;
  n->callers = e;
}

/* Remove the node from cgraph.  */

void
cgraph_remove_node (struct cgraph_node *node)
{
  void **slot;
  bool check_dead = 1;
  struct cgraph_node *node2, *node2_next;

  while (node->callers)
    cgraph_remove_edge (node->callers);
  while (node->callees)
    cgraph_remove_edge (node->callees);
  /* Nested functions can be removed now, provided they are inlined into
     this function, or are not inlined and do not have their address taken.  */
  for (node2 = node->nested; node2; )
    {
      node2_next = node2->next_nested;
      if (node2 
	  && ((!node2->global.inlined_to && !TREE_ADDRESSABLE (node2->decl))
	      || node2->global.inlined_to == node))
	cgraph_remove_node (node2);
      node2 = node2_next;
    }
  if (node->origin)
    {
      struct cgraph_node **node2 = &node->origin->nested;

      while (*node2 != node)
	node2 = &(*node2)->next_nested;
      *node2 = node->next_nested;
    }
  if (node->previous)
    node->previous->next = node->next;
  else
    cgraph_nodes = node->next;

  if (node->next)
    node->next->previous = node->previous;
  slot = htab_find_slot (cgraph_hash, node, NO_INSERT);
  if (*slot == node)
    {
      if (node->next_clone)
	{
	  struct cgraph_node *new_node = node->next_clone;
	  struct cgraph_node *n;
	  *slot = new_node;
	  
	  /* Make the next clone be the master clone */
	  for (n = new_node; n; n = n->next_clone) 
	    n->master_clone = new_node;
	
	  new_node->master_clone = new_node;
	}
      else
	{
          htab_clear_slot (cgraph_hash, slot);
	  if (!dump_enabled_p (TDI_tree_all))
	    {
              DECL_SAVED_TREE (node->decl) = NULL;
	      DECL_STRUCT_FUNCTION (node->decl) = NULL;
	    }
	  check_dead = false;
	}
    }
  else
    {
      struct cgraph_node *n;

      for (n = *slot; n->next_clone != node; n = n->next_clone)
	continue;
      n->next_clone = node->next_clone;
    }

  /* Work out whether we still need a function body (either there is inline
     clone or there is out of line function whose body is not written).  */
  if (check_dead && flag_unit_at_a_time)
    {
      struct cgraph_node *n;

      for (n = *slot; n; n = n->next_clone)
	if (n->global.inlined_to
	    || (!n->global.inlined_to
		&& !TREE_ASM_WRITTEN (n->decl) && !DECL_EXTERNAL (n->decl)))
	  break;
      if (!n && !dump_enabled_p (TDI_tree_all))
	{
	  DECL_SAVED_TREE (node->decl) = NULL;
	  DECL_STRUCT_FUNCTION (node->decl) = NULL;
          DECL_INITIAL (node->decl) = error_mark_node;
	}
    }
  cgraph_n_nodes--;
  /* Do not free the structure itself so the walk over chain can continue.  */
}

/* Notify finalize_compilation_unit that given node is reachable.  */

void
cgraph_mark_reachable_node (struct cgraph_node *node)
{
  if (!node->reachable && node->local.finalized)
    {
      notice_global_symbol (node->decl);
      node->reachable = 1;

      node->next_needed = cgraph_nodes_queue;
      cgraph_nodes_queue = node;
    }
}

/* Likewise indicate that a node is needed, i.e. reachable via some
   external means.  */

void
cgraph_mark_needed_node (struct cgraph_node *node)
{
  node->needed = 1;
  cgraph_mark_reachable_node (node);
}

/* Return local info for the compiled function.  */

struct cgraph_local_info *
cgraph_local_info (tree decl)
{
  struct cgraph_node *node;
  
  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  node = cgraph_node (decl);
  return &node->local;
}

/* Return local info for the compiled function.  */

struct cgraph_global_info *
cgraph_global_info (tree decl)
{
  struct cgraph_node *node;
  
  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL && cgraph_global_info_ready);
  node = cgraph_node (decl);
  return &node->global;
}

/* Return local info for the compiled function.  */

struct cgraph_rtl_info *
cgraph_rtl_info (tree decl)
{
  struct cgraph_node *node;
  
  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  node = cgraph_node (decl);
  if (decl != current_function_decl
      && !TREE_ASM_WRITTEN (node->decl))
    return NULL;
  return &node->rtl;
}

/* Return name of the node used in debug output.  */
const char *
cgraph_node_name (struct cgraph_node *node)
{
  return lang_hooks.decl_printable_name (node->decl, 2);
}
/* Return name of the node used in debug output.  */
static const char *
cgraph_varpool_node_name (struct cgraph_varpool_node *node)
{
  return lang_hooks.decl_printable_name (node->decl, 2);
}

/* Names used to print out the availability enum.  */
static const char * const availability_names[] = 
  {"unset", "not_available", "overwrittable",
   "overwrittable_but_inlinable", "available", "local"};

/* Dump given cgraph node.  */
void
dump_cgraph_node (FILE *f, struct cgraph_node *node)
{
  struct cgraph_node *n;
  struct cgraph_edge *edge;

  fprintf (f, "%s/%i:", cgraph_node_name (node), node->uid);
  if (node->master_clone && node->master_clone->uid != node->uid)
    fprintf (f, "(%i)", node->master_clone->uid);
  if (node->count)
    fprintf (f, " executed "HOST_WIDEST_INT_PRINT_DEC"x",
	     (HOST_WIDEST_INT)node->count);
  if (node->global.inlined_to)
    fprintf (f, " (inline copy in %s/%i)",
	     cgraph_node_name (node->global.inlined_to),
	     node->global.inlined_to->uid);
  if (cgraph_function_flags_ready)
    fprintf (f, " availability:%s", 
	     availability_names [cgraph_function_body_availability (node)]);
  if (node->local.self_insns)
    fprintf (f, " %i insns", node->local.self_insns);
  if (node->local.self_insns)
    fprintf (f, " %i insns", node->local.self_insns);
  if (node->global.insns && node->global.insns != node->local.self_insns)
    fprintf (f, " (%i after inlining)", node->global.insns);
  if (node->origin)
    fprintf (f, " nested in: %s", cgraph_node_name (node->origin));
  if (node->needed)
    fprintf (f, " needed");
  else if (node->reachable)
    fprintf (f, " reachable");
  if (DECL_SAVED_TREE (node->decl))
    fprintf (f, " tree");
  if (node->output)
    fprintf (f, " output");
  if (node->local.local)
    fprintf (f, " local");
  if (node->local.externally_visible)
    fprintf (f, " externally_visible");
  if (node->local.finalized)
    fprintf (f, " finalized");
  if (node->local.disregard_inline_limits)
    fprintf (f, " always_inline");
  else if (node->local.inlinable)
    fprintf (f, " inlinable");
  if (node->local.redefined_extern_inline)
    fprintf (f, " redefined_extern_inline");
  if (TREE_ASM_WRITTEN (node->decl))
    fprintf (f, " asm_written");

  fprintf (f, "\n  called by: ");
  for (edge = node->callers; edge; edge = edge->next_caller)
    {
      fprintf (f, "%s/%i ", cgraph_node_name (edge->caller),
	       edge->caller->uid);
      if (!edge->inline_failed)
	fprintf(f, "(inlined) ");
      if (edge->count)
	fprintf (f, "("HOST_WIDEST_INT_PRINT_DEC"x) ",
		 (HOST_WIDEST_INT)edge->count);
      if (edge->loop_nest)
	fprintf (f, "(nested in %i loops) ", edge->loop_nest);
    }

  fprintf (f, "\n  calls: ");
  for (edge = node->callees; edge; edge = edge->next_callee)
    {
      fprintf (f, "%s/%i ", cgraph_node_name (edge->callee),
	       edge->callee->uid);
      if (!edge->inline_failed)
	fprintf(f, "(inlined) ");
      if (edge->count)
	fprintf (f, "("HOST_WIDEST_INT_PRINT_DEC"x) ",
		 (HOST_WIDEST_INT)edge->count);
      if (edge->loop_nest)
	fprintf (f, "(nested in %i loops) ", edge->loop_nest);
    }
  fprintf (f, "\n  cycle: ");
  n = node->next_cycle;
  while (n)
    fprintf (f, "%s/%i ", cgraph_node_name (n), n->uid);
  fprintf (f, "\n");
}
/* Dump the callgraph.  */

void
dump_cgraph (FILE *f)
{
  struct cgraph_node *node;

  fprintf (f, "callgraph:\n\n");
  for (node = cgraph_nodes; node; node = node->next)
    dump_cgraph_node (f, node);
}

/* Dump given cgraph node.  */
void
dump_cgraph_varpool_node (FILE *f, struct cgraph_varpool_node *node)
{
  fprintf (f, "%s:", cgraph_varpool_node_name (node));
  fprintf (f, " availability:%s", availability_names [cgraph_variable_initializer_availability (node)]);
  if (DECL_INITIAL (node->decl))
    fprintf (f, " initialized");
  if (node->needed)
    fprintf (f, " needed");
  if (node->analyzed)
    fprintf (f, " analyzed");
  if (node->finalized)
    fprintf (f, " finalized");
  if (node->output)
    fprintf (f, " output");
  if (node->externally_visible)
    fprintf (f, " externally_visible");
  fprintf (f, "\n");
}

/* Dump the callgraph.  */

void
dump_varpool (FILE *f)
{
  struct cgraph_varpool_node *node;

  fprintf (f, "variable pool:\n\n");
  for (node = cgraph_varpool_nodes; node; node = node->next_needed)
    dump_cgraph_varpool_node (f, node);
}

/* Returns a hash code for P.  */

static hashval_t
hash_varpool_node (const void *p)
{
  const struct cgraph_varpool_node *n = p;
  return (hashval_t) DECL_UID (n->decl);
}

/* Returns nonzero if P1 and P2 are equal.  */

static int
eq_varpool_node (const void *p1, const void *p2)
{
  const struct cgraph_varpool_node *n1 = p1, *n2 = p2;
  return DECL_UID (n1->decl) == DECL_UID (n2->decl);
}

/* Return cgraph_varpool node assigned to DECL.  Create new one when needed.  */
struct cgraph_varpool_node *
cgraph_varpool_node (tree decl)
{
  struct cgraph_varpool_node key, *node, **slot;

  gcc_assert (DECL_P (decl) && TREE_CODE (decl) != FUNCTION_DECL);

  if (!cgraph_varpool_hash)
    cgraph_varpool_hash = htab_create_ggc (10, hash_varpool_node,
				           eq_varpool_node, NULL);
  key.decl = decl;
  slot = (struct cgraph_varpool_node **)
    htab_find_slot (cgraph_varpool_hash, &key, INSERT);
  if (*slot)
    return *slot;
  node = ggc_alloc_cleared (sizeof (*node));
  node->decl = decl;
  node->next = cgraph_varpool_nodes;
  cgraph_varpool_n_nodes++;
  cgraph_varpool_nodes = node;
  *slot = node;
  return node;
}

struct cgraph_varpool_node *
cgraph_varpool_node_for_asm (tree asmname)
{
  struct cgraph_varpool_node *node;

  for (node = cgraph_varpool_nodes; node ; node = node->next)
    if (decl_assembler_name_equal (node->decl, asmname))
      return node;

  return NULL;
}

/* Set the DECL_ASSEMBLER_NAME and update cgraph hashtables.  */
void
change_decl_assembler_name (tree decl, tree name)
{
  if (!DECL_ASSEMBLER_NAME_SET_P (decl))
    {
      SET_DECL_ASSEMBLER_NAME (decl, name);
      return;
    }
  if (name == DECL_ASSEMBLER_NAME (decl))
    return;

  if (TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (decl))
      && DECL_RTL_SET_P (decl))
    warning ("%D renamed after being referenced in assembly", decl);

  SET_DECL_ASSEMBLER_NAME (decl, name);
}

/* Helper function for finalization code - add node into lists so it will
   be analyzed and compiled.  */
void
cgraph_varpool_enqueue_needed_node (struct cgraph_varpool_node *node)
{
  if (cgraph_varpool_last_needed_node)
    cgraph_varpool_last_needed_node->next_needed = node;
  cgraph_varpool_last_needed_node = node;
  node->next_needed = NULL;
  if (!cgraph_varpool_nodes_queue)
    cgraph_varpool_nodes_queue = node;
  if (!cgraph_varpool_first_unanalyzed_node)
    cgraph_varpool_first_unanalyzed_node = node;
  notice_global_symbol (node->decl);
}

/* Reset the queue of needed nodes.  */
void
cgraph_varpool_reset_queue (void)
{
  cgraph_varpool_last_needed_node = NULL;
  cgraph_varpool_nodes_queue = NULL;
  cgraph_varpool_first_unanalyzed_node = NULL;
}

/* Notify finalize_compilation_unit that given node is reachable
   or needed.  */
void
cgraph_varpool_mark_needed_node (struct cgraph_varpool_node *node)
{
  if (!node->needed && node->finalized)
    cgraph_varpool_enqueue_needed_node (node);
  node->needed = 1;
}

/* Determine if variable DECL is needed.  That is, visible to something
   either outside this translation unit, something magic in the system
   configury, or (if not doing unit-at-a-time) to something we haven't
   seen yet.  */

static bool
decide_is_variable_needed (struct cgraph_varpool_node *node, tree decl)
{
  /* If the user told us it is used, then it must be so.  */
  if (lookup_attribute ("used", DECL_ATTRIBUTES (decl)))
    {
      if (TREE_PUBLIC (decl))
        node->externally_visible = true;
      return true;
    }

  /* ??? If the assembler name is set by hand, it is possible to assemble
     the name later after finalizing the function and the fact is noticed
     in assemble_name then.  This is arguably a bug.  */
  if (DECL_ASSEMBLER_NAME_SET_P (decl)
      && TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (decl)))
    {
      if (TREE_PUBLIC (decl))
        node->externally_visible = true;
      return true;
    }

  /* If we decided it was needed before, but at the time we didn't have
     the definition available, then it's still needed.  */
  if (node->needed)
    return true;

  /* Externally visible functions must be output.  The exception is
     COMDAT functions that must be output only when they are needed.  */
  if (TREE_PUBLIC (decl) && !DECL_COMDAT (decl) && !DECL_EXTERNAL (decl))
    return true;

  if (flag_unit_at_a_time)
    return false;

  /* If not doing unit at a time, then we'll only defer this function
     if its marked for inlining.  Otherwise we want to emit it now.  */

  /* We want to emit COMDAT variables only when absolutely necessary.  */
  if (DECL_COMDAT (decl))
    return false;
  return true;
}

void
cgraph_varpool_finalize_decl (tree decl)
{
  struct cgraph_varpool_node *node = cgraph_varpool_node (decl);
 
  /* The first declaration of a variable that comes through this function
     decides whether it is global (in C, has external linkage)
     or local (in C, has internal linkage).  So do nothing more
     if this function has already run.  */
  if (node->finalized)
    {
      if (cgraph_global_info_ready || !flag_unit_at_a_time)
	cgraph_varpool_assemble_pending_decls ();
      return;
    }
  if (node->needed)
    cgraph_varpool_enqueue_needed_node (node);
  node->finalized = true;

  if (decide_is_variable_needed (node, decl))
    cgraph_varpool_mark_needed_node (node);
  if (cgraph_global_info_ready || !flag_unit_at_a_time)
    cgraph_varpool_assemble_pending_decls ();
}


/* Return true when the DECL can possibly be inlined.  */
bool
cgraph_function_possibly_inlined_p (tree decl)
{
  if (!cgraph_global_info_ready)
    return (DECL_INLINE (decl) && !flag_really_no_inline);
  return DECL_POSSIBLY_INLINED (decl);
}

/* Create clone of E in the node N represented by CALL_EXPR the callgraph.  */
struct cgraph_edge *
cgraph_clone_edge (struct cgraph_edge *e, struct cgraph_node *n,
		   tree call_expr, int count_scale, int loop_nest)
{
  struct cgraph_edge *new;

  new = cgraph_create_edge (n, e->callee, call_expr,
                            e->count * count_scale / REG_BR_PROB_BASE,
		            e->loop_nest + loop_nest);

  new->inline_failed = e->inline_failed;
  e->count -= new->count;
  return new;
}

/* Create node representing clone of N executed COUNT times.  Decrease
   the execution counts from original node too.  */
struct cgraph_node *
cgraph_clone_node (struct cgraph_node *n, gcov_type count, int loop_nest)
{
  struct cgraph_node *new = cgraph_create_node ();
  struct cgraph_edge *e;
  int count_scale;

  new->decl = n->decl;
  new->origin = n->origin;
  if (new->origin)
    {
      new->next_nested = new->origin->nested;
      new->origin->nested = new;
    }
  new->analyzed = n->analyzed;
  new->static_vars_info = n->static_vars_info;
  new->local = n->local;
  new->global = n->global;
  new->rtl = n->rtl;
  new->master_clone = n->master_clone;
  new->count = count;
  if (n->count)
    count_scale = new->count * REG_BR_PROB_BASE / n->count;
  else
    count_scale = 0;
  n->count -= count;

  for (e = n->callees;e; e=e->next_callee)
    cgraph_clone_edge (e, new, e->call_expr, count_scale, loop_nest);

  new->next_clone = n->next_clone;
  n->next_clone = new;

  return new;
}

/* Return true if N is an master_clone, (see cgraph_master_clone).  */

bool
cgraph_is_master_clone (struct cgraph_node *n)
{
  return (n == cgraph_master_clone (n));
}

/* Return true if N is an immortal_master_clone, (see
   cgraph_immortal_master_clone).  */

bool
cgraph_is_immortal_master_clone (struct cgraph_node *n)
{
  return (n == cgraph_immortal_master_clone (n));
}

/* Return pointer to the callgraph node presenting master clone of N.
   For functions defined outside the compilation unit, NULL is
   returned.  For inline functions that have no out of line clone to
   be output this the choice of which is the master is random and this
   node will not exist after inlining has happened.  For inline
   functions that do have an out of line clone to be output, the
   master clone is a cgraph node that will not be released before any
   of the clones.  */

struct cgraph_node *
cgraph_master_clone (struct cgraph_node *n)
{
  enum availability avail = cgraph_function_body_availability (n);
   
  if (avail == AVAIL_NOT_AVAILABLE || avail == AVAIL_OVERWRITABLE)
    return NULL;

  if (!n->master_clone) 
    n->master_clone = cgraph_node (n->decl);
  
  return n->master_clone;
}

/* Return pointer to the callgraph node presenting immortal master
   clone of N, if one exists, NULL otherwise.  The immortal master
   clone is a cgraph node that will not be released before any of the
   clones and thus it is usefull for holding information about
   function body that is the same for all clones.  Inline functions
   that have no out of line clone to be output have a master node that
   is not immortal.  For functions defined outside the compilation
   unit, NULL is returned.  */

struct cgraph_node *
cgraph_immortal_master_clone (struct cgraph_node *n)
{
  struct cgraph_node *master = cgraph_master_clone (n);
  if (!master) 
    return NULL;

  if (master->global.inlined_to)
    return NULL;
  return master;
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
  enum availability avail = node->local.avail;
  enum availability old_avail = avail;

  gcc_assert (cgraph_function_flags_ready);
#if 0
  if (avail != AVAIL_UNSET)
    return avail; 
#endif
  if (!node->local.finalized)
    avail = AVAIL_NOT_AVAILABLE;
  else if (node->local.local)
    avail = AVAIL_LOCAL;
  else if (!node->local.externally_visible)
    avail = AVAIL_AVAILABLE;

  /* If the function can be overwritten, return OVERWRITABLE.  Take
     care at least of two notable extensions - the COMDAT functions
     used to share template instantiations in C++ (this is symmetric
     to code cp_cannot_inline_tree_fn and probably shall be shared and
     the inlinability hooks completelly elliminated).

     ??? Does the C++ one definition rule allow us to always return
     AVAIL_AVAILABLE here?  That would be good reason to preserve this
     hook Similarly deal with extern inline functions - this is again
     neccesary to get C++ shared functions having keyed templates
     right and in the C extension documentation we probably should
     document the requirement of both versions of function (extern
     inline and offline) having same side effect characteristics as
     good optimization is what this optimization is about.  */
  
  else if (!(*targetm.binds_local_p) (node->decl)
	   && !DECL_COMDAT (node->decl) && !DECL_EXTERNAL (node->decl))
    if (DECL_DECLARED_INLINE_P (node->decl))
      avail = AVAIL_OVERWRITABLE_BUT_INLINABLE;
    else 
      avail = AVAIL_OVERWRITABLE;
  else avail = AVAIL_AVAILABLE;

  if ((old_avail != AVAIL_UNSET) && (avail != old_avail)
      && (avail != AVAIL_LOCAL || old_avail != AVAIL_AVAILABLE))
    abort ();

  node->local.avail = avail;
  return avail;
}

/* Return variable availability.  See cgraph.h for description of individual
   return values.  */
enum availability
cgraph_variable_initializer_availability (struct cgraph_varpool_node *node)
{
  gcc_assert (cgraph_function_flags_ready);
  if (!node->finalized)
    return AVAIL_NOT_AVAILABLE;
  if (!TREE_PUBLIC (node->decl))
    return AVAIL_AVAILABLE;
  /* If the variable can be overwritted, return OVERWRITABLE.  Takes
     care of at least two notable extensions - the COMDAT variables
     used to share template instantiations in C++.  */
  if (!(*targetm.binds_local_p) (node->decl) && !DECL_COMDAT (node->decl))
    return AVAIL_OVERWRITABLE;
  return AVAIL_AVAILABLE;
}

/* Return true when CALLER_DECL should be inlined into CALLEE_DECL.  */

bool
cgraph_inline_p (struct cgraph_edge *e, const char **reason)
{
  *reason = e->inline_failed;
  return !e->inline_failed;
}

#include "gt-cgraph.h"
