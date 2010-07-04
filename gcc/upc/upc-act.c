/* Implement UPC semantics
   Copyright (C) 2001 Free Software Foundation, Inc.
   Original Implementation by Jesse M. Draper <jdraper@super.org>
   and William W. Carlson <wwc@super.org>.
   Ported to SGI Irix 6.5 and the gcc 2.95.2 baseline by
   Gary Funck <gary@intrepid.com> and Nenad Vukicevic <nenad@intrepid.com>.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "coretypes.h"
#include "system.h"
#include "tree.h"
#include "tree-iterator.h"
#include "ggc.h"
#include "hashtab.h"
#include "input.h"
#include "c-tree.h"
#include "langhooks.h"
#include "flags.h"
#include "opts.h"
#include "options.h"
#include "output.h"
#include "toplev.h"
#include "gimple.h"
#include "tm.h"
#include "function.h"
#include "target.h"
#include "upc-act.h"
#include "upc-pts.h"
#include "cgraph.h"
#include "c-family/c-common.h"
#include "c-family/c-pragma.h"
/* define decl_default_tls_model() prototype */
#include "rtl.h"

static int contains_pts_refs_p (tree);
static int recursive_count_upc_threads_refs (tree);
static tree upc_blocksizeof (location_t, tree);
static void upc_lang_layout_decl (tree, tree);
static int upc_lang_layout_decl_p (tree, tree);
static void upc_parse_init (void);
static int upc_sizeof_type_check (const char *, tree);
static void upc_build_init_func (const tree);

/* Given a shared variable's VAR_DECL node, map to another
   VAR_DECL that has same external symbol name, with
   the "shared" qualifier removed from its type.  This
   "shadow variable" is used to generate conventional
   address constants when referring to a shared variable.  */

struct GTY(()) uid_tree_map
{
  unsigned int uid;
  tree to;
};

/* Hash the UID of a shared variable to its unshared shadow variable.  */
static GTY ((param_is (struct uid_tree_map))) htab_t unshared_vars;

static hashval_t uid_tree_map_hash (const void *);
static int uid_tree_map_eq (const void *, const void *);

static GTY(()) tree upc_init_stmt_list;
static GTY(()) section *upc_init_array_section;

static tree lookup_unshared_var (const tree);
static tree unshared_var_name (const tree);
static tree create_unshared_var (location_t, const tree);
static void map_unshared_var (const tree, const tree);
static tree unshared_var_addr (location_t, const tree);


   
/* Process UPC specific command line switches */

int
upc_handle_option (size_t scode, const char *arg, int value, int kind)
{
  enum opt_code code = (enum opt_code) scode;
  int result = 1;
  switch (code)
    {
    default:
      result = c_common_handle_option (scode, arg, value, kind);
      break;
    case OPT_dwarf_2_upc:
      use_upc_dwarf2_extensions = value;
      break;
    case OPT_fupc_inline_lib:
      if ((value == 1) && (flag_upc_instrument == 1))
        error ("-fupc-instrument is incompatible with -fupc-inline-lib");
      flag_upc_inline_lib = value;
      break;
    case OPT_fupc_instrument:
      if ((value == 1) && (flag_upc_inline_lib == 1))
        error ("-fupc-instrument is incompatible with -fupc-inline-lib");
      flag_upc_instrument = value;
      break;
    case OPT_fupc_instrument_functions:
      if ((value == 1) && (flag_upc_inline_lib == 1))
        error ("-fupc-instrument-functions is incompatible with -fupc-inline-lib");
      flag_upc_instrument = value;
      flag_upc_instrument_functions = value;
      break;
    case OPT_fupc_pthreads_model_tls:
      flag_upc_pthreads = 1;
      upc_pthreads_model = upc_pthreads_tls_model;
      break;
    case OPT_fupc_pthreads_per_process_:
      if (value > UPC_MAX_THREADS)
	{
	  char msg[100];
	  sprintf (msg, "THREADS value exceeds implementation limit of %d",
		   UPC_MAX_THREADS);
	  error (msg);
	  value = 1;
	}
      flag_upc_pthreads_per_process = value;
      break;
    case OPT_fupc_threads_:
      if (value > UPC_MAX_THREADS)
	{
	  char msg[100];
	  sprintf (msg, "THREADS value exceeds implementation limit of %d",
		   UPC_MAX_THREADS);
	  error (msg);
	  value = 1;
	}
      flag_upc_threads = value;
      break;
    case OPT_lang_upc:
      flag_upc = value;
      break;
    }
  return result;
}

bool
upc_lang_init (void)
{
  if (!targetm.have_named_sections)
    {
      fatal_error ("UPC is not implemented on this target. The target linker does not support separately linked sections.");
    }
  /* c_obj_common_init is also called from regular 'C'
     It will return 'false' if we're pre-processing only. */
  if (c_objc_common_init () == false)
    return false;
  upc_parse_init ();
  return true;
}

void
upc_finish (void)
{
}

/* UPC specific initialization */

