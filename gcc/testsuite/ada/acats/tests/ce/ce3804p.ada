-- CE3804P.ADA

--                             Grant of Unlimited Rights
--
--     Under contracts F33600-87-D-0337, F33600-84-D-0280, MDA903-79-C-0687,
--     F08630-91-C-0015, and DCA100-97-D-0025, the U.S. Government obtained 
--     unlimited rights in the software and documentation contained herein.
--     Unlimited rights are defined in DFAR 252.227-7013(a)(19).  By making 
--     this public release, the Government intends to confer upon all 
--     recipients unlimited rights  equal to those held by the Government.  
--     These rights include rights to use, duplicate, release or disclose the 
--     released technical data and computer software in whole or in part, in 
--     any manner and for any purpose whatsoever, and to have or permit others 
--     to do so.
--
--                                    DISCLAIMER
--
--     ALL MATERIALS OR INFORMATION HEREIN RELEASED, MADE AVAILABLE OR
--     DISCLOSED ARE AS IS.  THE GOVERNMENT MAKES NO EXPRESS OR IMPLIED 
--     WARRANTY AS TO ANY MATTER WHATSOEVER, INCLUDING THE CONDITIONS OF THE
--     SOFTWARE, DOCUMENTATION OR OTHER INFORMATION RELEASED, MADE AVAILABLE 
--     OR DISCLOSED, OR THE OWNERSHIP, MERCHANTABILITY, OR FITNESS FOR A
--     PARTICULAR PURPOSE OF SAID MATERIAL.
--*
-- OBJECTIVE:
--     CHECK THAT FIXED_IO GET RAISES CONSTRAINT_ERROR WHEN THE VALUE
--     SUPPLIED BY WIDTH IS NEGATIVE, WIDTH IS GREATER THAN FIELD'LAST
--     WHEN FIELD'LAST IS LESS THAN INTEGER'LAST, OR THE VALUE READ IS
--     OUT OF RANGE OF THE ITEM PARAMETER, BUT WITHIN THE RANGE OF THE
--     SUBTYPE USED TO INSTANTIATE FIXED_IO.

-- APPLICABILITY CRITERIA:
--     THIS TEST IS APPLICABLE ONLY TO IMPLEMENTATIONS WHICH SUPPORT
--     TEXT FILES.

-- HISTORY:
--     DWC 09/15/87  CREATED ORIGINAL TEST.
--     JRL 06/07/96  Added call to Ident_Int in expressions involving
--                   Field'Last, to make the expressions non-static and
--                   prevent compile-time rejection. Corrected typo.

WITH REPORT;
USE REPORT;
WITH TEXT_IO;
USE TEXT_IO;

PROCEDURE CE3804P IS
     INCOMPLETE : EXCEPTION;

