/* PR optimization/12198

   This was a miscompilation of a switch expressions because
   the "Case Ranges" extension wasn't handled in tree-cfg.c.  */

/* { dg-do compile } */
/* { dg-options "-O -fdump-tree-optimized" } */

int main() 
{ 
   int i; 
   i = 2; 
   switch (i) 
      { 
      case 1 ... 5: 
         goto L1; 
      default: 
         abort (); 
         goto L1; 
      } 
   L1: 
   exit(0); 
}

/* The abort() call clearly is unreachable.  Only the "extern abort"
   declaration should survive optimization.  */
/* { dg-final { scan-tree-dump-times "abort" 1 "optimized"} } */
