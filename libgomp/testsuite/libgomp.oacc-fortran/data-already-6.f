! { dg-skip-if "" { *-*-* } { "*" } { "-DACC_MEM_SHARED=0" } }

      IMPLICIT NONE
      INCLUDE "openacc_lib.h"

      INTEGER I

      CALL ACC_PRESENT_OR_COPYIN (I)
!$ACC ENTER DATA CREATE (I)

      END

! { dg-shouldfail "" }
! { dg-output "already mapped to" }