void
upc_cpp_builtins (cpp_reader *pfile)
{
  char def_buf[256];
  cpp_define (pfile, "__GCC_UPC__=1");
  cpp_define (pfile, "__UPC__=1");
  cpp_define (pfile, "__UPC_VERSION__=200505L");
  (void) sprintf (def_buf, "UPC_MAX_BLOCK_SIZE=%s", UPC_MAX_BLOCK_SIZE_STRING);
  cpp_define (pfile, def_buf);
#if defined(UPC_PTS_PACKED_REP)
  cpp_define (pfile, "__UPC_PTS_PACKED_REP__=1");
#elif defined(UPC_PTS_STRUCT_REP)
  cpp_define (pfile, "__UPC_PTS_STRUCT_REP__=1");
  (void) sprintf (def_buf, "__UPC_VADDR_TYPE__=%s", UPC_PTS_VADDR_TYPE);
  cpp_define (pfile, def_buf);
  (void) sprintf (def_buf, "__UPC_THREAD_TYPE__=%s", UPC_PTS_THREAD_TYPE);
  cpp_define (pfile, def_buf);
  (void) sprintf (def_buf, "__UPC_PHASE_TYPE__=%s", UPC_PTS_PHASE_TYPE);
  cpp_define (pfile, def_buf);
#ifdef UPC_PTS_VADDR_FIELD
  (void) sprintf (def_buf, "__UPC_VADDR_FIELD__=%s", UPC_PTS_VADDR_FIELD);
  cpp_define (pfile, def_buf);
#endif
#ifdef UPC_PTS_THREAD_FIELD
  (void) sprintf (def_buf, "__UPC_THREAD_FIELD__=%s", UPC_PTS_THREAD_FIELD);
  cpp_define (pfile, def_buf);
#endif
#ifdef UPC_PTS_PHASE_FIELD
  (void) sprintf (def_buf, "__UPC_PHASE_FIELD__=%s", UPC_PTS_PHASE_FIELD);
  cpp_define (pfile, def_buf);
#endif
#elif defined(UPC_PTS_WORD_PAIR_REP)
  cpp_define (pfile, "__UPC_PTS_WORD_PAIR_REP__=1");
#endif
#ifdef UPC_PTS_VADDR_FIRST
  (void) sprintf (def_buf, "__UPC_VADDR_FIRST__=%d", UPC_PTS_VADDR_FIRST);
  cpp_define (pfile, def_buf);
#endif
  (void) sprintf (def_buf, "__UPC_PTS_SIZE__=%d", UPC_PTS_SIZE);
  cpp_define (pfile, def_buf);
  (void) sprintf (def_buf, "__UPC_VADDR_SIZE__=%d", UPC_PTS_VADDR_SIZE);
  cpp_define (pfile, def_buf);
  (void) sprintf (def_buf, "__UPC_THREAD_SIZE__=%d", UPC_PTS_THREAD_SIZE);
  cpp_define (pfile, def_buf);
  (void) sprintf (def_buf, "__UPC_PHASE_SIZE__=%d", UPC_PTS_PHASE_SIZE);
  cpp_define (pfile, def_buf);
  if (flag_upc_threads)
    {
      cpp_define (pfile, "__UPC_STATIC_THREADS__=1");
      (void) sprintf (def_buf, "THREADS=%d", flag_upc_threads);
      cpp_define (pfile, def_buf);
    }
  else
    {
      cpp_define (pfile, "__UPC_DYNAMIC_THREADS__=1");
    }
  if (flag_upc_pthreads
      && (upc_pthreads_model == upc_pthreads_tls_model))  
    {
      cpp_define (pfile, "__UPC_PTHREADS_MODEL_TLS__=1");
      if (flag_upc_pthreads_per_process)
        {
          cpp_define (pfile, "__UPC_STATIC_PTHREADS__=1");
          (void) sprintf (def_buf, "PTHREADS=%d",
	                      flag_upc_pthreads_per_process);
          cpp_define (pfile, def_buf);
	}
      else
        {
          cpp_define (pfile, "__UPC_DYNAMIC_PTHREADS__=1");
	}
    }
  /* Collectives are supported. */
  cpp_define (parse_in, "__UPC_COLLECTIVE__=1");
  /* If instrumentation is enabled, then disable inlining of the runtime.  */
  if (flag_upc_instrument)
    flag_upc_inline_lib = 0;
  /* If -f[no-]upc-inline-lib hasn't been asserted, force inlining of the
     runtime library if optimization is enabled.  */
  if (flag_upc_inline_lib < 0)
    flag_upc_inline_lib = (optimize >= 1);
  if (flag_upc_inline_lib)
    cpp_define (parse_in, "__UPC_INLINE_LIB__=1");
  /* UPC profiling capabilities are implemented.  */
  cpp_define (parse_in, "__UPC_PUPC__=1");
  /* UPC profiling instrumentation code will be generated.  */
  if (flag_upc_instrument)
    {
      cpp_define (parse_in, "__UPC_PUPC_INST__=1");
    }
}

static void
upc_parse_init (void)
{
  set_lang_layout_decl_p (upc_lang_layout_decl_p);
  set_lang_layout_decl (upc_lang_layout_decl);
  upc_pts_init_type ();
  unshared_vars = htab_create_ggc (101, uid_tree_map_hash, uid_tree_map_eq, NULL);
  upc_init_stmt_list = NULL;
}

tree
upc_build_sync_stmt (location_t loc, tree sync_kind, tree sync_expr)
{
  return add_stmt (build_stmt (loc, UPC_SYNC_STMT, sync_kind, sync_expr)); 
}

static
int
upc_sizeof_type_check (const char *op_name, tree type)
{
  enum tree_code code = TREE_CODE (type);
  if (code == ERROR_MARK)
    {
      return 0;
    }
  else if (!COMPLETE_TYPE_P (type))
    {
      c_incomplete_type_error (NULL_TREE, type);
      return 0;
    }
  else if (code == FUNCTION_TYPE)
    {
      error ("%s applied to a function type", op_name);
      return 0;
    }
  else if (code == VOID_TYPE)
    {
      error ("%s applied to a void type", op_name);
      return 0;
    }
  else if (!upc_shared_type_p (type))
    {
      error ("%s applied to a non-shared type", op_name);
      return 0;
    }
  return 1;
}

/* Compute the value of the `upc_blocksizeof' operator.  */

static
tree
upc_blocksizeof (location_t ARG_UNUSED(loc), tree type)
{
  tree block_factor = size_one_node;
  if (!type || TREE_CODE (type) == ERROR_MARK)
    return error_mark_node;
  if (upc_sizeof_type_check ("upc_blocksizeof", type))
    block_factor = upc_get_block_factor (type);
  return block_factor;
}

/* Return the result of upc_blocksizeof applied to EXPR.  */

struct c_expr
upc_blocksizeof_expr (location_t loc, struct c_expr expr)
{
  struct c_expr ret;
  if (expr.value == error_mark_node)
    {
      ret.value = error_mark_node;
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (false);
    }
  else
    {
      ret.value = upc_blocksizeof (loc, TREE_TYPE (expr.value));
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (C_TYPE_VARIABLE_SIZE (TREE_TYPE (expr.value)));
    }
  return ret;
}

/* Return the result of upc_blocksizeof applied to T, a structure
   for the type name passed to sizeof (rather than the type itself).  */

struct c_expr
upc_blocksizeof_type (location_t loc, struct c_type_name *t)
{
  tree type;
  struct c_expr ret;
  type = groktypename (t, NULL, NULL);
  ret.value = upc_blocksizeof (loc, type);
  ret.original_code = ERROR_MARK;
  ret.original_type = NULL_TREE;
  pop_maybe_used (C_TYPE_VARIABLE_SIZE (type));
  return ret;
}

/* Compute the value of the `upc_elemsizeof' operator.  */

static
tree
upc_elemsizeof (location_t loc, tree type)
{
  tree elem_size;

  if (!(type && upc_sizeof_type_check ("upc_elemsizeof", type)))
    return size_int (1);
  elem_size = c_sizeof (loc, strip_array_types (type));
  return elem_size;
}

/* Return the result of upc_elemsizeof applied to EXPR.  */

struct c_expr
upc_elemsizeof_expr (location_t loc, struct c_expr expr)
{
  struct c_expr ret;
  if (expr.value == error_mark_node)
    {
      ret.value = error_mark_node;
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (false);
    }
  else
    {
      ret.value = upc_elemsizeof (loc, TREE_TYPE (expr.value));
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (C_TYPE_VARIABLE_SIZE (TREE_TYPE (expr.value)));
    }
  return ret;
}

/* Return the result of upc_elemsizeof applied to T, a structure
   for the type name passed to sizeof (rather than the type itself).  */

struct c_expr
upc_elemsizeof_type (location_t loc, struct c_type_name *t)
{
  tree type;
  struct c_expr ret;
  type = groktypename (t, NULL, NULL);
  ret.value = upc_elemsizeof (loc, type);
  ret.original_code = ERROR_MARK;
  ret.original_type = NULL_TREE;
  pop_maybe_used (C_TYPE_VARIABLE_SIZE (type));
  return ret;
}

