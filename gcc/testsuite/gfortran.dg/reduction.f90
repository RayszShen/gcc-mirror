! { dg-do run }
! PR 16946
! Not all allowed combinations of arguments for MAXVAL, MINVAL,
! PRODUCT and SUM were supported.
program reduction_mask
  implicit none
  logical :: equal(3)
  
  integer, parameter :: res(4*9) = (/ 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, &
       1, 1, 1, 1, 1, 6, 6, 6, 2, 2, 2, 2, 2, 2, 6, 6, 6, 3, 3, 3, 3, 3, 3 /)
  integer :: val(4*9)
  
  equal = (/ .true., .true., .false. /)
  
  ! use all combinations of the dim and mask arguments for the
  ! reduction intrinsics
  val( 1) = maxval((/ 1, 2, 3 /))
  val( 2) = maxval((/ 1, 2, 3 /), 1)
  val( 3) = maxval((/ 1, 2, 3 /), dim=1)
  val( 4) = maxval((/ 1, 2, 3 /), equal)
  val( 5) = maxval((/ 1, 2, 3 /), mask=equal)
  val( 6) = maxval((/ 1, 2, 3 /), 1, equal)
  val( 7) = maxval((/ 1, 2, 3 /), 1, mask=equal)
  val( 8) = maxval((/ 1, 2, 3 /), dim=1, mask=equal)
  val( 9) = maxval((/ 1, 2, 3 /), mask=equal, dim=1)
       
  val(10) = minval((/ 1, 2, 3 /))
  val(11) = minval((/ 1, 2, 3 /), 1)
  val(12) = minval((/ 1, 2, 3 /), dim=1)
  val(13) = minval((/ 1, 2, 3 /), equal)
  val(14) = minval((/ 1, 2, 3 /), mask=equal)
  val(15) = minval((/ 1, 2, 3 /), 1, equal)
  val(16) = minval((/ 1, 2, 3 /), 1, mask=equal)
  val(17) = minval((/ 1, 2, 3 /), dim=1, mask=equal)
  val(18) = minval((/ 1, 2, 3 /), mask=equal, dim=1)
       
  val(19) = product((/ 1, 2, 3 /))
  val(20) = product((/ 1, 2, 3 /), 1)
  val(21) = product((/ 1, 2, 3 /), dim=1)
  val(22) = product((/ 1, 2, 3 /), equal)
  val(23) = product((/ 1, 2, 3 /), mask=equal)
  val(24) = product((/ 1, 2, 3 /), 1, equal)
  val(25) = product((/ 1, 2, 3 /), 1, mask=equal)
  val(26) = product((/ 1, 2, 3 /), dim=1, mask=equal)
  val(27) = product((/ 1, 2, 3 /), mask=equal, dim=1)
       
  val(28) = sum((/ 1, 2, 3 /))
  val(29) = sum((/ 1, 2, 3 /), 1)
  val(30) = sum((/ 1, 2, 3 /), dim=1)
  val(31) = sum((/ 1, 2, 3 /), equal)
  val(32) = sum((/ 1, 2, 3 /), mask=equal)
  val(33) = sum((/ 1, 2, 3 /), 1, equal)
  val(34) = sum((/ 1, 2, 3 /), 1, mask=equal)
  val(35) = sum((/ 1, 2, 3 /), dim=1, mask=equal)
  val(36) = sum((/ 1, 2, 3 /), mask=equal, dim=1)
  
  if (any (val /= res)) call abort
end program reduction_mask
