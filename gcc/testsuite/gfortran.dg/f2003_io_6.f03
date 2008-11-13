! { dg-do run }
! { dg-xfail-run-if "PR37839" { { ia64-*-hpux* hppa*-*-* powerpc*-*-* sparc*-sun-solaris2.* } && ilp32 } { "*" }  { "" } }
! Test case prepared by Jerry DeLisle  <jvdelisle@gcc.gnu.org>
! Test of decimal="comma" in namelist, checks separators
implicit none
integer :: i
real :: a(6) = 0.0
character(len=30) :: str = '&nm a = 1,3; 4, 5; 5; 7; /'
namelist /nm/ a
read(str,nml=nm,decimal='comma')
if (any(a.ne.[ 1.3, 4.0, 5.0, 5.0, 7.0, 0.0 ])) call abort
end
