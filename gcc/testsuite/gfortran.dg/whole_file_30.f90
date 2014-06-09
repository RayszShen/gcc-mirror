! { dg-do compile }
! Test the fix for the problem described in PR46818.
! Note that the module file is kept for whole_file_31.f90
!
! Contributed by Martien Hulsen  <m.a.hulsen@tue.nl>
! and reduced by Tobias Burnus  <burnus@gcc.gnu.org>
!
! ============== system_defs.f90 =============
module system_defs_m
  type sysvector_t
    integer :: probnr = 0
    real, allocatable, dimension(:) :: u
  end type sysvector_t
end module system_defs_m
! DO NOT CLEAN UP THE MODULE FILE - whole_file_31.f90 does it.
! { dg-final { keep-modules "" } }