/* Compute the value of the `upc_localsizeof' operator.  This value
   can vary from thread to thread if the array size is not a multiple of
   (THREADS * block_factor * upc_elementsizeof (type)). */

static
tree
upc_localsizeof (location_t loc, tree type)
{
  tree block_factor, local_size, total_size, elt_type, elt_size;

  if (!(type && upc_sizeof_type_check ("upc_localsizeof", type)))
    return size_one_node;

  /* for scalars, return sizeof */

  if (TREE_CODE(type) != ARRAY_TYPE)
    return c_sizeof(loc, type);

  block_factor = upc_blocksizeof (loc, type);
  total_size = TYPE_SIZE (type);
  elt_type = strip_array_types (type);
  if (!elt_type || TREE_CODE (elt_type) == ERROR_MARK)
    return size_one_node;
  elt_size = TYPE_SIZE (elt_type);

  if (UPC_TYPE_HAS_THREADS_FACTOR (type))
    {
      /* Neither an indefinite block size, nor static threads
         environment should be asserted if "threads factor"
	 is present. */
      if (integer_zerop (block_factor) || flag_upc_threads)
        abort ();
      /* The type size has already been scaled to give
         the per thread contribution (in bits). */
      local_size = total_size;
    }
  if (integer_zerop (block_factor))
    {
      /* local size is total size, because the entire
         object lives on a single thread. */
      local_size = total_size;
    }
  else
    {
      tree n_threads, n_elts, n_full_blocks;
      tree n_full_blocks_per_thread, n_elts_in_full_blocks;
      tree n_rem_elts, n_local_elts;
      block_factor = convert (bitsizetype, block_factor);
      n_elts = size_binop (EXACT_DIV_EXPR, total_size, elt_size);
      if (UPC_TYPE_HAS_THREADS_FACTOR (type))
        {
	   /* In the worst case, round up the local contribution
	      to the next multiple of the block factor */
	   n_local_elts = size_binop (MULT_EXPR,
	              size_binop (CEIL_DIV_EXPR, n_elts, block_factor),
		      block_factor);
	}
      else
        {
	  n_threads = convert (bitsizetype, upc_num_threads ());
	  n_full_blocks = size_binop (FLOOR_DIV_EXPR, n_elts, block_factor);
	  n_full_blocks_per_thread = size_binop (FLOOR_DIV_EXPR,
				       n_full_blocks, n_threads);
	  n_elts_in_full_blocks    = size_binop (MULT_EXPR,
	    size_binop (MULT_EXPR, n_full_blocks_per_thread, n_threads),
	    block_factor);
	  n_rem_elts = size_binop (MINUS_EXPR, n_elts,
	                           n_elts_in_full_blocks);
	  n_local_elts = size_binop (PLUS_EXPR,
			    size_binop (MULT_EXPR,
			       n_full_blocks_per_thread, block_factor),
			    size_binop (MIN_EXPR, n_rem_elts,
			       block_factor));
	}
      local_size = size_binop (MULT_EXPR, n_local_elts, elt_size);
    }

  /* Convert local size into bytes, and return result. */

  local_size = convert (sizetype, local_size);
  local_size = size_binop (CEIL_DIV_EXPR, local_size,
                           size_int (BITS_PER_UNIT));
  return local_size;
}

/* Return the result of upc_localsizeof applied to EXPR.  */

struct c_expr
upc_localsizeof_expr (location_t loc, struct c_expr expr)
{
  struct c_expr ret;
  if (expr.value == error_mark_node)
    {
      ret.value = error_mark_node;
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (false);
    }
  else
    {
      ret.value = upc_localsizeof (loc, TREE_TYPE (expr.value));
      ret.original_code = ERROR_MARK;
      ret.original_type = NULL_TREE;
      pop_maybe_used (C_TYPE_VARIABLE_SIZE (TREE_TYPE (expr.value)));
    }
  return ret;
}

/* Return the result of upc_localsizeof applied to T, a structure
   for the type name passed to sizeof (rather than the type itself).  */

struct c_expr
upc_localsizeof_type (location_t loc, struct c_type_name *t)
{
  tree type;
  struct c_expr ret;
  type = groktypename (t, NULL, NULL);
  ret.value = upc_localsizeof (loc, type);
  ret.original_code = ERROR_MARK;
  ret.original_type = NULL_TREE;
  pop_maybe_used (C_TYPE_VARIABLE_SIZE (type));
  return ret;
}

/****** UPC tree-related checks, and operations **************/

/* Traverse the expression and return the number of times
   THREADS is referenced.  */

static
int
recursive_count_upc_threads_refs (tree expr)
{
  enum tree_code code;
  int i;
  int count = 0;
  if (expr == NULL_TREE)
    return 0;
  code = TREE_CODE (expr);
  switch (TREE_CODE_CLASS (code))
    {
    case tcc_unary:
    case tcc_binary:
    case tcc_comparison:
    case tcc_expression:
    case tcc_reference:
    case tcc_statement:
      for (i = 0; i < TREE_CODE_LENGTH (code); i++)
	count += recursive_count_upc_threads_refs (TREE_OPERAND (expr, i));
      break;
    case tcc_declaration:
      if (expr == lookup_name (get_identifier ("THREADS")))
	count = 1;
      break;
    default:
      break;
    }
  return count;
}

/* Count the number of references to THREADS inside `expr'. */

int
count_upc_threads_refs (tree expr)
{
  return recursive_count_upc_threads_refs (expr);
}

/* Test that `expr' an expression tree where THREADS appears on
   the left or the right hand side of a multply, in a series
   of zero or more mulitplies.  For proper operation, the caller
   should ensure that THREADS is referenced only once,
   by calling count_upc_threads_refs () prior to calling this routine. */

int
is_multiple_of_upc_threads (tree expr)
{
  enum tree_code code;
  if (expr == NULL_TREE)
    return 0;
  if (expr == lookup_name (get_identifier ("THREADS")))
    return 1;
  code = TREE_CODE (expr);
  if (code == MULT_EXPR)
    return is_multiple_of_upc_threads (TREE_OPERAND (expr, 0))
	   | is_multiple_of_upc_threads (TREE_OPERAND (expr, 1));
  if ((code == NOP_EXPR) || (code == NON_LVALUE_EXPR)
      || (code == CONVERT_EXPR))
    return is_multiple_of_upc_threads (TREE_OPERAND (expr, 0));
  return 0;
}

/* Find all references to THREADS and change them into the constant `1'.
   This is done so that fold () when applied to the dimension of a
   shared array will yield the local size of the array */

void
set_upc_threads_refs_to_one (tree *expr)
{
  enum tree_code code;
  int i;
  if (*expr == NULL_TREE)
    return;
  code = TREE_CODE (*expr);
  switch (TREE_CODE_CLASS (code))
    {
    case tcc_unary:
    case tcc_binary:
    case tcc_comparison:
    case tcc_expression:
    case tcc_reference:
    case tcc_statement:
      for (i = 0; i < TREE_CODE_LENGTH (code); i++)
	set_upc_threads_refs_to_one (&TREE_OPERAND (*expr, i));
      break;
    case tcc_declaration:
      if (*expr == lookup_name (get_identifier ("THREADS")))
	*expr = integer_one_node;
      break;
    default:
      break;
    }
  return;
}

