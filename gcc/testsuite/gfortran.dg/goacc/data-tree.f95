! { dg-do compile } 
! { dg-additional-options "-fdump-tree-original" } 

program test
  implicit none
  integer :: q, i, j, k, m, n, o, p, r, s, t, u, v, w
  logical :: l = .true.

  !$acc data if(l) copy(i), copyin(j), copyout(k), create(m) &
  !$acc no_create(n) &
  !$acc present(o), pcopy(p), pcopyin(r), pcopyout(s), pcreate(t) &
  !$acc deviceptr(u)
  !$acc end data

end program test
! { dg-final { scan-tree-dump-times "pragma acc data" 1 "original" } } 

! { dg-final { scan-tree-dump-times "if" 1 "original" } }
! { dg-final { scan-tree-dump-times "map\\(tofrom:i\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(to:j\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(from:k\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(alloc:m\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(no_alloc:n\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(force_present:o\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(tofrom:p\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(to:r\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(from:s\\)" 1 "original" } } 
! { dg-final { scan-tree-dump-times "map\\(alloc:t\\)" 1 "original" } } 

! { dg-final { scan-tree-dump-times "map\\(force_deviceptr:u\\)" 1 "original" } } 
