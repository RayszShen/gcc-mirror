------------------------------------------------------------------------------
--                                                                          --
--                    GNAT RUN-TIME LIBRARY COMPONENTS                      --
--                                                                          --
--        S Y S T E M . C O M P A R E _ A R R A Y _ S I G N E D _ 6 4       --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2002-2008, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the  Free Software Foundation,  51  Franklin  Street,  Fifth  Floor, --
-- Boston, MA 02110-1301, USA.                                              --
--                                                                          --
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

with System.Address_Operations; use System.Address_Operations;

with Ada.Unchecked_Conversion;

package body System.Compare_Array_Signed_64 is

   type Word is range -2**63 .. 2**63 - 1;
   for Word'Size use 64;
   --  Used to process operands by words

   type Uword is new Word;
   for Uword'Alignment use 1;
   --  Used to process operands when unaligned

   type WP is access Word;
   type UP is access Uword;

   function W is new Ada.Unchecked_Conversion (Address, WP);
   function U is new Ada.Unchecked_Conversion (Address, UP);

   -----------------------
   -- Compare_Array_S64 --
   -----------------------

   function Compare_Array_S64
     (Left      : System.Address;
      Right     : System.Address;
      Left_Len  : Natural;
      Right_Len : Natural) return Integer
   is
      Clen : Natural := Natural'Min (Left_Len, Right_Len);
      --  Number of elements left to compare

      L : Address := Left;
      R : Address := Right;
      --  Pointers to next elements to compare

   begin
      --  Case of going by aligned double words

      if ModA (OrA (Left, Right), 8) = 0 then
         while Clen /= 0 loop
            if W (L).all /= W (R).all then
               if W (L).all > W (R).all then
                  return +1;
               else
                  return -1;
               end if;
            end if;

            Clen := Clen - 1;
            L := AddA (L, 8);
            R := AddA (R, 8);
         end loop;

      --  Case of going by unaligned double words

      else
         while Clen /= 0 loop
            if U (L).all /= U (R).all then
               if U (L).all > U (R).all then
                  return +1;
               else
                  return -1;
               end if;
            end if;

            Clen := Clen - 1;
            L := AddA (L, 8);
            R := AddA (R, 8);
         end loop;
      end if;

      --  Here if common section equal, result decided by lengths

      if Left_Len = Right_Len then
         return 0;
      elsif Left_Len > Right_Len then
         return +1;
      else
         return -1;
      end if;
   end Compare_Array_S64;

end System.Compare_Array_Signed_64;
