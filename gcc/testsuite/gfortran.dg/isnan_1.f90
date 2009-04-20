! Test for the ISNAN intrinsic
!
! { dg-do run }
! { dg-options "-pedantic-errors -mieee" { target sh*-*-* } }
!
  implicit none
  real :: x
  x = -1.0
  x = sqrt(x)
  if (.not. isnan(x)) call abort
  x = 0.0
  x = x / x
  if (.not. isnan(x)) call abort

  x = 5.0
  if (isnan(x)) call abort
  x = huge(x)
  x = 2*x
  if (isnan(x)) call abort
end
