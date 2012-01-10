#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "langhooks.h"
#include "c-tree.h"
#include "c-lang.h"
#include "flags.h"
#include "output.h"
#include "intl.h"
#include "target.h"
#include "tree-iterator.h"
#include "bitmap.h"
#include "gimple.h"
#include "c-family/c-objc.h"

void replace_array_notations (tree *, tree *, tree *, int);
void find_rank (tree, int *);
void extract_array_notation_exprs (tree, tree **, int *);
tree fix_conditional_array_notations (tree);
struct c_expr fix_array_notation_expr (location_t, enum tree_code,
				       struct c_expr);
void
find_rank (tree array, int *rank)
{
  tree ii_tree;
  int current_rank = 0, ii = 0;
  
  if (!array)
    return;
  else if (TREE_CODE (array) == ARRAY_NOTATION_REF)
    {
      for (ii_tree = array;
	   ii_tree && TREE_CODE (ii_tree) == ARRAY_NOTATION_REF;
	   ii_tree = ARRAY_NOTATION_ARRAY (ii_tree))
	current_rank++;
      
      if (*rank != 0 && *rank != current_rank)
	error ("Rank Mismatch!");
      else if (*rank == 0)
	*rank = current_rank;
    }
  else
    {
      if (TREE_CODE (array) == CALL_EXPR
	  || TREE_CODE (array) == AGGR_INIT_EXPR)
	{
	  if (TREE_CODE (TREE_OPERAND (array, 0)) == INTEGER_CST)
	    {
	      int length = TREE_INT_CST_LOW (TREE_OPERAND (array, 0));
	      for (ii = 0; ii < length; ii++)
		find_rank (TREE_OPERAND (array, ii), rank);
	    }
	  else
	    gcc_unreachable ();
	}
      else
	{
	  for (ii = 0; ii < TREE_CODE_LENGTH (TREE_CODE (array)); ii++)
	    find_rank (TREE_OPERAND (array, ii), rank);
	}
    }
  return;
}

void
extract_array_notation_exprs (tree node, tree **array_list, int *list_size)
{
  int ii = 0;
  tree *new_array_list = NULL;
  if (!node)
    return;
  else if (TREE_CODE (node) == ARRAY_NOTATION_REF)
    {
      ii = *list_size;
      new_array_list =
	(tree *) xrealloc (*array_list, (ii + 1) * sizeof (tree));
      gcc_assert (new_array_list);
      new_array_list[ii] = node;
      ii++;
      *list_size = ii;
      *array_list = new_array_list;
      return;
    }
  else if (TREE_CODE (node) == STATEMENT_LIST)
    {
      tree_stmt_iterator ii_tsi;
      for (ii_tsi = tsi_start (node); !tsi_end_p (ii_tsi); tsi_next (&ii_tsi))
	extract_array_notation_exprs (*tsi_stmt_ptr (ii_tsi), array_list,
				      list_size);
    }
  else if (TREE_CODE (node) == CALL_EXPR || TREE_CODE (node) == AGGR_INIT_EXPR)
    {
      if (TREE_CODE (TREE_OPERAND (node, 0)) == INTEGER_CST)
	{
	  int length = TREE_INT_CST_LOW (TREE_OPERAND (node, 0));

	  for (ii = 0; ii < length; ii++)
	    extract_array_notation_exprs (TREE_OPERAND (node, ii), array_list,
					  list_size);
	}
      else
	gcc_unreachable  (); /* should not get here */
	  
    } 
  else
    {
      for (ii = 0; ii < TREE_CODE_LENGTH (TREE_CODE (node)); ii++)
	extract_array_notation_exprs (TREE_OPERAND (node, ii), array_list,
				      list_size);
    }
  return;
}

void
replace_array_notations (tree *orig, tree *list, tree *array_operand,
			 int array_size)
{
  int ii = 0;
  
  if (array_size == 0 || *list == NULL || !*orig)
    return;

  if (TREE_CODE (*orig) == ARRAY_NOTATION_REF)
    {
      for (ii = 0; ii < array_size; ii++)
	{
	  if (*orig == list[ii])
	    *orig = array_operand[ii];
	}
    }
  else if (TREE_CODE (*orig) == STATEMENT_LIST)
    {
      tree_stmt_iterator ii_tsi;
      for (ii_tsi = tsi_start (*orig); !tsi_end_p (ii_tsi); tsi_next (&ii_tsi))
	replace_array_notations (tsi_stmt_ptr (ii_tsi), list, array_operand,
				 array_size);
    }
  else if (TREE_CODE (*orig) == CALL_EXPR
	   || TREE_CODE (*orig) == AGGR_INIT_EXPR)
    {
      if (TREE_CODE (TREE_OPERAND (*orig, 0)) == INTEGER_CST)
	{
	  int length = TREE_INT_CST_LOW (TREE_OPERAND (*orig, 0));
	  for (ii = 0; ii < length; ii++)
	    replace_array_notations (&TREE_OPERAND (*orig, ii), list,
				     array_operand, array_size);
	}
      else
	gcc_unreachable (); /* should not get here! */
    }
  else
    {
      for (ii = 0; ii < TREE_CODE_LENGTH (TREE_CODE (*orig)); ii++)
	{
	  replace_array_notations (&TREE_OPERAND (*orig, ii), list,
				   array_operand, array_size);
	}
    }
  return;
}

