/* Declarations for rtx-reader support for gen* routines.
   Copyright (C) 2000, 2002, 2003 Free Software Foundation, Inc.

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

#ifndef GCC_GENSUPPORT_H
#define GCC_GENSUPPORT_H

struct obstack;
extern struct obstack *rtl_obstack;
extern const char *in_fname;

extern int init_md_reader_args_cb (int, char **, bool (*)(const char *));
extern int init_md_reader_args (int, char **);
extern rtx read_md_rtx (int *, int *);

extern void message_with_line (int, const char *, ...)
     ATTRIBUTE_PRINTF_2;

/* Set this to 0 to disable automatic elision of insn patterns which
   can never be used in this configuration.  See genconditions.c.
   Must be set before calling init_md_reader.  */
extern int insn_elision;

/* If this is 1, the insn elision table doesn't even exist yet;
   maybe_eval_c_test will always return -1.  This is distinct from
   insn_elision because genflags and gencodes need to see all the
   patterns, but treat elided patterns differently.  */
extern const int insn_elision_unavailable;

/* If the C test passed as the argument can be evaluated at compile
   time, return its truth value; else return -1.  The test must have
   appeared somewhere in the machine description when genconditions
   was run.  */
extern int maybe_eval_c_test (const char *);

/* This table should not be accessed directly; use maybe_eval_c_test.  */
struct c_test
{
  const char *expr;
  int value;
};

extern const struct c_test insn_conditions[];
extern const size_t n_insn_conditions;

#ifdef __HASHTAB_H__
extern hashval_t hash_c_test (const void *);
extern int cmp_c_test (const void *, const void *);
#endif

extern int n_comma_elts	(const char *);
extern const char *scan_comma_elt (const char **);

/* Predicate handling: helper functions and data structures.  */

struct pred_data
{
  struct pred_data *next;	/* for iterating over the set of all preds */
  const char *name;		/* predicate name */
  bool special;			/* special handling of modes? */

  /* data used primarily by genpreds.c */
  const char *c_block;		/* C test block */
  rtx exp;			/* RTL test expression */

  /* data used primarily by genrecog.c */
  enum rtx_code singleton;	/* if pred takes only one code, that code */
  bool allows_non_lvalue;	/* if pred allows non-lvalue expressions */
  bool allows_non_const;	/* if pred allows non-const expressions */
  bool codes[NUM_RTX_CODE];	/* set of codes accepted */
};

extern struct pred_data *first_predicate;
extern struct pred_data *lookup_predicate (const char *);
extern void add_predicate (struct pred_data *);

#define FOR_ALL_PREDICATES(p) for (p = first_predicate; p; p = p->next)

#endif /* GCC_GENSUPPORT_H */