BEGIN

     TEST ("CE3804P", "CHECK THAT FLOAT_IO GET RAISES " &
                      "CONSTRAINT_ERROR WHEN THE VALUE SUPPLIED " &
                      "BY WIDTH IS NEGATIVE, WIDTH IS GREATER THAN " &
                      "FIELD'LAST WHEN FIELD'LAST IS LESS THAN " &
                      "INTEGER'LAST, OR THE VALUE READ IS OUT OF " &
                      "RANGE OF THE ITEM PARAMETER, BUT WITHIN THE " &
                      "RANGE OF THE SUBTYPE USED TO INSTANTIATE " &
                      "FLOAT_IO.");

     DECLARE
          TYPE FIXED IS DELTA 0.25 RANGE 0.0 .. 10.0;
          FT : FILE_TYPE;
          PACKAGE FX_IO IS NEW FIXED_IO (FIXED);
          USE FX_IO;
          X : FIXED RANGE 0.0 .. 5.0;

     BEGIN
          BEGIN
               GET (FT, X, IDENT_INT(-3));
               FAILED ("CONSTRAINT_ERROR NOT RAISED FOR NEGATIVE " &
                       "WIDTH");
          EXCEPTION
               WHEN CONSTRAINT_ERROR =>
                    NULL;
               WHEN STATUS_ERROR =>
                    FAILED ("STATUS_ERROR RAISED INSTEAD OF " &
                            "CONSTRAINT_ERROR FOR NEGATIVE WIDTH");
               WHEN OTHERS =>
                    FAILED ("WRONG EXCEPTION RAISED FOR NEGATIVE " &
                            "WIDTH");
          END;

          IF FIELD'LAST < INTEGER'LAST THEN
               BEGIN
                    GET (X, FIELD'LAST + Ident_Int(1));
                    FAILED ("CONSTRAINT_ERROR NOT RAISED - " &
                            "FIELD'LAST + 1 WIDTH - DEFAULT");
               EXCEPTION
                    WHEN CONSTRAINT_ERROR =>
                         NULL;
                    WHEN OTHERS =>
                         FAILED ("WRONG EXCEPTION RAISED - " &
                                 "FIELD'LAST + 1 WIDTH - DEFAULT");
               END;
          END IF;

          BEGIN
               CREATE (FT, OUT_FILE, LEGAL_FILE_NAME);
          EXCEPTION
               WHEN USE_ERROR =>
                    NOT_APPLICABLE ("USE_ERROR RAISED; TEXT CREATE " &
                                    "WITH OUT_FILE MODE");
                    RAISE INCOMPLETE;
               WHEN NAME_ERROR =>
                    NOT_APPLICABLE ("NAME_ERROR RAISED; TEXT CREATE " &
                                    "WITH OUT_FILE MODE");
                    RAISE INCOMPLETE;
          END;

          PUT (FT, "1.0");
          NEW_LINE (FT);
          PUT (FT, "8.0");
          NEW_LINE (FT);
          PUT (FT, "2.0");
          NEW_LINE (FT);
          PUT (FT, "3.0");

          CLOSE (FT);

          BEGIN
               OPEN (FT, IN_FILE, LEGAL_FILE_NAME);
          EXCEPTION
               WHEN USE_ERROR =>
                    NOT_APPLICABLE ("USE_ERROR RAISED; TEXT OPEN " &
                                    "WITH IN_FILE MODE");
                    RAISE INCOMPLETE;
          END;

          GET (FT, X);
          IF X /= 1.0 THEN
               FAILED ("WRONG VALUE READ WITH EXTERNAL FILE");
          END IF;

          SKIP_LINE (FT);

          BEGIN
               GET (FT, X, 3);
               FAILED ("CONSTRAINT_ERROR NOT RAISED - " &
                       "OUT OF RANGE");
          EXCEPTION
               WHEN CONSTRAINT_ERROR =>
                    NULL;
               WHEN OTHERS =>
                    FAILED ("WRONG EXCEPTION RAISED - " &
                            "OUT OF RANGE");
          END;

          SKIP_LINE (FT);

          BEGIN
               GET (FT, X, IDENT_INT(-1));
               FAILED ("CONSTRAINT_ERROR NOT RAISED - " &
                       "NEGATIVE WIDTH WITH EXTERNAL FILE");
          EXCEPTION
               WHEN CONSTRAINT_ERROR =>
                    NULL;
               WHEN OTHERS =>
                    FAILED ("WRONG EXCEPTION RAISED - " &
                            "NEGATIVE WIDTH WITH EXTERNAL FILE");
          END;

          IF FIELD'LAST < INTEGER'LAST THEN
               BEGIN
                    GET (FT, X, FIELD'LAST + Ident_Int(1));
                    FAILED ("CONSTRAINT_ERROR NOT RAISED - " &
                            "FIELD'LAST + 1 WIDTH WITH " &
                            "EXTERNAL FILE");
               EXCEPTION
                    WHEN CONSTRAINT_ERROR =>
                         NULL;
                    WHEN OTHERS =>
                         FAILED ("WRONG EXCEPTION RAISED - " &
                                 "FIELD'LAST + 1 WIDTH WITH " &
                                 "EXTERNAL FILE");
               END;
          END IF;

          SKIP_LINE (FT);

          BEGIN
               GET (FT, X, 3);
          EXCEPTION
               WHEN CONSTRAINT_ERROR =>
                    FAILED ("CONSTRAINT_ERROR RAISED; VALID WIDTH " &
                            "WITH EXTERNAL FILE");
               WHEN OTHERS =>
                    FAILED ("UNEXPECTED ERROR RAISED; VALID WIDTH " &
                            "WITH EXTERNAL FILE");
          END;

          BEGIN
               DELETE (FT);
          EXCEPTION
               WHEN USE_ERROR =>
                    NULL;
          END;

     EXCEPTION
          WHEN INCOMPLETE =>
               NULL;
     END;

     RESULT;

END CE3804P;
