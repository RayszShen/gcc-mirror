/* Iterator routines for manipulating GENERIC and GIMPLE tree statements.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.
   Contributed by Andrew MacLeod  <amacleod@redhat.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "tree-gimple.h"
#include "tree-iterator.h"
#include "ggc.h"


/* This is a cache of STATEMENT_LIST nodes.  We create and destroy them
   fairly often during gimplification.  */

static GTY ((deletable (""))) tree stmt_list_cache;

tree
alloc_stmt_list (void)
{
  tree list = stmt_list_cache;
  if (list)
    {
      stmt_list_cache = TREE_CHAIN (list);
      TREE_CHAIN (list) = NULL;
      TREE_SIDE_EFFECTS (list) = 0;
    }
  else
    {
      list = make_node (STATEMENT_LIST);
      TREE_TYPE (list) = void_type_node;
    }
  return list;
}

void
free_stmt_list (tree t)
{
#ifdef ENABLE_CHECKING
  if (STATEMENT_LIST_HEAD (t) || STATEMENT_LIST_TAIL (t))
    abort ();
#endif
  TREE_CHAIN (t) = stmt_list_cache;
  stmt_list_cache = t;
}

/* Links a statement, or a chain of statements, before the current stmt.  */

void
tsi_link_before (tree_stmt_iterator *i, tree t, enum tsi_iterator_update mode)
{
  struct tree_statement_list_node *head, *tail, *cur;

  /* Die on looping.  */
  if (t == i->container)
    abort ();

  TREE_SIDE_EFFECTS (i->container) = 1;

  if (TREE_CODE (t) == STATEMENT_LIST)
    {
      head = STATEMENT_LIST_HEAD (t);
      tail = STATEMENT_LIST_TAIL (t);
      STATEMENT_LIST_HEAD (t) = NULL;
      STATEMENT_LIST_TAIL (t) = NULL;

      free_stmt_list (t);

      /* Empty statement lists need no work.  */
      if (!head || !tail)
	{
	  if (head != tail)
	    abort ();
	  return;
	}
    }
  else
    {
      head = ggc_alloc (sizeof (*head));
      head->prev = NULL;
      head->next = NULL;
      head->stmt = t;
      tail = head;
    }

  cur = i->ptr;

  /* Link it into the list.  */
  if (cur)
    {
      head->prev = cur->prev;
      if (head->prev)
	head->prev->next = head;
      else
	STATEMENT_LIST_HEAD (i->container) = head;
      tail->next = cur;
      cur->prev = tail;
    }
  else
    {
      if (STATEMENT_LIST_TAIL (i->container))
	abort ();
      STATEMENT_LIST_HEAD (i->container) = head;
      STATEMENT_LIST_TAIL (i->container) = tail;
    }

  /* Update the iterator, if requested.  */
  switch (mode)
    {
    case TSI_NEW_STMT:
    case TSI_CONTINUE_LINKING:
    case TSI_CHAIN_START:
      i->ptr = head;
      break;
    case TSI_CHAIN_END:
      i->ptr = tail;
      break;
    case TSI_SAME_STMT:
      if (!cur)
	abort ();
      break;
    }
}

/* Links a statement, or a chain of statements, after the current stmt.  */

void
tsi_link_after (tree_stmt_iterator *i, tree t, enum tsi_iterator_update mode)
{
  struct tree_statement_list_node *head, *tail, *cur;

  /* Die on looping.  */
  if (t == i->container)
    abort ();

  TREE_SIDE_EFFECTS (i->container) = 1;

  if (TREE_CODE (t) == STATEMENT_LIST)
    {
      head = STATEMENT_LIST_HEAD (t);
      tail = STATEMENT_LIST_TAIL (t);
      STATEMENT_LIST_HEAD (t) = NULL;
      STATEMENT_LIST_TAIL (t) = NULL;

      free_stmt_list (t);

      /* Empty statement lists need no work.  */
      if (!head || !tail)
	{
	  if (head != tail)
	    abort ();
	  return;
	}
    }
  else
    {
      head = ggc_alloc (sizeof (*head));
      head->prev = NULL;
      head->next = NULL;
      head->stmt = t;
      tail = head;
    }

  cur = i->ptr;

  /* Link it into the list.  */
  if (cur)
    {
      tail->next = cur->next;
      if (tail->next)
	tail->next->prev = tail;
      else
	STATEMENT_LIST_TAIL (i->container) = tail;
      head->prev = cur;
      cur->next = head;
    }
  else
    {
      if (STATEMENT_LIST_TAIL (i->container))
	abort ();
      STATEMENT_LIST_HEAD (i->container) = head;
      STATEMENT_LIST_TAIL (i->container) = tail;
    }

  /* Update the iterator, if requested.  */
  switch (mode)
    {
    case TSI_NEW_STMT:
    case TSI_CHAIN_START:
      i->ptr = head;
      break;
    case TSI_CONTINUE_LINKING:
    case TSI_CHAIN_END:
      i->ptr = tail;
      break;
    case TSI_SAME_STMT:
      if (!cur)
        abort ();
      break;
    }
}