tree
build_array_notation_expr (location_t location, tree lhs, tree lhs_origtype,
			   enum tree_code modifycode, location_t rhs_loc,
			   tree rhs, tree rhs_origtype)
{
  bool *lhs_vector = NULL, **rhs_vector = NULL;
  tree *lhs_array = NULL, **rhs_array = NULL;
  tree array_expr_lhs = NULL_TREE, array_expr_rhs = NULL_TREE;
  tree array_expr = NULL_TREE;
  tree *lhs_value = NULL, **rhs_value = NULL;
  tree *lhs_stride = NULL, *lhs_length = NULL, *lhs_start = NULL;
  tree **rhs_stride = NULL, **rhs_length = NULL, **rhs_start = NULL;
  tree loop = NULL_TREE, *lhs_var = NULL, *rhs_var = NULL;
  tree *body_label = NULL, *body_label_expr = NULL;
  tree *exit_label = NULL, *exit_label_expr = NULL, *cond_expr = NULL;
  tree *if_stmt_label = NULL;
  tree *lhs_expr_incr = NULL, *rhs_expr_incr = NULL;
  tree *lhs_ind_init = NULL, *rhs_ind_init = NULL;
  bool *lhs_count_down = NULL, **rhs_count_down = NULL;
  tree *lhs_compare = NULL, *rhs_compare = NULL, *rhs_array_operand = NULL;
  int lhs_rank = 0, rhs_rank = 0, ii = 0, jj = 0;
  tree ii_tree = NULL_TREE;
  tree *rhs_list = NULL;
  int rhs_list_size = 0;
  
  find_rank (lhs, &lhs_rank);
  find_rank (rhs, &rhs_rank);

  extract_array_notation_exprs (rhs, &rhs_list, &rhs_list_size);
  
  if (lhs_rank == 0 && rhs_rank != 0)
    {
      error_at (location, "Left Hand-side rank cannot be scalar when "
		"right-hand side is not");
      return error_mark_node;
    }
  if (lhs_rank != 0 && rhs_rank != 0 && lhs_rank != rhs_rank)
    {
      error_at (location, "Rank-mismatch");
      return error_mark_node;
    }
  
  lhs_vector = (bool *) xmalloc (sizeof (bool) * lhs_rank);
  rhs_vector = (bool **) xmalloc (sizeof (bool *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_vector[ii] = (bool *) xmalloc (sizeof (bool) * rhs_rank);
  
  lhs_array = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_array = (tree **) xmalloc (sizeof (tree *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_array[ii] = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_value = (tree *) xmalloc (sizeof (tree) * lhs_rank);

  rhs_value = (tree **) xmalloc (sizeof (tree *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_value[ii] = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_stride = (tree *) xmalloc (sizeof (tree) * lhs_rank);

  rhs_stride = (tree **) xmalloc (sizeof (tree *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_stride[ii] = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_length = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_length = (tree **) xmalloc (sizeof (tree *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_length[ii] = (tree *) xmalloc (sizeof (tree) * rhs_rank);
  

  lhs_start = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_start = (tree **) xmalloc (sizeof (tree *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_start[ii] = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_var = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_var = (tree *) xmalloc (sizeof (tree) * rhs_rank);
  

  /* The reason why we are just using lhs_rank for this is because we have the
   * following scenarios:
   * LHS_RANK == RHS_RANK
   * LHS_RANK != RHS_RANK && RHS_RANK = 0
   *
   * In both the scenarios, just checking the LHS_RANK is OK
   */
  body_label = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  body_label_expr = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  exit_label = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  exit_label_expr = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  cond_expr = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  if_stmt_label = (tree *) xmalloc (sizeof (tree) * lhs_rank);

  lhs_expr_incr = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_expr_incr = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_ind_init = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_ind_init = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  lhs_count_down = (bool *) xmalloc (sizeof (bool) * lhs_rank);
  rhs_count_down = (bool **) xmalloc (sizeof (bool *) * rhs_list_size);
  for (ii = 0; ii < rhs_list_size; ii++)
    rhs_count_down[ii] = (bool *) xmalloc (sizeof (bool) * rhs_rank);

  lhs_compare = (tree *) xmalloc (sizeof (tree) * lhs_rank);
  rhs_compare = (tree *) xmalloc (sizeof (tree) * rhs_rank);

  rhs_array_operand = (tree *) xmalloc (sizeof (tree) * rhs_list_size);

  ii = 0;
  for (ii_tree = lhs; ii_tree && TREE_CODE (ii_tree) == ARRAY_NOTATION_REF;
       ii_tree = ARRAY_NOTATION_ARRAY (ii_tree))
    {
      lhs_array[ii] = ii_tree;
      ii++;
    }

  if (rhs_rank)
    {
      for (ii = 0; ii < rhs_list_size; ii++)
	{ 
	  jj = 0; 
	  for (ii_tree = rhs_list[ii];
	       ii_tree && TREE_CODE (ii_tree) == ARRAY_NOTATION_REF;
	       ii_tree = ARRAY_NOTATION_ARRAY (ii_tree))
	    {
	      rhs_array[ii][jj] = ii_tree;
	      jj++;
	    }
	}
    }

  if (TREE_CODE (lhs) == ARRAY_NOTATION_REF)
    {
      for (ii = 0; ii < lhs_rank; ii++)
	{
	  if (TREE_CODE (lhs_array[ii]) == ARRAY_NOTATION_REF)
	    {
	      lhs_value[ii] = ARRAY_NOTATION_ARRAY (lhs_array[ii]);
	      lhs_start[ii] = ARRAY_NOTATION_START (lhs_array[ii]);
	      lhs_length[ii] = ARRAY_NOTATION_LENGTH (lhs_array[ii]);
	      lhs_stride[ii] = ARRAY_NOTATION_STRIDE (lhs_array[ii]);
	      lhs_vector[ii] = true;
	      /* IF the stride value is variable (i.e. not constant) then
	       * assume that the length is positive
	       */
	      if (!TREE_CONSTANT (lhs_length[ii]))
		lhs_count_down[ii] = false;
	      else if (tree_int_cst_lt
		       (lhs_length[ii],
			build_int_cst (TREE_TYPE (lhs_length[ii]), 0)))
		lhs_count_down[ii] = true;
	      else
		lhs_count_down[ii] = false;
	    }
	  else
	    lhs_vector[ii] = false;
	}
    }

  for (ii = 0; ii < rhs_list_size; ii++)
    {
      if (TREE_CODE (rhs_list[ii]) == ARRAY_NOTATION_REF)
	{
	  for (jj = 0; jj < rhs_rank; jj++)
	    {
	      if (TREE_CODE (rhs_array[ii][jj]) == ARRAY_NOTATION_REF)
		{
		  rhs_value[ii][jj]  = ARRAY_NOTATION_ARRAY (rhs_array[ii][jj]);
		  rhs_start[ii][jj]  = ARRAY_NOTATION_START (rhs_array[ii][jj]);
		  rhs_length[ii][jj] = ARRAY_NOTATION_LENGTH (rhs_array[ii][jj]);
		  rhs_stride[ii][jj] = ARRAY_NOTATION_STRIDE (rhs_array[ii][jj]);
		  rhs_vector[ii][jj] = true;
		  /* If the stride value is variable (i.e. not constant) then
		   * assume that the length is positive
		   */
		  if (!TREE_CONSTANT (rhs_length[ii][jj]))
		    rhs_count_down[ii][jj] = false;
		  else if (tree_int_cst_lt
			   (rhs_length[ii][jj],
			    build_int_cst (TREE_TYPE (rhs_length[ii][jj]), 0)))
		    rhs_count_down[ii][jj] = true;
		  else
		    rhs_count_down[ii][jj] = false;	
		}
	      else
		rhs_vector[ii][jj] = false;
	    }
	}
    }

  loop = push_stmt_list();

  for (ii = 0; ii < lhs_rank; ii++)
    {
      if (lhs_vector[ii])
	{
	  lhs_var[ii] = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE,
				    TREE_TYPE (lhs_start[ii]));
	  lhs_ind_init[ii] = build_modify_expr
	    (UNKNOWN_LOCATION, lhs_var[ii], TREE_TYPE (lhs_var[ii]),
	     modifycode,
	     UNKNOWN_LOCATION, build_int_cst (TREE_TYPE (lhs_start[ii]), 0),
	     TREE_TYPE (lhs_start[ii]));
	  
	}
    }

  for (ii = 0; ii < rhs_rank; ii++)
    {
      /* When we have a polynomial, we assume that the indices are of type
       * integer
       */
      rhs_var[ii] = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE,
				integer_type_node);
      rhs_ind_init[ii] = build_modify_expr
	(UNKNOWN_LOCATION, rhs_var[ii], TREE_TYPE (rhs_var[ii]),
	 modifycode,
	 UNKNOWN_LOCATION, build_int_cst (TREE_TYPE (rhs_var[ii]), 0),
	 TREE_TYPE (rhs_var[ii]));
    }
  

  for (ii = 0; ii < lhs_rank ; ii++)
    {
      /* this will create the if statement label */
      if_stmt_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				      void_type_node);
      DECL_CONTEXT (if_stmt_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (if_stmt_label[ii]) = 0;
      DECL_IGNORED_P (if_stmt_label[ii]) = 1;
  
      /* this label statment will point to the loop body */
      body_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (body_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (body_label[ii]) = 0;
      DECL_IGNORED_P (body_label[ii]) = 1;
      body_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, body_label[ii]);

      /* this will create the exit label..i.e. where the while loop will branch
	 out of
      */
      exit_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (exit_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (exit_label[ii]) = 0;
      DECL_IGNORED_P (exit_label[ii]) = 1;
      exit_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, exit_label[ii]);
    }

  if (lhs_rank)
    {
      /* The last ARRAY_NOTATION element's ARRAY component should be the array's
       * base value
       */
      array_expr_lhs = lhs_value[lhs_rank - 1];
      for (ii = lhs_rank - 1; ii >= 0; ii--)
	{
	  /* Array[start_index + (induction_var * stride)] */
	  array_expr_lhs = build_array_ref
	    (location, array_expr_lhs,
	     build2 (PLUS_EXPR, TREE_TYPE (lhs_var[ii]), lhs_start[ii],
		     build2 (MULT_EXPR, TREE_TYPE (lhs_var[ii]), lhs_var[ii],
			     lhs_stride[ii])));
	  if (lhs_count_down[ii])
	    lhs_expr_incr[ii] =
	      build2 (MODIFY_EXPR, void_type_node, lhs_var[ii],
		      build2 (PLUS_EXPR, TREE_TYPE (lhs_var[ii]), lhs_var[ii],
			      build_int_cst (TREE_TYPE (lhs_var[ii]), -1)));
	  else
	    lhs_expr_incr[ii] =
	      build2 (MODIFY_EXPR, void_type_node, lhs_var[ii],
		      build2 (PLUS_EXPR, TREE_TYPE (lhs_var[ii]), lhs_var[ii],
			      build_int_cst (TREE_TYPE (lhs_var[ii]), 1)));
	}
    }

  if (rhs_rank)
    {
      for (ii = 0; ii < rhs_list_size; ii++)
	{
	  if (rhs_vector[ii][0])
	    {
	      rhs_array_operand[ii] = rhs_value[ii][rhs_rank - 1];
	      gcc_assert (rhs_array_operand[ii]);
	      for (jj = rhs_rank - 1; jj >= 0; jj--)
		{
		  if (rhs_count_down[ii][jj])
		    {
		      /* Array[start_index - (induction_var * stride)] */
		      rhs_array_operand[ii] = build_array_ref
			(location, rhs_array_operand[ii],
			 build2 (MINUS_EXPR, TREE_TYPE (rhs_var[jj]),
				 rhs_start[ii][jj],
				 build2 (MULT_EXPR, TREE_TYPE (rhs_var[jj]),
					 rhs_var[jj],
					 rhs_stride[ii][jj])));
		    }
		  else
		    {
		      /* Array[start_index  + (induction_var * stride)] */
		      rhs_array_operand[ii] = build_array_ref
			(location, rhs_array_operand[ii],
			 build2 (PLUS_EXPR, TREE_TYPE (rhs_var[jj]),
				 rhs_start[ii][jj],
				 build2 (MULT_EXPR, TREE_TYPE (rhs_var[jj]),
					 rhs_var[jj],
					 rhs_stride[ii][jj])));
		    }
		}
	    }
	}
      replace_array_notations (&rhs, rhs_list, rhs_array_operand,
				 rhs_list_size);
	array_expr_rhs = rhs;
    }
  else
    {
      array_expr_rhs = rhs;
      rhs_expr_incr[0] = NULL_TREE;
    }

  for (ii = 0; ii < rhs_rank; ii++)
    {
      rhs_expr_incr[ii] = build2
	(MODIFY_EXPR, void_type_node, rhs_var[ii],
	 build2 (PLUS_EXPR, TREE_TYPE (rhs_var[ii]), rhs_var[ii],
		 build_int_cst (TREE_TYPE (rhs_var[ii]), 1)));
    }
      

  array_expr = build_modify_expr (location, array_expr_lhs,
				  lhs_origtype, modifycode, rhs_loc,
				  array_expr_rhs, rhs_origtype);

  for (jj = 0; jj < lhs_rank; jj++)
    {
      if (rhs_rank && rhs_expr_incr[jj])
	{
	  if (lhs_count_down[jj])
	    lhs_compare[jj] = build2
	      (GT_EXPR, boolean_type_node, lhs_var[jj], lhs_length[jj]);
	  
	  else
	    lhs_compare[jj] = build2
	      (LT_EXPR, boolean_type_node, lhs_var[jj], lhs_length[jj]);


	  /* What we are doing here is this:
	   * We always count up, so:
	   *    if (length is negative ==> which means we count down)
	   *       we multiply length by -1 and count up => ii < -LENGTH
	   *    else
	   *       we just count up, so we compare for  ii < LENGTH
	   */
	  if (rhs_count_down[0][jj])
	      rhs_compare[jj] = build2
		(LT_EXPR, boolean_type_node, rhs_var[jj],
		 build2 (MULT_EXPR, TREE_TYPE (rhs_var[jj]), rhs_length[0][jj],
			 build_int_cst (TREE_TYPE (rhs_var[jj]), -1)));
	  else
	    rhs_compare[jj] = build2 (LT_EXPR, boolean_type_node, rhs_var[jj],
				      rhs_length[0][jj]);
      
	  cond_expr[jj] = build2 (TRUTH_ANDIF_EXPR, void_type_node,
				  lhs_compare[jj], rhs_compare[jj]);
	}
      else
	{
	  if (lhs_count_down[jj])
	    cond_expr[jj] = build2
	      (GT_EXPR, boolean_type_node, lhs_var[jj], lhs_length[jj]);
	  else
	    cond_expr[jj] = build2
	      (LT_EXPR, boolean_type_node, lhs_var[jj], lhs_length[jj]);
	}
    }
  
  /* The following statements will do the following:
   * <if_stmt_label>: (in order from outermost to innermost)
   *                  if (cond_expr) then go to body_label
   *                  else                go to exit_label
   * <body_label>:
   *                  array expression
   *
   *                  (the increment, goto and exit_label goes from innermost to
   *                   outermost).
   *                  ii++ and jj++
   *                  go to if_stmt_label
   * <exit_label>:
   *                  <REST OF CODE>
   */

  
  for (ii = 0; ii < lhs_rank; ii++)
    {
      add_stmt (lhs_ind_init [ii]);
      if (rhs_rank)
	add_stmt (rhs_ind_init[ii]);
      add_stmt (build1 (LABEL_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (build3 (COND_EXPR, void_type_node, cond_expr[ii],
			build1 (GOTO_EXPR, void_type_node, body_label[ii]),
			build1 (GOTO_EXPR, void_type_node, exit_label[ii])));

      add_stmt (body_label_expr[ii]);
    }
  
  add_stmt (array_expr);

  for (ii = lhs_rank - 1; ii >= 0; ii--)
    {
      add_stmt (lhs_expr_incr[ii]);
      if (rhs_rank && rhs_expr_incr[ii])
	add_stmt (rhs_expr_incr[ii]);
  
      add_stmt (build1 (GOTO_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (exit_label_expr[ii]);
    }
   
  pop_stmt_list (loop);

  return loop;
}

static tree
fix_conditional_array_notations_1 (tree stmt)
{
  tree *array_list = NULL;
  int list_size = 0;
  tree cond = NULL;
  int rank = 0, ii = 0, jj = 0;
  tree **array_ops, *array_var, *array_operand, jj_tree, loop;
  tree **array_value, **array_stride, **array_length, **array_start;
  tree *body_label, *body_label_expr, *exit_label, *exit_label_expr;
  tree *compare_expr, *if_stmt_label, *expr_incr, *ind_init;
  bool **count_down, **array_vector;

  if (TREE_CODE (stmt) == COND_EXPR)
    cond = COND_EXPR_COND (stmt);
  else if (TREE_CODE (stmt) == SWITCH_EXPR)
    cond = SWITCH_COND (stmt);
  else if (TREE_CODE (stmt) == FOR_STMT || TREE_CODE (stmt) == CILK_FOR_STMT)
    cond = FOR_COND (stmt);
  else
    return stmt;

  find_rank (cond, &rank);
  if (rank == 0)
    return stmt;  
  
  extract_array_notation_exprs (cond, &array_list, &list_size);

  if (*array_list == NULL_TREE || list_size == 0)
    return stmt;

  array_ops = (tree **) xmalloc (sizeof (tree *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    array_ops[ii] = (tree *) xmalloc (sizeof (tree) * rank);
  
  array_vector = (bool **) xmalloc (sizeof (bool *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    array_vector[ii] = (bool *) xmalloc (sizeof (bool) * rank);

  array_value = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_stride = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_length = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_start = (tree **) xmalloc (sizeof (tree *) * list_size);

  for (ii = 0; ii < list_size; ii++)
    {
      array_value[ii]  = (tree *) xmalloc (sizeof (tree) * rank);
      array_stride[ii] = (tree *) xmalloc (sizeof (tree) * rank);
      array_length[ii] = (tree *) xmalloc (sizeof (tree) * rank);
      array_start[ii]  = (tree *) xmalloc (sizeof (tree) * rank);
    }

  body_label = (tree *) xmalloc(sizeof (tree) * rank);
  body_label_expr = (tree *) xmalloc (sizeof (tree) * rank);
  exit_label = (tree *) xmalloc (sizeof (tree) * rank);
  exit_label_expr = (tree *) xmalloc (sizeof (tree) * rank);
  compare_expr = (tree *) xmalloc (sizeof (tree) * rank);
  if_stmt_label = (tree *) xmalloc (sizeof (tree) * rank);
  
  expr_incr = (tree *) xmalloc (sizeof (tree) * rank);
  ind_init = (tree *) xmalloc (sizeof (tree) * rank);
  
  count_down = (bool **) xmalloc (sizeof (bool *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    count_down[ii] = (bool *) xmalloc (sizeof (bool) * rank);

  array_operand = (tree *) xmalloc (sizeof (tree) * list_size);
  
  array_var = (tree *) xmalloc (sizeof (tree) * rank);
  

  for (ii = 0; ii < list_size; ii++)
    {
      jj = 0;
      for (jj_tree = array_list[ii];
	   jj_tree && TREE_CODE (jj_tree) == ARRAY_NOTATION_REF;
	   jj_tree = ARRAY_NOTATION_ARRAY (jj_tree))
	{
	  array_ops[ii][jj] = jj_tree;
	  jj++;
	}
    }

  for (ii = 0; ii < list_size; ii++)
    {
      if (TREE_CODE (array_list[ii]) == ARRAY_NOTATION_REF)
	{
	  for (jj = 0; jj < rank; jj++)
	    {
	      if (TREE_CODE (array_ops[ii][jj]) == ARRAY_NOTATION_REF)
		{
		  array_value[ii][jj] =
		    ARRAY_NOTATION_ARRAY (array_ops[ii][jj]);
		  array_start[ii][jj] =
		    ARRAY_NOTATION_START (array_ops[ii][jj]);
		  array_length[ii][jj] =
		    ARRAY_NOTATION_LENGTH (array_ops[ii][jj]);
		  array_stride[ii][jj] =
		    ARRAY_NOTATION_STRIDE (array_ops[ii][jj]);
		  array_vector[ii][jj] = true;

		  if (!TREE_CONSTANT (array_length[ii][jj]))
		      count_down[ii][jj] = false;
		  else if (tree_int_cst_lt
			   (array_length[ii][jj],
			    build_int_cst (TREE_TYPE (array_length[ii][jj]),
					   0)))
		    count_down[ii][jj] = true;
		  else
		    count_down[ii][jj] = false;
		}
	      else
		array_vector[ii][jj] = false;
	    }
	}
    }

  loop = push_stmt_list();

  for (ii = 0; ii < rank; ii++)
    {
  
      array_var[ii] = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE,
				  integer_type_node);
      ind_init[ii] =
	build_modify_expr (UNKNOWN_LOCATION, array_var[ii],
			   TREE_TYPE (array_var[ii]), NOP_EXPR,
			   UNKNOWN_LOCATION,
			   build_int_cst (TREE_TYPE (array_var[ii]), 0),
			   TREE_TYPE (array_var[ii]));
	
    }

  for (ii = 0; ii < rank ; ii++)
    {
      /* this will create the if statement label */
      if_stmt_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				      void_type_node);
      DECL_CONTEXT (if_stmt_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (if_stmt_label[ii]) = 0;
      DECL_IGNORED_P (if_stmt_label[ii]) = 1;
  
      /* this label statment will point to the loop body */
      body_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (body_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (body_label[ii]) = 0;
      DECL_IGNORED_P (body_label[ii]) = 1;
      body_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, body_label[ii]);

      /* this will create the exit label..i.e. where the while loop will branch
	 out of
      */
      exit_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (exit_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (exit_label[ii]) = 0;
      DECL_IGNORED_P (exit_label[ii]) = 1;
      exit_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, exit_label[ii]);
    }

  for (ii = 0; ii < list_size; ii++)
    {
      if (array_vector[ii][0])
	{
	  array_operand[ii] = array_value[ii][rank - 1];
	  gcc_assert (array_operand[ii]);

	  for (jj = rank - 1; jj >= 0; jj--)
	    {
	      if (count_down[ii][jj])
		{
		  /* Array[start_index - (induction_var * stride)] */
		  array_operand[ii] = build_array_ref
		    (UNKNOWN_LOCATION, array_operand[ii],
		     build2 (MINUS_EXPR, TREE_TYPE (array_var[jj]),
			     array_start[ii][jj],
			     build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
				     array_var[jj], array_stride[ii][jj])));
		}
	      else
		{
		  /* Array[start_index + (induction_var * stride)] */
		  array_operand[ii] = build_array_ref
		    (UNKNOWN_LOCATION, array_operand[ii],
		     build2 (PLUS_EXPR, TREE_TYPE (array_var[jj]),
			     array_start[ii][jj],
			     build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
				     array_var[jj], array_stride[ii][jj])));
		}
	    }
	}
    }
  replace_array_notations (&stmt, array_list, array_operand, list_size);

  for (ii = 0; ii < rank; ii++)
    {
      expr_incr[ii] =
	build2 (MODIFY_EXPR, void_type_node, array_var[ii],
		build2 (PLUS_EXPR, TREE_TYPE (array_var[ii]), array_var[ii],
			build_int_cst (TREE_TYPE (array_var[ii]), 1)));
    }
  
  for (jj = 0; jj < rank; jj++)
    {
      if (rank && expr_incr[jj])
	{
	  if (count_down[0][jj])
	    compare_expr[jj] =
	      build2 (LT_EXPR, boolean_type_node, array_var[jj],
		      build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
			      array_length[0][jj],
			      build_int_cst (TREE_TYPE (array_var[jj]), -1)));
	  else
	    compare_expr[jj] = build2 (LT_EXPR, boolean_type_node,
				       array_var[jj], array_length[0][jj]);
	}
    }
  
  for (ii = 0; ii < rank; ii++)
    {
      add_stmt (ind_init [ii]);

      add_stmt (build1 (LABEL_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (build3 (COND_EXPR, void_type_node, compare_expr[ii],
			build1 (GOTO_EXPR, void_type_node, body_label[ii]),
			build1 (GOTO_EXPR, void_type_node, exit_label[ii])));
      add_stmt (body_label_expr[ii]);
    }

  add_stmt (stmt);

  for (ii = rank - 1; ii >= 0; ii--)
    {
      add_stmt (expr_incr[ii]);
      add_stmt (build1 (GOTO_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (exit_label_expr[ii]);
    }

  pop_stmt_list (loop);

  free (body_label);
  free (body_label_expr);
  free (exit_label);
  free (exit_label_expr);
  free (compare_expr);
  free (if_stmt_label);
  free (expr_incr);
  free (ind_init);
  free (array_operand);
  free (array_var);
  
  for (ii = 0; ii < list_size; ii++)
    {
      free (count_down[ii]);
      free (array_value[ii]);
      free (array_stride[ii]);
      free (array_length[ii]);
      free (array_start[ii]);
      free (array_ops[ii]);
      free (array_vector[ii]);
    }

  free (count_down);
  free (array_value);
  free (array_stride);
  free (array_length);
  free (array_start);
  free (array_ops);
  free (array_vector);

  return loop;
}

tree
fix_conditional_array_notations (tree stmt)
{
  if (TREE_CODE (stmt) == STATEMENT_LIST)
    {
      tree_stmt_iterator tsi;
      for (tsi = tsi_start (stmt); !tsi_end_p (tsi); tsi_next (&tsi))
	{
	  tree single_stmt = *tsi_stmt_ptr (tsi);
	  *tsi_stmt_ptr (tsi) =
	    fix_conditional_array_notations_1 (single_stmt);
	}
      return stmt;
    }
  else
    return fix_conditional_array_notations_1 (stmt);
}

struct c_expr 
fix_array_notation_expr (location_t location, enum tree_code code,
			 struct c_expr arg)
{

  tree *array_list = NULL;
  int list_size = 0;
  int rank = 0, ii = 0, jj = 0;
  tree **array_ops, *array_var, *array_operand, jj_tree, loop;
  tree **array_value, **array_stride, **array_length, **array_start;
  tree *body_label, *body_label_expr, *exit_label, *exit_label_expr;
  tree *compare_expr, *if_stmt_label, *expr_incr, *ind_init;
  bool **count_down, **array_vector;
  
  find_rank (arg.value, &rank);
  if (rank == 0)
    return arg;  
  
  extract_array_notation_exprs (arg.value, &array_list, &list_size);

  if (*array_list == NULL_TREE || list_size == 0)
    return arg;

  array_ops = (tree **) xmalloc (sizeof (tree *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    array_ops[ii] = (tree *) xmalloc (sizeof (tree) * rank);
  
  array_vector = (bool **) xmalloc (sizeof (bool *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    array_vector[ii] = (bool *) xmalloc (sizeof (bool) * rank);

  array_value = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_stride = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_length = (tree **) xmalloc (sizeof (tree *) * list_size);
  array_start = (tree **) xmalloc (sizeof (tree *) * list_size);

  for (ii = 0; ii < list_size; ii++)
    {
      array_value[ii]  = (tree *) xmalloc (sizeof (tree) * rank);
      array_stride[ii] = (tree *) xmalloc (sizeof (tree) * rank);
      array_length[ii] = (tree *) xmalloc (sizeof (tree) * rank);
      array_start[ii]  = (tree *) xmalloc (sizeof (tree) * rank);
    }

  body_label = (tree *) xmalloc(sizeof (tree) * rank);
  body_label_expr = (tree *) xmalloc (sizeof (tree) * rank);
  exit_label = (tree *) xmalloc (sizeof (tree) * rank);
  exit_label_expr = (tree *) xmalloc (sizeof (tree) * rank);
  compare_expr = (tree *) xmalloc (sizeof (tree) * rank);
  if_stmt_label = (tree *) xmalloc (sizeof (tree) * rank);
  
  expr_incr = (tree *) xmalloc (sizeof (tree) * rank);
  ind_init = (tree *) xmalloc (sizeof (tree) * rank);
  
  count_down = (bool **) xmalloc (sizeof (bool *) * list_size);
  for (ii = 0; ii < list_size; ii++)
    count_down[ii] = (bool *) xmalloc (sizeof (bool) * rank);

  array_operand = (tree *) xmalloc (sizeof (tree) * list_size);
  
  array_var = (tree *) xmalloc (sizeof (tree) * rank);
  

  for (ii = 0; ii < list_size; ii++)
    {
      jj = 0;
      for (jj_tree = array_list[ii];
	   jj_tree && TREE_CODE (jj_tree) == ARRAY_NOTATION_REF;
	   jj_tree = ARRAY_NOTATION_ARRAY (jj_tree))
	{
	  array_ops[ii][jj] = jj_tree;
	  jj++;
	}
    }

  for (ii = 0; ii < list_size; ii++)
    {
      if (TREE_CODE (array_list[ii]) == ARRAY_NOTATION_REF)
	{
	  for (jj = 0; jj < rank; jj++)
	    {
	      if (TREE_CODE (array_ops[ii][jj]) == ARRAY_NOTATION_REF)
		{
		  array_value[ii][jj] =
		    ARRAY_NOTATION_ARRAY (array_ops[ii][jj]);
		  array_start[ii][jj] =
		    ARRAY_NOTATION_START (array_ops[ii][jj]);
		  array_length[ii][jj] =
		    ARRAY_NOTATION_LENGTH (array_ops[ii][jj]);
		  array_stride[ii][jj] =
		    ARRAY_NOTATION_STRIDE (array_ops[ii][jj]);
		  array_vector[ii][jj] = true;

		  if (!TREE_CONSTANT (array_length[ii][jj]))
		      count_down[ii][jj] = false;
		  else if (tree_int_cst_lt
			   (array_length[ii][jj],
			    build_int_cst (TREE_TYPE (array_length[ii][jj]),
					   0)))
		    count_down[ii][jj] = true;
		  else
		    count_down[ii][jj] = false;
		}
	      else
		array_vector[ii][jj] = false;
	    }
	}
    }

  loop = push_stmt_list();

  for (ii = 0; ii < rank; ii++)
    {
  
      array_var[ii] = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE,
				  integer_type_node);
      ind_init[ii] =
	build_modify_expr (UNKNOWN_LOCATION, array_var[ii],
			   TREE_TYPE (array_var[ii]), NOP_EXPR,
			   UNKNOWN_LOCATION,
			   build_int_cst (TREE_TYPE (array_var[ii]), 0),
			   TREE_TYPE (array_var[ii]));
	
    }

  for (ii = 0; ii < rank ; ii++)
    {
      /* this will create the if statement label */
      if_stmt_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				      void_type_node);
      DECL_CONTEXT (if_stmt_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (if_stmt_label[ii]) = 0;
      DECL_IGNORED_P (if_stmt_label[ii]) = 1;
  
      /* this label statment will point to the loop body */
      body_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (body_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (body_label[ii]) = 0;
      DECL_IGNORED_P (body_label[ii]) = 1;
      body_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, body_label[ii]);

      /* this will create the exit label..i.e. where the while loop will branch
	 out of
      */
      exit_label[ii] = build_decl (UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE,
				   void_type_node);
      DECL_CONTEXT (exit_label[ii]) = current_function_decl;
      DECL_ARTIFICIAL (exit_label[ii]) = 0;
      DECL_IGNORED_P (exit_label[ii]) = 1;
      exit_label_expr[ii] = build1 (LABEL_EXPR, void_type_node, exit_label[ii]);
    }

  for (ii = 0; ii < list_size; ii++)
    {
      if (array_vector[ii][0])
	{
	  array_operand[ii] = array_value[ii][rank - 1];
	  gcc_assert (array_operand[ii]);

	  for (jj = rank - 1; jj >= 0; jj--)
	    {
	      if (count_down[ii][jj])
		{
		  /* Array[start_index - (induction_var * stride)] */
		  array_operand[ii] = build_array_ref
		    (UNKNOWN_LOCATION, array_operand[ii],
		     build2 (MINUS_EXPR, TREE_TYPE (array_var[jj]),
			     array_start[ii][jj],
			     build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
				     array_var[jj], array_stride[ii][jj])));
		}
	      else
		{
		  /* Array[start_index + (induction_var * stride)] */
		  array_operand[ii] = build_array_ref
		    (UNKNOWN_LOCATION, array_operand[ii],
		     build2 (PLUS_EXPR, TREE_TYPE (array_var[jj]),
			     array_start[ii][jj],
			     build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
				     array_var[jj], array_stride[ii][jj])));
		}
	    }
	}
    }
  replace_array_notations (&arg.value, array_list, array_operand, list_size);

  for (ii = 0; ii < rank; ii++)
    {
      expr_incr[ii] =
	build2 (MODIFY_EXPR, void_type_node, array_var[ii],
		build2 (PLUS_EXPR, TREE_TYPE (array_var[ii]), array_var[ii],
			build_int_cst (TREE_TYPE (array_var[ii]), 1)));
    }
  
  for (jj = 0; jj < rank; jj++)
    {
      if (rank && expr_incr[jj])
	{
	  if (count_down[0][jj])
	    compare_expr[jj] =
	      build2 (LT_EXPR, boolean_type_node, array_var[jj],
		      build2 (MULT_EXPR, TREE_TYPE (array_var[jj]),
			      array_length[0][jj],
			      build_int_cst (TREE_TYPE (array_var[jj]), -1)));
	  else
	    compare_expr[jj] = build2 (LT_EXPR, boolean_type_node,
				       array_var[jj], array_length[0][jj]);
	}
    }
  
  for (ii = 0; ii < rank; ii++)
    {
      add_stmt (ind_init [ii]);

      add_stmt (build1 (LABEL_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (build3 (COND_EXPR, void_type_node, compare_expr[ii],
			build1 (GOTO_EXPR, void_type_node, body_label[ii]),
			build1 (GOTO_EXPR, void_type_node, exit_label[ii])));
      add_stmt (body_label_expr[ii]);
    }

  if (code == POSTINCREMENT_EXPR || code == POSTDECREMENT_EXPR)
    {
      arg = default_function_array_read_conversion (location, arg);
      arg.value = build_unary_op (location, code, arg.value, 0);
    }
  else if (code == PREINCREMENT_EXPR || code == PREDECREMENT_EXPR)
    {
      arg = default_function_array_read_conversion (location, arg);
      arg = parser_build_unary_op (location, code, arg);
    }

  add_stmt (arg.value);
  
  for (ii = rank - 1; ii >= 0; ii--)
    {
      add_stmt (expr_incr[ii]);
      add_stmt (build1 (GOTO_EXPR, void_type_node, if_stmt_label[ii]));
      add_stmt (exit_label_expr[ii]);
    }

  pop_stmt_list (loop);

  free (body_label);
  free (body_label_expr);
  free (exit_label);
  free (exit_label_expr);
  free (compare_expr);
  free (if_stmt_label);
  free (expr_incr);
  free (ind_init);
  free (array_operand);
  free (array_var);
  
  for (ii = 0; ii < list_size; ii++)
    {
      free (count_down[ii]);
      free (array_value[ii]);
      free (array_stride[ii]);
      free (array_length[ii]);
      free (array_start[ii]);
      free (array_ops[ii]);
      free (array_vector[ii]);
    }

  free (count_down);
  free (array_value);
  free (array_stride);
  free (array_length);
  free (array_start);
  free (array_ops);
  free (array_vector);

  arg.value = loop;
  return arg;
}
  
