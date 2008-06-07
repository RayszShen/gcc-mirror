! { dg-do run }
! Tests the fix for PR35743 and PR35745.
!
! Contributed by Dick Hendrickson <dick.hendrickson@gmail.com>
!
program try_rg0025
  logical lda(5)
  lda = (/(i/2*2 .ne. I, i=1,5)/)
  call PR35743 (lda,  1,  2,  3,  5,  6, -1, -2)
  CALL PR34745
end program

! Previously, the negative mask size would not be detected.
SUBROUTINE PR35743 (LDA,nf1,nf2,nf3,nf5,nf6,mf1,mf2)
  type unseq
    real  r
  end type unseq
  TYPE(UNSEQ) TDA1L(6)
  LOGICAL LDA(NF5)
  TDA1L(1:6)%r = 1.0
  WHERE (LDA(NF6:NF3))
    TDA1L(MF1:NF5:MF1) = TDA1L(NF6:NF2)
  ENDWHERE
END SUBROUTINE

! Previously, the expression in the WHERE block would be evaluated
! ouside the loop generated by the where.
SUBROUTINE PR34745
  INTEGER IDA(10)
  REAL RDA(10)
  RDA    = 1.0
  nf0 = 0
  WHERE (RDA < -15.0)
    IDA = 1/NF0 + 2
  ENDWHERE
END SUBROUTINE