/* Remove a stmt from the tree list.  The iterator is updated to point to
   the next stmt.  */

void
tsi_delink (tree_stmt_iterator *i)
{
  struct tree_statement_list_node *cur, *next, *prev;

  cur = i->ptr;
  next = cur->next;
  prev = cur->prev;

  if (prev)
    prev->next = next;
  else
    STATEMENT_LIST_HEAD (i->container) = next;
  if (next)
    next->prev = prev;
  else
    STATEMENT_LIST_TAIL (i->container) = prev;

  if (!next && !prev)
    TREE_SIDE_EFFECTS (i->container) = 0;

  i->ptr = next;
}

/* Move all statements in the statement list after I to a new
   statement list.  I itself is unchanged.  */

tree
tsi_split_statement_list_after (const tree_stmt_iterator *i)
{
  struct tree_statement_list_node *cur, *next;
  tree old_sl, new_sl;

  cur = i->ptr;
  /* How can we possibly split after the end, or before the beginning?  */
  if (cur == NULL)
    abort ();
  next = cur->next;

  old_sl = i->container;
  new_sl = alloc_stmt_list ();
  TREE_SIDE_EFFECTS (new_sl) = 1;

  STATEMENT_LIST_HEAD (new_sl) = next;
  STATEMENT_LIST_TAIL (new_sl) = STATEMENT_LIST_TAIL (old_sl);
  STATEMENT_LIST_TAIL (old_sl) = cur;
  cur->next = NULL;
  next->prev = NULL;

  return new_sl;
}

/* Move all statements in the statement list before I to a new
   statement list.  I is set to the head of the new list.  */

tree
tsi_split_statement_list_before (tree_stmt_iterator *i)
{
  struct tree_statement_list_node *cur, *prev;
  tree old_sl, new_sl;

  cur = i->ptr;
  /* How can we possibly split after the end, or before the beginning?  */
  if (cur == NULL)
    abort ();
  prev = cur->prev;

  old_sl = i->container;
  new_sl = alloc_stmt_list ();
  TREE_SIDE_EFFECTS (new_sl) = 1;
  i->container = new_sl;

  STATEMENT_LIST_HEAD (new_sl) = cur;
  STATEMENT_LIST_TAIL (new_sl) = STATEMENT_LIST_TAIL (old_sl);
  STATEMENT_LIST_TAIL (old_sl) = prev;
  cur->prev = NULL;
  prev->next = NULL;

  return new_sl;
}

/* Return the first expression in a sequence of COMPOUND_EXPRs,
   or in a STATEMENT_LIST.  */

tree
expr_first (tree expr)
{
  if (expr == NULL_TREE)
    return expr;

  if (TREE_CODE (expr) == STATEMENT_LIST)
    {
      struct tree_statement_list_node *n = STATEMENT_LIST_HEAD (expr);
      return n ? n->stmt : NULL_TREE;
    }

  while (TREE_CODE (expr) == COMPOUND_EXPR)
    expr = TREE_OPERAND (expr, 0);
  return expr;
}

/* Return the last expression in a sequence of COMPOUND_EXPRs,
   or in a STATEMENT_LIST.  */

tree
expr_last (tree expr)
{
  if (expr == NULL_TREE)
    return expr;

  if (TREE_CODE (expr) == STATEMENT_LIST)
    {
      struct tree_statement_list_node *n = STATEMENT_LIST_TAIL (expr);
      return n ? n->stmt : NULL_TREE;
    }

  while (TREE_CODE (expr) == COMPOUND_EXPR)
    expr = TREE_OPERAND (expr, 1);
  return expr;
}

/* If EXPR is a single statement, naked or in a STATEMENT_LIST, then
   return it.  Otherwise return NULL.  */

tree 
expr_only (tree expr)
{
  if (expr == NULL_TREE)
    return NULL_TREE;

  if (TREE_CODE (expr) == STATEMENT_LIST)
    {
      struct tree_statement_list_node *n = STATEMENT_LIST_TAIL (expr);
      if (n && STATEMENT_LIST_HEAD (expr) == n)
	return n->stmt;
      else
	return NULL_TREE;
    }

  if (TREE_CODE (expr) == COMPOUND_EXPR)
    return NULL_TREE;

  return expr;
}

#include "gt-tree-iterator.h"
