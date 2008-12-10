! { dg-do compile }
!
! PR 36704: Procedure pointer as function result
!
! Contributed by Janus Weil <janus@gcc.gnu.org>

function f() result(r)
real, parameter :: r = 5.0    ! { dg-error "attribute conflicts" }
end function 

function g() result(s)
real :: a,b,c
namelist /s/ a,b,c    ! { dg-error "attribute conflicts" }
end function

function h() result(t)
type t    ! { dg-error "attribute conflicts" }
end function