/* Return the blocking factor of the UPC shared type, 'type'.
   If the blocking factor is NULL, then return default blocking
   factor of 1.  */

tree
upc_get_block_factor (const tree type)
{
  tree block_factor = size_one_node;
  const tree elt_type = strip_array_types (type);
  if (elt_type && (TREE_CODE (elt_type) != ERROR_MARK)
      && TYPE_BLOCK_FACTOR (elt_type))
    block_factor = TYPE_BLOCK_FACTOR (elt_type);
  return block_factor;
}

/* Given a UPC layout specifier, calculate the blocking factor
   add this into the type field of `type'.  Caller is responsible
   for checking that the block factor is being applied to a
   shared type.  Return the new, augmented type. */

tree
upc_set_block_factor (const enum tree_code decl_kind,
                      tree type,
                      tree layout_qualifier)
{
  tree block_factor = NULL_TREE;

  if (!type || (TREE_CODE (type) == ERROR_MARK))
    return error_mark_node;

  if (!layout_qualifier || (TREE_CODE (layout_qualifier) == ERROR_MARK))
    return type;

  if (TREE_CODE (type) == VOID_TYPE)
    {
      error ("layout qualifier cannot be applied to a void type");
      return type;
    }

  /* Layout qualifier is given as the subscript operand of an array ref. */

  if (TREE_CODE (layout_qualifier) != ARRAY_REF)
    abort ();
  layout_qualifier = TREE_OPERAND (layout_qualifier, 1);

  if (layout_qualifier == NULL_TREE)
    {
      /* layout qualifier is [], equivalent to specifying [0] */
      block_factor = size_zero_node;
    }
  else if ((TREE_CODE (layout_qualifier) == INDIRECT_REF)
           && ((TREE_OPERAND (layout_qualifier, 0)) == NULL_TREE))
    {
      tree elt_size, elt_type, n_threads;
      /* layout qualifier is [*] */
      if (!COMPLETE_TYPE_P (type))
        {
          error ("layout qualifier of the form [*] cannot be applied to an incomplete type");
          return type;
        }
      if (decl_kind == POINTER_TYPE)
        {
          error ("[*] qualifier may not be used in declaration of pointers");
          return type;
        }
      /* The blocking factor is given by this expression:
         ( sizeof(a) / upc_elemsizeof(a) + THREADS - 1 ) / THREADS,
         where 'a' is the array being distributed. */
      elt_type = strip_array_types (type);
      elt_size = TYPE_SIZE (elt_type);
      if (UPC_TYPE_HAS_THREADS_FACTOR (type))
        block_factor = size_binop (FLOOR_DIV_EXPR, TYPE_SIZE (type), elt_size);
      else
        {
          n_threads = convert (bitsizetype, upc_num_threads ());
          block_factor = size_binop (CEIL_DIV_EXPR,
                         size_binop (FLOOR_DIV_EXPR, TYPE_SIZE (type), elt_size),
	    	         n_threads);
        }
    }
  else
    {
      STRIP_NOPS (layout_qualifier);
      if (TREE_CODE (layout_qualifier) != INTEGER_CST)
        error ("layout qualifier is not an integral constant");
      else
        block_factor = fold (layout_qualifier);
    }

  if (!block_factor)
    return type;

  /* If the blocking factor is 1, leave TYPE_BLOCK_FACTOR()
     as NULL to normalize the representation. The UPC spec
     says all objects have a blocking factor of 1 if none
     is specified, so we always represent a blocking factor
     of 1 as a NULL. */

  if (tree_int_cst_equal (block_factor, size_one_node))
    return type;

  if (tree_int_cst_compare (block_factor, UPC_MAX_BLOCK_SIZE_CSTU) > 0)
    {
      error ("Maximum block size in this implementation is %s",
	        UPC_MAX_BLOCK_SIZE_STRING);
      return type;
    }

  gcc_assert (TREE_CODE (block_factor) == INTEGER_CST);

  /* UPC TODO: Eliminate duplicate variant type copies.  */
  block_factor = convert (sizetype, block_factor);
  type = build_variant_type_copy (type);
  if (TREE_CODE (type) == ARRAY_TYPE)
    {
      tree last = type;
      tree inner;
      while (TREE_CODE (TREE_TYPE (last)) == ARRAY_TYPE)
	last = TREE_TYPE (last);
      inner = build_variant_type_copy (TREE_TYPE (last));
      /* Push the blocking factor down to the array
         element type.  */
      TYPE_BLOCK_FACTOR (inner) = block_factor;
      TREE_TYPE (last) = inner;
    }
  else
    {
      TYPE_BLOCK_FACTOR (type) = block_factor;
    }
  return type;
}

/* Return TRUE if TYPE is a shared type.  For arrays, we need
   to query its element type, because array types are never
   qualified.  */

int
upc_shared_type_p (tree type)
{
  tree elt_type;
  if (!type || TREE_CODE (type) == ERROR_MARK)
    return 0;
  elt_type = strip_array_types (type);
  return TYPE_SHARED (elt_type);
}

void
upc_check_decl (tree decl)
{
  /* If this is a shared variable, make sure that it ends up
     in the executable file.  */
  if (decl
      && TREE_CODE (decl) == VAR_DECL
      && TREE_TYPE (decl)
      && upc_shared_type_p (TREE_TYPE (decl)))
    {
      TREE_USED (decl) = 1;
      TREE_ADDRESSABLE (decl) = 1;
      TREE_STATIC (decl) = 1;
      /* Work-around a problem where the front-end doesn't
	 properly process the used flags set above, on
	 static variables when flag_unit_at_a_time isn't set. */
      if ((TREE_STATIC (decl) && !DECL_EXTERNAL (decl))
	  && !flag_unit_at_a_time
	  && !lookup_attribute ("used", DECL_ATTRIBUTES (decl)))
        {
	   tree used_id = get_identifier ("used");
           tree used_attrib = tree_cons (used_id, NULL_TREE, NULL_TREE);
           decl_attributes (&decl, used_attrib, 0);
	}
    }
}

/* Return 1 if type TYPE contains any pointers-to-shared.  */

static int
contains_pts_refs_p (tree type)
{
  switch (TREE_CODE (type))
    {
    case POINTER_TYPE:
    case REFERENCE_TYPE:
      return upc_shared_type_p (TREE_TYPE (type));

    case RECORD_TYPE:
    case UNION_TYPE:
    case QUAL_UNION_TYPE:
      {
	tree fields;
	/* For a type that has fields, see if the fields have pointers.  */
	for (fields = TYPE_FIELDS (type); fields; fields = TREE_CHAIN (fields))
	  if (TREE_CODE (fields) == FIELD_DECL
	      && contains_pts_refs_p (TREE_TYPE (fields)))
	    return 1;
	return 0;
      }

    case ARRAY_TYPE:
      /* An array type contains pointers if its element type does.  */
      return contains_pts_refs_p (TREE_TYPE (type));

    default:
      return 0;
    }
}

