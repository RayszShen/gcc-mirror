! { dg-do run }

module udr2
  type dt
    integer :: x = 7
    integer :: y = 9
  end type
end module udr2
  use udr2, only : dt
!$omp declare reduction (foo : integer : omp_out = omp_out + omp_in)
  integer :: i, j(2:4,3:5)
!$omp declare reduction (bar : integer : &
!$omp & omp_out = omp_out + iand (omp_in, -4)) initializer (omp_priv = 3)
  interface operator (+)
    function notdefined(x, y)
      use udr2, only : dt
      type(dt), intent (in) :: x, y
      type(dt) :: notdefined
    end function
  end interface
  type (dt) :: d(2:4,3:5)
!$omp declare reduction (+ : dt : omp_out%x = omp_out%x &
!$omp & + iand (omp_in%x, -8))
!$omp declare reduction (foo : dt : omp_out%x = iand (omp_in%x, -8) &
!$omp & + omp_out%x) initializer (omp_priv = dt (5, 21))
  j = 0
!$omp parallel do reduction (foo : j)
  do i = 1, 100
    j = j + i
  end do
  if (any(j .ne. 5050)) call abort
  j = 3
!$omp parallel do reduction (bar : j)
  do i = 1, 100
    j = j + 4 * i
  end do
  if (any(j .ne. (5050 * 4 + 3))) call abort
!$omp parallel do reduction (+ : d)
  do i = 1, 100
    if (any(d%y .ne. 9)) call abort
    d%x = d%x + 8 * i
  end do
  if (any(d%x .ne. (5050 * 8 + 7)) .or. any(d%y .ne. 9)) call abort
  d = dt (5, 21)
!$omp parallel do reduction (foo : d)
  do i = 1, 100
    if (any(d%y .ne. 21)) call abort
    d%x = d%x + 8 * i
  end do
  if (any(d%x .ne. (5050 * 8 + 5)) .or. any(d%y .ne. 21)) call abort
end
