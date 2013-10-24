/* PR preprocessor/57824 */
/* { dg-do compile } */
/* { dg-options "-std=gnu99 -fdump-tree-optimized-lineno -save-temps" { target c } } */
/* { dg-options "-std=c++11 -fdump-tree-optimized-lineno -save-temps" { target c++ } } */

const char x[] = R"(
abc
def
ghi
)";

int
main ()
{
  extern void foo (); foo ();
  return 0;
}

/* Verify call to foo is on line 15.  */
/* { dg-final { scan-tree-dump ": 15\[]:]\[^\n\r]*foo" "optimized" } } */
/* { dg-final { cleanup-tree-dump "optimized" } } */
/* { dg-final { cleanup-saved-temps } } */