int
upc_check_decl_init (tree decl, tree init)
{
  tree init_type;
  int is_shared_var_decl_init;
  int is_decl_init_with_shared_addr_refs;
  int is_upc_decl;
  if (!(decl && init && TREE_TYPE (decl) && TREE_TYPE (init)))
    return 0;
  if ((TREE_CODE (decl) == ERROR_MARK)
       || (TREE_CODE (TREE_TYPE (decl)) == ERROR_MARK)
       || (TREE_CODE (init) == ERROR_MARK)
       || (TREE_CODE (TREE_TYPE (init)) == ERROR_MARK))
    return 0;
  init_type = TREE_TYPE (init);
  is_shared_var_decl_init = (TREE_CODE (decl) == VAR_DECL)
                             && TREE_TYPE (decl)
		             && upc_shared_type_p (TREE_TYPE (decl));
  is_decl_init_with_shared_addr_refs = TREE_STATIC (decl)
			     && contains_pts_refs_p (init_type);
  is_upc_decl = (is_shared_var_decl_init
                 || is_decl_init_with_shared_addr_refs);
  return is_upc_decl;
}

tree
upc_get_unshared_type (tree type)
{
  tree u_type = type;
  if (TREE_CODE (type) == ARRAY_TYPE)
    {
      const tree elem_type = TREE_TYPE(type);
      const tree u_elem_type = upc_get_unshared_type (elem_type);
      if (u_elem_type != elem_type)
        {
          for (u_type = TYPE_MAIN_VARIANT (type);
               u_type && TREE_TYPE(u_type) != u_elem_type;
               u_type = TYPE_NEXT_VARIANT (u_type)) /* loop */;
          if (!u_type)
            {
              u_type = build_variant_type_copy (type);
              TREE_TYPE (u_type) = u_elem_type;
            }
        }
    }
  else
    {
      const int quals = TYPE_QUALS (type);
      const int u_quals = quals & ~(TYPE_QUAL_SHARED
                                    | TYPE_QUAL_RELAXED
                                    | TYPE_QUAL_STRICT);
      u_type = build_qualified_type (type, u_quals);
    }
  return u_type;
}

void
upc_decl_init (tree decl, tree init)
{
  tree cur_stmt_list_save, init_stmt;
  if (!upc_init_stmt_list)
    upc_init_stmt_list = alloc_stmt_list ();
  cur_stmt_list_save = cur_stmt_list;
  cur_stmt_list = upc_init_stmt_list;
  init_stmt = build2 (INIT_EXPR, void_type_node, decl, init);
  add_stmt (init_stmt);
  cur_stmt_list = cur_stmt_list_save;
}

/* Return TRUE if DECL's size is zero, and DECL is a shared array. */

static
int
upc_lang_layout_decl_p (tree decl, tree type)
{
  int need_to_size_shared_array_decl = 0;
  tree t = type;

  if (decl && DECL_SIZE (decl) == 0)
    {
      while (t != NULL && TREE_CODE (t) == ARRAY_TYPE
	     && TREE_CODE (TREE_TYPE (t)) == ARRAY_TYPE)
	t = TREE_TYPE(t);

      need_to_size_shared_array_decl = t && TREE_CODE (t) == ARRAY_TYPE
					 && upc_shared_type_p (TREE_TYPE (t));
    }

  return need_to_size_shared_array_decl;
}


/* Shared variables are given their own link section on
   most target platforms, and if compiling in pthreads mode
   regular local file scope variables are made thread local. */

void
upc_set_decl_section (tree decl)
{
  if (TREE_SHARED (decl))
    {
#ifdef UPC_SHARED_SECTION_NAME
      /* shared variables are placed in their own shared section */
      int slen = strlen (UPC_SHARED_SECTION_NAME);
      DECL_SECTION_NAME (decl) =
	 build_string (slen, UPC_SHARED_SECTION_NAME);
#endif
    }
  else if (flag_upc_pthreads
           && ((TREE_STATIC (decl) && (DECL_SECTION_NAME (decl) == NULL_TREE))
	       || DECL_EXTERNAL (decl)))
    {
      /* If we're compiling with -fupc-pthreads asserted
	 and this is a static scoped object which
	 is either declared in a system header file,
	 or is being compiled in a UPC setting,
	 then assign the object to the
	 thread local storage (TLS) section. */
      extern int c_header_level; /* in c-lex.c */
      if (compiling_upc && (c_header_level <= 0))
	{
	  if (upc_pthreads_model == upc_pthreads_tls_model)
	    {
              DECL_TLS_MODEL (decl) = decl_default_tls_model (decl);
	      DECL_COMMON (decl) = 0;
	    }
	  else
	    /* Only the TLS model is currently implemented. */
	    abort ();
	}
    }
}

/* Given that TYPE describes a shared array, and that DECL's size hasn't
   been calculated, size the type and adust the size attributes in DECL. */

static
void
upc_lang_layout_decl (tree decl, tree type)
{
  tree t = type;

  while (TREE_CODE (t) == ARRAY_TYPE
	 && TREE_CODE (TREE_TYPE (t)) == ARRAY_TYPE)
    t = TREE_TYPE(t);

  if (TREE_CODE (t) == ARRAY_TYPE && upc_shared_type_p (TREE_TYPE (t)))
    {
      tree elt_type = TREE_TYPE (t);
      tree elt_size = TYPE_SIZE (elt_type);
      tree block_factor = TYPE_BLOCK_FACTOR (elt_type)
               ? convert (bitsizetype, TYPE_BLOCK_FACTOR (elt_type)) : NULL;
      tree t_size = TYPE_SIZE (type);
      tree n_elem = size_binop (FLOOR_DIV_EXPR, t_size, elt_size);
      tree n_threads = convert (bitsizetype, upc_num_threads ());
      if (UPC_TYPE_HAS_THREADS_FACTOR (type))
	{
	  if (block_factor)
	    if (!integer_zerop (block_factor))
	      {
		tree t1, t2;
		block_factor = convert (bitsizetype, block_factor);
		t1 = size_binop (CEIL_DIV_EXPR, n_elem, block_factor);
		t2 = size_binop (MULT_EXPR, t1, block_factor);
		DECL_SIZE (decl) = size_binop (MULT_EXPR, t2, elt_size);
	      }
	    else
	      DECL_SIZE (decl) = size_binop (MULT_EXPR, elt_size,
					     n_threads);
	  else
	    DECL_SIZE (decl) = t_size;
	}
      else if (!(block_factor && integer_zerop (block_factor)))
	{
	  /* We want to allocate ceiling of n_elem/n_threads elements per
	     thread, where n_elem is the total number of elements in
	     the array.  If the array is blocked, then we allocate
	     ((ceiling of (ceiling of n_elem/block_factor)/n_threads) *
	     block_factor) elements per thread. */
	  tree n_elem_per_thread;
	  if (block_factor)
	    {
	      tree block_count, blocks_per_thread;
	      block_count = size_binop (CEIL_DIV_EXPR,
					n_elem, block_factor);
	      blocks_per_thread = size_binop (CEIL_DIV_EXPR,
					block_count, n_threads);
	      n_elem_per_thread = size_binop (MULT_EXPR,
					blocks_per_thread, block_factor);
	    }
	  else
	    n_elem_per_thread = size_binop (CEIL_DIV_EXPR,
	                                n_elem, n_threads);
	  
	  /* In the special case of an array of size 1, we know that
	     we want a constant size no matter what n_threads is.  Make
	     the size a constant so that declarations of shared int x[1]
	     will work for runtime specification of threads. */
	  if (integer_onep (n_elem))
	    DECL_SIZE (decl) = elt_size;
	  else
	    DECL_SIZE (decl) = size_binop (MULT_EXPR, n_elem_per_thread,
					   elt_size);
	}
      else
	/* We want to allocate the entire array on one thread. */
	DECL_SIZE (decl) = TYPE_SIZE (type);
    }
  else
    DECL_SIZE (decl) = TYPE_SIZE (type);

  DECL_SIZE_UNIT (decl)
      = convert (sizetype, size_binop (CEIL_DIV_EXPR, DECL_SIZE (decl),
				       bitsize_unit_node));
}

