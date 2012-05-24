! { dg-do compile }

MODULE YOMPHY0
REAL :: ECMNP
REAL :: SCO
REAL :: USDMLT
END MODULE YOMPHY0
SUBROUTINE ACCONV ( KIDIA,KFDIA,KLON,KTDIA,KLEV,&
                    &CDLOCK)
USE YOMPHY0  , ONLY : ECMNP    ,SCO      ,USDMLT
REAL :: PAPHIF(KLON,KLEV),PCVGQ(KLON,KLEV)&
    &,PFPLCL(KLON,0:KLEV),PFPLCN(KLON,0:KLEV),PSTRCU(KLON,0:KLEV)&
    &,PSTRCV(KLON,0:KLEV)
INTEGER :: KNLAB(KLON,KLEV),KNND(KLON)
REAL :: ZCP(KLON,KLEV),ZLHE(KLON,KLEV),ZDSE(KLON,KLEV)&
    &,ZPOII(KLON),ZALF(KLON),ZLN(KLON),ZUN(KLON),ZVN(KLON)&
    &,ZPOIL(KLON)
DO JLEV=KLEV-1,KTDIA,-1
  DO JIT=1,NBITER
    ZLN(JLON)=MAX(0.,ZLN(JLON)&
     &-(ZQW(JLON,JLEV)-ZQN(JLON)&
     &*(PQ(JLON,JLEV+1)-ZQN(JLON))))*KNLAB(JLON,JLEV)
  ENDDO
ENDDO
IF (ITOP < KLEV+1) THEN
  DO JLON=KIDIA,KFDIA
    ZZVAL=PFPLCL(JLON,KLEV)+PFPLCN(JLON,KLEV)-SCO
    KNND(JLON)=KNND(JLON)*MAX(0.,-SIGN(1.,0.-ZZVAL))
  ENDDO
  DO JLEV=ITOP,KLEV
    DO JLON=KIDIA,KFDIA
    ENDDO
  ENDDO
ENDIF
END SUBROUTINE ACCONV

! { dg-final { cleanup-tree-dump "vect" } }
