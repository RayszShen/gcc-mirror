------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--    A D A . C O N T A I N E R S . I N D E F I N I T E _ V E C T O R S     --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2004-2005, Free Software Foundation, Inc.         --
--                                                                          --
-- This specification is derived from the Ada Reference Manual for use with --
-- GNAT. The copyright notice above, and the license provisions that follow --
-- apply solely to the  contents of the part following the private keyword. --
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
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- This unit was originally developed by Matthew J Heaney.                  --
------------------------------------------------------------------------------

with Ada.Finalization;
with Ada.Streams;

generic
   type Index_Type is range <>;
   type Element_Type (<>) is private;

   with function "=" (Left, Right : Element_Type) return Boolean is <>;

package Ada.Containers.Indefinite_Vectors is
   pragma Preelaborate;

   subtype Extended_Index is Index_Type'Base
     range Index_Type'First - 1 ..
           Index_Type'Min (Index_Type'Base'Last - 1, Index_Type'Last) + 1;

   No_Index : constant Extended_Index := Extended_Index'First;

   type Vector is tagged private;

   type Cursor is private;

   Empty_Vector : constant Vector;

   No_Element : constant Cursor;

   function "=" (Left, Right : Vector) return Boolean;

   function To_Vector (Length : Count_Type) return Vector;

   function To_Vector
     (New_Item : Element_Type;
      Length   : Count_Type) return Vector;

   function "&" (Left, Right : Vector) return Vector;

   function "&" (Left : Vector; Right : Element_Type) return Vector;

   function "&" (Left : Element_Type; Right : Vector) return Vector;

   function "&" (Left, Right : Element_Type) return Vector;

   function Capacity (Container : Vector) return Count_Type;

   procedure Reserve_Capacity
     (Container : in out Vector;
      Capacity  : Count_Type);

   function Length (Container : Vector) return Count_Type;

   procedure Set_Length
     (Container : in out Vector;
      Length    : Count_Type);

   function Is_Empty (Container : Vector) return Boolean;

   procedure Clear (Container : in out Vector);

   function To_Cursor
     (Container : Vector;
      Index     : Extended_Index) return Cursor;

   function To_Index (Position : Cursor) return Extended_Index;

   function Element
     (Container : Vector;
      Index     : Index_Type) return Element_Type;

   function Element (Position : Cursor) return Element_Type;

   procedure Replace_Element
     (Container : in out Vector;
      Index     : Index_Type;
      New_Item  : Element_Type);

   procedure Replace_Element
     (Container : in out Vector;
      Position  : Cursor;
      New_Item  : Element_Type);

   procedure Query_Element
     (Container : Vector;
      Index     : Index_Type;
      Process   : not null access procedure (Element : Element_Type));

   procedure Query_Element
     (Position : Cursor;
      Process  : not null access procedure (Element : Element_Type));

   procedure Update_Element
     (Container : in out Vector;
      Index     : Index_Type;
      Process   : not null access procedure (Element : in out Element_Type));

   procedure Update_Element
     (Container : in out Vector;
      Position  : Cursor;
      Process   : not null access procedure (Element : in out Element_Type));

   procedure Move (Target : in out Vector; Source : in out Vector);

   procedure Insert
     (Container : in out Vector;
      Before    : Extended_Index;
      New_Item  : Vector);

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Vector);

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Vector;
      Position  : out Cursor);

   procedure Insert
     (Container : in out Vector;
      Before    : Extended_Index;
      New_Item  : Element_Type;
      Count     : Count_Type := 1);

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Element_Type;
      Count     : Count_Type := 1);

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Element_Type;
      Position  : out Cursor;
      Count     : Count_Type := 1);

   procedure Prepend
     (Container : in out Vector;
      New_Item  : Vector);

   procedure Prepend
     (Container : in out Vector;
      New_Item  : Element_Type;
      Count     : Count_Type := 1);

   procedure Append
     (Container : in out Vector;
      New_Item  : Vector);

   procedure Append
     (Container : in out Vector;
      New_Item  : Element_Type;
      Count     : Count_Type := 1);

   procedure Insert_Space
     (Container : in out Vector;
      Before    : Extended_Index;
      Count     : Count_Type := 1);

   procedure Insert_Space
     (Container : in out Vector;
      Before    : Cursor;
      Position  : out Cursor;
      Count     : Count_Type := 1);

   procedure Delete
     (Container : in out Vector;
      Index     : Extended_Index;
      Count     : Count_Type := 1);

   procedure Delete
     (Container : in out Vector;
      Position  : in out Cursor;
      Count     : Count_Type := 1);

   procedure Delete_First
     (Container : in out Vector;
      Count     : Count_Type := 1);

   procedure Delete_Last
     (Container : in out Vector;
      Count     : Count_Type := 1);

   procedure Reverse_Elements (Container : in out Vector);

   procedure Swap (Container : in out Vector; I, J : Index_Type);

   procedure Swap (Container : in out Vector; I, J : Cursor);

   function First_Index (Container : Vector) return Index_Type;

   function First (Container : Vector) return Cursor;

   function First_Element (Container : Vector) return Element_Type;

   function Last_Index (Container : Vector) return Extended_Index;

   function Last (Container : Vector) return Cursor;

   function Last_Element (Container : Vector) return Element_Type;

   function Next (Position : Cursor) return Cursor;

   procedure Next (Position : in out Cursor);

   function Previous (Position : Cursor) return Cursor;

   procedure Previous (Position : in out Cursor);

   function Find_Index
     (Container : Vector;
      Item      : Element_Type;
      Index     : Index_Type := Index_Type'First) return Extended_Index;

   function Find
     (Container : Vector;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor;

   function Reverse_Find_Index
     (Container : Vector;
      Item      : Element_Type;
      Index     : Index_Type := Index_Type'Last) return Extended_Index;

   function Reverse_Find
     (Container : Vector;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor;

   function Contains
     (Container : Vector;
      Item      : Element_Type) return Boolean;

   function Has_Element (Position : Cursor) return Boolean;

   procedure Iterate
     (Container : Vector;
      Process   : not null access procedure (Position : Cursor));

   procedure Reverse_Iterate
     (Container : Vector;
      Process   : not null access procedure (Position : Cursor));

   generic
      with function "<" (Left, Right : Element_Type) return Boolean is <>;
   package Generic_Sorting is

      function Is_Sorted (Container : Vector) return Boolean;

      procedure Sort (Container : in out Vector);

      procedure Merge (Target : in out Vector; Source : in out Vector);

   end Generic_Sorting;

private

   pragma Inline (First_Index);
   pragma Inline (Last_Index);
   pragma Inline (Element);
   pragma Inline (First_Element);
   pragma Inline (Last_Element);
   pragma Inline (Query_Element);
   pragma Inline (Update_Element);
   pragma Inline (Replace_Element);
   pragma Inline (Contains);

   type Element_Access is access Element_Type;

   type Elements_Type is array (Index_Type range <>) of Element_Access;

   function "=" (L, R : Elements_Type) return Boolean is abstract;

   type Elements_Access is access Elements_Type;

   use Ada.Finalization;

   type Vector is new Controlled with record
      Elements : Elements_Access;
      Last     : Extended_Index := No_Index;
      Busy     : Natural := 0;
      Lock     : Natural := 0;
   end record;

   procedure Adjust (Container : in out Vector);

   procedure Finalize (Container : in out Vector);

   use Ada.Streams;

   procedure Write
     (Stream    : access Root_Stream_Type'Class;
      Container : Vector);

   for Vector'Write use Write;

   procedure Read
     (Stream    : access Root_Stream_Type'Class;
      Container : out Vector);

   for Vector'Read use Read;

   Empty_Vector : constant Vector := (Controlled with null, No_Index, 0, 0);

   type Vector_Access is access constant Vector;
   for Vector_Access'Storage_Size use 0;

   type Cursor is record
      Container : Vector_Access;
      Index     : Index_Type := Index_Type'First;
   end record;

   procedure Write
     (Stream   : access Root_Stream_Type'Class;
      Position : Cursor);

   for Cursor'Write use Write;

   procedure Read
     (Stream   : access Root_Stream_Type'Class;
      Position : out Cursor);

   for Cursor'Read use Read;

   No_Element : constant Cursor := Cursor'(null, Index_Type'First);

end Ada.Containers.Indefinite_Vectors;