/* Implement UPC's upc_forall `affinity' test, by augmenting the for statement's
   for_body by rewriting it into:
     if (affinity == MYTHREAD) for_body; */

tree
upc_affinity_test (location_t loc, tree for_body, tree affinity)
{
  tree mythread;
  tree affinity_test;
  tree if_stmt;
  
  if (affinity == NULL_TREE || for_body == NULL_TREE)
    return for_body;

  /* Make sure that the identifier "MYTHREAD" will be listed in the
     assembly file as .extern */

  mythread = lookup_name (get_identifier ("MYTHREAD"));
  assemble_external (mythread);
  TREE_USED (mythread) = 1;
  
  if (TREE_CODE (TREE_TYPE (affinity)) == POINTER_TYPE
      && upc_shared_type_p (TREE_TYPE (TREE_TYPE (affinity))))
    {
	/* then we have a pointer to a shared object and the affinity is
	   determined by the thread component of the address */
        const tree pts_rep = build1 (VIEW_CONVERT_EXPR, upc_pts_rep_type_node,
	                             save_expr (affinity));
	affinity = upc_pts_build_threadof (loc, pts_rep);
    }
  else if (TREE_CODE (TREE_TYPE (affinity)) == INTEGER_TYPE)
    {
      tree n_threads = upc_num_threads ();
      affinity = build_binary_op (loc, FLOOR_MOD_EXPR, affinity, n_threads, 0);
    }
  else
    {
      error ("Affinity expression is neither an integer nor the address of a shared object");
      return for_body;
    }
  
  /* Affinity now contains an integer value that can be compared to MY_THREAD.
     Create an expression that tests if affinity is equal to MY_THREAD. */

  if (!c_types_compatible_p (TREE_TYPE (affinity), TREE_TYPE (mythread)))
    affinity = convert (TREE_TYPE (mythread), affinity);
  affinity_test = c_objc_common_truthvalue_conversion (loc,
		    build_binary_op (loc, EQ_EXPR, affinity, mythread, 1));
  /* remove the MAYBE_CONST_EXPR's.  */
  affinity_test = c_fully_fold (affinity_test, false, NULL);

  /* generate the statement: if (<affinity>) <for_body>; */

  SET_EXPR_LOCATION (affinity_test, loc);
  if_stmt = build3 (COND_EXPR, void_type_node, affinity_test,
                    for_body, NULL_TREE);
  return if_stmt;
}

/*
 * upc_pts_cvt_op_p checks for conversion operations involving
 * shared pointers.  If either of the types involved
 * are shared pointers, return TRUE, else FALSE.
 */
int
upc_pts_cvt_op_p (tree exp)
{
  tree t1 = TREE_TYPE (exp);
  tree t2 = TREE_TYPE (TREE_OPERAND (exp, 0));
  int result = 0;
  if ((TREE_CODE (t1) == POINTER_TYPE)
       && (TREE_CODE (t2) == POINTER_TYPE))
    {
      tree tt1 = TREE_TYPE (t1);
      tree tt2 = TREE_TYPE (t2);
      if (upc_shared_type_p (tt1) || upc_shared_type_p (tt2))
        result = 1;
    }
  return result;
}

/* Check for the possible need to convert UPC-specific types.
   This routine must return 0, if it isn't absolutely certain
   that the types are equivalent.  */

int
upc_types_compatible_p (tree x, tree y)
{
  /* If C doesn't think they're compatible neither does UPC.  */
  if (!c_types_compatible_p (x, y))
    return 0;
  if (POINTER_TYPE_P (x) && POINTER_TYPE_P (y))
    {
      const tree ttx = TREE_TYPE (x);
      const tree tty = TREE_TYPE (y);
      if (upc_shared_type_p (ttx) && upc_shared_type_p (tty))
	{
	  tree bx, by, sx, sy;
	  int x_has_zero_phase, y_has_zero_phase;
	  int result;
          /* If both types are generic pointer-to-shared,
	     then they're compatible.  */
	  if (VOID_TYPE_P (ttx) && VOID_TYPE_P (tty))
	    return 1;
          /* Intermediate conversions to (shared void *) cannot
	     always be optimized away.  For example,
	     p1 = (shared void *)p2;
	     preserves the phase of p2, when assigning to p1.
	     We need to be conservative, and not consider conversions
	     with generic pointer-to-shared to be equivalent.  */
	  if (VOID_TYPE_P (ttx) != VOID_TYPE_P (tty))
	    return 0;
	  bx = upc_get_block_factor (ttx);
	  by = upc_get_block_factor (tty);
	  sx = TYPE_SIZE (ttx);
	  sy = TYPE_SIZE (tty);
	  x_has_zero_phase = (integer_zerop (bx) || integer_onep (bx));
	  y_has_zero_phase = (integer_zerop (by) || integer_onep (by));
	  /* Normalize type size so that 0 => NULL. */
	  if (sx && integer_zerop (sx)) sx = NULL_TREE;
	  if (sy && integer_zerop (sy)) sy = NULL_TREE;
	  /* If the target types have the same blocksize
	     (or they both have a phase value of zero) 
	     and the same size and the target types are
	     otherwise compatible, then the pointer-to-shared
	     types are compatible. */
	  result = (tree_int_cst_equal (bx, by)
	            || (x_has_zero_phase && y_has_zero_phase))
	           && tree_int_cst_equal (sx, sy);
	  return result;
	}
      /* If one is shared, and the other is local,
         then they aren't equivalent.  */
      else if (upc_shared_type_p (ttx) != upc_shared_type_p (tty))
	    return 0;
    }
  else if (upc_shared_type_p (x) || upc_shared_type_p (y))
    {
      /* In UPC, blocking factors can be applied to
         non-pointer objects/types. They're compatible
	 if the block sizes are equal.  */
      const tree bx = upc_get_block_factor (x);
      const tree by = upc_get_block_factor (y);
      return tree_int_cst_equal (bx, by)
             && c_types_compatible_p (TYPE_MAIN_VARIANT (x),
	                              TYPE_MAIN_VARIANT (y));
    }
  /* C thinks they're compatible, and there are no special
     UPC exceptions.  */
  return 1;
}

