------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             R E S T R I C T                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2004 Free Software Foundation, Inc.          --
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
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

with Atree;    use Atree;
with Casing;   use Casing;
with Errout;   use Errout;
with Fname;    use Fname;
with Fname.UF; use Fname.UF;
with Lib;      use Lib;
with Namet;    use Namet;
with Sinput;   use Sinput;
with Uname;    use Uname;

package body Restrict is

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Restriction_Msg (Msg : String; R : String; N : Node_Id);
   --  Output error message at node N with given text, replacing the
   --  '%' in the message with the name of the restriction given as R,
   --  cased according to the current identifier casing. We do not use
   --  the normal insertion mechanism, since this requires an entry
   --  in the Names table, and this table will be locked if we are
   --  generating a message from gigi.

   function Suppress_Restriction_Message (N : Node_Id) return Boolean;
   --  N is the node for a possible restriction violation message, but
   --  the message is to be suppressed if this is an internal file and
   --  this file is not the main unit.

   -------------------
   -- Abort_Allowed --
   -------------------

   function Abort_Allowed return Boolean is
   begin
      if Restrictions.Set (No_Abort_Statements)
        and then Restrictions.Set (Max_Asynchronous_Select_Nesting)
        and then Restrictions.Value (Max_Asynchronous_Select_Nesting) = 0
      then
         return False;
      else
         return True;
      end if;
   end Abort_Allowed;

   ------------------------------------
   -- Check_Elaboration_Code_Allowed --
   ------------------------------------

   procedure Check_Elaboration_Code_Allowed (N : Node_Id) is
   begin
      --  Avoid calling Namet.Unlock/Lock except when there is an error.
      --  Even in the error case it is a bit dubious, either gigi needs
      --  the table locked or it does not! ???

      if Restrictions.Set (No_Elaboration_Code)
        and then not Suppress_Restriction_Message (N)
      then
         Namet.Unlock;
         Check_Restriction (Restriction_Id'(No_Elaboration_Code), N);
         Namet.Lock;
      end if;
   end Check_Elaboration_Code_Allowed;

   ----------------------------------
   -- Check_No_Implicit_Heap_Alloc --
   ----------------------------------

   procedure Check_No_Implicit_Heap_Alloc (N : Node_Id) is
   begin
      Check_Restriction (Restriction_Id'(No_Implicit_Heap_Allocations), N);
   end Check_No_Implicit_Heap_Alloc;

   ---------------------------
   -- Check_Restricted_Unit --
   ---------------------------

   procedure Check_Restricted_Unit (U : Unit_Name_Type; N : Node_Id) is
   begin
      if Suppress_Restriction_Message (N) then
         return;

      elsif Is_Spec_Name (U) then
         declare
            Fnam : constant File_Name_Type :=
                     Get_File_Name (U, Subunit => False);

         begin
            if not Is_Predefined_File_Name (Fnam) then
               return;

            --  Predefined spec, needs checking against list

            else
               --  Pad name to 8 characters with blanks

               Get_Name_String (Fnam);
               Name_Len := Name_Len - 4;

               while Name_Len < 8 loop
                  Name_Len := Name_Len + 1;
                  Name_Buffer (Name_Len) := ' ';
               end loop;

               for J in Unit_Array'Range loop
                  if Name_Len = 8
                    and then Name_Buffer (1 .. 8) = Unit_Array (J).Filenm
                  then
                     Check_Restriction (Unit_Array (J).Res_Id, N);
                  end if;
               end loop;
            end if;
         end;
      end if;
   end Check_Restricted_Unit;

   -----------------------
   -- Check_Restriction --
   -----------------------

   procedure Check_Restriction
     (R : Restriction_Id;
      N : Node_Id;
      V : Uint := Uint_Minus_1)
   is
      Rimage : constant String := Restriction_Id'Image (R);

      VV : Integer;
      --  V converted to integer form. If V is greater than Integer'Last,
      --  it is reset to minus 1 (unknown value).

      procedure Update_Restrictions (Info : in out Restrictions_Info);
      --  Update violation information in Info.Violated and Info.Count

      -------------------------
      -- Update_Restrictions --
      -------------------------

      procedure Update_Restrictions (Info : in out Restrictions_Info) is
      begin
         --  If not violated, set as violated now

         if not Info.Violated (R) then
            Info.Violated (R) := True;

            if R in All_Parameter_Restrictions then
               if VV < 0 then
                  Info.Unknown (R) := True;
                  Info.Count (R) := 1;
               else
                  Info.Count (R) := VV;
               end if;
            end if;

         --  Otherwise if violated already and a parameter restriction,
         --  update count by maximizing or summing depending on restriction.

         elsif R in All_Parameter_Restrictions then

            --  If new value is unknown, result is unknown

            if VV < 0 then
               Info.Unknown (R) := True;

            --  If checked by maximization, do maximization

            elsif R in Checked_Max_Parameter_Restrictions then
               Info.Count (R) := Integer'Max (Info.Count (R), VV);

            --  If checked by adding, do add, checking for overflow

            elsif R in Checked_Add_Parameter_Restrictions then
               declare
                  pragma Unsuppress (Overflow_Check);
               begin
                  Info.Count (R) := Info.Count (R) + VV;
               exception
                  when Constraint_Error =>
                     Info.Count (R) := Integer'Last;
                     Info.Unknown (R) := True;
               end;

            --  Should not be able to come here, known counts should only
            --  occur for restrictions that are Checked_max or Checked_Sum.

            else
               raise Program_Error;
            end if;
         end if;
      end Update_Restrictions;

   --  Start of processing for Check_Restriction

   begin
      if UI_Is_In_Int_Range (V) then
         VV := Integer (UI_To_Int (V));
      else
         VV := -1;
      end if;

      --  Count can only be specified in the checked val parameter case

      pragma Assert (VV < 0 or else R in Checked_Val_Parameter_Restrictions);

      --  Nothing to do if value of zero specified for parameter restriction

      if VV = 0 then
         return;
      end if;

      --  Update current restrictions

      Update_Restrictions (Restrictions);

      --  If in main extended unit, update main restrictions as well

      if Current_Sem_Unit = Main_Unit
        or else In_Extended_Main_Source_Unit (N)
      then
         Update_Restrictions (Main_Restrictions);
      end if;

      --  Nothing to do if restriction message suppressed

      if Suppress_Restriction_Message (N) then
         null;

      --  If restriction not set, nothing to do

      elsif not Restrictions.Set (R) then
         null;

      --  Here if restriction set, check for violation (either this is a
      --  Boolean restriction, or a parameter restriction with a value of
      --  zero and an unknown count, or a parameter restriction with a
      --  known value that exceeds the restriction count).

      elsif R in All_Boolean_Restrictions
        or else (Restrictions.Unknown (R)
                   and then Restrictions.Value (R) = 0)
        or else Restrictions.Count (R) > Restrictions.Value (R)
      then
         Error_Msg_Sloc := Restrictions_Loc (R);

         --  If we have a location for the Restrictions pragma, output it

         if Error_Msg_Sloc > No_Location
           or else Error_Msg_Sloc = System_Location
         then
            if Restriction_Warnings (R) then
               Restriction_Msg ("|violation of restriction %#?", Rimage, N);
            else
               Restriction_Msg ("|violation of restriction %#", Rimage, N);
            end if;

         --  Otherwise we have the case of an implicit restriction
         --  (e.g. a restriction implicitly set by another pragma)

         else
            Restriction_Msg
              ("|violation of implicit restriction %", Rimage, N);
         end if;
      end if;
   end Check_Restriction;

   ----------------------------------------
   -- Cunit_Boolean_Restrictions_Restore --
   ----------------------------------------

   procedure Cunit_Boolean_Restrictions_Restore
     (R : Save_Cunit_Boolean_Restrictions)
   is
   begin
      for J in Cunit_Boolean_Restrictions loop
         Restrictions.Set (J) := R (J);
      end loop;
   end Cunit_Boolean_Restrictions_Restore;

   -------------------------------------
   -- Cunit_Boolean_Restrictions_Save --
   -------------------------------------

   function Cunit_Boolean_Restrictions_Save
     return Save_Cunit_Boolean_Restrictions
   is
      R : Save_Cunit_Boolean_Restrictions;

   begin
      for J in Cunit_Boolean_Restrictions loop
         R (J) := Restrictions.Set (J);
         Restrictions.Set (J) := False;
      end loop;

      return R;
   end Cunit_Boolean_Restrictions_Save;

   ------------------------
   -- Get_Restriction_Id --
   ------------------------

   function Get_Restriction_Id
     (N : Name_Id) return Restriction_Id
   is
   begin
      Get_Name_String (N);
      Set_Casing (All_Upper_Case);

      for J in All_Restrictions loop
         declare
            S : constant String := Restriction_Id'Image (J);
         begin
            if S = Name_Buffer (1 .. Name_Len) then
               return J;
            end if;
         end;
      end loop;

      return Not_A_Restriction_Id;
   end Get_Restriction_Id;

   -------------------------------
   -- No_Exception_Handlers_Set --
   -------------------------------

   function No_Exception_Handlers_Set return Boolean is
   begin
      return Restrictions.Set (No_Exception_Handlers);
   end No_Exception_Handlers_Set;

   ------------------------
   -- Restricted_Profile --
   ------------------------

   --  This implementation must be coordinated with Set_Restricted_Profile

   function Restricted_Profile return Boolean is
   begin
      return     Restrictions.Set (No_Abort_Statements)
        and then Restrictions.Set (No_Asynchronous_Control)
        and then Restrictions.Set (No_Entry_Queue)
        and then Restrictions.Set (No_Task_Hierarchy)
        and then Restrictions.Set (No_Task_Allocators)
        and then Restrictions.Set (No_Dynamic_Priorities)
        and then Restrictions.Set (No_Terminate_Alternatives)
        and then Restrictions.Set (No_Dynamic_Attachment)
        and then Restrictions.Set (No_Protected_Type_Allocators)
        and then Restrictions.Set (No_Local_Protected_Objects)
        and then Restrictions.Set (No_Requeue_Statements)
        and then Restrictions.Set (No_Task_Attributes_Package)
        and then Restrictions.Set (Max_Asynchronous_Select_Nesting)
        and then Restrictions.Set (Max_Task_Entries)
        and then Restrictions.Set (Max_Protected_Entries)
        and then Restrictions.Set (Max_Select_Alternatives)
        and then Restrictions.Value (Max_Asynchronous_Select_Nesting) =  0
        and then Restrictions.Value (Max_Task_Entries)                =  0
        and then Restrictions.Value (Max_Protected_Entries)           <= 1
        and then Restrictions.Value (Max_Select_Alternatives)         =  0;
   end Restricted_Profile;

   ------------------------
   -- Restriction_Active --
   ------------------------

   function Restriction_Active (R : All_Restrictions) return Boolean is
   begin
      return Restrictions.Set (R);
   end Restriction_Active;

   ---------------------
   -- Restriction_Msg --
   ---------------------

   procedure Restriction_Msg (Msg : String; R : String; N : Node_Id) is
      B : String (1 .. Msg'Length + 2 * R'Length + 1);
      P : Natural := 1;

   begin
      Name_Buffer (1 .. R'Last) := R;
      Name_Len := R'Length;
      Set_Casing (Identifier_Casing (Get_Source_File_Index (Sloc (N))));

      P := 0;
      for J in Msg'Range loop
         if Msg (J) = '%' then
            P := P + 1;
            B (P) := '`';

            --  Put characters of image in message, quoting upper case letters

            for J in 1 .. Name_Len loop
               if Name_Buffer (J) in 'A' .. 'Z' then
                  P := P + 1;
                  B (P) := ''';
               end if;

               P := P + 1;
               B (P) := Name_Buffer (J);
            end loop;

            P := P + 1;
            B (P) := '`';

         else
            P := P + 1;
            B (P) := Msg (J);
         end if;
      end loop;

      Error_Msg_N (B (1 .. P), N);
   end Restriction_Msg;

   -------------------
   -- Set_Ravenscar --
   -------------------

   procedure Set_Ravenscar (N : Node_Id) is
   begin
      Set_Restricted_Profile (N);
      Set_Restriction (Simple_Barriers,              N);
      Set_Restriction (No_Select_Statements,         N);
      Set_Restriction (No_Calendar,                  N);
      Set_Restriction (No_Entry_Queue,               N);
      Set_Restriction (No_Relative_Delay,            N);
      Set_Restriction (No_Task_Termination,          N);
      Set_Restriction (No_Implicit_Heap_Allocations, N);
   end Set_Ravenscar;

   ----------------------------
   -- Set_Restricted_Profile --
   ----------------------------

   --  This must be coordinated with Restricted_Profile

   procedure Set_Restricted_Profile (N : Node_Id) is
   begin
      --  Set Boolean restrictions for Restricted Profile

      Set_Restriction (No_Abort_Statements,          N);
      Set_Restriction (No_Asynchronous_Control,      N);
      Set_Restriction (No_Entry_Queue,               N);
      Set_Restriction (No_Task_Hierarchy,            N);
      Set_Restriction (No_Task_Allocators,           N);
      Set_Restriction (No_Dynamic_Priorities,        N);
      Set_Restriction (No_Terminate_Alternatives,    N);
      Set_Restriction (No_Dynamic_Attachment,        N);
      Set_Restriction (No_Protected_Type_Allocators, N);
      Set_Restriction (No_Local_Protected_Objects,   N);
      Set_Restriction (No_Requeue_Statements,        N);
      Set_Restriction (No_Task_Attributes_Package,   N);

      --  Set parameter restrictions

      Set_Restriction (Max_Asynchronous_Select_Nesting, N, 0);
      Set_Restriction (Max_Task_Entries,                N, 0);
      Set_Restriction (Max_Select_Alternatives,         N, 0);
      Set_Restriction (Max_Protected_Entries,           N, 1);
   end Set_Restricted_Profile;

   ---------------------
   -- Set_Restriction --
   ---------------------

   --  Case of Boolean restriction

   procedure Set_Restriction
     (R : All_Boolean_Restrictions;
      N : Node_Id)
   is
   begin
      Restrictions.Set (R) := True;

      --  Set location, but preserve location of system
      --  restriction for nice error msg with run time name

      if Restrictions_Loc (R) /= System_Location then
         Restrictions_Loc (R) := Sloc (N);
      end if;

      --  Record the restriction if we are in the main unit,
      --  or in the extended main unit. The reason that we
      --  test separately for Main_Unit is that gnat.adc is
      --  processed with Current_Sem_Unit = Main_Unit, but
      --  nodes in gnat.adc do not appear to be the extended
      --  main source unit (they probably should do ???)

      if Current_Sem_Unit = Main_Unit
        or else In_Extended_Main_Source_Unit (N)
      then
         if not Restriction_Warnings (R) then
            Main_Restrictions.Set (R) := True;
         end if;
      end if;
   end Set_Restriction;

   --  Case of parameter restriction

   procedure Set_Restriction
     (R : All_Parameter_Restrictions;
      N : Node_Id;
      V : Integer)
   is
   begin
      if Restrictions.Set (R) then
         if V < Restrictions.Value (R) then
            Restrictions.Value (R) := V;
            Restrictions_Loc (R) := Sloc (N);
         end if;

      else
         Restrictions.Set (R) := True;
         Restrictions.Value (R) := V;
         Restrictions_Loc (R) := Sloc (N);
      end if;

      --  Record the restriction if we are in the main unit,
      --  or in the extended main unit. The reason that we
      --  test separately for Main_Unit is that gnat.adc is
      --  processed with Current_Sem_Unit = Main_Unit, but
      --  nodes in gnat.adc do not appear to be the extended
      --  main source unit (they probably should do ???)

      if Current_Sem_Unit = Main_Unit
        or else In_Extended_Main_Source_Unit (N)
      then
         if Main_Restrictions.Set (R) then
            if V < Main_Restrictions.Value (R) then
               Main_Restrictions.Value (R) := V;
            end if;

         elsif not Restriction_Warnings (R) then
            Main_Restrictions.Set (R) := True;
            Main_Restrictions.Value (R) := V;
         end if;
      end if;
   end Set_Restriction;

   ----------------------------------
   -- Suppress_Restriction_Message --
   ----------------------------------

   function Suppress_Restriction_Message (N : Node_Id) return Boolean is
   begin
      --  We only output messages for the extended main source unit

      if In_Extended_Main_Source_Unit (N) then
         return False;

      --  If loaded by rtsfind, then suppress message

      elsif Sloc (N) <= No_Location then
         return True;

      --  Otherwise suppress message if internal file

      else
         return Is_Internal_File_Name (Unit_File_Name (Get_Source_Unit (N)));
      end if;
   end Suppress_Restriction_Message;

   ---------------------
   -- Tasking_Allowed --
   ---------------------

   function Tasking_Allowed return Boolean is
   begin
      return not Restrictions.Set (No_Tasking)
        and then (not Restrictions.Set (Max_Tasks)
                    or else Restrictions.Value (Max_Tasks) > 0);
   end Tasking_Allowed;

end Restrict;
