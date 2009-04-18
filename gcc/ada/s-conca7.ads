------------------------------------------------------------------------------
--                                                                          --
--                 GNAT RUN-TIME LIBRARY (GNARL) COMPONENTS                 --
--                                                                          --
--                      S Y S T E M . C O N C A T _ 7                       --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--            Copyright (C) 2008-2009, Free Software Foundation, Inc.       --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.                                     --
--                                                                          --
-- As a special exception under Section 7 of GPL version 3, you are granted --
-- additional permissions described in the GCC Runtime Library Exception,   --
-- version 3.1, as published by the Free Software Foundation.               --
--                                                                          --
-- You should have received a copy of the GNU General Public License and    --
-- a copy of the GCC Runtime Library Exception along with this program;     --
-- see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This package contains a procedure for runtime concatenation of seven string
--  operands. It is used when we want to save space in the generated code.

pragma Compiler_Unit;

package System.Concat_7 is

   procedure Str_Concat_7
     (R                          : out String;
      S1, S2, S3, S4, S5, S6, S7 : String);
   --  Performs the operation R := S1 & S2 & S3 & S4 & S5 & S6 & S7. The bounds
   --  of R are known to be correct, so no bounds checks are required, and it
   --  is known that none of the input operands overlaps R. No assumptions can
   --  be made about the lower bounds of any of the operands.

end System.Concat_7;