/************* UPC SUPPORT *************/

tree
upc_num_threads (void)
{
  tree n;
  n = flag_upc_threads ? size_int (flag_upc_threads)
		  : lookup_name (get_identifier ("THREADS"));
  if (!n)
    {
      error ("THREADS is undefined."
             " When compiling preprocessd source,"
	     " all -fupc-* switches must be passed on the command line,"
	     " asserting the same values as supplied when the"
	     " original source file was preprocessed");
      abort ();
    }

  return n;
}

static
hashval_t
uid_tree_map_hash (const void *p)
{
  const struct uid_tree_map * const map = (const struct uid_tree_map *) p;
  return map->uid;
}

static
int
uid_tree_map_eq (const void *va, const void *vb)
{
  const struct uid_tree_map * const a = (const struct uid_tree_map *) va;
  const struct uid_tree_map * const b = (const struct uid_tree_map *) vb; 
  return a->uid == b->uid;
}

static
tree
lookup_unshared_var (const tree var)
{
  const struct uid_tree_map *h;
  struct uid_tree_map in;
  unsigned int uid;
  gcc_assert (var && TREE_CODE (var) == VAR_DECL);
  uid = DECL_UID (var);
  in.uid = uid;
  in.to = NULL_TREE;
  h = (struct uid_tree_map *) htab_find_with_hash (unshared_vars, &in, uid);
  return h ? h->to : NULL_TREE;
}

#define UNSHARE_PREFIX "_u_"

static
tree
unshared_var_name (const tree var)
{
  const tree name = DECL_NAME (var);
  const size_t len = IDENTIFIER_LENGTH (name);
  char *tmp_name = (char *) alloca (len + sizeof (UNSHARE_PREFIX));
  strcpy (tmp_name, UNSHARE_PREFIX);
  strcat (tmp_name, IDENTIFIER_POINTER (name));
  return get_identifier (tmp_name);
}

static
tree
create_unshared_var (location_t loc, const tree var)
{
  tree u_name, u_type, u;
  gcc_assert (var && TREE_CODE (var) == VAR_DECL);
  u_name = unshared_var_name (var);
  u_type = TYPE_MAIN_VARIANT (TREE_TYPE (var));
  u = build_decl (loc, VAR_DECL, u_name, u_type);
  TREE_USED (u) = 1;
  TREE_ADDRESSABLE (u) = 1;
  TREE_PUBLIC (u) = TREE_PUBLIC (var);
  TREE_STATIC (u) = TREE_STATIC(var);
  DECL_ARTIFICIAL (u) = 1;
  DECL_IGNORED_P (u) = 1;
  DECL_EXTERNAL (u) = DECL_EXTERNAL (var);
  DECL_SECTION_NAME (u) = DECL_SECTION_NAME (var);
  DECL_CONTEXT (u) = DECL_CONTEXT (var);
  /* Alias the unshared variable to the shared variable.  */
  SET_DECL_ASSEMBLER_NAME (u, DECL_ASSEMBLER_NAME (var));
  /* Make sure the variable is referenced.  */
  mark_decl_referenced (var);
  return u;
}

static
void
map_unshared_var (const tree var, const tree u_var)
{
  struct uid_tree_map *h;
  unsigned int uid;
  void **loc;
  gcc_assert (var && TREE_CODE (var) == VAR_DECL);
  gcc_assert (u_var && TREE_CODE (u_var) == VAR_DECL);
  uid = DECL_UID (var);
  h = ggc_alloc_uid_tree_map ();
  h->uid = uid;
  h->to = u_var;
  loc = htab_find_slot_with_hash (unshared_vars, h, uid, INSERT);
  *(struct uid_tree_map **) loc = h;
}

static
tree
unshared_var_addr (location_t loc, const tree var)
{
  tree unshared_var, addr;
  unshared_var = lookup_unshared_var (var);
  if (!unshared_var)
    {
      unshared_var = create_unshared_var (loc, var);
      map_unshared_var (var, unshared_var);
    }
  addr = build_fold_addr_expr (unshared_var);
  TREE_CONSTANT (addr) = 1;
  TREE_READONLY (addr) = 1;
  return addr;
}

/* Convert shared variable reference VAR into a shared pointer
   value of the form {0, 0, &VAR} */

tree
upc_build_shared_var_addr (location_t loc, tree type, tree var)
{
  tree var_addr, val;
  gcc_assert (TREE_CODE (var) == VAR_DECL && TREE_SHARED (var));
  gcc_assert (TREE_CODE (type) == POINTER_TYPE
              && upc_shared_type_p (TREE_TYPE (type)));
  /* Refer to a shadow variable that has the same type as VAR, but
     with the shared qualifier removed.  */
  var_addr = unshared_var_addr (loc, var);
#ifdef UPC_PTS_PACKED_REP
  {
    const tree char_ptr_type = build_pointer_type (char_type_node);
    tree shared_vaddr_base;
    /* Subtract off the shared section base address so that the
       resulting quantity will fit into the vaddr field.  */
    shared_vaddr_base = identifier_global_value (
                          get_identifier ("__upc_shared_start"));
    if (!shared_vaddr_base)
      shared_vaddr_base = identifier_global_value (
                            get_identifier ("UPCRL_shared_begin"));
    if (!shared_vaddr_base)
      fatal_error ("UPC shared section start address not found."
                   " Cannot find a definition for either"
		   " __upc_shared_start or UPCRL_shared_begin.");
    assemble_external (shared_vaddr_base);
    TREE_USED (shared_vaddr_base) = 1;
    shared_vaddr_base = build1 (ADDR_EXPR, char_ptr_type, shared_vaddr_base);
    var_addr  = build_binary_op (loc, MINUS_EXPR,
                           convert (ptrdiff_type_node, var_addr),
			   convert (ptrdiff_type_node, shared_vaddr_base), 0);
  }
#endif
  val = upc_pts_build_value (loc, type,
                             var_addr, integer_zero_node, integer_zero_node);
  return val;
}

/* Expand the pre/post increment/decrement of shared pointer
   into its equivalent expression tree. */

tree
upc_pts_increment (location_t location ATTRIBUTE_UNUSED,
                   enum tree_code code, tree arg)
{
  /* The result type is a pointer of the same type as the argument
     type after dropping the shared qualifier (for PTS's that happen
     to live in shared memory). */
  tree stable_arg = stabilize_reference (arg);
  tree val = (code == PREINCREMENT_EXPR || code == PREDECREMENT_EXPR)
	      ? stable_arg : save_expr (stable_arg);
  enum tree_code incr_op = (code == PREINCREMENT_EXPR
			    || code == POSTINCREMENT_EXPR)
		            ? PLUS_EXPR : MINUS_EXPR;
  tree incr_val, result;
  incr_val = upc_pts_int_sum (location, incr_op, val, integer_one_node);
  TREE_SIDE_EFFECTS (incr_val) = 1;
  result = build_modify_expr (location, arg, NULL_TREE, NOP_EXPR,
                              location, incr_val, NULL_TREE);
  if (code == POSTINCREMENT_EXPR || code == POSTDECREMENT_EXPR)
    result = build2 (COMPOUND_EXPR, TREE_TYPE (incr_val), result, val);
  return result;
}

