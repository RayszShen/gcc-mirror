! { dg-do run }
! { dg-require-effective-target fortran_large_real }
! Program to test the UNPACK intrinsic for large real type
program intrinsic_unpack
   implicit none
   integer,parameter :: k = selected_real_kind (precision (0.0_8) + 1)

   real(kind=k), dimension(3,3) :: ark, brk
   logical, dimension(3, 3) :: mask
   character(len=100) line1, line2
   integer i

   mask = reshape ((/.false.,.true.,.false.,.true.,.false.,.false.,&
                    &.false.,.false.,.true./), (/3, 3/));

   ark = reshape ((/1._k, 0._k, 0._k, 0._k, 1._k, 0._k, 0._k, 0._k, 1._k/), &
         (/3, 3/));
   brk = unpack ((/2._k, 3._k, 4._k/), mask, ark)
   if (any (brk .ne. reshape ((/1._k, 2._k, 0._k, 3._k, 1._k, 0._k, &
                               0._k, 0._k, 4._k/), (/3, 3/)))) &
      call abort
   write (line1,'(9F9.5)') brk
   write (line2,'(9F9.5)') unpack((/2._k, 3._k, 4._k/), mask, ark)
   if (line1 .ne. line2) call abort
   brk = -1._k
   brk = unpack ((/2._k, 3._k, 4._k/), mask, 0._k)
   if (any (brk .ne. reshape ((/0._k, 2._k, 0._k, 3._k, 0._k, 0._k, &
      0._k, 0._k, 4._k/), (/3, 3/)))) &
      call abort

end program
