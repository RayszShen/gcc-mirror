/* { dg-do compile } */
/* { dg-options "-O3 -fdump-tree-optimized"  } */
/* { dg-add-options bind_pic_locally } */

int i;

int foo ();

main ()
{
  return foo (i);
}

int foo (i)
     int i;
{
  return bar(i);
}

/* { dg-final { scan-tree-dump-times "bar"  2 "optimized"  } } */
/* { dg-final { cleanup-tree-dump "optimized" } } */
