------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                               W A R N S W                                --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1999-2010, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT; see file COPYING3.  If not, go to --
-- http://www.gnu.org/licenses for a complete copy of the license.          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This unit contains the routines used to handle setting of warning options

package Warnsw is

   -------------------
   -- Warning Flags --
   -------------------

   --  These flags are activated or deactivated by -gnatw switches and control
   --  whether warnings of a given class will be generated or not.

   --  Note: most of these flags are still in opt, but the plan is to move them
   --  here as time goes by.

   Warn_On_Record_Holes : Boolean := False;
   --  Warn when explicit record component clauses leave uncovered holes (gaps)
   --  in a record layout. Off by default, set by -gnatw.h (but not -gnatwa).

   Warn_On_Overridden_Size : Boolean := False;
   --  Warn when explicit record component clause or array component_size
   --  clause specifies a size that overrides a size for the typen which was
   --  set with an explicit size clause. Off by default, set by -gnatw.s (but
   --  not -gnatwa).

   -----------------
   -- Subprograms --
   -----------------

   function Set_Warning_Switch (C : Character) return Boolean;
   --  This function sets the warning switch or switches corresponding to the
   --  given character. It is used to process a -gnatw switch on the command
   --  line, or a character in a string literal in pragma Warnings. Returns
   --  True for valid warning character C, False for invalid character.

   function Set_Dot_Warning_Switch (C : Character) return Boolean;
   --  This function sets the warning switch or switches corresponding to the
   --  given character preceded by a dot. Used to process a -gnatw. switch on
   --  the command line or .C in a string literal in pragma Warnings. Returns
   --  True for valid warning character C, False for invalid character.

   procedure Set_GNAT_Mode_Warnings;
   --  This is called in -gnatg mode to set the warnings for gnat mode. It is
   --  also used to set the proper warning statuses for -gnatw.g.

end Warnsw;
