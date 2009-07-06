------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                     S Y S T E M . T A S K _ I N F O                      --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2009, Free Software Foundation, Inc.         --
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

--  This package body contains the routines associated with the implementation
--  of the Task_Info pragma.

--  This is the Solaris (native) version of this module

package body System.Task_Info is

   -----------------------------
   -- Bound_Thread_Attributes --
   -----------------------------

   function Bound_Thread_Attributes return Thread_Attributes is
   begin
      return (False, True);
   end Bound_Thread_Attributes;

   function Bound_Thread_Attributes (CPU : CPU_Number)
      return Thread_Attributes is
   begin
      return (True, True, CPU);
   end Bound_Thread_Attributes;

   ---------------------------------
   -- New_Bound_Thread_Attributes --
   ---------------------------------

   function New_Bound_Thread_Attributes return Task_Info_Type is
   begin
      return new Thread_Attributes'(False, True);
   end New_Bound_Thread_Attributes;

   function New_Bound_Thread_Attributes (CPU : CPU_Number)
      return Task_Info_Type is
   begin
      return new Thread_Attributes'(True, True, CPU);
   end New_Bound_Thread_Attributes;

   -----------------------------------
   -- New_Unbound_Thread_Attributes --
   -----------------------------------

   function New_Unbound_Thread_Attributes return Task_Info_Type is
   begin
      return new Thread_Attributes'(False, False);
   end New_Unbound_Thread_Attributes;

   -------------------------------
   -- Unbound_Thread_Attributes --
   -------------------------------

   function Unbound_Thread_Attributes return Thread_Attributes is
   begin
      return (False, False);
   end Unbound_Thread_Attributes;

end System.Task_Info;
