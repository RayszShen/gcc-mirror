! { dg-do run }
module m
   implicit character(8) (a-z)
contains
   function f(x)
      integer :: x
      integer :: f
      real :: e
      f = x
      return
   entry e(x)
      e = x
   end
end module

program p
   use m
   if (f(1) /= 1) call abort
   if (e(1) /= 1.0) call abort
end
