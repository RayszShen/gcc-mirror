! Program to test initialization of equivalence blocks.  PR13742.
! Some forms are not yet implemented.  These are indicated by !!$

subroutine test0s
  character*10 :: x = "abcdefghij" 
  character*10 :: y
  equivalence (x,y)

  character*10 :: xs(10) 
  character*10 :: ys(10)
  equivalence (xs,ys)
  data xs /10*"abcdefghij"/

  if (y.ne."abcdefghij") call abort
  if (ys(1).ne."abcdefghij") call abort
  if (ys(10).ne."abcdefghij") call abort
end
  
subroutine test0
  integer :: x = 123
  integer :: y
  equivalence (x,y)
  if (y.ne.123) call abort
end

subroutine test1
  integer :: a(3)
  integer :: x = 1
  integer :: y
  integer :: z = 3
  equivalence (a(1), x)
  equivalence (a(3), z)
  if (x.ne.1) call abort
  if (z.ne.3) call abort
  if (a(1).ne.1) call abort
  if (a(3).ne.3) call abort
end

subroutine test2
  integer :: x
  integer :: z
  integer :: a(3) = 123
  equivalence (a(1), x)
  equivalence (a(3), z)
  if (x.ne.123) call abort
  if (z.ne.123) call abort
end

subroutine test3
  integer :: x
!!$  integer :: y = 2
  integer :: z
  integer :: a(3)
  equivalence (a(1),x), (a(2),y), (a(3),z)
  data a(1) /1/, a(3) /3/
  if (x.ne.1) call abort
!!$  if (y.ne.2) call abort
  if (z.ne.3) call abort
end

subroutine test4
  integer a(2)
  integer b(2)
  integer c
  equivalence (a(2),b(1)), (b(2),c)
  data a/1,2/
  data c/3/
  if (b(1).ne.2) call abort
  if (b(2).ne.3) call abort
end

!!$subroutine test5
!!$  integer a(2)
!!$  integer b(2)
!!$  integer c
!!$  equivalence (a(2),b(1)), (b(2),c)
!!$  data a(1)/1/
!!$  data b(1)/2/
!!$  data c/3/
!!$  if (a(2).ne.2) call abort
!!$  if (b(2).ne.3) call abort
!!$  print *, "Passed test5"
!!$end
  
program main
  call test0s
  call test0
  call test1
  call test2
  call test3
  call test4
!!$  call test5
end

