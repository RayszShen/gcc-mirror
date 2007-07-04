! { dg-do compile }

      SUBROUTINE DO_BY_16(X, IAM, IPOINTS)
        REAL X(*)
        INTEGER IAM, IPOINTS
      END SUBROUTINE DO_BY_16
      SUBROUTINE SUBA36(X, NPOINTS)
        INTEGER NPOINTS
        REAL X(NPOINTS)
        INTEGER IAM, IPOINTS
        EXTERNAL OMP_SET_DYNAMIC, OMP_SET_NUM_THREADS
        INTEGER OMP_GET_NUM_THREADS, OMP_GET_THREAD_NUM
        CALL OMP_SET_DYNAMIC(.FALSE.)
        CALL OMP_SET_NUM_THREADS(16)
!$OMP PARALLEL SHARED(X,NPOINTS) PRIVATE(IAM, IPOINTS)
          IF (OMP_GET_NUM_THREADS() .NE. 16) THEN
            STOP
          ENDIF
          IAM = OMP_GET_THREAD_NUM()
          IPOINTS = NPOINTS/16
          CALL DO_BY_16(X,IAM,IPOINTS)
!$OMP END PARALLEL
      END SUBROUTINE SUBA36
