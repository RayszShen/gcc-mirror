------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             E X P _ C H 1 1                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2008, Free Software Foundation, Inc.         --
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

with Atree;    use Atree;
with Casing;   use Casing;
with Debug;    use Debug;
with Einfo;    use Einfo;
with Elists;   use Elists;
with Errout;   use Errout;
with Exp_Ch7;  use Exp_Ch7;
with Exp_Util; use Exp_Util;
with Namet;    use Namet;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Opt;      use Opt;
with Rtsfind;  use Rtsfind;
with Restrict; use Restrict;
with Rident;   use Rident;
with Sem;      use Sem;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Res;  use Sem_Res;
with Sem_Util; use Sem_Util;
with Sinfo;    use Sinfo;
with Sinput;   use Sinput;
with Snames;   use Snames;
with Stand;    use Stand;
with Stringt;  use Stringt;
with Targparm; use Targparm;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;

package body Exp_Ch11 is

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Warn_No_Exception_Propagation_Active (N : Node_Id);
   --  Generates warning that pragma Restrictions (No_Exception_Propagation)
   --  is in effect. Caller then generates appropriate continuation message.
   --  N is the node on which the warning is placed.

   procedure Warn_If_No_Propagation (N : Node_Id);
   --  Called for an exception raise that is not a local raise (and thus can
   --  not be optimized to a goto. Issues warning if No_Exception_Propagation
   --  restriction is set. N is the node for the raise or equivalent call.

   ---------------------------
   -- Expand_At_End_Handler --
   ---------------------------

   --  For a handled statement sequence that has a cleanup (At_End_Proc
   --  field set), an exception handler of the following form is required:

   --     exception
   --       when all others =>
   --          cleanup call
   --          raise;

   --  Note: this exception handler is treated rather specially by
   --  subsequent expansion in two respects:

   --    The normal call to Undefer_Abort is omitted
   --    The raise call does not do Defer_Abort

   --  This is because the current tasking code seems to assume that
   --  the call to the cleanup routine that is made from an exception
   --  handler for the abort signal is called with aborts deferred.

   --  This expansion is only done if we have front end exception handling.
   --  If we have back end exception handling, then the AT END handler is
   --  left alone, and cleanups (including the exceptional case) are handled
   --  by the back end.

   --  In the front end case, the exception handler described above handles
   --  the exceptional case. The AT END handler is left in the generated tree
   --  and the code generator (e.g. gigi) must still handle proper generation
   --  of cleanup calls for the non-exceptional case.

   procedure Expand_At_End_Handler (HSS : Node_Id; Block : Node_Id) is
      Clean   : constant Entity_Id  := Entity (At_End_Proc (HSS));
      Loc     : constant Source_Ptr := Sloc (Clean);
      Ohandle : Node_Id;
      Stmnts  : List_Id;

   begin
      pragma Assert (Present (Clean));
      pragma Assert (No (Exception_Handlers (HSS)));

      --  Don't expand if back end exception handling active

      if Exception_Mechanism = Back_End_Exceptions then
         return;
      end if;

      --  Don't expand an At End handler if we have already had configurable
      --  run-time violations, since likely this will just be a matter of
      --  generating useless cascaded messages

      if Configurable_Run_Time_Violations > 0 then
         return;
      end if;

      --  Don't expand an At End handler if we are not allowing exceptions
      --  or if exceptions are transformed into local gotos, and never
      --  propagated (No_Exception_Propagation).

      if No_Exception_Handlers_Set then
         return;
      end if;

      if Present (Block) then
         Push_Scope (Block);
      end if;

      Ohandle :=
        Make_Others_Choice (Loc);
      Set_All_Others (Ohandle);

      Stmnts := New_List (
        Make_Procedure_Call_Statement (Loc,
          Name => New_Occurrence_Of (Clean, Loc)));

      --  Generate reraise statement as last statement of AT-END handler,
      --  unless we are under control of No_Exception_Propagation, in which
      --  case no exception propagation is possible anyway, so we do not need
      --  a reraise (the AT END handler in this case is only for normal exits
      --  not for exceptional exits). Also, we flag the Reraise statement as
      --  being part of an AT END handler to prevent signalling this reraise
      --  as a violation of the restriction when it is not set.

      if not Restriction_Active (No_Exception_Propagation) then
         declare
            Rstm : constant Node_Id := Make_Raise_Statement (Loc);
         begin
            Set_From_At_End (Rstm);
            Append_To (Stmnts, Rstm);
         end;
      end if;

      Set_Exception_Handlers (HSS, New_List (
        Make_Implicit_Exception_Handler (Loc,
          Exception_Choices => New_List (Ohandle),
          Statements        => Stmnts)));

      Analyze_List (Stmnts, Suppress => All_Checks);
      Expand_Exception_Handlers (HSS);

      if Present (Block) then
         Pop_Scope;
      end if;
   end Expand_At_End_Handler;

   -------------------------------
   -- Expand_Exception_Handlers --
   -------------------------------

   procedure Expand_Exception_Handlers (HSS : Node_Id) is
      Handlrs       : constant List_Id    := Exception_Handlers (HSS);
      Loc           : constant Source_Ptr := Sloc (HSS);
      Handler       : Node_Id;
      Others_Choice : Boolean;
      Obj_Decl      : Node_Id;
      Next_Handler  : Node_Id;

      procedure Expand_Local_Exception_Handlers;
      --  This procedure handles the expansion of exception handlers for the
      --  optimization of local raise statements into goto statements.

      procedure Prepend_Call_To_Handler
        (Proc : RE_Id;
         Args : List_Id := No_List);
      --  Routine to prepend a call to the procedure referenced by Proc at
      --  the start of the handler code for the current Handler.

      procedure Replace_Raise_By_Goto (Raise_S : Node_Id; Goto_L1 : Node_Id);
      --  Raise_S is a raise statement (possibly expanded, and possibly of the
      --  form of a Raise_xxx_Error node with a condition. This procedure is
      --  called to replace the raise action with the (already analyzed) goto
      --  statement passed as Goto_L1. This procedure also takes care of the
      --  requirement of inserting a Local_Raise call where possible.

      -------------------------------------
      -- Expand_Local_Exception_Handlers --
      -------------------------------------

      --  There are two cases for this transformation. First the case of
      --  explicit raise statements. For this case, the transformation we do
      --  looks like this. Right now we have for example (where L1, L2 are
      --  exception labels)

      --  begin
      --     ...
      --     raise_exception (excep1'identity);  -- was raise excep1
      --     ...
      --     raise_exception (excep2'identity);  -- was raise excep2
      --     ...
      --  exception
      --     when excep1 =>
      --        estmts1
      --     when excep2 =>
      --        estmts2
      --  end;

      --  This gets transformed into:

      --  begin
      --     L1 : label;                        -- marked Exception_Junk
      --     L2 : label;                        -- marked Exception_Junk
      --     L3 : label;                        -- marked Exception_Junk

      --     begin                              -- marked Exception_Junk
      --        ...
      --        local_raise (excep1'address);   -- was raise excep1
      --        goto L1;
      --        ...
      --        local_raise (excep2'address);   -- was raise excep2
      --        goto L2;
      --        ...
      --     exception
      --        when excep1 =>
      --           goto L1;
      --        when excep2 =>
      --           goto L2;
      --     end;

      --     goto L3;        -- skip handler if no raise, marked Exception_Junk

      --     <<L1>>          -- local excep target label, marked Exception_Junk
      --        begin        -- marked Exception_Junk
      --           estmts1
      --        end;
      --        goto L3;     -- marked Exception_Junk

      --     <<L2>>          -- marked Exception_Junk
      --        begin        -- marked Exception_Junk
      --           estmts2
      --        end;
      --        goto L3;     -- marked Exception_Junk
      --     <<L3>>          -- marked Exception_Junk
      --  end;

      --  Note: the reason we wrap the original statement sequence in an
      --  inner block is that there may be raise statements within the
      --  sequence of statements in the handlers, and we must ensure that
      --  these are properly handled, and in particular, such raise statements
      --  must not reenter the same exception handlers.

      --  If the restriction No_Exception_Propagation is in effect, then we
      --  can omit the exception handlers.

      --  begin
      --     L1 : label;                        -- marked Exception_Junk
      --     L2 : label;                        -- marked Exception_Junk
      --     L3 : label;                        -- marked Exception_Junk

      --     begin                              -- marked Exception_Junk
      --        ...
      --        local_raise (excep1'address);   -- was raise excep1
      --        goto L1;
      --        ...
      --        local_raise (excep2'address);   -- was raise excep2
      --        goto L2;
      --        ...
      --     end;

      --     goto L3;        -- skip handler if no raise, marked Exception_Junk

      --     <<L1>>          -- local excep target label, marked Exception_Junk
      --        begin        -- marked Exception_Junk
      --           estmts1
      --        end;
      --        goto L3;     -- marked Exception_Junk

      --     <<L2>>          -- marked Exception_Junk
      --        begin        -- marked Exception_Junk
      --           estmts2
      --        end;

      --     <<L3>>          -- marked Exception_Junk
      --  end;

      --  The second case is for exceptions generated by the back end in one
      --  of three situations:

      --    1. Front end generates N_Raise_xxx_Error node
      --    2. Front end sets Do_xxx_Check flag in subexpression node
      --    3. Back end detects a situation where an exception is appropriate

      --  In all these cases, the current processing in gigi is to generate a
      --  call to the appropriate Rcheck_xx routine (where xx encodes both the
      --  exception message and the exception to be raised, Constraint_Error,
      --  Program_Error, or Storage_Error.

      --  We could handle some subcases of 1 using the same front end expansion
      --  into gotos, but even for case 1, we can't handle all cases, since
      --  generating gotos in the middle of expressions is not possible (it's
      --  possible at the gigi/gcc level, but not at the level of the GNAT
      --  tree).

      --  In any case, it seems easier to have a scheme which handles all three
      --  cases in a uniform manner. So here is how we proceed in this case.

      --  This procedure detects all handlers for these three exceptions,
      --  Constraint_Error, Program_Error and Storage_Error (including WHEN
      --  OTHERS handlers that cover one or more of these cases).

      --  If the handler meets the requirements for being the target of a local
      --  raise, then the front end does the expansion described previously,
      --  creating a label to be used as a goto target to raise the exception.
      --  However, no attempt is made in the front end to convert any related
      --  raise statements into gotos, e.g. all N_Raise_xxx_Error nodes are
      --  left unchanged and passed to the back end.

      --  Instead, the front end generates two nodes

      --     N_Push_Constraint_Error_Label
      --     N_Push_Program_Error_Label
      --     N_Push_Storage_Error_Label

      --       The Push node is generated at the start of the statements
      --       covered by the handler, and has as a parameter the label to be
      --       used as the raise target.

      --     N_Pop_Constraint_Error_Label
      --     N_Pop_Program_Error_Label
      --     N_Pop_Storage_Error_Label

      --       The Pop node is generated at the end of the covered statements
      --       and undoes the effect of the preceding corresponding Push node.

      --  In the case where the handler does NOT meet the requirements, the
      --  front end will still generate the Push and Pop nodes, but the label
      --  field in the Push node will be empty signifying that for this region
      --  of code, no optimization is possible.

      --  The back end must maintain three stacks, one for each exception case,
      --  the Push node pushes an entry onto the corresponding stack, and Pop
      --  node pops off the entry. Then instead of calling Rcheck_nn, if the
      --  corresponding top stack entry has an non-empty label, a goto is
      --  generated. This goto should be preceded by a call to Local_Raise as
      --  described above.

      --  An example of this transformation is as follows, given:

      --  declare
      --    A : Integer range 1 .. 10;
      --  begin
      --    A := B + C;
      --  exception
      --    when Constraint_Error =>
      --       estmts
      --  end;

      --  gets transformed to:

      --  declare
      --    A : Integer range 1 .. 10;

      --  begin
      --     L1 : label;
      --     L2 : label;

      --     begin
      --        %push_constraint_error_label (L1)
      --        R1b : constant long_long_integer := long_long_integer?(b) +
      --          long_long_integer?(c);
      --        [constraint_error when
      --          not (R1b in -16#8000_0000# .. 16#7FFF_FFFF#)
      --          "overflow check failed"]
      --        a := integer?(R1b);
      --        %pop_constraint_error_Label

      --     exception
      --        ...
      --        when constraint_error =>
      --           goto L1;
      --     end;

      --     goto L2;       -- skip handler when exception not raised
      --     <<L1>>         -- target label for local exception
      --     estmts
      --     <<L2>>
      --  end;

      --  Note: the generated labels and goto statements all have the flag
      --  Exception_Junk set True, so that Sem_Ch6.Check_Returns will ignore
      --  this generated exception stuff when checking for missing return
      --  statements (see circuitry in Check_Statement_Sequence).

      --  Note: All of the processing described above occurs only if
      --  restriction No_Exception_Propagation applies or debug flag .g is
      --  enabled.

      CE_Locally_Handled : Boolean := False;
      SE_Locally_Handled : Boolean := False;
      PE_Locally_Handled : Boolean := False;
      --  These three flags indicate whether a handler for the corresponding
      --  exception (CE=Constraint_Error, SE=Storage_Error, PE=Program_Error)
      --  is present. If so the switch is set to True, the Exception_Label
      --  field of the corresponding handler is set, and appropriate Push
      --  and Pop nodes are inserted into the code.

      Local_Expansion_Required : Boolean := False;
      --  Set True if we have at least one handler requiring local raise
      --  expansion as described above.

      procedure Expand_Local_Exception_Handlers is

         procedure Add_Exception_Label (H : Node_Id);
         --  H is an exception handler. First check for an Exception_Label
         --  already allocated for H. If none, allocate one, set the field in
         --  the handler node, add the label declaration, and set the flag
         --  Local_Expansion_Required. Note: if Local_Raise_Not_OK is set
         --  the call has no effect and Exception_Label is left empty.

         procedure Add_Label_Declaration (L : Entity_Id);
         --  Add an implicit declaration of the given label to the declaration
         --  list in the parent of the current sequence of handled statements.

         generic
            Exc_Locally_Handled : in out Boolean;
            --  Flag indicating whether a local handler for this exception
            --  has already been generated.

            with function Make_Push_Label (Loc : Source_Ptr) return Node_Id;
            --  Function to create a Push_xxx_Label node

            with function Make_Pop_Label (Loc : Source_Ptr) return Node_Id;
            --  Function to create a Pop_xxx_Label node

         procedure Generate_Push_Pop (H : Node_Id);
         --  Common code for Generate_Push_Pop_xxx below, used to generate an
         --  exception label and Push/Pop nodes for Constraint_Error,
         --  Program_Error, or Storage_Error.

         -------------------------
         -- Add_Exception_Label --
         -------------------------

         procedure Add_Exception_Label (H : Node_Id) is
         begin
            if No (Exception_Label (H))
              and then not Local_Raise_Not_OK (H)
              and then not Special_Exception_Package_Used
            then
               Local_Expansion_Required := True;

               declare
                  L : constant Entity_Id :=
                        Make_Defining_Identifier (Sloc (H),
                          Chars => New_Internal_Name ('L'));
               begin
                  Set_Exception_Label (H, L);
                  Add_Label_Declaration (L);
               end;
            end if;
         end Add_Exception_Label;

         ---------------------------
         -- Add_Label_Declaration --
         ---------------------------

         procedure Add_Label_Declaration (L : Entity_Id) is
            P : constant Node_Id := Parent (HSS);

            Decl_L : constant Node_Id :=
                       Make_Implicit_Label_Declaration (Loc,
                         Defining_Identifier => L);

         begin
            if Declarations (P) = No_List then
               Set_Declarations (P, Empty_List);
            end if;

            Append (Decl_L, Declarations (P));
            Analyze (Decl_L);
         end Add_Label_Declaration;

         -----------------------
         -- Generate_Push_Pop --
         -----------------------

         procedure Generate_Push_Pop (H : Node_Id) is
         begin
            if Exc_Locally_Handled then
               return;
            else
               Exc_Locally_Handled := True;
            end if;

            Add_Exception_Label (H);

            declare
               F : constant Node_Id := First (Statements (HSS));
               L : constant Node_Id := Last  (Statements (HSS));

               Push : constant Node_Id := Make_Push_Label (Sloc (F));
               Pop  : constant Node_Id := Make_Pop_Label  (Sloc (L));

            begin
               --  We make sure that a call to Get_Local_Raise_Call_Entity is
               --  made during front end processing, so that when we need it
               --  in the back end, it will already be available and loaded.

               Discard_Node (Get_Local_Raise_Call_Entity);

               --  Prepare and insert Push and Pop nodes

               Set_Exception_Label (Push, Exception_Label (H));
               Insert_Before (F, Push);
               Set_Analyzed (Push);

               Insert_After (L, Pop);
               Set_Analyzed (Pop);
            end;
         end Generate_Push_Pop;

         --  Local declarations

         Loc    : constant Source_Ptr := Sloc (HSS);
         Stmts  : List_Id := No_List;
         Choice : Node_Id;
         Excep  : Entity_Id;

         procedure Generate_Push_Pop_For_Constraint_Error is
           new Generate_Push_Pop
             (Exc_Locally_Handled => CE_Locally_Handled,
              Make_Push_Label     => Make_Push_Constraint_Error_Label,
              Make_Pop_Label      => Make_Pop_Constraint_Error_Label);
         --  If no Push/Pop has been generated for CE yet, then set the flag
         --  CE_Locally_Handled, allocate an Exception_Label for handler H (if
         --  not already done), and generate Push/Pop nodes for the exception
         --  label at the start and end of the statements of HSS.

         procedure Generate_Push_Pop_For_Program_Error is
           new Generate_Push_Pop
             (Exc_Locally_Handled => PE_Locally_Handled,
              Make_Push_Label     => Make_Push_Program_Error_Label,
              Make_Pop_Label      => Make_Pop_Program_Error_Label);
         --  If no Push/Pop has been generated for PE yet, then set the flag
         --  PE_Locally_Handled, allocate an Exception_Label for handler H (if
         --  not already done), and generate Push/Pop nodes for the exception
         --  label at the start and end of the statements of HSS.

         procedure Generate_Push_Pop_For_Storage_Error is
           new Generate_Push_Pop
             (Exc_Locally_Handled => SE_Locally_Handled,
              Make_Push_Label     => Make_Push_Storage_Error_Label,
              Make_Pop_Label      => Make_Pop_Storage_Error_Label);
         --  If no Push/Pop has been generated for SE yet, then set the flag
         --  SE_Locally_Handled, allocate an Exception_Label for handler H (if
         --  not already done), and generate Push/Pop nodes for the exception
         --  label at the start and end of the statements of HSS.

      --  Start of processing for Expand_Local_Exception_Handlers

      begin
         --  No processing if all exception handlers will get removed

         if Debug_Flag_Dot_X then
            return;
         end if;

         --  See for each handler if we have any local raises to expand

         Handler := First_Non_Pragma (Handlrs);
         while Present (Handler) loop

            --  Note, we do not test Local_Raise_Not_OK here, because in the
            --  case of Push/Pop generation we want to generate push with a
            --  null label. The Add_Exception_Label routine has no effect if
            --  Local_Raise_Not_OK is set, so this works as required.

            if Present (Local_Raise_Statements (Handler)) then
               Add_Exception_Label (Handler);
            end if;

            --  If we are doing local raise to goto optimization (restriction
            --  No_Exception_Propagation set or debug flag .g set), then check
            --  to see if handler handles CE, PE, SE and if so generate the
            --  appropriate push/pop sequence for the back end.

            if (Debug_Flag_Dot_G
                 or else Restriction_Active (No_Exception_Propagation))
              and then Has_Local_Raise (Handler)
            then
               Choice := First (Exception_Choices (Handler));
               while Present (Choice) loop
                  if Nkind (Choice) = N_Others_Choice
                    and then not All_Others (Choice)
                  then
                     Generate_Push_Pop_For_Constraint_Error (Handler);
                     Generate_Push_Pop_For_Program_Error    (Handler);
                     Generate_Push_Pop_For_Storage_Error    (Handler);

                  elsif Is_Entity_Name (Choice) then
                     Excep := Get_Renamed_Entity (Entity (Choice));

                     if Excep = Standard_Constraint_Error then
                        Generate_Push_Pop_For_Constraint_Error (Handler);
                     elsif Excep = Standard_Program_Error then
                        Generate_Push_Pop_For_Program_Error    (Handler);
                     elsif Excep = Standard_Storage_Error then
                        Generate_Push_Pop_For_Storage_Error    (Handler);
                     end if;
                  end if;

                  Next (Choice);
               end loop;
            end if;

            Next_Non_Pragma (Handler);
         end loop;

         --  Nothing to do if no handlers requiring the goto transformation

         if not (Local_Expansion_Required) then
            return;
         end if;

         --  Prepare to do the transformation

         declare
            --  L3 is the label to exit the HSS

            L3_Dent : constant Entity_Id :=
                        Make_Defining_Identifier (Loc,
                          Chars => New_Internal_Name ('L'));

            Labl_L3 : constant Node_Id :=
                        Make_Label (Loc,
                          Identifier => New_Occurrence_Of (L3_Dent, Loc));

            Blk_Stm : Node_Id;
            Relmt   : Elmt_Id;

         begin
            Set_Exception_Junk (Labl_L3);
            Add_Label_Declaration (L3_Dent);

            --  Wrap existing statements and handlers in an inner block

            Blk_Stm :=
              Make_Block_Statement (Loc,
                Handled_Statement_Sequence => Relocate_Node (HSS));
            Set_Exception_Junk (Blk_Stm);

            Rewrite (HSS,
              Make_Handled_Sequence_Of_Statements (Loc,
                Statements => New_List (Blk_Stm)));

            --  Set block statement as analyzed, we don't want to actually call
            --  Analyze on this block, it would cause a recursion in exception
            --  handler processing which would mess things up.

            Set_Analyzed (Blk_Stm);

            --  Now loop through the exception handlers to deal with those that
            --  are targets of local raise statements.

            Handler := First_Non_Pragma (Handlrs);
            while Present (Handler) loop
               if Present (Exception_Label (Handler)) then

                  --  This handler needs the goto expansion

                  declare
                     Loc : constant Source_Ptr := Sloc (Handler);

                     --  L1 is the start label for this handler

                     L1_Dent : constant Entity_Id := Exception_Label (Handler);

                     Labl_L1 : constant Node_Id :=
                                 Make_Label (Loc,
                                   Identifier =>
                                     New_Occurrence_Of (L1_Dent, Loc));

                     --  Jump to L1 to be used as replacement for the original
                     --  handler (used in the case where exception propagation
                     --  may still occur).

                     Name_L1 : constant Node_Id :=
                                 New_Occurrence_Of (L1_Dent, Loc);

                     Goto_L1 : constant Node_Id :=
                                 Make_Goto_Statement (Loc,
                                   Name => Name_L1);

                     --  Jump to L3 to be used at the end of handler

                     Name_L3 : constant Node_Id :=
                                 New_Occurrence_Of (L3_Dent, Loc);

                     Goto_L3 : constant Node_Id :=
                                 Make_Goto_Statement (Loc,
                                   Name => Name_L3);

                     H_Stmts : constant List_Id := Statements (Handler);

                  begin
                     Set_Exception_Junk (Labl_L1);
                     Set_Exception_Junk (Goto_L3);

                     --  Note: we do NOT set Exception_Junk in Goto_L1, since
                     --  this is a real transfer of control that we want the
                     --  Sem_Ch6.Check_Returns procedure to recognize properly.

                     --  Replace handler by a goto L1. We can mark this as
                     --  analyzed since it is fully formed, and we don't
                     --  want it going through any further checks. We save
                     --  the last statement location in the goto L1 node for
                     --  the benefit of Sem_Ch6.Check_Returns.

                     Set_Statements (Handler, New_List (Goto_L1));
                     Set_Analyzed (Goto_L1);
                     Set_Etype (Name_L1, Standard_Void_Type);

                     --  Now replace all the raise statements by goto L1

                     if Present (Local_Raise_Statements (Handler)) then
                        Relmt := First_Elmt (Local_Raise_Statements (Handler));
                        while Present (Relmt) loop
                           declare
                              Raise_S : constant Node_Id := Node (Relmt);

                              Name_L1 : constant Node_Id :=
                                          New_Occurrence_Of (L1_Dent, Loc);

                              Goto_L1 : constant Node_Id :=
                                          Make_Goto_Statement (Loc,
                                            Name => Name_L1);

                           begin
                              --  Replace raise by goto L1

                              Set_Analyzed (Goto_L1);
                              Set_Etype (Name_L1, Standard_Void_Type);
                              Replace_Raise_By_Goto (Raise_S, Goto_L1);
                           end;

                           Next_Elmt (Relmt);
                        end loop;
                     end if;

                     --  Add a goto L3 at end of statement list in block. The
                     --  first time, this is what skips over the exception
                     --  handlers in the normal case. Subsequent times, it
                     --  terminates the execution of the previous handler code,
                     --  and skips subsequent handlers.

                     Stmts := Statements (HSS);

                     Insert_After (Last (Stmts), Goto_L3);
                     Set_Analyzed (Goto_L3);
                     Set_Etype (Name_L3, Standard_Void_Type);

                     --  Now we drop the label that marks the handler start,
                     --  followed by the statements of the handler.

                     Set_Etype (Identifier (Labl_L1), Standard_Void_Type);

                     Insert_After_And_Analyze (Last (Stmts), Labl_L1);

                     declare
                        Loc : constant Source_Ptr := Sloc (First (H_Stmts));
                        Blk : constant Node_Id :=
                                Make_Block_Statement (Loc,
                                  Handled_Statement_Sequence =>
                                    Make_Handled_Sequence_Of_Statements (Loc,
                                      Statements => H_Stmts));
                     begin
                        Set_Exception_Junk (Blk);
                        Insert_After_And_Analyze (Last (Stmts), Blk);
                     end;
                  end;

                  --  Here if we have local raise statements but the handler is
                  --  not suitable for processing with a local raise. In this
                  --  case we have to generate possible diagnostics.

               elsif Has_Local_Raise (Handler)
                 and then Local_Raise_Statements (Handler) /= No_Elist
               then
                  Relmt := First_Elmt (Local_Raise_Statements (Handler));
                  while Present (Relmt) loop
                     Warn_If_No_Propagation (Node (Relmt));
                     Next_Elmt (Relmt);
                  end loop;
               end if;

               Next (Handler);
            end loop;

            --  Only remaining step is to drop the L3 label and we are done

            Set_Etype (Identifier (Labl_L3), Standard_Void_Type);

            --  If we had at least one handler, then we drop the label after
            --  the last statement of that handler.

            if Stmts /= No_List then
               Insert_After_And_Analyze (Last (Stmts), Labl_L3);

            --  Otherwise we have removed all the handlers (this results from
            --  use of pragma Restrictions (No_Exception_Propagation), and we
            --  drop the label at the end of the statements of the HSS.

            else
               Insert_After_And_Analyze (Last (Statements (HSS)), Labl_L3);
            end if;

            return;
         end;
      end Expand_Local_Exception_Handlers;

      -----------------------------
      -- Prepend_Call_To_Handler --
      -----------------------------

      procedure Prepend_Call_To_Handler
        (Proc : RE_Id;
         Args : List_Id := No_List)
      is
         Ent : constant Entity_Id := RTE (Proc);

      begin
         --  If we have no Entity, then we are probably in no run time mode
         --  or some weird error has occurred. In either case do do nothing!

         if Present (Ent) then
            declare
               Call : constant Node_Id :=
                        Make_Procedure_Call_Statement (Loc,
                          Name => New_Occurrence_Of (RTE (Proc), Loc),
                          Parameter_Associations => Args);

            begin
               Prepend_To (Statements (Handler), Call);
               Analyze (Call, Suppress => All_Checks);
            end;
         end if;
      end Prepend_Call_To_Handler;

      ---------------------------
      -- Replace_Raise_By_Goto --
      ---------------------------

      procedure Replace_Raise_By_Goto (Raise_S : Node_Id; Goto_L1 : Node_Id) is
         Loc   : constant Source_Ptr := Sloc (Raise_S);
         Excep : Entity_Id;
         LR    : Node_Id;
         Cond  : Node_Id;
         Orig  : Node_Id;

      begin
         --  If we have a null statement, it means that there is no replacement
         --  needed (typically this results from a suppressed check).

         if Nkind (Raise_S) = N_Null_Statement then
            return;

         --  Test for Raise_xxx_Error

         elsif Nkind (Raise_S) = N_Raise_Constraint_Error then
            Excep := Standard_Constraint_Error;
            Cond  := Condition (Raise_S);

         elsif Nkind (Raise_S) = N_Raise_Storage_Error then
            Excep := Standard_Storage_Error;
            Cond := Condition (Raise_S);

         elsif Nkind (Raise_S) = N_Raise_Program_Error then
            Excep := Standard_Program_Error;
            Cond := Condition (Raise_S);

            --  The only other possibility is a node that is or used to be a
            --  simple raise statement.

         else
            Orig := Original_Node (Raise_S);
            pragma Assert (Nkind (Orig) = N_Raise_Statement
                             and then Present (Name (Orig))
                             and then No (Expression (Orig)));
            Excep := Entity (Name (Orig));
            Cond := Empty;
         end if;

         --  Here Excep is the exception to raise, and Cond is the condition
         --  First prepare the call to Local_Raise (excep'address).

         if RTE_Available (RE_Local_Raise) then
            LR :=
              Make_Procedure_Call_Statement (Loc,
                Name => New_Occurrence_Of (RTE (RE_Local_Raise), Loc),
                Parameter_Associations => New_List (
                  Unchecked_Convert_To (RTE (RE_Address),
                    Make_Attribute_Reference (Loc,
                      Prefix         => New_Occurrence_Of (Excep, Loc),
                      Attribute_Name => Name_Identity))));

            --  Use null statement if Local_Raise not available

         else
            LR :=
              Make_Null_Statement (Loc);
         end if;

         --  If there is no condition, we rewrite as

         --    begin
         --       Local_Raise (excep'Identity);
         --       goto L1;
         --    end;

         if No (Cond) then
            Rewrite (Raise_S,
              Make_Block_Statement (Loc,
                Handled_Statement_Sequence =>
                  Make_Handled_Sequence_Of_Statements (Loc,
                    Statements => New_List (LR, Goto_L1))));
            Set_Exception_Junk (Raise_S);

         --  If there is a condition, we rewrite as

         --    if condition then
         --       Local_Raise (excep'Identity);
         --       goto L1;
         --    end if;

         else
            Rewrite (Raise_S,
              Make_If_Statement (Loc,
                Condition       => Cond,
                Then_Statements => New_List (LR, Goto_L1)));
         end if;

         Analyze (Raise_S);
      end Replace_Raise_By_Goto;

   --  Start of processing for Expand_Exception_Handlers

   begin
      Expand_Local_Exception_Handlers;

      --  Loop through handlers

      Handler := First_Non_Pragma (Handlrs);
      Handler_Loop : while Present (Handler) loop
         Next_Handler := Next_Non_Pragma (Handler);

         --  Remove source handler if gnat debug flag .x is set

         if Debug_Flag_Dot_X and then Comes_From_Source (Handler) then
            Remove (Handler);

         --  Remove handler if no exception propagation, generating a warning
         --  if a source generated handler was not the target of a local raise.

         else
            if Restriction_Active (No_Exception_Propagation)
              and then not Has_Local_Raise (Handler)
              and then Comes_From_Source (Handler)
              and then Warn_On_Non_Local_Exception
            then
               Warn_No_Exception_Propagation_Active (Handler);
               Error_Msg_N
                 ("\?this handler can never be entered, and has been removed",
                  Handler);
            end if;

            if No_Exception_Propagation_Active then
               Remove (Handler);

            --  Exception handler is active and retained and must be processed

            else
               --  If an exception occurrence is present, then we must declare
               --  it and initialize it from the value stored in the TSD

               --     declare
               --        name : Exception_Occurrence;
               --     begin
               --        Save_Occurrence (name, Get_Current_Excep.all)
               --        ...
               --     end;

               if Present (Choice_Parameter (Handler)) then
                  declare
                     Cparm : constant Entity_Id  := Choice_Parameter (Handler);
                     Cloc  : constant Source_Ptr := Sloc (Cparm);
                     Hloc  : constant Source_Ptr := Sloc (Handler);
                     Save  : Node_Id;

                  begin
                     Save :=
                       Make_Procedure_Call_Statement (Loc,
                         Name =>
                           New_Occurrence_Of (RTE (RE_Save_Occurrence), Loc),
                         Parameter_Associations => New_List (
                           New_Occurrence_Of (Cparm, Cloc),
                           Make_Explicit_Dereference (Loc,
                             Make_Function_Call (Loc,
                               Name => Make_Explicit_Dereference (Loc,
                                 New_Occurrence_Of
                                   (RTE (RE_Get_Current_Excep), Loc))))));

                     Mark_Rewrite_Insertion (Save);
                     Prepend (Save, Statements (Handler));

                     Obj_Decl :=
                       Make_Object_Declaration
                         (Cloc,
                          Defining_Identifier => Cparm,
                          Object_Definition   =>
                            New_Occurrence_Of
                              (RTE (RE_Exception_Occurrence), Cloc));
                     Set_No_Initialization (Obj_Decl, True);

                     Rewrite (Handler,
                       Make_Exception_Handler (Hloc,
                         Choice_Parameter  => Empty,
                         Exception_Choices => Exception_Choices (Handler),

                         Statements => New_List (
                           Make_Block_Statement (Hloc,
                             Declarations => New_List (Obj_Decl),
                             Handled_Statement_Sequence =>
                               Make_Handled_Sequence_Of_Statements (Hloc,
                                 Statements => Statements (Handler))))));

                     --  Local raise statements can't occur, since exception
                     --  handlers with choice parameters are not allowed when
                     --  No_Exception_Propagation applies, so set attributes
                     --  accordingly.

                     Set_Local_Raise_Statements (Handler, No_Elist);
                     Set_Local_Raise_Not_OK (Handler);

                     Analyze_List
                       (Statements (Handler), Suppress => All_Checks);
                  end;
               end if;

               --  The processing at this point is rather different for the JVM
               --  case, so we completely separate the processing.

               --  For the VM case, we unconditionally call Update_Exception,
               --  passing a call to the intrinsic Current_Target_Exception
               --  (see JVM/.NET versions of Ada.Exceptions for details).

               if VM_Target /= No_VM then
                  declare
                     Arg : constant Node_Id :=
                             Make_Function_Call (Loc,
                               Name =>
                                 New_Occurrence_Of
                                   (RTE (RE_Current_Target_Exception), Loc));
                  begin
                     Prepend_Call_To_Handler
                       (RE_Update_Exception, New_List (Arg));
                  end;

                  --  For the normal case, we have to worry about the state of
                  --  abort deferral. Generally, we defer abort during runtime
                  --  handling of exceptions. When control is passed to the
                  --  handler, then in the normal case we undefer aborts. In
                  --  any case this entire handling is relevant only if aborts
                  --  are allowed!

               elsif Abort_Allowed then

                  --  There are some special cases in which we do not do the
                  --  undefer. In particular a finalization (AT END) handler
                  --  wants to operate with aborts still deferred.

                  --  We also suppress the call if this is the special handler
                  --  for Abort_Signal, since if we are aborting, we want to
                  --  keep aborts deferred (one abort is enough).

                  --  If abort really needs to be deferred the expander must
                  --  add this call explicitly, see
                  --  Expand_N_Asynchronous_Select.

                  Others_Choice :=
                    Nkind (First (Exception_Choices (Handler))) =
                                                         N_Others_Choice;

                  if (Others_Choice
                       or else Entity (First (Exception_Choices (Handler))) /=
                                                         Stand.Abort_Signal)
                    and then not
                      (Others_Choice
                        and then
                          All_Others (First (Exception_Choices (Handler))))
                    and then Abort_Allowed
                  then
                     Prepend_Call_To_Handler (RE_Abort_Undefer);
                  end if;
               end if;
            end if;
         end if;

         Handler := Next_Handler;
      end loop Handler_Loop;

      --  If all handlers got removed, then remove the list. Note we cannot
      --  reference HSS here, since expanding local handlers may have buried
      --  the handlers in an inner block.

      if Is_Empty_List (Handlrs) then
         Set_Exception_Handlers (Parent (Handlrs), No_List);
      end if;
   end Expand_Exception_Handlers;

   ------------------------------------
   -- Expand_N_Exception_Declaration --
   ------------------------------------

   --  Generates:
   --     exceptE : constant String := "A.B.EXCEP";   -- static data
   --     except : exception_data :=  (
   --                    Handled_By_Other => False,
   --                    Lang             => 'A',
   --                    Name_Length      => exceptE'Length,
   --                    Full_Name        => exceptE'Address,
   --                    HTable_Ptr       => null,
   --                    Import_Code      => 0,
   --                    Raise_Hook       => null,
   --                    );

   --  (protecting test only needed if not at library level)
   --
   --     exceptF : Boolean := True --  static data
   --     if exceptF then
   --        exceptF := False;
   --        Register_Exception (except'Unchecked_Access);
   --     end if;

   procedure Expand_N_Exception_Declaration (N : Node_Id) is
      Loc     : constant Source_Ptr := Sloc (N);
      Id      : constant Entity_Id  := Defining_Identifier (N);
      L       : List_Id             := New_List;
      Flag_Id : Entity_Id;

      Name_Exname : constant Name_Id := New_External_Name (Chars (Id), 'E');
      Exname      : constant Node_Id :=
                      Make_Defining_Identifier (Loc, Name_Exname);

   begin
      --  There is no expansion needed when compiling for the JVM since the
      --  JVM has a built-in exception mechanism. See 4jexcept.ads for details.

      if VM_Target /= No_VM then
         return;
      end if;

      --  Definition of the external name: nam : constant String := "A.B.NAME";

      Insert_Action (N,
        Make_Object_Declaration (Loc,
          Defining_Identifier => Exname,
          Constant_Present    => True,
          Object_Definition   => New_Occurrence_Of (Standard_String, Loc),
          Expression => Make_String_Literal (Loc, Full_Qualified_Name (Id))));

      Set_Is_Statically_Allocated (Exname);

      --  Create the aggregate list for type Standard.Exception_Type:
      --  Handled_By_Other component: False

      Append_To (L, New_Occurrence_Of (Standard_False, Loc));

      --  Lang component: 'A'

      Append_To (L,
        Make_Character_Literal (Loc,
          Chars              =>  Name_uA,
          Char_Literal_Value =>  UI_From_Int (Character'Pos ('A'))));

      --  Name_Length component: Nam'Length

      Append_To (L,
        Make_Attribute_Reference (Loc,
          Prefix         => New_Occurrence_Of (Exname, Loc),
          Attribute_Name => Name_Length));

      --  Full_Name component: Standard.A_Char!(Nam'Address)

      Append_To (L, Unchecked_Convert_To (Standard_A_Char,
        Make_Attribute_Reference (Loc,
          Prefix         => New_Occurrence_Of (Exname, Loc),
          Attribute_Name => Name_Address)));

      --  HTable_Ptr component: null

      Append_To (L, Make_Null (Loc));

      --  Import_Code component: 0

      Append_To (L, Make_Integer_Literal (Loc, 0));

      --  Raise_Hook component: null

      Append_To (L, Make_Null (Loc));

      Set_Expression (N, Make_Aggregate (Loc, Expressions => L));
      Analyze_And_Resolve (Expression (N), Etype (Id));

      --  Register_Exception (except'Unchecked_Access);

      if not No_Exception_Handlers_Set
        and then not Restriction_Active (No_Exception_Registration)
      then
         L := New_List (
                Make_Procedure_Call_Statement (Loc,
                  Name => New_Occurrence_Of (RTE (RE_Register_Exception), Loc),
                  Parameter_Associations => New_List (
                    Unchecked_Convert_To (RTE (RE_Exception_Data_Ptr),
                      Make_Attribute_Reference (Loc,
                        Prefix         => New_Occurrence_Of (Id, Loc),
                        Attribute_Name => Name_Unrestricted_Access)))));

         Set_Register_Exception_Call (Id, First (L));

         if not Is_Library_Level_Entity (Id) then
            Flag_Id :=  Make_Defining_Identifier (Loc,
                          New_External_Name (Chars (Id), 'F'));

            Insert_Action (N,
              Make_Object_Declaration (Loc,
                Defining_Identifier => Flag_Id,
                Object_Definition   =>
                  New_Occurrence_Of (Standard_Boolean, Loc),
                Expression          =>
                  New_Occurrence_Of (Standard_True, Loc)));

            Set_Is_Statically_Allocated (Flag_Id);

            Append_To (L,
              Make_Assignment_Statement (Loc,
                Name       => New_Occurrence_Of (Flag_Id, Loc),
                Expression => New_Occurrence_Of (Standard_False, Loc)));

            Insert_After_And_Analyze (N,
              Make_Implicit_If_Statement (N,
                Condition       => New_Occurrence_Of (Flag_Id, Loc),
                Then_Statements => L));

         else
            Insert_List_After_And_Analyze (N, L);
         end if;
      end if;
   end Expand_N_Exception_Declaration;

   ---------------------------------------------
   -- Expand_N_Handled_Sequence_Of_Statements --
   ---------------------------------------------

   procedure Expand_N_Handled_Sequence_Of_Statements (N : Node_Id) is
   begin
      --  Expand exception handlers

      if Present (Exception_Handlers (N))
        and then not Restriction_Active (No_Exception_Handlers)
      then
         Expand_Exception_Handlers (N);
      end if;

      --  If local exceptions are being expanded, the previous call will
      --  have rewritten the construct as a block and reanalyzed it. No
      --  further expansion is needed.

      if Analyzed (N) then
         return;
      end if;

      --  Add clean up actions if required

      if Nkind (Parent (N)) /= N_Package_Body
        and then Nkind (Parent (N)) /= N_Accept_Statement
        and then Nkind (Parent (N)) /= N_Extended_Return_Statement
        and then not Delay_Cleanups (Current_Scope)
      then
         Expand_Cleanup_Actions (Parent (N));
      else
         Set_First_Real_Statement (N, First (Statements (N)));
      end if;
   end Expand_N_Handled_Sequence_Of_Statements;

   -------------------------------------
   -- Expand_N_Raise_Constraint_Error --
   -------------------------------------

   procedure Expand_N_Raise_Constraint_Error (N : Node_Id) is
   begin
      --  We adjust the condition to deal with the C/Fortran boolean case. This
      --  may well not be necessary, as all such conditions are generated by
      --  the expander and probably are all standard boolean, but who knows
      --  what strange optimization in future may require this adjustment!

      Adjust_Condition (Condition (N));

      --  Now deal with possible local raise handling

      Possible_Local_Raise (N, Standard_Constraint_Error);
   end Expand_N_Raise_Constraint_Error;

   ----------------------------------
   -- Expand_N_Raise_Program_Error --
   ----------------------------------

   procedure Expand_N_Raise_Program_Error (N : Node_Id) is
   begin
      --  We adjust the condition to deal with the C/Fortran boolean case. This
      --  may well not be necessary, as all such conditions are generated by
      --  the expander and probably are all standard boolean, but who knows
      --  what strange optimization in future may require this adjustment!

      Adjust_Condition (Condition (N));

      --  Now deal with possible local raise handling

      Possible_Local_Raise (N, Standard_Program_Error);
   end Expand_N_Raise_Program_Error;

   ------------------------------
   -- Expand_N_Raise_Statement --
   ------------------------------

   procedure Expand_N_Raise_Statement (N : Node_Id) is
      Loc   : constant Source_Ptr := Sloc (N);
      Ehand : Node_Id;
      E     : Entity_Id;
      Str   : String_Id;
      H     : Node_Id;

   begin
      --  Processing for locally handled exception (exclude reraise case)

      if Present (Name (N)) and then Nkind (Name (N)) = N_Identifier then
         if Debug_Flag_Dot_G
           or else Restriction_Active (No_Exception_Propagation)
         then
            --  If we have a local handler, then note that this is potentially
            --  able to be transformed into a goto statement.

            H := Find_Local_Handler (Entity (Name (N)), N);

            if Present (H) then
               if Local_Raise_Statements (H) = No_Elist then
                  Set_Local_Raise_Statements (H, New_Elmt_List);
               end if;

               --  Append the new entry if it is not there already. Sometimes
               --  we have situations where due to reexpansion, the same node
               --  is analyzed twice and would otherwise be added twice.

               Append_Unique_Elmt (N, Local_Raise_Statements (H));
               Set_Has_Local_Raise (H);

            --  If no local handler, then generate no propagation warning

            else
               Warn_If_No_Propagation (N);
            end if;

         end if;
      end if;

      --  If a string expression is present, then the raise statement is
      --  converted to a call:

      --     Raise_Exception (exception-name'Identity, string);

      --  and there is nothing else to do.

      if Present (Expression (N)) then
         Rewrite (N,
           Make_Procedure_Call_Statement (Loc,
             Name => New_Occurrence_Of (RTE (RE_Raise_Exception), Loc),
             Parameter_Associations => New_List (
               Make_Attribute_Reference (Loc,
                 Prefix         => Name (N),
                 Attribute_Name => Name_Identity),
               Expression (N))));
         Analyze (N);
         return;
      end if;

      --  Remaining processing is for the case where no string expression
      --  is present.

      --  There is no expansion needed for statement "raise <exception>;" when
      --  compiling for the JVM since the JVM has a built-in exception
      --  mechanism. However we need to keep the expansion for "raise;"
      --  statements. See 4jexcept.ads for details.

      if Present (Name (N)) and then VM_Target /= No_VM then
         return;
      end if;

      --  Don't expand a raise statement that does not come from source
      --  if we have already had configurable run-time violations, since
      --  most likely it will be junk cascaded nonsense.

      if Configurable_Run_Time_Violations > 0
        and then not Comes_From_Source (N)
      then
         return;
      end if;

      --  Convert explicit raise of Program_Error, Constraint_Error, and
      --  Storage_Error into the corresponding raise (in High_Integrity_Mode
      --  all other raises will get normal expansion and be disallowed,
      --  but this is also faster in all modes).

      if Present (Name (N)) and then Nkind (Name (N)) = N_Identifier then
         if Entity (Name (N)) = Standard_Constraint_Error then
            Rewrite (N,
              Make_Raise_Constraint_Error (Loc,
                Reason => CE_Explicit_Raise));
            Analyze (N);
            return;

         elsif Entity (Name (N)) = Standard_Program_Error then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Explicit_Raise));
            Analyze (N);
            return;

         elsif Entity (Name (N)) = Standard_Storage_Error then
            Rewrite (N,
              Make_Raise_Storage_Error (Loc,
                Reason => SE_Explicit_Raise));
            Analyze (N);
            return;
         end if;
      end if;

      --  Case of name present, in this case we expand raise name to

      --    Raise_Exception (name'Identity, location_string);

      --  where location_string identifies the file/line of the raise

      if Present (Name (N)) then
         declare
            Id : Entity_Id := Entity (Name (N));

         begin
            Name_Len := 0;
            Build_Location_String (Loc);

            --  If the exception is a renaming, use the exception that it
            --  renames (which might be a predefined exception, e.g.).

            if Present (Renamed_Object (Id)) then
               Id := Renamed_Object (Id);
            end if;

            --  Build a C-compatible string in case of no exception handlers,
            --  since this is what the last chance handler is expecting.

            if No_Exception_Handlers_Set then

               --  Generate an empty message if configuration pragma
               --  Suppress_Exception_Locations is set for this unit.

               if Opt.Exception_Locations_Suppressed then
                  Name_Len := 1;
               else
                  Name_Len := Name_Len + 1;
               end if;

               Name_Buffer (Name_Len) := ASCII.NUL;
            end if;

            if Opt.Exception_Locations_Suppressed then
               Name_Len := 0;
            end if;

            Str := String_From_Name_Buffer;

            --  For VMS exceptions, convert the raise into a call to
            --  lib$stop so it will be handled by __gnat_error_handler.

            if Is_VMS_Exception (Id) then
               declare
                  Excep_Image : String_Id;
                  Cond        : Node_Id;

               begin
                  if Present (Interface_Name (Id)) then
                     Excep_Image := Strval (Interface_Name (Id));
                  else
                     Get_Name_String (Chars (Id));
                     Set_All_Upper_Case;
                     Excep_Image := String_From_Name_Buffer;
                  end if;

                  if Exception_Code (Id) /= No_Uint then
                     Cond :=
                       Make_Integer_Literal (Loc, Exception_Code (Id));
                  else
                     Cond :=
                       Unchecked_Convert_To (Standard_Integer,
                         Make_Function_Call (Loc,
                           Name => New_Occurrence_Of
                             (RTE (RE_Import_Value), Loc),
                           Parameter_Associations => New_List
                             (Make_String_Literal (Loc,
                               Strval => Excep_Image))));
                  end if;

                  Rewrite (N,
                    Make_Procedure_Call_Statement (Loc,
                      Name =>
                        New_Occurrence_Of (RTE (RE_Lib_Stop), Loc),
                      Parameter_Associations => New_List (Cond)));
                        Analyze_And_Resolve (Cond, Standard_Integer);
               end;

            --  Not VMS exception case, convert raise to call to the
            --  Raise_Exception routine.

            else
               Rewrite (N,
                 Make_Procedure_Call_Statement (Loc,
                    Name => New_Occurrence_Of (RTE (RE_Raise_Exception), Loc),
                    Parameter_Associations => New_List (
                      Make_Attribute_Reference (Loc,
                        Prefix => Name (N),
                        Attribute_Name => Name_Identity),
                      Make_String_Literal (Loc,
                        Strval => Str))));
            end if;
         end;

      --  Case of no name present (reraise). We rewrite the raise to:

      --    Reraise_Occurrence_Always (EO);

      --  where EO is the current exception occurrence. If the current handler
      --  does not have a choice parameter specification, then we provide one.

      else
         --  Find innermost enclosing exception handler (there must be one,
         --  since the semantics has already verified that this raise statement
         --  is valid, and a raise with no arguments is only permitted in the
         --  context of an exception handler.

         Ehand := Parent (N);
         while Nkind (Ehand) /= N_Exception_Handler loop
            Ehand := Parent (Ehand);
         end loop;

         --  Make exception choice parameter if none present. Note that we do
         --  not need to put the entity on the entity chain, since no one will
         --  be referencing this entity by normal visibility methods.

         if No (Choice_Parameter (Ehand)) then
            E := Make_Defining_Identifier (Loc, New_Internal_Name ('E'));
            Set_Choice_Parameter (Ehand, E);
            Set_Ekind (E, E_Variable);
            Set_Etype (E, RTE (RE_Exception_Occurrence));
            Set_Scope (E, Current_Scope);
         end if;

         --  Now rewrite the raise as a call to Reraise. A special case arises
         --  if this raise statement occurs in the context of a handler for
         --  all others (i.e. an at end handler). in this case we avoid
         --  the call to defer abort, cleanup routines are expected to be
         --  called in this case with aborts deferred.

         declare
            Ech : constant Node_Id := First (Exception_Choices (Ehand));
            Ent : Entity_Id;

         begin
            if Nkind (Ech) = N_Others_Choice
              and then All_Others (Ech)
            then
               Ent := RTE (RE_Reraise_Occurrence_No_Defer);
            else
               Ent := RTE (RE_Reraise_Occurrence_Always);
            end if;

            Rewrite (N,
              Make_Procedure_Call_Statement (Loc,
                Name => New_Occurrence_Of (Ent, Loc),
                Parameter_Associations => New_List (
                  New_Occurrence_Of (Choice_Parameter (Ehand), Loc))));
         end;
      end if;

      Analyze (N);
   end Expand_N_Raise_Statement;

   ----------------------------------
   -- Expand_N_Raise_Storage_Error --
   ----------------------------------

   procedure Expand_N_Raise_Storage_Error (N : Node_Id) is
   begin
      --  We adjust the condition to deal with the C/Fortran boolean case. This
      --  may well not be necessary, as all such conditions are generated by
      --  the expander and probably are all standard boolean, but who knows
      --  what strange optimization in future may require this adjustment!

      Adjust_Condition (Condition (N));

      --  Now deal with possible local raise handling

      Possible_Local_Raise (N, Standard_Storage_Error);
   end Expand_N_Raise_Storage_Error;

   --------------------------
   -- Possible_Local_Raise --
   --------------------------

   procedure Possible_Local_Raise (N : Node_Id; E : Entity_Id) is
   begin
      --  Nothing to do if local raise optimization not active

      if not Debug_Flag_Dot_G
        and then not Restriction_Active (No_Exception_Propagation)
      then
         return;
      end if;

      --  Nothing to do if original node was an explicit raise, because in
      --  that case, we already generated the required warning for the raise.

      if Nkind (Original_Node (N)) = N_Raise_Statement then
         return;
      end if;

      --  Otherwise see if we have a local handler for the exception

      declare
         H : constant Node_Id := Find_Local_Handler (E, N);

      begin
         --  If so, mark that it has a local raise

         if Present (H) then
            Set_Has_Local_Raise (H, True);

         --  Otherwise, if the No_Exception_Propagation restriction is active
         --  and the warning is enabled, generate the appropriate warnings.

         elsif Warn_On_Non_Local_Exception
           and then Restriction_Active (No_Exception_Propagation)
         then
            Warn_No_Exception_Propagation_Active (N);

            if Configurable_Run_Time_Mode then
               Error_Msg_NE
                 ("\?& may call Last_Chance_Handler", N, E);
            else
               Error_Msg_NE
                 ("\?& may result in unhandled exception", N, E);
            end if;
         end if;
      end;
   end Possible_Local_Raise;

   ------------------------------
   -- Expand_N_Subprogram_Info --
   ------------------------------

   procedure Expand_N_Subprogram_Info (N : Node_Id) is
      Loc : constant Source_Ptr := Sloc (N);

   begin
      --  For now, we replace an Expand_N_Subprogram_Info node with an
      --  attribute reference that gives the address of the procedure.
      --  This is because gigi does not yet recognize this node, and
      --  for the initial targets, this is the right value anyway.

      Rewrite (N,
        Make_Attribute_Reference (Loc,
          Prefix => Identifier (N),
          Attribute_Name => Name_Code_Address));

      Analyze_And_Resolve (N, RTE (RE_Code_Loc));
   end Expand_N_Subprogram_Info;

   ------------------------
   -- Find_Local_Handler --
   ------------------------

   function Find_Local_Handler
     (Ename : Entity_Id;
      Nod   : Node_Id) return Node_Id
   is
      N : Node_Id;
      P : Node_Id;
      H : Node_Id;
      C : Node_Id;

      SSE : Scope_Stack_Entry renames Scope_Stack.Table (Scope_Stack.Last);
      --  This is used to test for wrapped actions below

      ERaise  : Entity_Id;
      EHandle : Entity_Id;
      --  The entity Id's for the exception we are raising and handling, using
      --  the renamed exception if a Renamed_Entity is present.

   begin
      --  Never any local handler if all handlers removed

      if Debug_Flag_Dot_X then
         return Empty;
      end if;

      --  Get the exception we are raising, allowing for renaming

      ERaise := Get_Renamed_Entity (Ename);

      --  We need to check if the node we are looking at is contained in
      --

      --  Loop to search up the tree

      N := Nod;
      loop
         P := Parent (N);

         --  If we get to the top of the tree, or to a subprogram, task, entry,
         --  protected body, or accept statement without having found a
         --  matching handler, then there is no local handler.

         if No (P)
           or else Nkind (P) = N_Subprogram_Body
           or else Nkind (P) = N_Task_Body
           or else Nkind (P) = N_Protected_Body
           or else Nkind (P) = N_Entry_Body
           or else Nkind (P) = N_Accept_Statement
         then
            return Empty;

            --  Test for handled sequence of statements with at least one
            --  exception handler which might be the one we are looking for.

         elsif Nkind (P) = N_Handled_Sequence_Of_Statements
           and then Present (Exception_Handlers (P))
         then
            --  Before we proceed we need to check if the node N is covered
            --  by the statement part of P rather than one of its exception
            --  handlers (an exception handler obviously does not cover its
            --  own statements).

            --  This test is more delicate than might be thought. It is not
            --  just a matter of checking the Statements (P), because the node
            --  might be waiting to be wrapped in a transient scope, in which
            --  case it will end up in the block statements, even though it
            --  is not there now.

            if Is_List_Member (N)
              and then (List_Containing (N) = Statements (P)
                          or else
                        List_Containing (N) = SSE.Actions_To_Be_Wrapped_Before
                          or else
                        List_Containing (N) = SSE.Actions_To_Be_Wrapped_After)
            then
               --  Loop through exception handlers

               H := First (Exception_Handlers (P));
               while Present (H) loop

                  --  Loop through choices in one handler

                  C := First (Exception_Choices (H));
                  while Present (C) loop

                     --  Deal with others case

                     if Nkind (C) = N_Others_Choice then

                        --  Matching others handler, but we need to ensure
                        --  there is no choice parameter. If there is, then we
                        --  don't have a local handler after all (since we do
                        --  not allow choice parameters for local handlers).

                        if No (Choice_Parameter (H)) then
                           return H;
                        else
                           return Empty;
                        end if;

                     --  If not others must be entity name

                     elsif Nkind (C) /= N_Others_Choice then
                        pragma Assert (Is_Entity_Name (C));
                        pragma Assert (Present (Entity (C)));

                        --  Get exception being handled, dealing with renaming

                        EHandle := Get_Renamed_Entity (Entity (C));

                        --  If match, then check choice parameter

                        if ERaise = EHandle then
                           if No (Choice_Parameter (H)) then
                              return H;
                           else
                              return Empty;
                           end if;
                        end if;
                     end if;

                     Next (C);
                  end loop;

                  Next (H);
               end loop;
            end if;
         end if;

         N := P;
      end loop;
   end Find_Local_Handler;

   ---------------------------------
   -- Get_Local_Raise_Call_Entity --
   ---------------------------------

   --  Note: this is primary provided for use by the back end in generating
   --  calls to Local_Raise. But it would be too late in the back end to call
   --  RTE if this actually caused a load/analyze of the unit. So what we do
   --  is to ensure there is a dummy call to this function during front end
   --  processing so that the unit gets loaded then, and not later.

   Local_Raise_Call_Entity     : Entity_Id;
   Local_Raise_Call_Entity_Set : Boolean := False;

   function Get_Local_Raise_Call_Entity return Entity_Id is
   begin
      if not Local_Raise_Call_Entity_Set then
         Local_Raise_Call_Entity_Set := True;

         if RTE_Available (RE_Local_Raise) then
            Local_Raise_Call_Entity := RTE (RE_Local_Raise);
         else
            Local_Raise_Call_Entity := Empty;
         end if;
      end if;

      return Local_Raise_Call_Entity;
   end Get_Local_Raise_Call_Entity;

   -----------------------------
   -- Get_RT_Exception_Entity --
   -----------------------------

   function Get_RT_Exception_Entity (R : RT_Exception_Code) return Entity_Id is
   begin
      case R is
         when RT_CE_Exceptions => return Standard_Constraint_Error;
         when RT_PE_Exceptions => return Standard_Program_Error;
         when RT_SE_Exceptions => return Standard_Storage_Error;
      end case;
   end Get_RT_Exception_Entity;

   ----------------------
   -- Is_Non_Ada_Error --
   ----------------------

   function Is_Non_Ada_Error (E : Entity_Id) return Boolean is
   begin
      if not OpenVMS_On_Target then
         return False;
      end if;

      Get_Name_String (Chars (E));

      --  Note: it is a little irregular for the body of exp_ch11 to know
      --  the details of the encoding scheme for names, but on the other
      --  hand, gigi knows them, and this is for gigi's benefit anyway!

      if Name_Buffer (1 .. 30) /= "system__aux_dec__non_ada_error" then
         return False;
      end if;

      return True;
   end Is_Non_Ada_Error;

   ----------------------------
   -- Warn_If_No_Propagation --
   ----------------------------

   procedure Warn_If_No_Propagation (N : Node_Id) is
   begin
      if Restriction_Active (No_Exception_Propagation)
        and then Warn_On_Non_Local_Exception
      then
         Warn_No_Exception_Propagation_Active (N);

         if Configurable_Run_Time_Mode then
            Error_Msg_N
              ("\?Last_Chance_Handler will be called on exception", N);
         else
            Error_Msg_N
              ("\?execution may raise unhandled exception", N);
         end if;
      end if;
   end Warn_If_No_Propagation;

   ------------------------------------------
   -- Warn_No_Exception_Propagation_Active --
   ------------------------------------------

   procedure Warn_No_Exception_Propagation_Active (N : Node_Id) is
   begin
      Error_Msg_N
        ("?pragma Restrictions (No_Exception_Propagation) in effect", N);
   end Warn_No_Exception_Propagation_Active;

end Exp_Ch11;
