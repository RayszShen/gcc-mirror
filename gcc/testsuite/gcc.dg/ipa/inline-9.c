/* { dg-options "-Os -fdump-ipa-inline"  } */
int foo (void);
int test(int a)
{
 if (a>100)
   {
     foo();
     foo();
     foo();
     foo();
     foo();
     foo();
     foo();
     foo();
   }
}
int
main()
{
  for (int i=0;i<100;i++)
    test(i);
}
/* { dg-final { scan-tree-dump "Inlined 1 calls" "inline" } } */
