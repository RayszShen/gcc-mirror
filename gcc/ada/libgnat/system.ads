------------------------------------------------------------------------------
--                                                                          --
--                        GNAT RUN-TIME COMPONENTS                          --
--                                                                          --
--                               S Y S T E M                                --
--                                                                          --
--                                 S p e c                                  --
--                            (Compiler Version)                            --
--                                                                          --
--          Copyright (C) 1992-2018, Free Software Foundation, Inc.         --
--                                                                          --
-- This specification is derived from the Ada Reference Manual for use with --
-- GNAT. The copyright notice above, and the license provisions that follow --
-- apply solely to the  contents of the part following the private keyword. --
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

--  This version of System is a generic version that is used in building the
--  compiler. Right now, we have a host/target problem if we try to use the
--  "proper" System, and since the compiler itself does not care about most
--  System parameters, this generic version works fine.

pragma Restrictions (No_Implicit_Dynamic_Code);
--  We want to avoid trampolines in the compiler, so it can be used in systems
--  which prevent execution of code on the stack, e.g. in windows environments
--  with DEP (Data Execution Protection) enabled.

pragma Restrictions (No_Finalization);
--  Use restriction No_Finalization to avoid pulling finalization (not allowed
--  in GNAT) inside sem_spark.adb, when defining type Perm_Tree_Access as an
--  access type on incomplete type Perm_Tree_Wrapper (which is required for
--  defining a recursive type).

package System is
   pragma Pure;
   --  Note that we take advantage of the implementation permission to make
   --  this unit Pure instead of Preelaborable; see RM 13.7.1(15). In Ada
   --  2005, this is Pure in any case (AI-362).

   pragma No_Elaboration_Code_All;
   --  Allow the use of that restriction in units that WITH this unit

   type Name is (SYSTEM_NAME_GNAT);
   System_Name : constant Name := SYSTEM_NAME_GNAT;

   --  System-Dependent Named Numbers

   Min_Int               : constant := Long_Long_Integer'First;
   Max_Int               : constant := Long_Long_Integer'Last;

   Max_Binary_Modulus    : constant := 2 ** Long_Long_Integer'Size;
   Max_Nonbinary_Modulus : constant := 2 ** Integer'Size - 1;

   Max_Base_Digits       : constant := Long_Long_Float'Digits;
   Max_Digits            : constant := Long_Long_Float'Digits;

   Max_Mantissa          : constant := 63;
   Fine_Delta            : constant := 2.0 ** (-Max_Mantissa);

   Tick                  : constant := 0.01;

   --  Storage-related Declarations

   type Address is private;
   --  Note that we do NOT add pragma Preelaborable_Initialization in this
   --  version of System, since it is used for the compiler only, and typical
   --  earlier bootstrap compilers don't support this pragma. We don't need
   --  it in this context, so there is no problem in omitting it.
   Null_Address : constant Address;

   Storage_Unit : constant := Standard'Storage_Unit;
   Word_Size    : constant := Standard'Word_Size;
   Memory_Size  : constant := 2 ** Standard'Address_Size;

   --  Address comparison

   function "<"  (Left, Right : Address) return Boolean;
   function "<=" (Left, Right : Address) return Boolean;
   function ">"  (Left, Right : Address) return Boolean;
   function ">=" (Left, Right : Address) return Boolean;
   function "="  (Left, Right : Address) return Boolean;

   pragma Import (Intrinsic, "<");
   pragma Import (Intrinsic, "<=");
   pragma Import (Intrinsic, ">");
   pragma Import (Intrinsic, ">=");
   pragma Import (Intrinsic, "=");

   --  Other System-Dependent Declarations

   type Bit_Order is (High_Order_First, Low_Order_First);
   Default_Bit_Order : constant Bit_Order :=
                         Bit_Order'Val (Standard'Default_Bit_Order);
   pragma Warnings (Off, Default_Bit_Order); -- kill constant condition warning

   --  Priority-related Declarations (RM D.1)

   Max_Priority           : constant Positive := 30;
   Max_Interrupt_Priority : constant Positive := 31;

   subtype Any_Priority       is Integer      range  0 .. 31;
   subtype Priority           is Any_Priority range  0 .. 30;
   subtype Interrupt_Priority is Any_Priority range 31 .. 31;

   Default_Priority : constant Priority := 15;

private

   type Address is mod Memory_Size;
   Null_Address : constant Address := 0;

   --------------------------------------
   -- System Implementation Parameters --
   --------------------------------------

   --  These parameters provide information about the target that is used by
   --  the compiler. They are in the private part of System, where they can be
   --  accessed using the special circuitry in the Targparm unit whose source
   --  should be consulted for more detailed descriptions of the individual
   --  switch values.

   --  This version of system.ads is used only for building the compiler.
   --  We really ought to use the proper target system (i.e. the one that
   --  corresponds to the host for the compiler), but that causes as yet
   --  unsolved makefile problems. For the most part the setting of these
   --  parameters is not too critical for the compiler version (e.g. we
   --  do not use floating-point anyway in the compiler).

   AAMP                      : constant Boolean := False;
   Backend_Divide_Checks     : constant Boolean := False;
   Backend_Overflow_Checks   : constant Boolean := False;
   Command_Line_Args         : constant Boolean := True;
   Configurable_Run_Time     : constant Boolean := False;
   Denorm                    : constant Boolean := True;
   Duration_32_Bits          : constant Boolean := False;
   Exit_Status_Supported     : constant Boolean := True;
   Fractional_Fixed_Ops      : constant Boolean := False;
   Frontend_Layout           : constant Boolean := False;
   Machine_Overflows         : constant Boolean := False;
   Machine_Rounds            : constant Boolean := True;
   Preallocated_Stacks       : constant Boolean := False;
   Signed_Zeros              : constant Boolean := True;
   Stack_Check_Default       : constant Boolean := False;
   Stack_Check_Probes        : constant Boolean := False;
   Stack_Check_Limits        : constant Boolean := False;
   Support_Aggregates        : constant Boolean := True;
   Support_Composite_Assign  : constant Boolean := True;
   Support_Composite_Compare : constant Boolean := True;
   Support_Long_Shifts       : constant Boolean := True;
   Always_Compatible_Rep     : constant Boolean := True;
   Suppress_Standard_Library : constant Boolean := False;
   Use_Ada_Main_Program_Name : constant Boolean := False;
   Frontend_Exceptions       : constant Boolean := False;
   ZCX_By_Default            : constant Boolean := True;

   --  Obsolete entries, to be removed eventually (bootstrap issues)

   Front_End_ZCX_Support     : constant Boolean := False;
   High_Integrity_Mode       : constant Boolean := False;
   Long_Shifts_Inlined       : constant Boolean := True;
   Functions_Return_By_DSP   : constant Boolean := False;
   Support_64_Bit_Divides    : constant Boolean := True;
   GCC_ZCX_Support           : constant Boolean := True;

end System;
