/* { dg-do compile } */
/* { dg-options "-O1 -fdump-tree-ccp" } */

/* Check that ccp folds strlen of equally long strings, and that it does not
   fail to terminate when there is a nontrivial cycle in the corresponding
   ssa graph.  */

void foo(int i)
{
  char *s = "abcde";

  if (i)
    {
      s = "defgh";
      goto middle;
    }

start:

  bla ();

middle:

  if (bla ())
    goto start;

  bar (strlen (s));
}

/* There should be no calls to strlen.  */
/* { dg-final { scan-tree-dump-times "strlen" 0 "ccp"} } */
