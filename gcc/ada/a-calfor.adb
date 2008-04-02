------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--              A D A . C A L E N D A R . F O R M A T T I N G               --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2006-2007, Free Software Foundation, Inc.         --
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

with Ada.Calendar;            use Ada.Calendar;
with Ada.Calendar.Time_Zones; use Ada.Calendar.Time_Zones;

pragma Warnings (Off); -- temp till we fix out param warnings ???

package body Ada.Calendar.Formatting is

   --------------------------
   -- Implementation Notes --
   --------------------------

   --  All operations in this package are target and time representation
   --  independent, thus only one source file is needed for multiple targets.

   procedure Check_Char (S : String; C : Character; Index : Integer);
   --  Subsidiary to the two versions of Value. Determine whether the
   --  input string S has character C at position Index. Raise
   --  Constraint_Error if there is a mismatch.

   procedure Check_Digit (S : String; Index : Integer);
   --  Subsidiary to the two versions of Value. Determine whether the
   --  character of string S at position Index is a digit. This catches
   --  invalid input such as 1983-*1-j3 u5:n7:k9 which should be
   --  1983-01-03 05:07:09. Raise Constraint_Error if there is a mismatch.

   ----------------
   -- Check_Char --
   ----------------

   procedure Check_Char (S : String; C : Character; Index : Integer) is
   begin
      if S (Index) /= C then
         raise Constraint_Error;
      end if;
   end Check_Char;

   -----------------
   -- Check_Digit --
   -----------------

   procedure Check_Digit (S : String; Index : Integer) is
   begin
      if S (Index) not in '0' .. '9' then
         raise Constraint_Error;
      end if;
   end Check_Digit;

   ---------
   -- Day --
   ---------

   function Day
     (Date      : Time;
      Time_Zone : Time_Zones.Time_Offset := 0) return Day_Number
   is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, Mo, H, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le, Time_Zone);
      return D;
   end Day;

   -----------------
   -- Day_Of_Week --
   -----------------

   function Day_Of_Week (Date : Time) return Day_Name is
   begin
      return Day_Name'Val (Formatting_Operations.Day_Of_Week (Date));
   end Day_Of_Week;

   ----------
   -- Hour --
   ----------

   function Hour
     (Date      : Time;
      Time_Zone : Time_Zones.Time_Offset := 0) return Hour_Number
   is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, Mo, D, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le, Time_Zone);
      return H;
   end Hour;

   -----------
   -- Image --
   -----------

   function Image
     (Elapsed_Time          : Duration;
      Include_Time_Fraction : Boolean := False) return String
   is
      Hour       : Hour_Number;
      Minute     : Minute_Number;
      Second     : Second_Number;
      Sub_Second : Duration;
      SS_Nat     : Natural;

      Low  : Integer;
      High : Integer;

      Result : String := "-00:00:00.00";

   begin
      Split (abs (Elapsed_Time), Hour, Minute, Second, Sub_Second);

      --  Determine the two slice bounds for the result string depending on
      --  whether the input is negative and whether fractions are requested.

      if Elapsed_Time < 0.0 then
         Low := 1;
      else
         Low := 2;
      end if;

      if Include_Time_Fraction then
         High := 12;
      else
         High := 9;
      end if;

      --  Prevent rounding when converting to natural

      Sub_Second := Sub_Second * 100.0 - 0.5;
      SS_Nat := Natural (Sub_Second);

      declare
         Hour_Str   : constant String := Hour_Number'Image (Hour);
         Minute_Str : constant String := Minute_Number'Image (Minute);
         Second_Str : constant String := Second_Number'Image (Second);
         SS_Str     : constant String := Natural'Image (SS_Nat);

      begin
         --  Hour processing, positions 2 and 3

         if Hour < 10 then
            Result (3) := Hour_Str (2);
         else
            Result (2) := Hour_Str (2);
            Result (3) := Hour_Str (3);
         end if;

         --  Minute processing, positions 5 and 6

         if Minute < 10 then
            Result (6) := Minute_Str (2);
         else
            Result (5) := Minute_Str (2);
            Result (6) := Minute_Str (3);
         end if;

         --  Second processing, positions 8 and 9

         if Second < 10 then
            Result (9) := Second_Str (2);
         else
            Result (8) := Second_Str (2);
            Result (9) := Second_Str (3);
         end if;

         --  Optional sub second processing, positions 11 and 12

         if Include_Time_Fraction then
            if SS_Nat < 10 then
               Result (12) := SS_Str (2);
            else
               Result (11) := SS_Str (2);
               Result (12) := SS_Str (3);
            end if;
         end if;

         return Result (Low .. High);
      end;
   end Image;

   -----------
   -- Image --
   -----------

   function Image
     (Date                  : Time;
      Include_Time_Fraction : Boolean := False;
      Time_Zone             : Time_Zones.Time_Offset := 0) return String
   is
      Year        : Year_Number;
      Month       : Month_Number;
      Day         : Day_Number;
      Hour        : Hour_Number;
      Minute      : Minute_Number;
      Second      : Second_Number;
      Sub_Second  : Duration;
      SS_Nat      : Natural;
      Leap_Second : Boolean;

      Result : String := "0000-00-00 00:00:00.00";

   begin
      Split (Date, Year, Month, Day,
             Hour, Minute, Second, Sub_Second, Leap_Second, Time_Zone);

      --  Prevent rounding when converting to natural

      Sub_Second := Sub_Second * 100.0 - 0.5;
      SS_Nat := Natural (Sub_Second);

      declare
         Year_Str   : constant String := Year_Number'Image (Year);
         Month_Str  : constant String := Month_Number'Image (Month);
         Day_Str    : constant String := Day_Number'Image (Day);
         Hour_Str   : constant String := Hour_Number'Image (Hour);
         Minute_Str : constant String := Minute_Number'Image (Minute);
         Second_Str : constant String := Second_Number'Image (Second);
         SS_Str     : constant String := Natural'Image (SS_Nat);

      begin
         --  Year processing, positions 1, 2, 3 and 4

         Result (1) := Year_Str (2);
         Result (2) := Year_Str (3);
         Result (3) := Year_Str (4);
         Result (4) := Year_Str (5);

         --  Month processing, positions 6 and 7

         if Month < 10 then
            Result (7) := Month_Str (2);
         else
            Result (6) := Month_Str (2);
            Result (7) := Month_Str (3);
         end if;

         --  Day processing, positions 9 and 10

         if Day < 10 then
            Result (10) := Day_Str (2);
         else
            Result (9)  := Day_Str (2);
            Result (10) := Day_Str (3);
         end if;

         --  Hour processing, positions 12 and 13

         if Hour < 10 then
            Result (13) := Hour_Str (2);
         else
            Result (12) := Hour_Str (2);
            Result (13) := Hour_Str (3);
         end if;

         --  Minute processing, positions 15 and 16

         if Minute < 10 then
            Result (16) := Minute_Str (2);
         else
            Result (15) := Minute_Str (2);
            Result (16) := Minute_Str (3);
         end if;

         --  Second processing, positions 18 and 19

         if Second < 10 then
            Result (19) := Second_Str (2);
         else
            Result (18) := Second_Str (2);
            Result (19) := Second_Str (3);
         end if;

         --  Optional sub second processing, positions 21 and 22

         if Include_Time_Fraction then
            if SS_Nat < 10 then
               Result (22) := SS_Str (2);
            else
               Result (21) := SS_Str (2);
               Result (22) := SS_Str (3);
            end if;

            return Result;
         else
            return Result (1 .. 19);
         end if;
      end;
   end Image;

   ------------
   -- Minute --
   ------------

   function Minute
     (Date      : Time;
      Time_Zone : Time_Zones.Time_Offset := 0) return Minute_Number
   is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, Mo, D, H);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le, Time_Zone);
      return Mi;
   end Minute;

   -----------
   -- Month --
   -----------

   function Month
     (Date      : Time;
      Time_Zone : Time_Zones.Time_Offset := 0) return Month_Number
   is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, D, H, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le, Time_Zone);
      return Mo;
   end Month;

   ------------
   -- Second --
   ------------

   function Second (Date : Time) return Second_Number is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, Mo, D, H, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le);
      return Se;
   end Second;

   ----------------
   -- Seconds_Of --
   ----------------

   function Seconds_Of
     (Hour       : Hour_Number;
      Minute     : Minute_Number;
      Second     : Second_Number := 0;
      Sub_Second : Second_Duration := 0.0) return Day_Duration is

   begin
      --  Validity checks

      if not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Constraint_Error;
      end if;

      return Day_Duration (Hour   * 3_600) +
             Day_Duration (Minute *    60) +
             Day_Duration (Second)         +
             Sub_Second;
   end Seconds_Of;

   -----------
   -- Split --
   -----------

   procedure Split
     (Seconds    : Day_Duration;
      Hour       : out Hour_Number;
      Minute     : out Minute_Number;
      Second     : out Second_Number;
      Sub_Second : out Second_Duration)
   is
      Secs : Natural;

   begin
      --  Validity checks

      if not Seconds'Valid then
         raise Constraint_Error;
      end if;

      if Seconds = 0.0 then
         Secs := 0;
      else
         Secs := Natural (Seconds - 0.5);
      end if;

      Sub_Second := Second_Duration (Seconds - Day_Duration (Secs));
      Hour       := Hour_Number (Secs / 3_600);
      Secs       := Secs mod 3_600;
      Minute     := Minute_Number (Secs / 60);
      Second     := Second_Number (Secs mod 60);

      --  Validity checks

      if not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Time_Error;
      end if;
   end Split;

   -----------
   -- Split --
   -----------

   procedure Split
     (Date        : Time;
      Year        : out Year_Number;
      Month       : out Month_Number;
      Day         : out Day_Number;
      Seconds     : out Day_Duration;
      Leap_Second : out Boolean;
      Time_Zone   : Time_Zones.Time_Offset := 0)
   is
      H  : Integer;
      M  : Integer;
      Se : Integer;
      Su : Duration;
      Tz : constant Long_Integer := Long_Integer (Time_Zone);

   begin
      Formatting_Operations.Split
        (Date      => Date,
         Year      => Year,
         Month     => Month,
         Day       => Day,
         Day_Secs  => Seconds,
         Hour      => H,
         Minute    => M,
         Second    => Se,
         Sub_Sec   => Su,
         Leap_Sec  => Leap_Second,
         Time_Zone => Tz,
         Is_Ada_05 => True);

      --  Validity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Seconds'Valid
      then
         raise Time_Error;
      end if;
   end Split;

   -----------
   -- Split --
   -----------

   procedure Split
     (Date       : Time;
      Year       : out Year_Number;
      Month      : out Month_Number;
      Day        : out Day_Number;
      Hour       : out Hour_Number;
      Minute     : out Minute_Number;
      Second     : out Second_Number;
      Sub_Second : out Second_Duration;
      Time_Zone  : Time_Zones.Time_Offset := 0)
   is
      Dd : Day_Duration;
      Le : Boolean;
      Tz : constant Long_Integer := Long_Integer (Time_Zone);

   begin
      Formatting_Operations.Split
        (Date      => Date,
         Year      => Year,
         Month     => Month,
         Day       => Day,
         Day_Secs  => Dd,
         Hour      => Hour,
         Minute    => Minute,
         Second    => Second,
         Sub_Sec   => Sub_Second,
         Leap_Sec  => Le,
         Time_Zone => Tz,
         Is_Ada_05 => True);

      --  Validity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Time_Error;
      end if;
   end Split;

   -----------
   -- Split --
   -----------

   procedure Split
     (Date        : Time;
      Year        : out Year_Number;
      Month       : out Month_Number;
      Day         : out Day_Number;
      Hour        : out Hour_Number;
      Minute      : out Minute_Number;
      Second      : out Second_Number;
      Sub_Second  : out Second_Duration;
      Leap_Second : out Boolean;
      Time_Zone   : Time_Zones.Time_Offset := 0)
   is
      Dd : Day_Duration;
      Tz : constant Long_Integer := Long_Integer (Time_Zone);

   begin
      Formatting_Operations.Split
       (Date      => Date,
        Year      => Year,
        Month     => Month,
        Day       => Day,
        Day_Secs  => Dd,
        Hour      => Hour,
        Minute    => Minute,
        Second    => Second,
        Sub_Sec   => Sub_Second,
        Leap_Sec  => Leap_Second,
        Time_Zone => Tz,
        Is_Ada_05 => True);

      --  Validity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Time_Error;
      end if;
   end Split;

   ----------------
   -- Sub_Second --
   ----------------

   function Sub_Second (Date : Time) return Second_Duration is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Y, Mo, D, H, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le);
      return Ss;
   end Sub_Second;

   -------------
   -- Time_Of --
   -------------

   function Time_Of
     (Year        : Year_Number;
      Month       : Month_Number;
      Day         : Day_Number;
      Seconds     : Day_Duration := 0.0;
      Leap_Second : Boolean := False;
      Time_Zone   : Time_Zones.Time_Offset := 0) return Time
   is
      Adj_Year  : Year_Number  := Year;
      Adj_Month : Month_Number := Month;
      Adj_Day   : Day_Number   := Day;

      H  : constant Integer := 1;
      M  : constant Integer := 1;
      Se : constant Integer := 1;
      Ss : constant Duration := 0.1;
      Tz : constant Long_Integer := Long_Integer (Time_Zone);

   begin
      --  Validity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Seconds'Valid
        or else not Time_Zone'Valid
      then
         raise Constraint_Error;
      end if;

      --  A Seconds value of 86_400 denotes a new day. This case requires an
      --  adjustment to the input values.

      if Seconds = 86_400.0 then
         if Day < Days_In_Month (Month)
           or else (Is_Leap (Year)
                      and then Month = 2)
         then
            Adj_Day := Day + 1;
         else
            Adj_Day := 1;

            if Month < 12 then
               Adj_Month := Month + 1;
            else
               Adj_Month := 1;
               Adj_Year  := Year + 1;
            end if;
         end if;
      end if;

      return
        Formatting_Operations.Time_Of
          (Year         => Adj_Year,
           Month        => Adj_Month,
           Day          => Adj_Day,
           Day_Secs     => Seconds,
           Hour         => H,
           Minute       => M,
           Second       => Se,
           Sub_Sec      => Ss,
           Leap_Sec     => Leap_Second,
           Use_Day_Secs => True,
           Is_Ada_05    => True,
           Time_Zone    => Tz);
   end Time_Of;

   -------------
   -- Time_Of --
   -------------

   function Time_Of
     (Year        : Year_Number;
      Month       : Month_Number;
      Day         : Day_Number;
      Hour        : Hour_Number;
      Minute      : Minute_Number;
      Second      : Second_Number;
      Sub_Second  : Second_Duration := 0.0;
      Leap_Second : Boolean := False;
      Time_Zone   : Time_Zones.Time_Offset := 0) return Time
   is
      Dd : constant Day_Duration := Day_Duration'First;
      Tz : constant Long_Integer := Long_Integer (Time_Zone);

   begin
      --  Validity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
        or else not Time_Zone'Valid
      then
         raise Constraint_Error;
      end if;

      return
        Formatting_Operations.Time_Of
          (Year         => Year,
           Month        => Month,
           Day          => Day,
           Day_Secs     => Dd,
           Hour         => Hour,
           Minute       => Minute,
           Second       => Second,
           Sub_Sec      => Sub_Second,
           Leap_Sec     => Leap_Second,
           Use_Day_Secs => False,
           Is_Ada_05    => True,
           Time_Zone    => Tz);
   end Time_Of;

   -----------
   -- Value --
   -----------

   function Value
     (Date      : String;
      Time_Zone : Time_Zones.Time_Offset := 0) return Time
   is
      D          : String (1 .. 22);
      Year       : Year_Number;
      Month      : Month_Number;
      Day        : Day_Number;
      Hour       : Hour_Number;
      Minute     : Minute_Number;
      Second     : Second_Number;
      Sub_Second : Second_Duration := 0.0;

   begin
      --  Validity checks

      if not Time_Zone'Valid then
         raise Constraint_Error;
      end if;

      --  Length checks

      if Date'Length /= 19
        and then Date'Length /= 22
      then
         raise Constraint_Error;
      end if;

      --  After the correct length has been determined, it is safe to
      --  copy the Date in order to avoid Date'First + N indexing.

      D (1 .. Date'Length) := Date;

      --  Format checks

      Check_Char (D, '-', 5);
      Check_Char (D, '-', 8);
      Check_Char (D, ' ', 11);
      Check_Char (D, ':', 14);
      Check_Char (D, ':', 17);

      if Date'Length = 22 then
         Check_Char (D, '.', 20);
      end if;

      --  Leading zero checks

      Check_Digit (D, 6);
      Check_Digit (D, 9);
      Check_Digit (D, 12);
      Check_Digit (D, 15);
      Check_Digit (D, 18);

      if Date'Length = 22 then
         Check_Digit (D, 21);
      end if;

      --  Value extraction

      Year   := Year_Number   (Year_Number'Value   (D (1 .. 4)));
      Month  := Month_Number  (Month_Number'Value  (D (6 .. 7)));
      Day    := Day_Number    (Day_Number'Value    (D (9 .. 10)));
      Hour   := Hour_Number   (Hour_Number'Value   (D (12 .. 13)));
      Minute := Minute_Number (Minute_Number'Value (D (15 .. 16)));
      Second := Second_Number (Second_Number'Value (D (18 .. 19)));

      --  Optional part

      if Date'Length = 22 then
         Sub_Second := Second_Duration (Second_Duration'Value (D (20 .. 22)));
      end if;

      --  Sanity checks

      if not Year'Valid
        or else not Month'Valid
        or else not Day'Valid
        or else not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Constraint_Error;
      end if;

      return Time_Of (Year, Month, Day,
                      Hour, Minute, Second, Sub_Second, False, Time_Zone);

   exception
      when others => raise Constraint_Error;
   end Value;

   -----------
   -- Value --
   -----------

   function Value (Elapsed_Time : String) return Duration is
      D          : String (1 .. 11);
      Hour       : Hour_Number;
      Minute     : Minute_Number;
      Second     : Second_Number;
      Sub_Second : Second_Duration := 0.0;

   begin
      --  Length checks

      if Elapsed_Time'Length /= 8
        and then Elapsed_Time'Length /= 11
      then
         raise Constraint_Error;
      end if;

      --  After the correct length has been determined, it is safe to
      --  copy the Elapsed_Time in order to avoid Date'First + N indexing.

      D (1 .. Elapsed_Time'Length) := Elapsed_Time;

      --  Format checks

      Check_Char (D, ':', 3);
      Check_Char (D, ':', 6);

      if Elapsed_Time'Length = 11 then
         Check_Char (D, '.', 9);
      end if;

      --  Leading zero checks

      Check_Digit (D, 1);
      Check_Digit (D, 4);
      Check_Digit (D, 7);

      if Elapsed_Time'Length = 11 then
         Check_Digit (D, 10);
      end if;

      --  Value extraction

      Hour   := Hour_Number   (Hour_Number'Value   (D (1 .. 2)));
      Minute := Minute_Number (Minute_Number'Value (D (4 .. 5)));
      Second := Second_Number (Second_Number'Value (D (7 .. 8)));

      --  Optional part

      if Elapsed_Time'Length = 11 then
         Sub_Second := Second_Duration (Second_Duration'Value (D (9 .. 11)));
      end if;

      --  Sanity checks

      if not Hour'Valid
        or else not Minute'Valid
        or else not Second'Valid
        or else not Sub_Second'Valid
      then
         raise Constraint_Error;
      end if;

      return Seconds_Of (Hour, Minute, Second, Sub_Second);

   exception
      when others => raise Constraint_Error;
   end Value;

   ----------
   -- Year --
   ----------

   function Year
     (Date      : Time;
      Time_Zone : Time_Zones.Time_Offset := 0) return Year_Number
   is
      Y  : Year_Number;
      Mo : Month_Number;
      D  : Day_Number;
      H  : Hour_Number;
      Mi : Minute_Number;
      Se : Second_Number;
      Ss : Second_Duration;
      Le : Boolean;

      pragma Unreferenced (Mo, D, H, Mi);

   begin
      Split (Date, Y, Mo, D, H, Mi, Se, Ss, Le, Time_Zone);
      return Y;
   end Year;

end Ada.Calendar.Formatting;
