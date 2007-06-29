------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                                 F M A P                                  --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2001-2007, Free Software Foundation, Inc.         --
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
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

with Opt;    use Opt;
with Osint;  use Osint;
with Output; use Output;
with Table;
with Types;  use Types;

with System.OS_Lib; use System.OS_Lib;

with Unchecked_Conversion;

with GNAT.HTable;

package body Fmap is

   subtype Big_String is String (Positive);
   type Big_String_Ptr is access all Big_String;

   function To_Big_String_Ptr is new Unchecked_Conversion
     (Source_Buffer_Ptr, Big_String_Ptr);

   Max_Buffer : constant := 1_500;
   Buffer : String (1 .. Max_Buffer);
   --  Used to bufferize output when writing to a new mapping file

   Buffer_Last : Natural := 0;
   --  Index of last valid character in Buffer

   type Mapping is record
      Uname : Unit_Name_Type;
      Fname : File_Name_Type;
   end record;

   package File_Mapping is new Table.Table (
     Table_Component_Type => Mapping,
     Table_Index_Type     => Int,
     Table_Low_Bound      => 0,
     Table_Initial        => 1_000,
     Table_Increment      => 1_000,
     Table_Name           => "Fmap.File_Mapping");
   --  Mapping table to map unit names to file names

   package Path_Mapping is new Table.Table (
     Table_Component_Type => Mapping,
     Table_Index_Type     => Int,
     Table_Low_Bound      => 0,
     Table_Initial        => 1_000,
     Table_Increment      => 1_000,
     Table_Name           => "Fmap.Path_Mapping");
   --  Mapping table to map file names to path names

   type Header_Num is range 0 .. 1_000;

   function Hash (F : Unit_Name_Type) return Header_Num;
   --  Function used to compute hash of unit name

   No_Entry : constant Int := -1;
   --  Signals no entry in following table

   package Unit_Hash_Table is new GNAT.HTable.Simple_HTable (
     Header_Num => Header_Num,
     Element    => Int,
     No_Element => No_Entry,
     Key        => Unit_Name_Type,
     Hash       => Hash,
     Equal      => "=");
   --  Hash table to map unit names to file names. Used in conjunction with
   --  table File_Mapping above.

   function Hash (F : File_Name_Type) return Header_Num;
   --  Function used to compute hash of file name

   package File_Hash_Table is new GNAT.HTable.Simple_HTable (
     Header_Num => Header_Num,
     Element    => Int,
     No_Element => No_Entry,
     Key        => File_Name_Type,
     Hash       => Hash,
     Equal      => "=");
   --  Hash table to map file names to path names. Used in conjunction with
   --  table Path_Mapping above.

   Last_In_Table : Int := 0;

   package Forbidden_Names is new GNAT.HTable.Simple_HTable (
     Header_Num => Header_Num,
     Element    => Boolean,
     No_Element => False,
     Key        => File_Name_Type,
     Hash       => Hash,
     Equal      => "=");

   -----------------------------
   -- Add_Forbidden_File_Name --
   -----------------------------

   procedure Add_Forbidden_File_Name (Name : File_Name_Type) is
   begin
      Forbidden_Names.Set (Name, True);
   end Add_Forbidden_File_Name;

   ---------------------
   -- Add_To_File_Map --
   ---------------------

   procedure Add_To_File_Map
     (Unit_Name : Unit_Name_Type;
      File_Name : File_Name_Type;
      Path_Name : File_Name_Type)
   is
   begin
      File_Mapping.Increment_Last;
      Unit_Hash_Table.Set (Unit_Name, File_Mapping.Last);
      File_Mapping.Table (File_Mapping.Last) :=
        (Uname => Unit_Name, Fname => File_Name);
      Path_Mapping.Increment_Last;
      File_Hash_Table.Set (File_Name, Path_Mapping.Last);
      Path_Mapping.Table (Path_Mapping.Last) :=
        (Uname => Unit_Name, Fname => Path_Name);
   end Add_To_File_Map;

   ----------
   -- Hash --
   ----------

   function Hash (F : File_Name_Type) return Header_Num is
   begin
      return Header_Num (Int (F) rem Header_Num'Range_Length);
   end Hash;

   function Hash (F : Unit_Name_Type) return Header_Num is
   begin
      return Header_Num (Int (F) rem Header_Num'Range_Length);
   end Hash;

   ----------------
   -- Initialize --
   ----------------

   procedure Initialize (File_Name : String) is
      Src : Source_Buffer_Ptr;
      Hi  : Source_Ptr;
      BS  : Big_String_Ptr;
      SP  : String_Ptr;

      First : Positive := 1;
      Last  : Natural  := 0;

      Uname : Unit_Name_Type;
      Fname : File_Name_Type;
      Pname : File_Name_Type;

      procedure Empty_Tables;
      --  Remove all entries in case of incorrect mapping file

      function Find_File_Name return File_Name_Type;
      --  Return Error_File_Name for "/", otherwise call Name_Find
      --  What is this about, explanation required ???

      function Find_Unit_Name return Unit_Name_Type;
      --  Return Error_Unit_Name for "/", otherwise call Name_Find
      --  Even more mysterious??? function appeared when Find_Name was split
      --  for the two types, but this routine is definitely called!

      procedure Get_Line;
      --  Get a line from the mapping file

      procedure Report_Truncated;
      --  Report a warning when the mapping file is truncated
      --  (number of lines is not a multiple of 3).

      ------------------
      -- Empty_Tables --
      ------------------

      procedure Empty_Tables is
      begin
         Unit_Hash_Table.Reset;
         File_Hash_Table.Reset;
         Path_Mapping.Set_Last (0);
         File_Mapping.Set_Last (0);
         Last_In_Table := 0;
      end Empty_Tables;

      --------------------
      -- Find_File_Name --
      --------------------

      --  Why is only / illegal, why not \ on windows ???

      function Find_File_Name return File_Name_Type is
      begin
         if Name_Buffer (1 .. Name_Len) = "/" then
            return Error_File_Name;
         else
            return Name_Find;
         end if;
      end Find_File_Name;

      --------------------
      -- Find_Unit_Name --
      --------------------

      function Find_Unit_Name return Unit_Name_Type is
      begin
         return Unit_Name_Type (Find_File_Name);
         --  very odd ???
      end Find_Unit_Name;

      --------------
      -- Get_Line --
      --------------

      procedure Get_Line is
         use ASCII;

      begin
         First := Last + 1;

         --  If not at the end of file, skip the end of line

         while First < SP'Last
           and then (SP (First) = CR
                      or else SP (First) = LF
                      or else SP (First) = EOF)
         loop
            First := First + 1;
         end loop;

         --  If not at the end of file, find the end of this new line

         if First < SP'Last and then SP (First) /= EOF then
            Last := First;

            while Last < SP'Last
              and then SP (Last + 1) /= CR
              and then SP (Last + 1) /= LF
              and then SP (Last + 1) /= EOF
            loop
               Last := Last + 1;
            end loop;

         end if;
      end Get_Line;

      ----------------------
      -- Report_Truncated --
      ----------------------

      procedure Report_Truncated is
      begin
         Write_Str ("warning: mapping file """);
         Write_Str (File_Name);
         Write_Line (""" is truncated");
      end Report_Truncated;

   --  Start of processing for Initialize

   begin
      Empty_Tables;
      Name_Len := File_Name'Length;
      Name_Buffer (1 .. Name_Len) := File_Name;
      Read_Source_File (Name_Enter, 0, Hi, Src, Config);

      if Src = null then
         Write_Str ("warning: could not read mapping file """);
         Write_Str (File_Name);
         Write_Line ("""");

      else
         BS := To_Big_String_Ptr (Src);
         SP := BS (1 .. Natural (Hi))'Unrestricted_Access;

         loop
            --  Get the unit name

            Get_Line;

            --  Exit if end of file has been reached

            exit when First > Last;

            if (Last < First + 2) or else (SP (Last - 1) /= '%')
              or else (SP (Last) /= 's' and then SP (Last) /= 'b')
            then
               Write_Str ("warning: mapping file """);
               Write_Str (File_Name);
               Write_Line (""" is incorrectly formatted");
               Empty_Tables;
               return;
            end if;

            Name_Len := Last - First + 1;
            Name_Buffer (1 .. Name_Len) := SP (First .. Last);
            Uname := Find_Unit_Name;

            --  Get the file name

            Get_Line;

            --  If end of line has been reached, file is truncated

            if First > Last then
               Report_Truncated;
               Empty_Tables;
               return;
            end if;

            Name_Len := Last - First + 1;
            Name_Buffer (1 .. Name_Len) := SP (First .. Last);
            Canonical_Case_File_Name (Name_Buffer (1 .. Name_Len));
            Fname := Find_File_Name;

            --  Get the path name

            Get_Line;

            --  If end of line has been reached, file is truncated

            if First > Last then
               Report_Truncated;
               Empty_Tables;
               return;
            end if;

            Name_Len := Last - First + 1;
            Name_Buffer (1 .. Name_Len) := SP (First .. Last);
            Pname := Find_File_Name;

            --  Check for duplicate entries

            if Unit_Hash_Table.Get (Uname) /= No_Entry then
               Empty_Tables;
               return;
            end if;

            if File_Hash_Table.Get (Fname) /= No_Entry then
               Empty_Tables;
               return;
            end if;

            --  Add the mappings for this unit name

            Add_To_File_Map (Uname, Fname, Pname);
         end loop;
      end if;

      --  Record the length of the two mapping tables

      Last_In_Table := File_Mapping.Last;
   end Initialize;

   ----------------------
   -- Mapped_File_Name --
   ----------------------

   function Mapped_File_Name (Unit : Unit_Name_Type) return File_Name_Type is
      The_Index : constant Int := Unit_Hash_Table.Get (Unit);

   begin
      if The_Index = No_Entry then
         return No_File;
      else
         return File_Mapping.Table (The_Index).Fname;
      end if;
   end Mapped_File_Name;

   ----------------------
   -- Mapped_Path_Name --
   ----------------------

   function Mapped_Path_Name (File : File_Name_Type) return File_Name_Type is
      Index : Int := No_Entry;

   begin
      if Forbidden_Names.Get (File) then
         return Error_File_Name;
      end if;

      Index := File_Hash_Table.Get (File);

      if Index = No_Entry then
         return No_File;
      else
         return Path_Mapping.Table (Index).Fname;
      end if;
   end Mapped_Path_Name;

   --------------------------------
   -- Remove_Forbidden_File_Name --
   --------------------------------

   procedure Remove_Forbidden_File_Name (Name : File_Name_Type) is
   begin
      Forbidden_Names.Set (Name, False);
   end Remove_Forbidden_File_Name;

   ------------------
   -- Reset_Tables --
   ------------------

   procedure Reset_Tables is
   begin
      File_Mapping.Init;
      Path_Mapping.Init;
      Unit_Hash_Table.Reset;
      File_Hash_Table.Reset;
      Forbidden_Names.Reset;
      Last_In_Table := 0;
   end Reset_Tables;

   -------------------------
   -- Update_Mapping_File --
   -------------------------

   procedure Update_Mapping_File (File_Name : String) is
      File    : File_Descriptor;
      N_Bytes : Integer;

      Status : Boolean;
      --  For the call to Close

      procedure Put_Line (Name : Name_Id);
      --  Put Name as a line in the Mapping File

      --------------
      -- Put_Line --
      --------------

      procedure Put_Line (Name : Name_Id) is
      begin
         Get_Name_String (Name);

         --  If the Buffer is full, write it to the file

         if Buffer_Last + Name_Len + 1 > Buffer'Last then
            N_Bytes := Write (File, Buffer (1)'Address, Buffer_Last);

            if N_Bytes < Buffer_Last then
               Fail ("disk full");
            end if;

            Buffer_Last := 0;
         end if;

         --  Add the line to the Buffer

         Buffer (Buffer_Last + 1 .. Buffer_Last + Name_Len) :=
           Name_Buffer (1 .. Name_Len);
         Buffer_Last := Buffer_Last + Name_Len + 1;
         Buffer (Buffer_Last) := ASCII.LF;
      end Put_Line;

   --  Start of Update_Mapping_File

   begin

      --  Only Update if there are new entries in the mappings

      if Last_In_Table < File_Mapping.Last then

         --  If the tables have been emptied, recreate the file.
         --  Otherwise, append to it.

         if Last_In_Table = 0 then
            declare
               Discard : Boolean;

            begin
               Delete_File (File_Name, Discard);
            end;

            File := Create_File (File_Name, Binary);

         else
            File := Open_Read_Write (Name => File_Name, Fmode => Binary);
         end if;

         if File /= Invalid_FD then
            if Last_In_Table > 0 then
               Lseek (File, 0, Seek_End);
            end if;

            for Unit in Last_In_Table + 1 .. File_Mapping.Last loop
               Put_Line (Name_Id (File_Mapping.Table (Unit).Uname));
               Put_Line (Name_Id (File_Mapping.Table (Unit).Fname));
               Put_Line (Name_Id (Path_Mapping.Table (Unit).Fname));
            end loop;

            --  Before closing the file, write the buffer to the file.
            --  It is guaranteed that the Buffer is not empty, because
            --  Put_Line has been called at least 3 times, and after
            --  a call to Put_Line, the Buffer is not empty.

            N_Bytes := Write (File, Buffer (1)'Address, Buffer_Last);

            if N_Bytes < Buffer_Last then
               Fail ("disk full");
            end if;

            Close (File, Status);

            if not Status then
               Fail ("disk full");
            end if;

         elsif not Quiet_Output then
            Write_Str ("warning: could not open mapping file """);
            Write_Str (File_Name);
            Write_Line (""" for update");
         end if;

      end if;
   end Update_Mapping_File;

end Fmap;
