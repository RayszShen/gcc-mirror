------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                               G N A T D L L                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                            $Revision: 1.6 $
--                                                                          --
--          Copyright (C) 1997-2000, Free Software Foundation, Inc.         --
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

--  GNATDLL is a Windows specific tool to build DLL.
--  Both relocatable and non-relocatable DLL are supported

with Ada.Text_IO;
with Ada.Strings.Unbounded;
with Ada.Exceptions;
with Ada.Command_Line;
with GNAT.OS_Lib;
with GNAT.Command_Line;
with Gnatvsn;

with MDLL.Files;
with MDLL.Tools;

procedure Gnatdll is

   use GNAT;
   use Ada;
   use MDLL;
   use Ada.Strings.Unbounded;

   use type OS_Lib.Argument_List;

   procedure Syntax;
   --  print out usage.

   procedure Check (Filename : in String);
   --  check that filename exist.

   procedure Parse_Command_Line;
   --  parse the command line arguments of gnatdll.

   procedure Check_Context;
   --  check the context before runing any commands to build the library.



   Syntax_Error  : exception;
   Context_Error : exception;

   Help          : Boolean := False;

   Version : constant String := Gnatvsn.Gnat_Version_String;

   --  default address for non relocatable DLL (Win32)

   Default_DLL_Address : constant String := "0x11000000";

   Lib_Filename        : Unbounded_String := Null_Unbounded_String;
   Def_Filename        : Unbounded_String := Null_Unbounded_String;
   List_Filename       : Unbounded_String := Null_Unbounded_String;
   DLL_Address         : Unbounded_String :=
     To_Unbounded_String (Default_DLL_Address);

   --  list of objects to put inside the library

   Objects_Files : Argument_List_Access := Null_Argument_List_Access;

   --  for each Ada files specified we keep record of the corresponding
   --  Ali. This list of ali is used to build the binder program.

   Ali_Files     : Argument_List_Access := Null_Argument_List_Access;

   --  a list of options set in the command line.

   Options       : Argument_List_Access := Null_Argument_List_Access;

   --  gnat linker and binder args options

   Largs_Options : Argument_List_Access := Null_Argument_List_Access;
   Bargs_Options : Argument_List_Access := Null_Argument_List_Access;


   type Build_Mode_State is (Import_Lib, Dynamic_Lib, Nil);

   Build_Mode             : Build_Mode_State := Nil;
   Must_Build_Relocatable : Boolean := True;
   Build_Import           : Boolean := True;

   ------------
   -- Syntax --
   ------------

   procedure Syntax is
      use Text_IO;
   begin
      Put_Line ("Usage : gnatdll [options] [list-of-files]");
      New_Line;
      Put_Line
        ("[list-of-files] a list of Ada libraries (.ali) and/or " &
         "foreign object files");
      New_Line;
      Put_Line ("[options] can be");
      Put_Line ("   -h       help - display this message");
      Put_Line ("   -v       verbose");
      Put_Line ("   -q       quiet");
      Put_Line ("   -k       remove @nn suffix from exported names");
      Put_Line ("   -Idir    Specify source and object files search path");

      Put_Line ("   -l file  " &
                "file contains a list-of-files to be added to the library");
      Put_Line ("   -e file  definition file containing exports");
      Put_Line
        ("   -d file  put objects in the relocatable dynamic library <file>");
      Put_Line ("   -a[addr] build non-relocatable DLL at address <addr>");
      Put_Line ("            if <addr> is not specified use " &
                Default_DLL_Address);
      Put_Line ("   -n       no-import - do not create the import library");
      Put_Line ("   -bargs   binder option");
      Put_Line ("   -largs   linker (library builder) option");
   end Syntax;

   -----------
   -- Check --
   -----------

   procedure Check (Filename : in String) is
   begin
      if not OS_Lib.Is_Regular_File (Filename) then
         Exceptions.Raise_Exception (Context_Error'Identity,
                                     "Error: " & Filename & " not found.");
      end if;
   end Check;

   ------------------------
   -- Parse_Command_Line --
   ------------------------

   procedure Parse_Command_Line is

      use GNAT.Command_Line;

      procedure Add_File (Filename : in String);
      --  add one file to the list of file to handle

      procedure Add_Files_From_List (List_Filename : in String);
      --  add the files listed in List_Filename (one by line) to the list
      --  of file to handle

      procedure Ali_To_Object_List;
      --  for each ali file in Afiles set put a corresponding object file in
      --  Ofiles set.

      --  these are arbitrary limits, a better way will be to use linked list.

      Max_Files   : constant := 5_000;
      Max_Options : constant :=   100;

      --  objects files to put in the library

      Ofiles : OS_Lib.Argument_List (1 .. Max_Files);
      O      : Positive := Ofiles'First;

      --  ali files.

      Afiles : OS_Lib.Argument_List (1 .. Max_Files);
      A      : Positive := Afiles'First;

      --  gcc options.

      Gopts  : OS_Lib.Argument_List (1 .. Max_Options);
      G      : Positive := Gopts'First;

      --  largs options

      Lopts  : OS_Lib.Argument_List (1 .. Max_Options);
      L      : Positive := Lopts'First;

      --  bargs options

      Bopts  : OS_Lib.Argument_List (1 .. Max_Options);
      B      : Positive := Bopts'First;

      --------------
      -- Add_File --
      --------------

      procedure Add_File (Filename : in String) is
      begin
         --  others files are to be put inside the dynamic library

         if Files.Is_Ali (Filename) then

            Check (Filename);

            --  record it to generate the binder program when
            --  building dynamic library

            Afiles (A) := new String'(Filename);
            A := A + 1;

         elsif Files.Is_Obj (Filename) then

            Check (Filename);

            --  just record this object file

            Ofiles (O) := new String'(Filename);
            O := O + 1;

         else
            --  unknown file type

            Exceptions.Raise_Exception
              (Syntax_Error'Identity,
               "don't know what to do with " & Filename & " !");
         end if;
      end Add_File;

      -------------------------
      -- Add_Files_From_List --
      -------------------------

      procedure Add_Files_From_List (List_Filename : in String) is
         File   : Text_IO.File_Type;
         Buffer : String (1 .. 500);
         Last   : Natural;
      begin
         Text_IO.Open (File, Text_IO.In_File, List_Filename);

         while not Text_IO.End_Of_File (File) loop
            Text_IO.Get_Line (File, Buffer, Last);
            Add_File (Buffer (1 .. Last));
         end loop;

         Text_IO.Close (File);
      end Add_Files_From_List;

      ------------------------
      -- Ali_To_Object_List --
      ------------------------

      procedure Ali_To_Object_List is
      begin
         for K in 1 .. A - 1 loop
            Ofiles (O) := new String'(Files.Ext_To (Afiles (K).all, "o"));
            O := O + 1;
         end loop;
      end Ali_To_Object_List;

   begin

      Initialize_Option_Scan ('-', False, "bargs largs");

      --  scan gnatdll switches

      loop
         case Getopt ("h v q k a? d: e: l: n I:") is

            when ASCII.Nul =>
               exit;

            when 'h' =>
               Help := True;

            when 'v' =>
               --  verbose mode on.

               MDLL.Verbose := True;
               if MDLL.Quiet then
                  Exceptions.Raise_Exception
                    (Syntax_Error'Identity,
                     "impossible to use -q and -v together.");
               end if;

            when 'q' =>
               --  quiet mode on.

               MDLL.Quiet := True;
               if MDLL.Verbose then
                  Exceptions.Raise_Exception
                    (Syntax_Error'Identity,
                     "impossible to use -v and -q together.");
               end if;

            when 'k' =>

               MDLL.Kill_Suffix := True;

            when 'a' =>

               if Parameter = "" then

                  --  default address for a relocatable dynamic library.
                  --  address for a non relocatable dynamic library.

                  DLL_Address := To_Unbounded_String (Default_DLL_Address);

               else
                  DLL_Address := To_Unbounded_String (Parameter);
               end if;

               Must_Build_Relocatable := False;

            when 'e' =>

               Def_Filename := To_Unbounded_String (Parameter);

            when 'd' =>

               --  build a non relocatable DLL.

               Lib_Filename := To_Unbounded_String (Parameter);

               if Def_Filename = Null_Unbounded_String then
                  Def_Filename := To_Unbounded_String
                    (Files.Ext_To (Parameter, "def"));
               end if;

               Build_Mode := Dynamic_Lib;

            when 'n' =>

               Build_Import := False;

            when 'l' =>
               List_Filename := To_Unbounded_String (Parameter);

            when 'I' =>
               Gopts (G) := new String'("-I" & Parameter);
               G := G + 1;

            when others =>
               raise Invalid_Switch;

         end case;

      end loop;

      --  get parameters

      loop
         declare
            File : constant String := Get_Argument (Do_Expansion => True);
         begin
            exit when File'Length = 0;
            Add_File (File);
         end;
      end loop;

      --  get largs parameters

      Goto_Section ("largs");

      loop
         case Getopt ("*") is

            when ASCII.Nul =>
               exit;

            when others =>
               Lopts (L) := new String'(Full_Switch);
               L := L + 1;

         end case;
      end loop;

      --  get bargs parameters

      Goto_Section ("bargs");

      loop
         case Getopt ("*") is

            when ASCII.Nul =>
               exit;

            when others =>
               Bopts (B) := new String'(Full_Switch);
               B := B + 1;

         end case;
      end loop;

      --  if list filename has been specified parse it

      if List_Filename /= Null_Unbounded_String then
         Add_Files_From_List (To_String (List_Filename));
      end if;

      --  check if the set of parameters are compatible.

      if Build_Mode = Nil and then not Help and then not Verbose then
         Exceptions.Raise_Exception
           (Syntax_Error'Identity,
            "nothing to do.");
      end if;

      --  check if we want to build an import library (option -e and no file
      --  specified)

      if Build_Mode = Dynamic_Lib
        and then A = Afiles'First
        and then O = Ofiles'First
      then
         Build_Mode := Import_Lib;
      end if;

      if O /= Ofiles'First then
         Objects_Files := new OS_Lib.Argument_List'(Ofiles (1 .. O - 1));
      end if;

      if A /= Afiles'First then
         Ali_Files     := new OS_Lib.Argument_List'(Afiles (1 .. A - 1));
      end if;

      if G /= Gopts'First then
         Options       := new OS_Lib.Argument_List'(Gopts (1 .. G - 1));
      end if;

      if L /= Lopts'First then
         Largs_Options := new OS_Lib.Argument_List'(Lopts (1 .. L - 1));
      end if;

      if B /= Bopts'First then
         Bargs_Options := new OS_Lib.Argument_List'(Bopts (1 .. B - 1));
      end if;

   exception

      when Invalid_Switch    =>
         Exceptions.Raise_Exception
           (Syntax_Error'Identity,
            Message => "Invalid Switch " & Full_Switch);

      when Invalid_Parameter =>
         Exceptions.Raise_Exception
           (Syntax_Error'Identity,
            Message => "No parameter for " & Full_Switch);

   end Parse_Command_Line;

   -------------------
   -- Check_Context --
   -------------------

   procedure Check_Context is
   begin

      Check (To_String (Def_Filename));

      --  check that each object file specified exist
      --  raises Context_Error if it does not.

      for F in Objects_Files'Range loop
         Check (Objects_Files (F).all);
      end loop;
   end Check_Context;