tree
upc_pts_int_sum (location_t location ATTRIBUTE_UNUSED,
                 enum tree_code resultcode, tree ptrop, tree intop)
{
  /* The result type is a pointer of the same type that is being added,
     after dropping the shared qualifier (for PTS's that happen
     to live in shared memory). */
  tree ttype = TREE_TYPE (ptrop);
  int quals_minus_shared = TYPE_QUALS (ttype) & !TYPE_QUAL_SHARED;
  tree result_type = c_build_qualified_type (ttype, quals_minus_shared);
  tree result_targ_type = TREE_TYPE (result_type);
  tree base_type = strip_array_types (result_targ_type);
  tree result;

  /* We have a pointer to a shared object.  For pointers to
     simple objects, just build a "resultcode" tree with the intop and
     let upc_genricize() handle the arithmetic correctly.  For pointers to
     arrays, compute the number of elements represented by the intop
     and build a "resultcode" tree with the ptrop and that number. */
  if (result_targ_type != base_type)
    {
      gcc_assert (TREE_CODE (result_targ_type) == ARRAY_TYPE);
      if (TREE_CODE (TYPE_SIZE (result_targ_type)) == INTEGER_CST)
	{
	  tree elt_cnt;
	  tree n_threads = convert (sizetype, upc_num_threads ());
	  int size = TREE_INT_CST_LOW (TYPE_SIZE (result_targ_type));
	  int elt_size = TREE_INT_CST_LOW (TYPE_SIZE (base_type));
	  elt_cnt = size_int (size / elt_size);
	  if (UPC_TYPE_HAS_THREADS_FACTOR (result_targ_type))
	    elt_cnt = size_binop (MULT_EXPR, n_threads, elt_cnt);
	  intop = convert (sizetype, intop);
	  intop = size_binop (MULT_EXPR, intop, elt_cnt);
	}
      else
	{
	  tree elt_cnt;
	  tree size = TYPE_SIZE (result_targ_type);
	  tree elt_size = TYPE_SIZE (base_type);
	  elt_cnt = build2 (EXACT_DIV_EXPR, sizetype, size, elt_size);
	  intop = convert (sizetype, intop);
	  intop = build2 (MULT_EXPR, sizetype, intop, elt_cnt);
	}
    }
  gcc_assert (resultcode == PLUS_EXPR || resultcode == MINUS_EXPR);
  if (resultcode == MINUS_EXPR)
    intop = build1 (NEGATE_EXPR, TREE_TYPE (intop), intop);
  intop = fold (intop);
  intop = convert (sizetype, intop);
  result = build2 (POINTER_PLUS_EXPR, result_type, ptrop, intop);
  /* Althoough there may be some specific cases where the
     addition of a constant integer to a shared pointer can
     be calculated at compile-time, in the more general
     cases the calculation must be made at runtime, so
     we mark the resulting sum as non-constant.  This will
     avoid situations where the compiler attempts to convert
     things like &A[14] where A is a shared array into a
     compile-time constant. */
  TREE_CONSTANT (result) = 0;
  return result;
}

tree
upc_pts_diff (tree op0, tree op1)
{
  const tree target_type = TREE_TYPE (TREE_TYPE (op0));
  tree result;
  /* The two pointers must both point to shared objects.  */
  if ( (upc_shared_type_p (target_type)
	&& !upc_shared_type_p (TREE_TYPE (TREE_TYPE (op1))))
      || (upc_shared_type_p (TREE_TYPE (TREE_TYPE (op1)))
	  && !upc_shared_type_p (target_type)))
    {
      error ("Attempt to take the difference of shared and nonshared pointers");
      return error_mark_node;
    }
  result = build2 (MINUS_EXPR, ptrdiff_type_node, op0, op1);
  return result;
}

int
is_valid_pts_p (tree exp)
{
  tree type = TREE_TYPE (exp);
  return (TREE_CODE (type) == POINTER_TYPE)
         && upc_shared_type_p (TREE_TYPE (type));
}

static
void
upc_build_init_func (const tree stmt_list)
{
  tree init_func_id = get_identifier (UPC_INIT_DECLS_FUNC);
  struct c_declspecs *specs;
  struct c_typespec void_spec;
  struct c_declarator *init_func_decl;
  struct c_arg_info args;
  tree init_func, t_list;
  location_t loc = input_location;
  rtx init_func_symbol;
  int decl_ok;
  memset (&void_spec, '\0', sizeof (struct c_typespec));
  void_spec.kind = ctsk_typedef;
  void_spec.spec = lookup_name (get_identifier ("void"));
  specs = declspecs_add_type (loc, build_null_declspecs (), void_spec);
  init_func_decl = build_id_declarator (init_func_id);
  init_func_decl->id_loc = loc;
  memset (&args, '\0', sizeof (struct c_arg_info));
  args.types = tree_cons (NULL_TREE, void_type_node, NULL_TREE);
  init_func_decl = build_function_declarator (&args, init_func_decl);
  decl_ok = start_function (specs, init_func_decl, NULL_TREE);
  gcc_assert (decl_ok);
  store_parm_decls ();
  init_func = current_function_decl;
  DECL_SOURCE_LOCATION (current_function_decl) = loc;
  TREE_PUBLIC (current_function_decl) = 0;
  TREE_USED (current_function_decl) = 1;
  DECL_SECTION_NAME (current_function_decl) =
     build_string (strlen (UPC_INIT_SECTION_NAME), UPC_INIT_SECTION_NAME);
  /* Swap the statement list that we've built up,
     for the current statement list.  */
  t_list = c_begin_compound_stmt (true);
  TREE_CHAIN (stmt_list) = TREE_CHAIN (t_list);
  cur_stmt_list = stmt_list;
  free_stmt_list (t_list);
  t_list = c_end_compound_stmt (loc, stmt_list, true);
  add_stmt (t_list);
  finish_function ();
  gcc_assert (DECL_RTL (init_func));
  upc_init_array_section = get_section (UPC_INIT_ARRAY_SECTION_NAME,
				        0, NULL);
  mark_decl_referenced (init_func);
  init_func_symbol = XEXP (DECL_RTL (init_func), 0);
  assemble_addr_to_section (init_func_symbol, upc_init_array_section);
}

void
upc_finish_file (void)
{
  upc_write_init_func ();
  upc_free_unshared_var_table ();
}

void
upc_write_init_func (void)
{
  if (upc_init_stmt_list)
    {
      upc_build_init_func (upc_init_stmt_list);
      upc_init_stmt_list = NULL;
    }
}

void
upc_free_unshared_var_table (void)
{
  if (unshared_vars)
    {
      htab_delete (unshared_vars);
      unshared_vars = NULL;
    }
}

#include "gt-upc-upc-act.h"
