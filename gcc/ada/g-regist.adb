------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                         G N A T . R E G I S T R Y                        --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                            $Revision: 1.2 $
--                                                                          --
--              Copyright (C) 2001 Free Software Foundation, Inc.           --
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
-- GNAT is maintained by Ada Core Technologies Inc (http://www.gnat.com).   --
--                                                                          --
------------------------------------------------------------------------------

with Ada.Exceptions;
with Interfaces.C;
with System;

package body GNAT.Registry is

   use Ada;
   use System;

   ------------------------------
   -- Binding to the Win32 API --
   ------------------------------

   subtype LONG is Interfaces.C.long;
   subtype ULONG is Interfaces.C.unsigned_long;
   subtype DWORD is ULONG;

   type    PULONG is access all ULONG;
   subtype PDWORD is PULONG;
   subtype LPDWORD is PDWORD;

   subtype Error_Code is LONG;

   subtype REGSAM is LONG;

   type PHKEY is access all HKEY;

   ERROR_SUCCESS : constant Error_Code := 0;

   REG_SZ : constant := 1;

   function RegCloseKey (Key : HKEY) return LONG;
   pragma Import (Stdcall, RegCloseKey, "RegCloseKey");

   function RegCreateKeyEx
     (Key                  : HKEY;
      lpSubKey             : Address;
      Reserved             : DWORD;
      lpClass              : Address;
      dwOptions            : DWORD;
      samDesired           : REGSAM;
      lpSecurityAttributes : Address;
      phkResult            : PHKEY;
      lpdwDisposition      : LPDWORD)
      return                 LONG;
   pragma Import (Stdcall, RegCreateKeyEx, "RegCreateKeyExA");

   function RegDeleteKey
     (Key      : HKEY;
      lpSubKey : Address)
      return     LONG;
   pragma Import (Stdcall, RegDeleteKey, "RegDeleteKeyA");

   function RegDeleteValue
     (Key         : HKEY;
      lpValueName : Address)
      return        LONG;
   pragma Import (Stdcall, RegDeleteValue, "RegDeleteValueA");

   function RegEnumValue
     (Key           : HKEY;
      dwIndex       : DWORD;
      lpValueName   : Address;
      lpcbValueName : LPDWORD;
      lpReserved    : LPDWORD;
      lpType        : LPDWORD;
      lpData        : Address;
      lpcbData      : LPDWORD)
      return          LONG;
   pragma Import (Stdcall, RegEnumValue, "RegEnumValueA");

   function RegOpenKeyEx
     (Key        : HKEY;
      lpSubKey   : Address;
      ulOptions  : DWORD;
      samDesired : REGSAM;
      phkResult  : PHKEY)
      return       LONG;
   pragma Import (Stdcall, RegOpenKeyEx, "RegOpenKeyExA");

   function RegQueryValueEx
     (Key         : HKEY;
      lpValueName : Address;
      lpReserved  : LPDWORD;
      lpType      : LPDWORD;
      lpData      : Address;
      lpcbData    : LPDWORD)
      return        LONG;
   pragma Import (Stdcall, RegQueryValueEx, "RegQueryValueExA");

   function RegSetValueEx
     (Key         : HKEY;
      lpValueName : Address;
      Reserved    : DWORD;
      dwType      : DWORD;
      lpData      : Address;
      cbData      : DWORD)
      return        LONG;
   pragma Import (Stdcall, RegSetValueEx, "RegSetValueExA");

   -----------------------
   -- Local Subprograms --
   -----------------------

   function To_C_Mode (Mode : Key_Mode) return REGSAM;
   --  Returns the Win32 mode value for the Key_Mode value.

   procedure Check_Result (Result : LONG; Message : String);
   --  Checks value Result and raise the exception Registry_Error if it is not
   --  equal to ERROR_SUCCESS. Message and the error value (Result) is added
   --  to the exception message.

   ------------------
   -- Check_Result --
   ------------------

   procedure Check_Result (Result : LONG; Message : String) is
      use type LONG;

   begin
      if Result /= ERROR_SUCCESS then
         Exceptions.Raise_Exception
           (Registry_Error'Identity,
            Message & " (" & LONG'Image (Result) & ')');
      end if;
   end Check_Result;

   ---------------
   -- Close_Key --
   ---------------

   procedure Close_Key (Key : HKEY) is
      Result : LONG;

   begin
      Result := RegCloseKey (Key);
      Check_Result (Result, "Close_Key");
   end Close_Key;

   ----------------
   -- Create_Key --
   ----------------

   function Create_Key
     (From_Key : HKEY;
      Sub_Key  : String;
      Mode     : Key_Mode := Read_Write)
      return     HKEY
   is
      use type REGSAM;
      use type DWORD;

      REG_OPTION_NON_VOLATILE : constant := 16#0#;

      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      C_Class   : constant String := "" & ASCII.Nul;
      C_Mode    : constant REGSAM := To_C_Mode (Mode);

      New_Key : aliased HKEY;
      Result  : LONG;
      Dispos  : aliased DWORD;

   begin
      Result := RegCreateKeyEx
        (From_Key,
         C_Sub_Key (C_Sub_Key'First)'Address,
         0,
         C_Class (C_Class'First)'Address,
         REG_OPTION_NON_VOLATILE,
         C_Mode,
         Null_Address,
         New_Key'Unchecked_Access,
         Dispos'Unchecked_Access);

      Check_Result (Result, "Create_Key " & Sub_Key);
      return New_Key;
   end Create_Key;

   ----------------
   -- Delete_Key --
   ----------------

   procedure Delete_Key (From_Key : HKEY; Sub_Key : String) is
      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      Result    : LONG;

   begin
      Result := RegDeleteKey (From_Key, C_Sub_Key (C_Sub_Key'First)'Address);
      Check_Result (Result, "Delete_Key " & Sub_Key);
   end Delete_Key;

   ------------------
   -- Delete_Value --
   ------------------

   procedure Delete_Value (From_Key : HKEY; Sub_Key : String) is
      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      Result    : LONG;

   begin
      Result := RegDeleteValue (From_Key, C_Sub_Key (C_Sub_Key'First)'Address);
      Check_Result (Result, "Delete_Value " & Sub_Key);
   end Delete_Value;

   -------------------------
   -- For_Every_Key_Value --
   -------------------------

   procedure For_Every_Key_Value (From_Key : HKEY) is
      use type LONG;
      use type ULONG;

      Index  : ULONG := 0;
      Result : LONG;

      Sub_Key : String (1 .. 100);
      pragma Warnings (Off, Sub_Key);

      Value : String (1 .. 100);
      pragma Warnings (Off, Value);

      Size_Sub_Key : aliased ULONG;
      Size_Value   : aliased ULONG;
      Type_Sub_Key : aliased DWORD;

      Quit : Boolean;

   begin
      loop
         Size_Sub_Key := Sub_Key'Length;
         Size_Value   := Value'Length;

         Result := RegEnumValue
           (From_Key, Index,
            Sub_Key (1)'Address,
            Size_Sub_Key'Unchecked_Access,
            null,
            Type_Sub_Key'Unchecked_Access,
            Value (1)'Address,
            Size_Value'Unchecked_Access);

         exit when not (Result = ERROR_SUCCESS);

         if Type_Sub_Key = REG_SZ then
            Quit := False;

            Action (Natural (Index) + 1,
                    Sub_Key (1 .. Integer (Size_Sub_Key)),
                    Value (1 .. Integer (Size_Value) - 1),
                    Quit);

            exit when Quit;

            Index := Index + 1;
         end if;

      end loop;
   end For_Every_Key_Value;

   ----------------
   -- Key_Exists --
   ----------------

   function Key_Exists
     (From_Key : HKEY;
      Sub_Key  : String)
      return     Boolean
   is
      New_Key : HKEY;

   begin
      New_Key := Open_Key (From_Key, Sub_Key);
      Close_Key (New_Key);

      --  We have been able to open the key so it exists

      return True;

   exception
      when Registry_Error =>

         --  An error occurred, the key was not found

         return False;
   end Key_Exists;

   --------------
   -- Open_Key --
   --------------

   function Open_Key
     (From_Key : HKEY;
      Sub_Key  : String;
      Mode     : Key_Mode := Read_Only)
      return     HKEY
   is
      use type REGSAM;

      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      C_Mode    : constant REGSAM := To_C_Mode (Mode);

      New_Key   : aliased HKEY;
      Result    : LONG;

   begin
      Result := RegOpenKeyEx
        (From_Key,
         C_Sub_Key (C_Sub_Key'First)'Address,
         0,
         C_Mode,
         New_Key'Unchecked_Access);

      Check_Result (Result, "Open_Key " & Sub_Key);
      return New_Key;
   end Open_Key;

   -----------------
   -- Query_Value --
   -----------------

   function Query_Value
     (From_Key : HKEY;
      Sub_Key  : String)
      return     String
   is
      use type LONG;
      use type ULONG;

      Value : String (1 .. 100);
      pragma Warnings (Off, Value);

      Size_Value : aliased ULONG;
      Type_Value : aliased DWORD;

      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      Result    : LONG;

   begin
      Size_Value := Value'Length;

      Result := RegQueryValueEx
        (From_Key,
         C_Sub_Key (C_Sub_Key'First)'Address,
         null,
         Type_Value'Unchecked_Access,
         Value (Value'First)'Address,
         Size_Value'Unchecked_Access);

      Check_Result (Result, "Query_Value " & Sub_Key & " key");

      return Value (1 .. Integer (Size_Value - 1));
   end Query_Value;

   ---------------
   -- Set_Value --
   ---------------

   procedure Set_Value
     (From_Key : HKEY;
      Sub_Key  : String;
      Value    : String)
   is
      C_Sub_Key : constant String := Sub_Key & ASCII.Nul;
      C_Value   : constant String := Value & ASCII.Nul;

      Result : LONG;

   begin
      Result := RegSetValueEx
        (From_Key,
         C_Sub_Key (C_Sub_Key'First)'Address,
         0,
         REG_SZ,
         C_Value (C_Value'First)'Address,
         C_Value'Length);

      Check_Result (Result, "Set_Value " & Sub_Key & " key");
   end Set_Value;

   ---------------
   -- To_C_Mode --
   ---------------

   function To_C_Mode (Mode : Key_Mode) return REGSAM is
      use type REGSAM;

      KEY_READ  : constant :=  16#20019#;
      KEY_WRITE : constant :=  16#20006#;

   begin
      case Mode is
         when Read_Only =>
            return KEY_READ;

         when Read_Write =>
            return KEY_READ + KEY_WRITE;
      end case;
   end To_C_Mode;

end GNAT.Registry;