begin

   if Ada.Command_Line.Argument_Count = 0 then
      Help := True;
   else
      Parse_Command_Line;
   end if;

   if MDLL.Verbose or else Help then
      Text_IO.New_Line;
      Text_IO.Put_Line ("GNATDLL " & Version & " - Dynamic Libraries Builder");
      Text_IO.New_Line;
   end if;

   MDLL.Tools.Locate;

   if Help
     or else (MDLL.Verbose and then Ada.Command_Line.Argument_Count = 1)
   then
      Syntax;
   else
      Check_Context;

      case Build_Mode is

         when Import_Lib =>
            MDLL.Build_Import_Library (To_String (Lib_Filename),
                                       To_String (Def_Filename));

         when Dynamic_Lib =>
            MDLL.Build_Dynamic_Library
              (Objects_Files.all,
               Ali_Files.all,
               Options.all,
               Bargs_Options.all,
               Largs_Options.all,
               To_String (Lib_Filename),
               To_String (Def_Filename),
               To_String (DLL_Address),
               Build_Import,
               Must_Build_Relocatable);

         when Nil =>
            null;

      end case;

   end if;

   Ada.Command_Line.Set_Exit_Status (Ada.Command_Line.Success);

exception

   when SE : Syntax_Error =>
      Text_IO.Put_Line ("Syntax error : " & Exceptions.Exception_Message (SE));
      Text_IO.New_Line;
      Syntax;
      Ada.Command_Line.Set_Exit_Status (Ada.Command_Line.Failure);

   when E : Tools_Error | Context_Error =>
      Text_IO.Put_Line (Exceptions.Exception_Message (E));
      Ada.Command_Line.Set_Exit_Status (Ada.Command_Line.Failure);

   when others =>
      Text_IO.Put_Line ("gnatdll: INTERNAL ERROR. Please report");
      Ada.Command_Line.Set_Exit_Status (Ada.Command_Line.Failure);

end Gnatdll;
