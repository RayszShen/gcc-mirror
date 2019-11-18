! Ensure that printf on the offload device works.

! { dg-do run }
! { dg-output "The answer is 42(\n|\r\n|\r)+" }
! { dg-xfail-if "no write for nvidia" { openacc_nvidia_accel_selected } }                                                                                                                                                                

program main
  implicit none
  integer :: var = 42

!$acc parallel
  write (0, '("The answer is ", I2)') var
!$acc end parallel

end program main
