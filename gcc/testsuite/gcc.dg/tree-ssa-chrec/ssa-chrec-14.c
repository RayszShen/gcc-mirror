/* { dg-do compile } */ 
/* { dg-options "-O1 -fscalar-evolutions -fdump-tree-scev" } */


int main (void)
{
  int a = -100;
  int b = 2;
  int c = 3;
  int d = 4;
  
  while (d)
    {
      if (a < 30)
	a += b;
      else
	a += c;
      
      b += 1;
      c += 5;
      
      /* Exercises the initial condition of A after the if-phi-node.  */
      d = d + a;
    }
}

/* The analyzer has to detect the following evolution function:
   b  ->  {2, +, 1}_1
   c  ->  {3, +, 5}_1
   a  ->  {-100, +, {[2, 3], +, [1, 5]}_1}_1
   d  ->  {4, +, {[-98, -97], +, {[2, 3], +, [1, 5]}_1}_1}_1
*/

/* { dg-final { diff-tree-dumps "scev" } } */

