------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              M E M R O O T                               --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                            $Revision: 1.1 $
--                                                                          --
--            Copyright (C) 1997-2001 Ada Core Technologies, Inc.           --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, --
-- MA 02111-1307, USA.                                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

--  This package offers basic types that deal with gdb backtraces related
--  to memory allocation. A memory root (root_id) is a backtrace
--  referencing the actual point of allocation along with counters
--  recording various information concerning allocation at this root.

--  A back trace is composed of Frames (Frame_Id) which themselves are
--  nothing else than a subprogram call at a source location which can be
--  represented by three strings: subprogram name, file name and line
--  number. All the needed strings are entered in a table and referenced
--  through a Name_Id in order to avoid duplication.

with System.Storage_Elements; use System.Storage_Elements;
with Ada.Text_IO;             use Ada.Text_IO;

package Memroot is

   --  Work with instrumented allocation routines
   Gmem_Mode  : Boolean := False;

   --  Simple abstract type for names. A name is a sequence of letters.

   type Name_Id is new Natural;
   No_Name_Id : constant Name_Id := 0;

   function Enter_Name (S : String) return Name_Id;
   function Image      (N : Name_Id) return String;

   --  Simple abstract type for a backtrace frame. A frame is composed by
   --  a subprogram name, a file name and a line reference.

   type Frame_Id is new Natural;
   No_Frame_Id : constant Frame_Id := 0;

   function Enter_Frame (Name, File, Line : Name_Id) return Frame_Id;

   type Frame_Array is array (Natural range <>) of Frame_Id;

   --  Simple abstract type for an allocation root. It is composed by a set
   --  of frames, the number of allocation, the total size of allocated
   --  memory, and the high water mark.  An iterator is also provided to
   --  iterate over all the entered allocation roots.

   type Root_Id is new Natural;
   No_Root_Id : constant Root_Id := 0;

   function Read_BT (BT_Depth : Integer; FT : File_Type) return Root_Id;
   --  Read a backtrace from file FT whose maximum frame number is given by
   --  BT_Depth and returns the corresponding Allocation root.

   function Enter_Root  (Fr : Frame_Array) return Root_Id;
   --  Create an allocation root from the frames that compose it

   function Frames_Of   (B  : Root_Id) return Frame_Array;
   --  Retreives the Frames of the root's backtrace

   procedure Print_BT (B  : Root_Id);
   --  Prints on standard out the backtrace associated with the root B

   function Get_First return Root_Id;
   function Get_Next  return Root_Id;
   --  Iterator to iterate over roots

   procedure Set_Nb_Alloc (B : Root_Id; V : Integer);
   function      Nb_Alloc (B : Root_Id) return Integer;
   --  Access and modify the number of allocation counter associated with
   --  this allocation root. If the value is negative, it means that this is
   --  not an allocation root but a deallocation root (this can only happen
   --  in erroneous situations where there are more frees than allocations).

   procedure Set_Alloc_Size (B : Root_Id; V : Storage_Count);
   function      Alloc_Size (B : Root_Id) return Storage_Count;
   --  Access and modify the total allocated memory counter associated with
   --  this allocation root.

   procedure Set_High_Water_Mark (B : Root_Id; V : Storage_Count);
   function  High_Water_Mark     (B : Root_Id) return Storage_Count;
   --  Access and modify the high water mark associated with this
   --  allocation root. The high water mark is the maximum value, over
   --  time, of the Alloc_Size.

end Memroot;
