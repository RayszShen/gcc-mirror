------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--                A D A . C O N T A I N E R S . V E C T O R S               --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2004-2011, Free Software Foundation, Inc.         --
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
-- This unit was originally developed by Matthew J Heaney.                  --
------------------------------------------------------------------------------

with Ada.Containers.Generic_Array_Sort;
with Ada.Unchecked_Deallocation;

with System; use type System.Address;

package body Ada.Containers.Vectors is

   procedure Free is
     new Ada.Unchecked_Deallocation (Elements_Type, Elements_Access);

   type Iterator is new Limited_Controlled and
     Vector_Iterator_Interfaces.Reversible_Iterator with
   record
      Container : Vector_Access;
      Index     : Index_Type;
   end record;

   overriding procedure Finalize (Object : in out Iterator);

   overriding function First (Object : Iterator) return Cursor;
   overriding function Last  (Object : Iterator) return Cursor;
   overriding function Next
     (Object : Iterator;
      Position : Cursor) return Cursor;
   overriding function Previous
     (Object   : Iterator;
      Position : Cursor) return Cursor;

   ---------
   -- "&" --
   ---------

   function "&" (Left, Right : Vector) return Vector is
      LN   : constant Count_Type := Length (Left);
      RN   : constant Count_Type := Length (Right);
      N    : Count_Type'Base;  -- length of result
      J    : Count_Type'Base;  -- for computing intermediate index values
      Last : Index_Type'Base;  -- Last index of result

   begin
      --  We decide that the capacity of the result is the sum of the lengths
      --  of the vector parameters. We could decide to make it larger, but we
      --  have no basis for knowing how much larger, so we just allocate the
      --  minimum amount of storage.

      --  Here we handle the easy cases first, when one of the vector
      --  parameters is empty. (We say "easy" because there's nothing to
      --  compute, that can potentially overflow.)

      if LN = 0 then
         if RN = 0 then
            return Empty_Vector;
         end if;

         declare
            RE : Elements_Array renames
                   Right.Elements.EA (Index_Type'First .. Right.Last);

            Elements : constant Elements_Access :=
                         new Elements_Type'(Right.Last, RE);

         begin
            return (Controlled with Elements, Right.Last, 0, 0);
         end;
      end if;

      if RN = 0 then
         declare
            LE : Elements_Array renames
                   Left.Elements.EA (Index_Type'First .. Left.Last);

            Elements : constant Elements_Access :=
                         new Elements_Type'(Left.Last, LE);

         begin
            return (Controlled with Elements, Left.Last, 0, 0);
         end;

      end if;

      --  Neither of the vector parameters is empty, so must compute the length
      --  of the result vector and its last index. (This is the harder case,
      --  because our computations must avoid overflow.)

      --  There are two constraints we need to satisfy. The first constraint is
      --  that a container cannot have more than Count_Type'Last elements, so
      --  we must check the sum of the combined lengths. Note that we cannot
      --  simply add the lengths, because of the possibility of overflow.

      if LN > Count_Type'Last - RN then
         raise Constraint_Error with "new length is out of range";
      end if;

      --  It is now safe compute the length of the new vector, without fear of
      --  overflow.

      N := LN + RN;

      --  The second constraint is that the new Last index value cannot
      --  exceed Index_Type'Last. We use the wider of Index_Type'Base and
      --  Count_Type'Base as the type for intermediate values.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We perform a two-part test. First we determine whether the
         --  computed Last value lies in the base range of the type, and then
         --  determine whether it lies in the range of the index (sub)type.

         --  Last must satisfy this relation:
         --    First + Length - 1 <= Last
         --  We regroup terms:
         --    First - 1 <= Last - Length
         --  Which can rewrite as:
         --    No_Index <= Last - Length

         if Index_Type'Base'Last - Index_Type'Base (N) < No_Index then
            raise Constraint_Error with "new length is out of range";
         end if;

         --  We now know that the computed value of Last is within the base
         --  range of the type, so it is safe to compute its value:

         Last := No_Index + Index_Type'Base (N);

         --  Finally we test whether the value is within the range of the
         --  generic actual index subtype:

         if Last > Index_Type'Last then
            raise Constraint_Error with "new length is out of range";
         end if;

      elsif Index_Type'First <= 0 then

         --  Here we can compute Last directly, in the normal way. We know that
         --  No_Index is less than 0, so there is no danger of overflow when
         --  adding the (positive) value of length.

         J := Count_Type'Base (No_Index) + N;  -- Last

         if J > Count_Type'Base (Index_Type'Last) then
            raise Constraint_Error with "new length is out of range";
         end if;

         --  We know that the computed value (having type Count_Type) of Last
         --  is within the range of the generic actual index subtype, so it is
         --  safe to convert to Index_Type:

         Last := Index_Type'Base (J);

      else
         --  Here Index_Type'First (and Index_Type'Last) is positive, so we
         --  must test the length indirectly (by working backwards from the
         --  largest possible value of Last), in order to prevent overflow.

         J := Count_Type'Base (Index_Type'Last) - N;  -- No_Index

         if J < Count_Type'Base (No_Index) then
            raise Constraint_Error with "new length is out of range";
         end if;

         --  We have determined that the result length would not create a Last
         --  index value outside of the range of Index_Type, so we can now
         --  safely compute its value.

         Last := Index_Type'Base (Count_Type'Base (No_Index) + N);
      end if;

      declare
         LE : Elements_Array renames
                Left.Elements.EA (Index_Type'First .. Left.Last);

         RE : Elements_Array renames
                Right.Elements.EA (Index_Type'First .. Right.Last);

         Elements : constant Elements_Access :=
                      new Elements_Type'(Last, LE & RE);

      begin
         return (Controlled with Elements, Last, 0, 0);
      end;
   end "&";

   function "&" (Left  : Vector; Right : Element_Type) return Vector is
   begin
      --  We decide that the capacity of the result is the sum of the lengths
      --  of the parameters. We could decide to make it larger, but we have no
      --  basis for knowing how much larger, so we just allocate the minimum
      --  amount of storage.

      --  Handle easy case first, when the vector parameter (Left) is empty

      if Left.Is_Empty then
         declare
            Elements : constant Elements_Access :=
                         new Elements_Type'
                               (Last => Index_Type'First,
                                EA   => (others => Right));

         begin
            return (Controlled with Elements, Index_Type'First, 0, 0);
         end;
      end if;

      --  The vector parameter is not empty, so we must compute the length of
      --  the result vector and its last index, but in such a way that overflow
      --  is avoided. We must satisfy two constraints: the new length cannot
      --  exceed Count_Type'Last, and the new Last index cannot exceed
      --  Index_Type'Last.

      if Left.Length = Count_Type'Last then
         raise Constraint_Error with "new length is out of range";
      end if;

      if Left.Last >= Index_Type'Last then
         raise Constraint_Error with "new length is out of range";
      end if;

      declare
         Last : constant Index_Type := Left.Last + 1;

         LE : Elements_Array renames
           Left.Elements.EA (Index_Type'First .. Left.Last);

         Elements : constant Elements_Access :=
                      new Elements_Type'(Last => Last, EA => LE & Right);

      begin
         return (Controlled with Elements, Last, 0, 0);
      end;
   end "&";

   function "&" (Left  : Element_Type; Right : Vector) return Vector is
   begin
      --  We decide that the capacity of the result is the sum of the lengths
      --  of the parameters. We could decide to make it larger, but we have no
      --  basis for knowing how much larger, so we just allocate the minimum
      --  amount of storage.

      --  Handle easy case first, when the vector parameter (Right) is empty

      if Right.Is_Empty then
         declare
            Elements : constant Elements_Access :=
                         new Elements_Type'
                               (Last => Index_Type'First,
                                EA   => (others => Left));

         begin
            return (Controlled with Elements, Index_Type'First, 0, 0);
         end;
      end if;

      --  The vector parameter is not empty, so we must compute the length of
      --  the result vector and its last index, but in such a way that overflow
      --  is avoided. We must satisfy two constraints: the new length cannot
      --  exceed Count_Type'Last, and the new Last index cannot exceed
      --  Index_Type'Last.

      if Right.Length = Count_Type'Last then
         raise Constraint_Error with "new length is out of range";
      end if;

      if Right.Last >= Index_Type'Last then
         raise Constraint_Error with "new length is out of range";
      end if;

      declare
         Last : constant Index_Type := Right.Last + 1;

         RE : Elements_Array renames
                Right.Elements.EA (Index_Type'First .. Right.Last);

         Elements : constant Elements_Access :=
                      new Elements_Type'
                        (Last => Last,
                         EA   => Left & RE);

      begin
         return (Controlled with Elements, Last, 0, 0);
      end;
   end "&";

   function "&" (Left, Right : Element_Type) return Vector is
   begin
      --  We decide that the capacity of the result is the sum of the lengths
      --  of the parameters. We could decide to make it larger, but we have no
      --  basis for knowing how much larger, so we just allocate the minimum
      --  amount of storage.

      --  We must compute the length of the result vector and its last index,
      --  but in such a way that overflow is avoided. We must satisfy two
      --  constraints: the new length cannot exceed Count_Type'Last (here, we
      --  know that that condition is satisfied), and the new Last index cannot
      --  exceed Index_Type'Last.

      if Index_Type'First >= Index_Type'Last then
         raise Constraint_Error with "new length is out of range";
      end if;

      declare
         Last : constant Index_Type := Index_Type'First + 1;

         Elements : constant Elements_Access :=
                      new Elements_Type'
                            (Last => Last,
                             EA   => (Left, Right));

      begin
         return (Controlled with Elements, Last, 0, 0);
      end;
   end "&";

   ---------
   -- "=" --
   ---------

   overriding function "=" (Left, Right : Vector) return Boolean is
   begin
      if Left'Address = Right'Address then
         return True;
      end if;

      if Left.Last /= Right.Last then
         return False;
      end if;

      for J in Index_Type range Index_Type'First .. Left.Last loop
         if Left.Elements.EA (J) /= Right.Elements.EA (J) then
            return False;
         end if;
      end loop;

      return True;
   end "=";

   ------------
   -- Adjust --
   ------------

   procedure Adjust (Container : in out Vector) is
   begin
      if Container.Last = No_Index then
         Container.Elements := null;
         return;
      end if;

      declare
         L  : constant Index_Type := Container.Last;
         EA : Elements_Array renames
                Container.Elements.EA (Index_Type'First .. L);

      begin
         Container.Elements := null;
         Container.Busy := 0;
         Container.Lock := 0;

         --  Note: it may seem that the following assignment to Container.Last
         --  is useless, since we assign it to L below. However this code is
         --  used in case 'new Elements_Type' below raises an exception, to
         --  keep Container in a consistent state.

         Container.Last := No_Index;
         Container.Elements := new Elements_Type'(L, EA);
         Container.Last := L;
      end;
   end Adjust;

   ------------
   -- Append --
   ------------

   procedure Append (Container : in out Vector; New_Item : Vector) is
   begin
      if Is_Empty (New_Item) then
         return;
      end if;

      if Container.Last = Index_Type'Last then
         raise Constraint_Error with "vector is already at its maximum length";
      end if;

      Insert
        (Container,
         Container.Last + 1,
         New_Item);
   end Append;

   procedure Append
     (Container : in out Vector;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
   begin
      if Count = 0 then
         return;
      end if;

      if Container.Last = Index_Type'Last then
         raise Constraint_Error with "vector is already at its maximum length";
      end if;

      Insert
        (Container,
         Container.Last + 1,
         New_Item,
         Count);
   end Append;

   ------------
   -- Assign --
   ------------

   procedure Assign (Target : in out Vector; Source : Vector) is
   begin
      if Target'Address = Source'Address then
         return;
      end if;

      Target.Clear;
      Target.Append (Source);
   end Assign;

   --------------
   -- Capacity --
   --------------

   function Capacity (Container : Vector) return Count_Type is
   begin
      if Container.Elements = null then
         return 0;
      else
         return Container.Elements.EA'Length;
      end if;
   end Capacity;

   -----------
   -- Clear --
   -----------

   procedure Clear (Container : in out Vector) is
   begin
      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      else
         Container.Last := No_Index;
      end if;
   end Clear;

   --------------
   -- Contains --
   --------------

   function Contains
     (Container : Vector;
      Item      : Element_Type) return Boolean
   is
   begin
      return Find_Index (Container, Item) /= No_Index;
   end Contains;

   ----------
   -- Copy --
   ----------

   function Copy
     (Source   : Vector;
      Capacity : Count_Type := 0) return Vector
   is
      C : Count_Type;

   begin
      if Capacity = 0 then
         C := Source.Length;

      elsif Capacity >= Source.Length then
         C := Capacity;

      else
         raise Capacity_Error
           with "Requested capacity is less than Source length";
      end if;

      return Target : Vector do
         Target.Reserve_Capacity (C);
         Target.Assign (Source);
      end return;
   end Copy;

   ------------
   -- Delete --
   ------------

   procedure Delete
     (Container : in out Vector;
      Index     : Extended_Index;
      Count     : Count_Type := 1)
   is
      Old_Last : constant Index_Type'Base := Container.Last;
      New_Last : Index_Type'Base;
      Count2   : Count_Type'Base;  -- count of items from Index to Old_Last
      J        : Index_Type'Base;  -- first index of items that slide down

   begin
      --  Delete removes items from the vector, the number of which is the
      --  minimum of the specified Count and the items (if any) that exist from
      --  Index to Container.Last. There are no constraints on the specified
      --  value of Count (it can be larger than what's available at this
      --  position in the vector, for example), but there are constraints on
      --  the allowed values of the Index.

      --  As a precondition on the generic actual Index_Type, the base type
      --  must include Index_Type'Pred (Index_Type'First); this is the value
      --  that Container.Last assumes when the vector is empty. However, we do
      --  not allow that as the value for Index when specifying which items
      --  should be deleted, so we must manually check. (That the user is
      --  allowed to specify the value at all here is a consequence of the
      --  declaration of the Extended_Index subtype, which includes the values
      --  in the base range that immediately precede and immediately follow the
      --  values in the Index_Type.)

      if Index < Index_Type'First then
         raise Constraint_Error with "Index is out of range (too small)";
      end if;

      --  We do allow a value greater than Container.Last to be specified as
      --  the Index, but only if it's immediately greater. This allows the
      --  corner case of deleting no items from the back end of the vector to
      --  be treated as a no-op. (It is assumed that specifying an index value
      --  greater than Last + 1 indicates some deeper flaw in the caller's
      --  algorithm, so that case is treated as a proper error.)

      if Index > Old_Last then
         if Index > Old_Last + 1 then
            raise Constraint_Error with "Index is out of range (too large)";
         end if;

         return;
      end if;

      --  Here and elsewhere we treat deleting 0 items from the container as a
      --  no-op, even when the container is busy, so we simply return.

      if Count = 0 then
         return;
      end if;

      --  The tampering bits exist to prevent an item from being deleted (or
      --  otherwise harmfully manipulated) while it is being visited. Query,
      --  Update, and Iterate increment the busy count on entry, and decrement
      --  the count on exit. Delete checks the count to determine whether it is
      --  being called while the associated callback procedure is executing.

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      --  We first calculate what's available for deletion starting at
      --  Index. Here and elsewhere we use the wider of Index_Type'Base and
      --  Count_Type'Base as the type for intermediate values. (See function
      --  Length for more information.)

      if Count_Type'Base'Last >= Index_Type'Pos (Index_Type'Base'Last) then
         Count2 := Count_Type'Base (Old_Last) - Count_Type'Base (Index) + 1;

      else
         Count2 := Count_Type'Base (Old_Last - Index + 1);
      end if;

      --  If more elements are requested (Count) for deletion than are
      --  available (Count2) for deletion beginning at Index, then everything
      --  from Index is deleted. There are no elements to slide down, and so
      --  all we need to do is set the value of Container.Last.

      if Count >= Count2 then
         Container.Last := Index - 1;
         return;
      end if;

      --  There are some elements aren't being deleted (the requested count was
      --  less than the available count), so we must slide them down to
      --  Index. We first calculate the index values of the respective array
      --  slices, using the wider of Index_Type'Base and Count_Type'Base as the
      --  type for intermediate calculations. For the elements that slide down,
      --  index value New_Last is the last index value of their new home, and
      --  index value J is the first index of their old home.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         New_Last := Old_Last - Index_Type'Base (Count);
         J := Index + Index_Type'Base (Count);

      else
         New_Last := Index_Type'Base (Count_Type'Base (Old_Last) - Count);
         J := Index_Type'Base (Count_Type'Base (Index) + Count);
      end if;

      --  The internal elements array isn't guaranteed to exist unless we have
      --  elements, but we have that guarantee here because we know we have
      --  elements to slide.  The array index values for each slice have
      --  already been determined, so we just slide down to Index the elements
      --  that weren't deleted.

      declare
         EA : Elements_Array renames Container.Elements.EA;

      begin
         EA (Index .. New_Last) := EA (J .. Old_Last);
         Container.Last := New_Last;
      end;
   end Delete;

   procedure Delete
     (Container : in out Vector;
      Position  : in out Cursor;
      Count     : Count_Type := 1)
   is
      pragma Warnings (Off, Position);

   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor denotes wrong container";
      end if;

      if Position.Index > Container.Last then
         raise Program_Error with "Position index is out of range";
      end if;

      Delete (Container, Position.Index, Count);
      Position := No_Element;
   end Delete;

   ------------------
   -- Delete_First --
   ------------------

   procedure Delete_First
     (Container : in out Vector;
      Count     : Count_Type := 1)
   is
   begin
      if Count = 0 then
         return;
      end if;

      if Count >= Length (Container) then
         Clear (Container);
         return;
      end if;

      Delete (Container, Index_Type'First, Count);
   end Delete_First;

   -----------------
   -- Delete_Last --
   -----------------

   procedure Delete_Last
     (Container : in out Vector;
      Count     : Count_Type := 1)
   is
   begin
      --  It is not permitted to delete items while the container is busy (for
      --  example, we're in the middle of a passive iteration). However, we
      --  always treat deleting 0 items as a no-op, even when we're busy, so we
      --  simply return without checking.

      if Count = 0 then
         return;
      end if;

      --  The tampering bits exist to prevent an item from being deleted (or
      --  otherwise harmfully manipulated) while it is being visited. Query,
      --  Update, and Iterate increment the busy count on entry, and decrement
      --  the count on exit. Delete_Last checks the count to determine whether
      --  it is being called while the associated callback procedure is
      --  executing.

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      --  There is no restriction on how large Count can be when deleting
      --  items. If it is equal or greater than the current length, then this
      --  is equivalent to clearing the vector. (In particular, there's no need
      --  for us to actually calculate the new value for Last.)

      --  If the requested count is less than the current length, then we must
      --  calculate the new value for Last. For the type we use the widest of
      --  Index_Type'Base and Count_Type'Base for the intermediate values of
      --  our calculation.  (See the comments in Length for more information.)

      if Count >= Container.Length then
         Container.Last := No_Index;

      elsif Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         Container.Last := Container.Last - Index_Type'Base (Count);

      else
         Container.Last :=
           Index_Type'Base (Count_Type'Base (Container.Last) - Count);
      end if;
   end Delete_Last;

   -------------
   -- Element --
   -------------

   function Element
     (Container : Vector;
      Index     : Index_Type) return Element_Type
   is
   begin
      if Index > Container.Last then
         raise Constraint_Error with "Index is out of range";
      end if;

      return Container.Elements.EA (Index);
   end Element;

   function Element (Position : Cursor) return Element_Type is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      elsif Position.Index > Position.Container.Last then
         raise Constraint_Error with "Position cursor is out of range";
      else
         return Position.Container.Elements.EA (Position.Index);
      end if;
   end Element;

   --------------
   -- Finalize --
   --------------

   procedure Finalize (Container : in out Vector) is
      X : Elements_Access := Container.Elements;

   begin
      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      Container.Elements := null;
      Container.Last := No_Index;
      Free (X);
   end Finalize;

   procedure Finalize (Object : in out Iterator) is
   begin
      if Object.Container /= null then
         declare
            B : Natural renames Object.Container.all.Busy;
         begin
            B := B - 1;
         end;
      end if;
   end Finalize;

   ----------
   -- Find --
   ----------

   function Find
     (Container : Vector;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor
   is
   begin
      if Position.Container /= null then
         if Position.Container /= Container'Unrestricted_Access then
            raise Program_Error with "Position cursor denotes wrong container";
         end if;

         if Position.Index > Container.Last then
            raise Program_Error with "Position index is out of range";
         end if;
      end if;

      for J in Position.Index .. Container.Last loop
         if Container.Elements.EA (J) = Item then
            return (Container'Unrestricted_Access, J);
         end if;
      end loop;

      return No_Element;
   end Find;

   ----------------
   -- Find_Index --
   ----------------

   function Find_Index
     (Container : Vector;
      Item      : Element_Type;
      Index     : Index_Type := Index_Type'First) return Extended_Index
   is
   begin
      for Indx in Index .. Container.Last loop
         if Container.Elements.EA (Indx) = Item then
            return Indx;
         end if;
      end loop;

      return No_Index;
   end Find_Index;

   -----------
   -- First --
   -----------

   function First (Container : Vector) return Cursor is
   begin
      if Is_Empty (Container) then
         return No_Element;
      else
         return (Container'Unrestricted_Access, Index_Type'First);
      end if;
   end First;

   function First (Object : Iterator) return Cursor is
   begin
      if Is_Empty (Object.Container.all) then
         return No_Element;
      else
         return (Object.Container, Index_Type'First);
      end if;
   end First;

   -------------------
   -- First_Element --
   -------------------

   function First_Element (Container : Vector) return Element_Type is
   begin
      if Container.Last = No_Index then
         raise Constraint_Error with "Container is empty";
      else
         return Container.Elements.EA (Index_Type'First);
      end if;
   end First_Element;

   -----------------
   -- First_Index --
   -----------------

   function First_Index (Container : Vector) return Index_Type is
      pragma Unreferenced (Container);
   begin
      return Index_Type'First;
   end First_Index;

   ---------------------
   -- Generic_Sorting --
   ---------------------

   package body Generic_Sorting is

      ---------------
      -- Is_Sorted --
      ---------------

      function Is_Sorted (Container : Vector) return Boolean is
      begin
         if Container.Last <= Index_Type'First then
            return True;
         end if;

         declare
            EA : Elements_Array renames Container.Elements.EA;
         begin
            for J in Index_Type'First .. Container.Last - 1 loop
               if EA (J + 1) < EA (J) then
                  return False;
               end if;
            end loop;
         end;

         return True;
      end Is_Sorted;

      -----------
      -- Merge --
      -----------

      procedure Merge (Target, Source : in out Vector) is
         I : Index_Type'Base := Target.Last;
         J : Index_Type'Base;

      begin
         --  The semantics of Merge changed slightly per AI05-0021. It was
         --  originally the case that if Target and Source denoted the same
         --  container object, then the GNAT implementation of Merge did
         --  nothing. However, it was argued that RM05 did not precisely
         --  specify the semantics for this corner case. The decision of the
         --  ARG was that if Target and Source denote the same non-empty
         --  container object, then Program_Error is raised.

         if Source.Last < Index_Type'First then  -- Source is empty
            return;
         end if;

         if Target'Address = Source'Address then
            raise Program_Error with
              "Target and Source denote same non-empty container";
         end if;

         if Target.Last < Index_Type'First then  -- Target is empty
            Move (Target => Target, Source => Source);
            return;
         end if;

         if Source.Busy > 0 then
            raise Program_Error with
              "attempt to tamper with cursors (vector is busy)";
         end if;

         Target.Set_Length (Length (Target) + Length (Source));

         declare
            TA : Elements_Array renames Target.Elements.EA;
            SA : Elements_Array renames Source.Elements.EA;

         begin
            J := Target.Last;
            while Source.Last >= Index_Type'First loop
               pragma Assert (Source.Last <= Index_Type'First
                                or else not (SA (Source.Last) <
                                             SA (Source.Last - 1)));

               if I < Index_Type'First then
                  TA (Index_Type'First .. J) :=
                    SA (Index_Type'First .. Source.Last);

                  Source.Last := No_Index;
                  return;
               end if;

               pragma Assert (I <= Index_Type'First
                                or else not (TA (I) < TA (I - 1)));

               if SA (Source.Last) < TA (I) then
                  TA (J) := TA (I);
                  I := I - 1;

               else
                  TA (J) := SA (Source.Last);
                  Source.Last := Source.Last - 1;
               end if;

               J := J - 1;
            end loop;
         end;
      end Merge;

      ----------
      -- Sort --
      ----------

      procedure Sort (Container : in out Vector)
      is
         procedure Sort is
            new Generic_Array_Sort
             (Index_Type   => Index_Type,
              Element_Type => Element_Type,
              Array_Type   => Elements_Array,
              "<"          => "<");

      begin
         if Container.Last <= Index_Type'First then
            return;
         end if;

         if Container.Lock > 0 then
            raise Program_Error with
              "attempt to tamper with elements (vector is locked)";
         end if;

         Sort (Container.Elements.EA (Index_Type'First .. Container.Last));
      end Sort;

   end Generic_Sorting;

   -----------------
   -- Has_Element --
   -----------------

   function Has_Element (Position : Cursor) return Boolean is
   begin
      return Position /= No_Element;
   end Has_Element;

   ------------
   -- Insert --
   ------------

   procedure Insert
     (Container : in out Vector;
      Before    : Extended_Index;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
      Old_Length : constant Count_Type := Container.Length;

      Max_Length : Count_Type'Base;  -- determined from range of Index_Type
      New_Length : Count_Type'Base;  -- sum of current length and Count
      New_Last   : Index_Type'Base;  -- last index of vector after insertion

      Index : Index_Type'Base;  -- scratch for intermediate values
      J     : Count_Type'Base;  -- scratch

      New_Capacity : Count_Type'Base;  -- length of new, expanded array
      Dst_Last     : Index_Type'Base;  -- last index of new, expanded array
      Dst          : Elements_Access;  -- new, expanded internal array

   begin
      --  As a precondition on the generic actual Index_Type, the base type
      --  must include Index_Type'Pred (Index_Type'First); this is the value
      --  that Container.Last assumes when the vector is empty. However, we do
      --  not allow that as the value for Index when specifying where the new
      --  items should be inserted, so we must manually check. (That the user
      --  is allowed to specify the value at all here is a consequence of the
      --  declaration of the Extended_Index subtype, which includes the values
      --  in the base range that immediately precede and immediately follow the
      --  values in the Index_Type.)

      if Before < Index_Type'First then
         raise Constraint_Error with
           "Before index is out of range (too small)";
      end if;

      --  We do allow a value greater than Container.Last to be specified as
      --  the Index, but only if it's immediately greater. This allows for the
      --  case of appending items to the back end of the vector. (It is assumed
      --  that specifying an index value greater than Last + 1 indicates some
      --  deeper flaw in the caller's algorithm, so that case is treated as a
      --  proper error.)

      if Before > Container.Last
        and then Before > Container.Last + 1
      then
         raise Constraint_Error with
           "Before index is out of range (too large)";
      end if;

      --  We treat inserting 0 items into the container as a no-op, even when
      --  the container is busy, so we simply return.

      if Count = 0 then
         return;
      end if;

      --  There are two constraints we need to satisfy. The first constraint is
      --  that a container cannot have more than Count_Type'Last elements, so
      --  we must check the sum of the current length and the insertion count.
      --  Note: we cannot simply add these values, because of the possibility
      --  of overflow.

      if Old_Length > Count_Type'Last - Count then
         raise Constraint_Error with "Count is out of range";
      end if;

      --  It is now safe compute the length of the new vector, without fear of
      --  overflow.

      New_Length := Old_Length + Count;

      --  The second constraint is that the new Last index value cannot exceed
      --  Index_Type'Last. In each branch below, we calculate the maximum
      --  length (computed from the range of values in Index_Type), and then
      --  compare the new length to the maximum length. If the new length is
      --  acceptable, then we compute the new last index from that.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We have to handle the case when there might be more values in the
         --  range of Index_Type than in the range of Count_Type.

         if Index_Type'First <= 0 then

            --  We know that No_Index (the same as Index_Type'First - 1) is
            --  less than 0, so it is safe to compute the following sum without
            --  fear of overflow.

            Index := No_Index + Index_Type'Base (Count_Type'Last);

            if Index <= Index_Type'Last then

               --  We have determined that range of Index_Type has at least as
               --  many values as in Count_Type, so Count_Type'Last is the
               --  maximum number of items that are allowed.

               Max_Length := Count_Type'Last;

            else
               --  The range of Index_Type has fewer values than in Count_Type,
               --  so the maximum number of items is computed from the range of
               --  the Index_Type.

               Max_Length := Count_Type'Base (Index_Type'Last - No_Index);
            end if;

         else
            --  No_Index is equal or greater than 0, so we can safely compute
            --  the difference without fear of overflow (which we would have to
            --  worry about if No_Index were less than 0, but that case is
            --  handled above).

            Max_Length := Count_Type'Base (Index_Type'Last - No_Index);
         end if;

      elsif Index_Type'First <= 0 then

         --  We know that No_Index (the same as Index_Type'First - 1) is less
         --  than 0, so it is safe to compute the following sum without fear of
         --  overflow.

         J := Count_Type'Base (No_Index) + Count_Type'Last;

         if J <= Count_Type'Base (Index_Type'Last) then

            --  We have determined that range of Index_Type has at least as
            --  many values as in Count_Type, so Count_Type'Last is the maximum
            --  number of items that are allowed.

            Max_Length := Count_Type'Last;

         else
            --  The range of Index_Type has fewer values than Count_Type does,
            --  so the maximum number of items is computed from the range of
            --  the Index_Type.

            Max_Length :=
              Count_Type'Base (Index_Type'Last) - Count_Type'Base (No_Index);
         end if;

      else
         --  No_Index is equal or greater than 0, so we can safely compute the
         --  difference without fear of overflow (which we would have to worry
         --  about if No_Index were less than 0, but that case is handled
         --  above).

         Max_Length :=
           Count_Type'Base (Index_Type'Last) - Count_Type'Base (No_Index);
      end if;

      --  We have just computed the maximum length (number of items). We must
      --  now compare the requested length to the maximum length, as we do not
      --  allow a vector expand beyond the maximum (because that would create
      --  an internal array with a last index value greater than
      --  Index_Type'Last, with no way to index those elements).

      if New_Length > Max_Length then
         raise Constraint_Error with "Count is out of range";
      end if;

      --  New_Last is the last index value of the items in the container after
      --  insertion.  Use the wider of Index_Type'Base and Count_Type'Base to
      --  compute its value from the New_Length.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         New_Last := No_Index + Index_Type'Base (New_Length);
      else
         New_Last := Index_Type'Base (Count_Type'Base (No_Index) + New_Length);
      end if;

      if Container.Elements = null then
         pragma Assert (Container.Last = No_Index);

         --  This is the simplest case, with which we must always begin: we're
         --  inserting items into an empty vector that hasn't allocated an
         --  internal array yet. Note that we don't need to check the busy bit
         --  here, because an empty container cannot be busy.

         --  In order to preserve container invariants, we allocate the new
         --  internal array first, before setting the Last index value, in case
         --  the allocation fails (which can happen either because there is no
         --  storage available, or because element initialization fails).

         Container.Elements := new Elements_Type'
                                     (Last => New_Last,
                                      EA   => (others => New_Item));

         --  The allocation of the new, internal array succeeded, so it is now
         --  safe to update the Last index, restoring container invariants.

         Container.Last := New_Last;

         return;
      end if;

      --  The tampering bits exist to prevent an item from being harmfully
      --  manipulated while it is being visited. Query, Update, and Iterate
      --  increment the busy count on entry, and decrement the count on
      --  exit. Insert checks the count to determine whether it is being called
      --  while the associated callback procedure is executing.

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      --  An internal array has already been allocated, so we must determine
      --  whether there is enough unused storage for the new items.

      if New_Length <= Container.Elements.EA'Length then

         --  In this case, we're inserting elements into a vector that has
         --  already allocated an internal array, and the existing array has
         --  enough unused storage for the new items.

         declare
            EA : Elements_Array renames Container.Elements.EA;

         begin
            if Before > Container.Last then

               --  The new items are being appended to the vector, so no
               --  sliding of existing elements is required.

               EA (Before .. New_Last) := (others => New_Item);

            else
               --  The new items are being inserted before some existing
               --  elements, so we must slide the existing elements up to their
               --  new home. We use the wider of Index_Type'Base and
               --  Count_Type'Base as the type for intermediate index values.

               if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
                  Index := Before + Index_Type'Base (Count);

               else
                  Index := Index_Type'Base (Count_Type'Base (Before) + Count);
               end if;

               EA (Index .. New_Last) := EA (Before .. Container.Last);
               EA (Before .. Index - 1) := (others => New_Item);
            end if;
         end;

         Container.Last := New_Last;
         return;
      end if;

      --  In this case, we're inserting elements into a vector that has already
      --  allocated an internal array, but the existing array does not have
      --  enough storage, so we must allocate a new, longer array. In order to
      --  guarantee that the amortized insertion cost is O(1), we always
      --  allocate an array whose length is some power-of-two factor of the
      --  current array length. (The new array cannot have a length less than
      --  the New_Length of the container, but its last index value cannot be
      --  greater than Index_Type'Last.)

      New_Capacity := Count_Type'Max (1, Container.Elements.EA'Length);
      while New_Capacity < New_Length loop
         if New_Capacity > Count_Type'Last / 2 then
            New_Capacity := Count_Type'Last;
            exit;
         end if;

         New_Capacity := 2 * New_Capacity;
      end loop;

      if New_Capacity > Max_Length then

         --  We have reached the limit of capacity, so no further expansion
         --  will occur. (This is not a problem, as there is never a need to
         --  have more capacity than the maximum container length.)

         New_Capacity := Max_Length;
      end if;

      --  We have computed the length of the new internal array (and this is
      --  what "vector capacity" means), so use that to compute its last index.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         Dst_Last := No_Index + Index_Type'Base (New_Capacity);

      else
         Dst_Last :=
           Index_Type'Base (Count_Type'Base (No_Index) + New_Capacity);
      end if;

      --  Now we allocate the new, longer internal array. If the allocation
      --  fails, we have not changed any container state, so no side-effect
      --  will occur as a result of propagating the exception.

      Dst := new Elements_Type (Dst_Last);

      --  We have our new internal array. All that needs to be done now is to
      --  copy the existing items (if any) from the old array (the "source"
      --  array, object SA below) to the new array (the "destination" array,
      --  object DA below), and then deallocate the old array.

      declare
         SA : Elements_Array renames Container.Elements.EA; -- source
         DA : Elements_Array renames Dst.EA;                -- destination

      begin
         DA (Index_Type'First .. Before - 1) :=
           SA (Index_Type'First .. Before - 1);

         if Before > Container.Last then
            DA (Before .. New_Last) := (others => New_Item);

         else
            --  The new items are being inserted before some existing elements,
            --  so we must slide the existing elements up to their new home.

            if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
               Index := Before + Index_Type'Base (Count);

            else
               Index := Index_Type'Base (Count_Type'Base (Before) + Count);
            end if;

            DA (Before .. Index - 1) := (others => New_Item);
            DA (Index .. New_Last) := SA (Before .. Container.Last);
         end if;

      exception
         when others =>
            Free (Dst);
            raise;
      end;

      --  We have successfully copied the items onto the new array, so the
      --  final thing to do is deallocate the old array.

      declare
         X : Elements_Access := Container.Elements;
      begin
         --  We first isolate the old internal array, removing it from the
         --  container and replacing it with the new internal array, before we
         --  deallocate the old array (which can fail if finalization of
         --  elements propagates an exception).

         Container.Elements := Dst;
         Container.Last := New_Last;

         --  The container invariants have been restored, so it is now safe to
         --  attempt to deallocate the old array.

         Free (X);
      end;
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Extended_Index;
      New_Item  : Vector)
   is
      N : constant Count_Type := Length (New_Item);
      J : Index_Type'Base;

   begin
      --  Use Insert_Space to create the "hole" (the destination slice) into
      --  which we copy the source items.

      Insert_Space (Container, Before, Count => N);

      if N = 0 then

         --  There's nothing else to do here (vetting of parameters was
         --  performed already in Insert_Space), so we simply return.

         return;
      end if;

      --  We calculate the last index value of the destination slice using the
      --  wider of Index_Type'Base and count_Type'Base.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         J := (Before - 1) + Index_Type'Base (N);

      else
         J := Index_Type'Base (Count_Type'Base (Before - 1) + N);
      end if;

      if Container'Address /= New_Item'Address then

         --  This is the simple case.  New_Item denotes an object different
         --  from Container, so there's nothing special we need to do to copy
         --  the source items to their destination, because all of the source
         --  items are contiguous.

         Container.Elements.EA (Before .. J) :=
           New_Item.Elements.EA (Index_Type'First .. New_Item.Last);

         return;
      end if;

      --  New_Item denotes the same object as Container, so an insertion has
      --  potentially split the source items. The destination is always the
      --  range [Before, J], but the source is [Index_Type'First, Before) and
      --  (J, Container.Last]. We perform the copy in two steps, using each of
      --  the two slices of the source items.

      declare
         L : constant Index_Type'Base := Before - 1;

         subtype Src_Index_Subtype is Index_Type'Base range
           Index_Type'First .. L;

         Src : Elements_Array renames
           Container.Elements.EA (Src_Index_Subtype);

         K : Index_Type'Base;

      begin
         --  We first copy the source items that precede the space we
         --  inserted. Index value K is the last index of that portion
         --  destination that receives this slice of the source. (If Before
         --  equals Index_Type'First, then this first source slice will be
         --  empty, which is harmless.)

         if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
            K := L + Index_Type'Base (Src'Length);

         else
            K := Index_Type'Base (Count_Type'Base (L) + Src'Length);
         end if;

         Container.Elements.EA (Before .. K) := Src;

         if Src'Length = N then

            --  The new items were effectively appended to the container, so we
            --  have already copied all of the items that need to be copied.
            --  We return early here, even though the source slice below is
            --  empty (so the assignment would be harmless), because we want to
            --  avoid computing J + 1, which will overflow if J equals
            --  Index_Type'Base'Last.

            return;
         end if;
      end;

      declare
         --  Note that we want to avoid computing J + 1 here, in case J equals
         --  Index_Type'Base'Last. We prevent that by returning early above,
         --  immediately after copying the first slice of the source, and
         --  determining that this second slice of the source is empty.

         F : constant Index_Type'Base := J + 1;

         subtype Src_Index_Subtype is Index_Type'Base range
           F .. Container.Last;

         Src : Elements_Array renames
           Container.Elements.EA (Src_Index_Subtype);

         K : Index_Type'Base;

      begin
         --  We next copy the source items that follow the space we inserted.
         --  Index value K is the first index of that portion of the
         --  destination that receives this slice of the source. (For the
         --  reasons given above, this slice is guaranteed to be non-empty.)

         if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
            K := F - Index_Type'Base (Src'Length);

         else
            K := Index_Type'Base (Count_Type'Base (F) - Src'Length);
         end if;

         Container.Elements.EA (K .. J) := Src;
      end;
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Vector)
   is
      Index : Index_Type'Base;

   begin
      if Before.Container /= null
        and then Before.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Before cursor denotes wrong container";
      end if;

      if Is_Empty (New_Item) then
         return;
      end if;

      if Before.Container = null
        or else Before.Index > Container.Last
      then
         if Container.Last = Index_Type'Last then
            raise Constraint_Error with
              "vector is already at its maximum length";
         end if;

         Index := Container.Last + 1;

      else
         Index := Before.Index;
      end if;

      Insert (Container, Index, New_Item);
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Vector;
      Position  : out Cursor)
   is
      Index : Index_Type'Base;

   begin
      if Before.Container /= null
        and then Before.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Before cursor denotes wrong container";
      end if;

      if Is_Empty (New_Item) then
         if Before.Container = null
           or else Before.Index > Container.Last
         then
            Position := No_Element;
         else
            Position := (Container'Unrestricted_Access, Before.Index);
         end if;

         return;
      end if;

      if Before.Container = null
        or else Before.Index > Container.Last
      then
         if Container.Last = Index_Type'Last then
            raise Constraint_Error with
              "vector is already at its maximum length";
         end if;

         Index := Container.Last + 1;

      else
         Index := Before.Index;
      end if;

      Insert (Container, Index, New_Item);

      Position := (Container'Unrestricted_Access, Index);
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
      Index : Index_Type'Base;

   begin
      if Before.Container /= null
        and then Before.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Before cursor denotes wrong container";
      end if;

      if Count = 0 then
         return;
      end if;

      if Before.Container = null
        or else Before.Index > Container.Last
      then
         if Container.Last = Index_Type'Last then
            raise Constraint_Error with
              "vector is already at its maximum length";
         else
            Index := Container.Last + 1;
         end if;

      else
         Index := Before.Index;
      end if;

      Insert (Container, Index, New_Item, Count);
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      New_Item  : Element_Type;
      Position  : out Cursor;
      Count     : Count_Type := 1)
   is
      Index : Index_Type'Base;

   begin
      if Before.Container /= null
        and then Before.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Before cursor denotes wrong container";
      end if;

      if Count = 0 then
         if Before.Container = null
           or else Before.Index > Container.Last
         then
            Position := No_Element;
         else
            Position := (Container'Unrestricted_Access, Before.Index);
         end if;

         return;
      end if;

      if Before.Container = null
        or else Before.Index > Container.Last
      then
         if Container.Last = Index_Type'Last then
            raise Constraint_Error with
              "vector is already at its maximum length";
         end if;

         Index := Container.Last + 1;

      else
         Index := Before.Index;
      end if;

      Insert (Container, Index, New_Item, Count);

      Position := (Container'Unrestricted_Access, Index);
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Extended_Index;
      Count     : Count_Type := 1)
   is
      New_Item : Element_Type;  -- Default-initialized value
      pragma Warnings (Off, New_Item);

   begin
      Insert (Container, Before, New_Item, Count);
   end Insert;

   procedure Insert
     (Container : in out Vector;
      Before    : Cursor;
      Position  : out Cursor;
      Count     : Count_Type := 1)
   is
      New_Item : Element_Type;  -- Default-initialized value
      pragma Warnings (Off, New_Item);

   begin
      Insert (Container, Before, New_Item, Position, Count);
   end Insert;

   ------------------
   -- Insert_Space --
   ------------------

   procedure Insert_Space
     (Container : in out Vector;
      Before    : Extended_Index;
      Count     : Count_Type := 1)
   is
      Old_Length : constant Count_Type := Container.Length;

      Max_Length : Count_Type'Base;  -- determined from range of Index_Type
      New_Length : Count_Type'Base;  -- sum of current length and Count
      New_Last   : Index_Type'Base;  -- last index of vector after insertion

      Index : Index_Type'Base;  -- scratch for intermediate values
      J     : Count_Type'Base;  -- scratch

      New_Capacity : Count_Type'Base;  -- length of new, expanded array
      Dst_Last     : Index_Type'Base;  -- last index of new, expanded array
      Dst          : Elements_Access;  -- new, expanded internal array

   begin
      --  As a precondition on the generic actual Index_Type, the base type
      --  must include Index_Type'Pred (Index_Type'First); this is the value
      --  that Container.Last assumes when the vector is empty. However, we do
      --  not allow that as the value for Index when specifying where the new
      --  items should be inserted, so we must manually check. (That the user
      --  is allowed to specify the value at all here is a consequence of the
      --  declaration of the Extended_Index subtype, which includes the values
      --  in the base range that immediately precede and immediately follow the
      --  values in the Index_Type.)

      if Before < Index_Type'First then
         raise Constraint_Error with
           "Before index is out of range (too small)";
      end if;

      --  We do allow a value greater than Container.Last to be specified as
      --  the Index, but only if it's immediately greater. This allows for the
      --  case of appending items to the back end of the vector. (It is assumed
      --  that specifying an index value greater than Last + 1 indicates some
      --  deeper flaw in the caller's algorithm, so that case is treated as a
      --  proper error.)

      if Before > Container.Last
        and then Before > Container.Last + 1
      then
         raise Constraint_Error with
           "Before index is out of range (too large)";
      end if;

      --  We treat inserting 0 items into the container as a no-op, even when
      --  the container is busy, so we simply return.

      if Count = 0 then
         return;
      end if;

      --  There are two constraints we need to satisfy. The first constraint is
      --  that a container cannot have more than Count_Type'Last elements, so
      --  we must check the sum of the current length and the insertion count.
      --  Note: we cannot simply add these values, because of the possibility
      --  of overflow.

      if Old_Length > Count_Type'Last - Count then
         raise Constraint_Error with "Count is out of range";
      end if;

      --  It is now safe compute the length of the new vector, without fear of
      --  overflow.

      New_Length := Old_Length + Count;

      --  The second constraint is that the new Last index value cannot exceed
      --  Index_Type'Last. In each branch below, we calculate the maximum
      --  length (computed from the range of values in Index_Type), and then
      --  compare the new length to the maximum length. If the new length is
      --  acceptable, then we compute the new last index from that.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We have to handle the case when there might be more values in the
         --  range of Index_Type than in the range of Count_Type.

         if Index_Type'First <= 0 then

            --  We know that No_Index (the same as Index_Type'First - 1) is
            --  less than 0, so it is safe to compute the following sum without
            --  fear of overflow.

            Index := No_Index + Index_Type'Base (Count_Type'Last);

            if Index <= Index_Type'Last then

               --  We have determined that range of Index_Type has at least as
               --  many values as in Count_Type, so Count_Type'Last is the
               --  maximum number of items that are allowed.

               Max_Length := Count_Type'Last;

            else
               --  The range of Index_Type has fewer values than in Count_Type,
               --  so the maximum number of items is computed from the range of
               --  the Index_Type.

               Max_Length := Count_Type'Base (Index_Type'Last - No_Index);
            end if;

         else
            --  No_Index is equal or greater than 0, so we can safely compute
            --  the difference without fear of overflow (which we would have to
            --  worry about if No_Index were less than 0, but that case is
            --  handled above).

            Max_Length := Count_Type'Base (Index_Type'Last - No_Index);
         end if;

      elsif Index_Type'First <= 0 then

         --  We know that No_Index (the same as Index_Type'First - 1) is less
         --  than 0, so it is safe to compute the following sum without fear of
         --  overflow.

         J := Count_Type'Base (No_Index) + Count_Type'Last;

         if J <= Count_Type'Base (Index_Type'Last) then

            --  We have determined that range of Index_Type has at least as
            --  many values as in Count_Type, so Count_Type'Last is the maximum
            --  number of items that are allowed.

            Max_Length := Count_Type'Last;

         else
            --  The range of Index_Type has fewer values than Count_Type does,
            --  so the maximum number of items is computed from the range of
            --  the Index_Type.

            Max_Length :=
              Count_Type'Base (Index_Type'Last) - Count_Type'Base (No_Index);
         end if;

      else
         --  No_Index is equal or greater than 0, so we can safely compute the
         --  difference without fear of overflow (which we would have to worry
         --  about if No_Index were less than 0, but that case is handled
         --  above).

         Max_Length :=
           Count_Type'Base (Index_Type'Last) - Count_Type'Base (No_Index);
      end if;

      --  We have just computed the maximum length (number of items). We must
      --  now compare the requested length to the maximum length, as we do not
      --  allow a vector expand beyond the maximum (because that would create
      --  an internal array with a last index value greater than
      --  Index_Type'Last, with no way to index those elements).

      if New_Length > Max_Length then
         raise Constraint_Error with "Count is out of range";
      end if;

      --  New_Last is the last index value of the items in the container after
      --  insertion.  Use the wider of Index_Type'Base and Count_Type'Base to
      --  compute its value from the New_Length.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         New_Last := No_Index + Index_Type'Base (New_Length);

      else
         New_Last := Index_Type'Base (Count_Type'Base (No_Index) + New_Length);
      end if;

      if Container.Elements = null then
         pragma Assert (Container.Last = No_Index);

         --  This is the simplest case, with which we must always begin: we're
         --  inserting items into an empty vector that hasn't allocated an
         --  internal array yet. Note that we don't need to check the busy bit
         --  here, because an empty container cannot be busy.

         --  In order to preserve container invariants, we allocate the new
         --  internal array first, before setting the Last index value, in case
         --  the allocation fails (which can happen either because there is no
         --  storage available, or because default-valued element
         --  initialization fails).

         Container.Elements := new Elements_Type (New_Last);

         --  The allocation of the new, internal array succeeded, so it is now
         --  safe to update the Last index, restoring container invariants.

         Container.Last := New_Last;

         return;
      end if;

      --  The tampering bits exist to prevent an item from being harmfully
      --  manipulated while it is being visited. Query, Update, and Iterate
      --  increment the busy count on entry, and decrement the count on
      --  exit. Insert checks the count to determine whether it is being called
      --  while the associated callback procedure is executing.

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      --  An internal array has already been allocated, so we must determine
      --  whether there is enough unused storage for the new items.

      if New_Last <= Container.Elements.Last then

         --  In this case, we're inserting space into a vector that has already
         --  allocated an internal array, and the existing array has enough
         --  unused storage for the new items.

         declare
            EA : Elements_Array renames Container.Elements.EA;

         begin
            if Before <= Container.Last then

               --  The space is being inserted before some existing elements,
               --  so we must slide the existing elements up to their new
               --  home. We use the wider of Index_Type'Base and
               --  Count_Type'Base as the type for intermediate index values.

               if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
                  Index := Before + Index_Type'Base (Count);

               else
                  Index := Index_Type'Base (Count_Type'Base (Before) + Count);
               end if;

               EA (Index .. New_Last) := EA (Before .. Container.Last);
            end if;
         end;

         Container.Last := New_Last;
         return;
      end if;

      --  In this case, we're inserting space into a vector that has already
      --  allocated an internal array, but the existing array does not have
      --  enough storage, so we must allocate a new, longer array. In order to
      --  guarantee that the amortized insertion cost is O(1), we always
      --  allocate an array whose length is some power-of-two factor of the
      --  current array length. (The new array cannot have a length less than
      --  the New_Length of the container, but its last index value cannot be
      --  greater than Index_Type'Last.)

      New_Capacity := Count_Type'Max (1, Container.Elements.EA'Length);
      while New_Capacity < New_Length loop
         if New_Capacity > Count_Type'Last / 2 then
            New_Capacity := Count_Type'Last;
            exit;
         end if;

         New_Capacity := 2 * New_Capacity;
      end loop;

      if New_Capacity > Max_Length then

         --  We have reached the limit of capacity, so no further expansion
         --  will occur. (This is not a problem, as there is never a need to
         --  have more capacity than the maximum container length.)

         New_Capacity := Max_Length;
      end if;

      --  We have computed the length of the new internal array (and this is
      --  what "vector capacity" means), so use that to compute its last index.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
         Dst_Last := No_Index + Index_Type'Base (New_Capacity);

      else
         Dst_Last :=
           Index_Type'Base (Count_Type'Base (No_Index) + New_Capacity);
      end if;

      --  Now we allocate the new, longer internal array. If the allocation
      --  fails, we have not changed any container state, so no side-effect
      --  will occur as a result of propagating the exception.

      Dst := new Elements_Type (Dst_Last);

      --  We have our new internal array. All that needs to be done now is to
      --  copy the existing items (if any) from the old array (the "source"
      --  array, object SA below) to the new array (the "destination" array,
      --  object DA below), and then deallocate the old array.

      declare
         SA : Elements_Array renames Container.Elements.EA;  -- source
         DA : Elements_Array renames Dst.EA;                 -- destination

      begin
         DA (Index_Type'First .. Before - 1) :=
           SA (Index_Type'First .. Before - 1);

         if Before <= Container.Last then

            --  The space is being inserted before some existing elements, so
            --  we must slide the existing elements up to their new home.

            if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then
               Index := Before + Index_Type'Base (Count);

            else
               Index := Index_Type'Base (Count_Type'Base (Before) + Count);
            end if;

            DA (Index .. New_Last) := SA (Before .. Container.Last);
         end if;

      exception
         when others =>
            Free (Dst);
            raise;
      end;

      --  We have successfully copied the items onto the new array, so the
      --  final thing to do is restore invariants, and deallocate the old
      --  array.

      declare
         X : Elements_Access := Container.Elements;

      begin
         --  We first isolate the old internal array, removing it from the
         --  container and replacing it with the new internal array, before we
         --  deallocate the old array (which can fail if finalization of
         --  elements propagates an exception).

         Container.Elements := Dst;
         Container.Last := New_Last;

         --  The container invariants have been restored, so it is now safe to
         --  attempt to deallocate the old array.

         Free (X);
      end;
   end Insert_Space;

   procedure Insert_Space
     (Container : in out Vector;
      Before    : Cursor;
      Position  : out Cursor;
      Count     : Count_Type := 1)
   is
      Index : Index_Type'Base;

   begin
      if Before.Container /= null
        and then Before.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Before cursor denotes wrong container";
      end if;

      if Count = 0 then
         if Before.Container = null
           or else Before.Index > Container.Last
         then
            Position := No_Element;
         else
            Position := (Container'Unrestricted_Access, Before.Index);
         end if;

         return;
      end if;

      if Before.Container = null
        or else Before.Index > Container.Last
      then
         if Container.Last = Index_Type'Last then
            raise Constraint_Error with
              "vector is already at its maximum length";
         else
            Index := Container.Last + 1;
         end if;

      else
         Index := Before.Index;
      end if;

      Insert_Space (Container, Index, Count => Count);

      Position := (Container'Unrestricted_Access, Index);
   end Insert_Space;

   --------------
   -- Is_Empty --
   --------------

   function Is_Empty (Container : Vector) return Boolean is
   begin
      return Container.Last < Index_Type'First;
   end Is_Empty;

   -------------
   -- Iterate --
   -------------

   procedure Iterate
     (Container : Vector;
      Process   : not null access procedure (Position : Cursor))
   is
      B : Natural renames Container'Unrestricted_Access.all.Busy;

   begin
      B := B + 1;

      begin
         for Indx in Index_Type'First .. Container.Last loop
            Process (Cursor'(Container'Unrestricted_Access, Indx));
         end loop;
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Iterate;

   function Iterate
     (Container : Vector)
      return Vector_Iterator_Interfaces.Reversible_Iterator'Class
   is
      B  : Natural renames Container'Unrestricted_Access.all.Busy;

   begin
      return It : constant Iterator :=
                    (Limited_Controlled with
                       Container => Container'Unrestricted_Access,
                       Index     => Index_Type'First)
      do
         B := B + 1;
      end return;
   end Iterate;

   function Iterate
     (Container : Vector;
      Start     : Cursor)
      return Vector_Iterator_Interfaces.Reversible_Iterator'class
   is
      B  : Natural renames Container'Unrestricted_Access.all.Busy;

   begin
      return It : constant Iterator :=
                    (Limited_Controlled with
                       Container => Container'Unrestricted_Access,
                       Index     => Start.Index)
      do
         B := B + 1;
      end return;
   end Iterate;

   ----------
   -- Last --
   ----------

   function Last (Container : Vector) return Cursor is
   begin
      if Is_Empty (Container) then
         return No_Element;
      else
         return (Container'Unrestricted_Access, Container.Last);
      end if;
   end Last;

   function Last (Object : Iterator) return Cursor is
   begin
      if Is_Empty (Object.Container.all) then
         return No_Element;
      else
         return (Object.Container, Object.Container.Last);
      end if;
   end Last;

   ------------------
   -- Last_Element --
   ------------------

   function Last_Element (Container : Vector) return Element_Type is
   begin
      if Container.Last = No_Index then
         raise Constraint_Error with "Container is empty";
      else
         return Container.Elements.EA (Container.Last);
      end if;
   end Last_Element;

   ----------------
   -- Last_Index --
   ----------------

   function Last_Index (Container : Vector) return Extended_Index is
   begin
      return Container.Last;
   end Last_Index;

   ------------
   -- Length --
   ------------

   function Length (Container : Vector) return Count_Type is
      L : constant Index_Type'Base := Container.Last;
      F : constant Index_Type := Index_Type'First;

   begin
      --  The base range of the index type (Index_Type'Base) might not include
      --  all values for length (Count_Type). Contrariwise, the index type
      --  might include values outside the range of length.  Hence we use
      --  whatever type is wider for intermediate values when calculating
      --  length. Note that no matter what the index type is, the maximum
      --  length to which a vector is allowed to grow is always the minimum
      --  of Count_Type'Last and (IT'Last - IT'First + 1).

      --  For example, an Index_Type with range -127 .. 127 is only guaranteed
      --  to have a base range of -128 .. 127, but the corresponding vector
      --  would have lengths in the range 0 .. 255. In this case we would need
      --  to use Count_Type'Base for intermediate values.

      --  Another case would be the index range -2**63 + 1 .. -2**63 + 10. The
      --  vector would have a maximum length of 10, but the index values lie
      --  outside the range of Count_Type (which is only 32 bits). In this
      --  case we would need to use Index_Type'Base for intermediate values.

      if Count_Type'Base'Last >= Index_Type'Pos (Index_Type'Base'Last) then
         return Count_Type'Base (L) - Count_Type'Base (F) + 1;
      else
         return Count_Type (L - F + 1);
      end if;
   end Length;

   ----------
   -- Move --
   ----------

   procedure Move
     (Target : in out Vector;
      Source : in out Vector)
   is
   begin
      if Target'Address = Source'Address then
         return;
      end if;

      if Target.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (Target is busy)";
      end if;

      if Source.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (Source is busy)";
      end if;

      declare
         Target_Elements : constant Elements_Access := Target.Elements;
      begin
         Target.Elements := Source.Elements;
         Source.Elements := Target_Elements;
      end;

      Target.Last := Source.Last;
      Source.Last := No_Index;
   end Move;

   ----------
   -- Next --
   ----------

   function Next (Position : Cursor) return Cursor is
   begin
      if Position.Container = null then
         return No_Element;
      elsif Position.Index < Position.Container.Last then
         return (Position.Container, Position.Index + 1);
      else
         return No_Element;
      end if;
   end Next;

   function Next (Object : Iterator; Position : Cursor) return Cursor is
   begin
      if Position.Index < Object.Container.Last then
         return (Object.Container, Position.Index + 1);
      else
         return No_Element;
      end if;
   end Next;

   procedure Next (Position : in out Cursor) is
   begin
      if Position.Container = null then
         return;
      elsif Position.Index < Position.Container.Last then
         Position.Index := Position.Index + 1;
      else
         Position := No_Element;
      end if;
   end Next;

   -------------
   -- Prepend --
   -------------

   procedure Prepend (Container : in out Vector; New_Item : Vector) is
   begin
      Insert (Container, Index_Type'First, New_Item);
   end Prepend;

   procedure Prepend
     (Container : in out Vector;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
   begin
      Insert (Container,
              Index_Type'First,
              New_Item,
              Count);
   end Prepend;

   --------------
   -- Previous --
   --------------

   function Previous (Position : Cursor) return Cursor is
   begin
      if Position.Container = null then
         return No_Element;
      elsif Position.Index > Index_Type'First then
         return (Position.Container, Position.Index - 1);
      else
         return No_Element;
      end if;
   end Previous;

   function Previous (Object : Iterator; Position : Cursor) return Cursor is
   begin
      if Position.Index > Index_Type'First then
         return (Object.Container, Position.Index - 1);
      else
         return No_Element;
      end if;
   end Previous;

   procedure Previous (Position : in out Cursor) is
   begin
      if Position.Container = null then
         return;
      elsif Position.Index > Index_Type'First then
         Position.Index := Position.Index - 1;
      else
         Position := No_Element;
      end if;
   end Previous;

   -------------------
   -- Query_Element --
   -------------------

   procedure Query_Element
     (Container : Vector;
      Index     : Index_Type;
      Process   : not null access procedure (Element : Element_Type))
   is
      V : Vector renames Container'Unrestricted_Access.all;
      B : Natural renames V.Busy;
      L : Natural renames V.Lock;

   begin
      if Index > Container.Last then
         raise Constraint_Error with "Index is out of range";
      end if;

      B := B + 1;
      L := L + 1;

      begin
         Process (V.Elements.EA (Index));
      exception
         when others =>
            L := L - 1;
            B := B - 1;
            raise;
      end;

      L := L - 1;
      B := B - 1;
   end Query_Element;

   procedure Query_Element
     (Position : Cursor;
      Process  : not null access procedure (Element : Element_Type))
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      Query_Element (Position.Container.all, Position.Index, Process);
   end Query_Element;

   ----------
   -- Read --
   ----------

   procedure Read
     (Stream    : not null access Root_Stream_Type'Class;
      Container : out Vector)
   is
      Length : Count_Type'Base;
      Last   : Index_Type'Base := No_Index;

   begin
      Clear (Container);

      Count_Type'Base'Read (Stream, Length);

      if Length > Capacity (Container) then
         Reserve_Capacity (Container, Capacity => Length);
      end if;

      for J in Count_Type range 1 .. Length loop
         Last := Last + 1;
         Element_Type'Read (Stream, Container.Elements.EA (Last));
         Container.Last := Last;
      end loop;
   end Read;

   procedure Read
     (Stream   : not null access Root_Stream_Type'Class;
      Position : out Cursor)
   is
   begin
      raise Program_Error with "attempt to stream vector cursor";
   end Read;

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out Reference_Type)
   is
   begin
      raise Program_Error with "attempt to stream reference";
   end Read;

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out Constant_Reference_Type)
   is
   begin
      raise Program_Error with "attempt to stream reference";
   end Read;

   ---------------
   -- Reference --
   ---------------

   function Constant_Reference
     (Container : Vector;
      Position  : Cursor)    --  SHOULD BE ALIASED
      return Constant_Reference_Type
   is
   begin
      pragma Unreferenced (Container);

      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Index > Position.Container.Last then
         raise Constraint_Error with "Position cursor is out of range";
      end if;

      return
       (Element =>
          Position.Container.Elements.EA (Position.Index)'Access);
   end Constant_Reference;

   function Constant_Reference
     (Container : Vector;
      Position  : Index_Type)
      return Constant_Reference_Type
   is
   begin
      if Position > Container.Last then
         raise Constraint_Error with "Index is out of range";
      else
         return (Element => Container.Elements.EA (Position)'Access);
      end if;
   end Constant_Reference;

   function Reference (Container : Vector; Position : Cursor)
   return Reference_Type is
   begin
      pragma Unreferenced (Container);

      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Index > Position.Container.Last then
         raise Constraint_Error with "Position cursor is out of range";
      end if;

      return
        (Element => Position.Container.Elements.EA (Position.Index)'Access);
   end Reference;

   function Reference (Container : Vector; Position : Index_Type)
   return Reference_Type is
   begin
      if Position > Container.Last then
         raise Constraint_Error with "Index is out of range";
      else
         return (Element => Container.Elements.EA (Position)'Access);
      end if;
   end Reference;

   ---------------------
   -- Replace_Element --
   ---------------------

   procedure Replace_Element
     (Container : in out Vector;
      Index     : Index_Type;
      New_Item  : Element_Type)
   is
   begin
      if Index > Container.Last then
         raise Constraint_Error with "Index is out of range";
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (vector is locked)";
      end if;

      Container.Elements.EA (Index) := New_Item;
   end Replace_Element;

   procedure Replace_Element
     (Container : in out Vector;
      Position  : Cursor;
      New_Item  : Element_Type)
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor denotes wrong container";
      end if;

      if Position.Index > Container.Last then
         raise Constraint_Error with "Position cursor is out of range";
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (vector is locked)";
      end if;

      Container.Elements.EA (Position.Index) := New_Item;
   end Replace_Element;

   ----------------------
   -- Reserve_Capacity --
   ----------------------

   procedure Reserve_Capacity
     (Container : in out Vector;
      Capacity  : Count_Type)
   is
      N : constant Count_Type := Length (Container);

      Index : Count_Type'Base;
      Last  : Index_Type'Base;

   begin
      --  Reserve_Capacity can be used to either expand the storage available
      --  for elements (this would be its typical use, in anticipation of
      --  future insertion), or to trim back storage. In the latter case,
      --  storage can only be trimmed back to the limit of the container
      --  length. Note that Reserve_Capacity neither deletes (active) elements
      --  nor inserts elements; it only affects container capacity, never
      --  container length.

      if Capacity = 0 then

         --  This is a request to trim back storage, to the minimum amount
         --  possible given the current state of the container.

         if N = 0 then

            --  The container is empty, so in this unique case we can
            --  deallocate the entire internal array. Note that an empty
            --  container can never be busy, so there's no need to check the
            --  tampering bits.

            declare
               X : Elements_Access := Container.Elements;

            begin
               --  First we remove the internal array from the container, to
               --  handle the case when the deallocation raises an exception.

               Container.Elements := null;

               --  Container invariants have been restored, so it is now safe
               --  to attempt to deallocate the internal array.

               Free (X);
            end;

         elsif N < Container.Elements.EA'Length then

            --  The container is not empty, and the current length is less than
            --  the current capacity, so there's storage available to trim. In
            --  this case, we allocate a new internal array having a length
            --  that exactly matches the number of items in the
            --  container. (Reserve_Capacity does not delete active elements,
            --  so this is the best we can do with respect to minimizing
            --  storage).

            if Container.Busy > 0 then
               raise Program_Error with
                 "attempt to tamper with cursors (vector is busy)";
            end if;

            declare
               subtype Src_Index_Subtype is Index_Type'Base range
                 Index_Type'First .. Container.Last;

               Src : Elements_Array renames
                       Container.Elements.EA (Src_Index_Subtype);

               X : Elements_Access := Container.Elements;

            begin
               --  Although we have isolated the old internal array that we're
               --  going to deallocate, we don't deallocate it until we have
               --  successfully allocated a new one. If there is an exception
               --  during allocation (either because there is not enough
               --  storage, or because initialization of the elements fails),
               --  we let it propagate without causing any side-effect.

               Container.Elements := new Elements_Type'(Container.Last, Src);

               --  We have successfully allocated a new internal array (with a
               --  smaller length than the old one, and containing a copy of
               --  just the active elements in the container), so it is now
               --  safe to attempt to deallocate the old array. The old array
               --  has been isolated, and container invariants have been
               --  restored, so if the deallocation fails (because finalization
               --  of the elements fails), we simply let it propagate.

               Free (X);
            end;
         end if;

         return;
      end if;

      --  Reserve_Capacity can be used to expand the storage available for
      --  elements, but we do not let the capacity grow beyond the number of
      --  values in Index_Type'Range. (Were it otherwise, there would be no way
      --  to refer to the elements with an index value greater than
      --  Index_Type'Last, so that storage would be wasted.) Here we compute
      --  the Last index value of the new internal array, in a way that avoids
      --  any possibility of overflow.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We perform a two-part test. First we determine whether the
         --  computed Last value lies in the base range of the type, and then
         --  determine whether it lies in the range of the index (sub)type.

         --  Last must satisfy this relation:
         --    First + Length - 1 <= Last
         --  We regroup terms:
         --    First - 1 <= Last - Length
         --  Which can rewrite as:
         --    No_Index <= Last - Length

         if Index_Type'Base'Last - Index_Type'Base (Capacity) < No_Index then
            raise Constraint_Error with "Capacity is out of range";
         end if;

         --  We now know that the computed value of Last is within the base
         --  range of the type, so it is safe to compute its value:

         Last := No_Index + Index_Type'Base (Capacity);

         --  Finally we test whether the value is within the range of the
         --  generic actual index subtype:

         if Last > Index_Type'Last then
            raise Constraint_Error with "Capacity is out of range";
         end if;

      elsif Index_Type'First <= 0 then

         --  Here we can compute Last directly, in the normal way. We know that
         --  No_Index is less than 0, so there is no danger of overflow when
         --  adding the (positive) value of Capacity.

         Index := Count_Type'Base (No_Index) + Capacity;  -- Last

         if Index > Count_Type'Base (Index_Type'Last) then
            raise Constraint_Error with "Capacity is out of range";
         end if;

         --  We know that the computed value (having type Count_Type) of Last
         --  is within the range of the generic actual index subtype, so it is
         --  safe to convert to Index_Type:

         Last := Index_Type'Base (Index);

      else
         --  Here Index_Type'First (and Index_Type'Last) is positive, so we
         --  must test the length indirectly (by working backwards from the
         --  largest possible value of Last), in order to prevent overflow.

         Index := Count_Type'Base (Index_Type'Last) - Capacity;  -- No_Index

         if Index < Count_Type'Base (No_Index) then
            raise Constraint_Error with "Capacity is out of range";
         end if;

         --  We have determined that the value of Capacity would not create a
         --  Last index value outside of the range of Index_Type, so we can now
         --  safely compute its value.

         Last := Index_Type'Base (Count_Type'Base (No_Index) + Capacity);
      end if;

      --  The requested capacity is non-zero, but we don't know yet whether
      --  this is a request for expansion or contraction of storage.

      if Container.Elements = null then

         --  The container is empty (it doesn't even have an internal array),
         --  so this represents a request to allocate (expand) storage having
         --  the given capacity.

         Container.Elements := new Elements_Type (Last);
         return;
      end if;

      if Capacity <= N then

         --  This is a request to trim back storage, but only to the limit of
         --  what's already in the container. (Reserve_Capacity never deletes
         --  active elements, it only reclaims excess storage.)

         if N < Container.Elements.EA'Length then

            --  The container is not empty (because the requested capacity is
            --  positive, and less than or equal to the container length), and
            --  the current length is less than the current capacity, so
            --  there's storage available to trim. In this case, we allocate a
            --  new internal array having a length that exactly matches the
            --  number of items in the container.

            if Container.Busy > 0 then
               raise Program_Error with
                 "attempt to tamper with cursors (vector is busy)";
            end if;

            declare
               subtype Src_Index_Subtype is Index_Type'Base range
                 Index_Type'First .. Container.Last;

               Src : Elements_Array renames
                       Container.Elements.EA (Src_Index_Subtype);

               X : Elements_Access := Container.Elements;

            begin
               --  Although we have isolated the old internal array that we're
               --  going to deallocate, we don't deallocate it until we have
               --  successfully allocated a new one. If there is an exception
               --  during allocation (either because there is not enough
               --  storage, or because initialization of the elements fails),
               --  we let it propagate without causing any side-effect.

               Container.Elements := new Elements_Type'(Container.Last, Src);

               --  We have successfully allocated a new internal array (with a
               --  smaller length than the old one, and containing a copy of
               --  just the active elements in the container), so it is now
               --  safe to attempt to deallocate the old array. The old array
               --  has been isolated, and container invariants have been
               --  restored, so if the deallocation fails (because finalization
               --  of the elements fails), we simply let it propagate.

               Free (X);
            end;
         end if;

         return;
      end if;

      --  The requested capacity is larger than the container length (the
      --  number of active elements). Whether this represents a request for
      --  expansion or contraction of the current capacity depends on what the
      --  current capacity is.

      if Capacity = Container.Elements.EA'Length then

         --  The requested capacity matches the existing capacity, so there's
         --  nothing to do here. We treat this case as a no-op, and simply
         --  return without checking the busy bit.

         return;
      end if;

      --  There is a change in the capacity of a non-empty container, so a new
      --  internal array will be allocated. (The length of the new internal
      --  array could be less or greater than the old internal array. We know
      --  only that the length of the new internal array is greater than the
      --  number of active elements in the container.) We must check whether
      --  the container is busy before doing anything else.

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (vector is busy)";
      end if;

      --  We now allocate a new internal array, having a length different from
      --  its current value.

      declare
         E : Elements_Access := new Elements_Type (Last);

      begin
         --  We have successfully allocated the new internal array. We first
         --  attempt to copy the existing elements from the old internal array
         --  ("src" elements) onto the new internal array ("tgt" elements).

         declare
            subtype Index_Subtype is Index_Type'Base range
              Index_Type'First .. Container.Last;

            Src : Elements_Array renames
                    Container.Elements.EA (Index_Subtype);

            Tgt : Elements_Array renames E.EA (Index_Subtype);

         begin
            Tgt := Src;

         exception
            when others =>
               Free (E);
               raise;
         end;

         --  We have successfully copied the existing elements onto the new
         --  internal array, so now we can attempt to deallocate the old one.

         declare
            X : Elements_Access := Container.Elements;

         begin
            --  First we isolate the old internal array, and replace it in the
            --  container with the new internal array.

            Container.Elements := E;

            --  Container invariants have been restored, so it is now safe to
            --  attempt to deallocate the old internal array.

            Free (X);
         end;
      end;
   end Reserve_Capacity;

   ----------------------
   -- Reverse_Elements --
   ----------------------

   procedure Reverse_Elements (Container : in out Vector) is
   begin
      if Container.Length <= 1 then
         return;
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (vector is locked)";
      end if;

      declare
         I, J : Index_Type;
         E    : Elements_Type renames Container.Elements.all;

      begin
         I := Index_Type'First;
         J := Container.Last;
         while I < J loop
            declare
               EI : constant Element_Type := E.EA (I);

            begin
               E.EA (I) := E.EA (J);
               E.EA (J) := EI;
            end;

            I := I + 1;
            J := J - 1;
         end loop;
      end;
   end Reverse_Elements;

   ------------------
   -- Reverse_Find --
   ------------------

   function Reverse_Find
     (Container : Vector;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor
   is
      Last : Index_Type'Base;

   begin
      if Position.Container /= null
        and then Position.Container /= Container'Unrestricted_Access
      then
         raise Program_Error with "Position cursor denotes wrong container";
      end if;

      Last :=
        (if Position.Container = null or else Position.Index > Container.Last
         then Container.Last
         else Position.Index);

      for Indx in reverse Index_Type'First .. Last loop
         if Container.Elements.EA (Indx) = Item then
            return (Container'Unrestricted_Access, Indx);
         end if;
      end loop;

      return No_Element;
   end Reverse_Find;

   ------------------------
   -- Reverse_Find_Index --
   ------------------------

   function Reverse_Find_Index
     (Container : Vector;
      Item      : Element_Type;
      Index     : Index_Type := Index_Type'Last) return Extended_Index
   is
      Last : constant Index_Type'Base :=
               Index_Type'Min (Container.Last, Index);

   begin
      for Indx in reverse Index_Type'First .. Last loop
         if Container.Elements.EA (Indx) = Item then
            return Indx;
         end if;
      end loop;

      return No_Index;
   end Reverse_Find_Index;

   ---------------------
   -- Reverse_Iterate --
   ---------------------

   procedure Reverse_Iterate
     (Container : Vector;
      Process   : not null access procedure (Position : Cursor))
   is
      V : Vector renames Container'Unrestricted_Access.all;
      B : Natural renames V.Busy;

   begin
      B := B + 1;

      begin
         for Indx in reverse Index_Type'First .. Container.Last loop
            Process (Cursor'(Container'Unrestricted_Access, Indx));
         end loop;
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Reverse_Iterate;

   ----------------
   -- Set_Length --
   ----------------

   procedure Set_Length (Container : in out Vector; Length : Count_Type) is
      Count : constant Count_Type'Base := Container.Length - Length;

   begin
      --  Set_Length allows the user to set the length explicitly, instead of
      --  implicitly as a side-effect of deletion or insertion. If the
      --  requested length is less then the current length, this is equivalent
      --  to deleting items from the back end of the vector. If the requested
      --  length is greater than the current length, then this is equivalent to
      --  inserting "space" (nonce items) at the end.

      if Count >= 0 then
         Container.Delete_Last (Count);

      elsif Container.Last >= Index_Type'Last then
         raise Constraint_Error with "vector is already at its maximum length";

      else
         Container.Insert_Space (Container.Last + 1, -Count);
      end if;
   end Set_Length;

   ----------
   -- Swap --
   ----------

   procedure Swap (Container : in out Vector; I, J : Index_Type) is
   begin
      if I > Container.Last then
         raise Constraint_Error with "I index is out of range";
      end if;

      if J > Container.Last then
         raise Constraint_Error with "J index is out of range";
      end if;

      if I = J then
         return;
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (vector is locked)";
      end if;

      declare
         EI_Copy : constant Element_Type := Container.Elements.EA (I);
      begin
         Container.Elements.EA (I) := Container.Elements.EA (J);
         Container.Elements.EA (J) := EI_Copy;
      end;
   end Swap;

   procedure Swap (Container : in out Vector; I, J : Cursor) is
   begin
      if I.Container = null then
         raise Constraint_Error with "I cursor has no element";
      end if;

      if J.Container = null then
         raise Constraint_Error with "J cursor has no element";
      end if;

      if I.Container /= Container'Unrestricted_Access then
         raise Program_Error with "I cursor denotes wrong container";
      end if;

      if J.Container /= Container'Unrestricted_Access then
         raise Program_Error with "J cursor denotes wrong container";
      end if;

      Swap (Container, I.Index, J.Index);
   end Swap;

   ---------------
   -- To_Cursor --
   ---------------

   function To_Cursor
     (Container : Vector;
      Index     : Extended_Index) return Cursor
   is
   begin
      if Index not in Index_Type'First .. Container.Last then
         return No_Element;
      else
         return (Container'Unrestricted_Access, Index);
      end if;
   end To_Cursor;

   --------------
   -- To_Index --
   --------------

   function To_Index (Position : Cursor) return Extended_Index is
   begin
      if Position.Container = null then
         return No_Index;
      end if;

      if Position.Index <= Position.Container.Last then
         return Position.Index;
      end if;

      return No_Index;
   end To_Index;

   ---------------
   -- To_Vector --
   ---------------

   function To_Vector (Length : Count_Type) return Vector is
      Index    : Count_Type'Base;
      Last     : Index_Type'Base;
      Elements : Elements_Access;

   begin
      if Length = 0 then
         return Empty_Vector;
      end if;

      --  We create a vector object with a capacity that matches the specified
      --  Length, but we do not allow the vector capacity (the length of the
      --  internal array) to exceed the number of values in Index_Type'Range
      --  (otherwise, there would be no way to refer to those components via an
      --  index).  We must therefore check whether the specified Length would
      --  create a Last index value greater than Index_Type'Last.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We perform a two-part test. First we determine whether the
         --  computed Last value lies in the base range of the type, and then
         --  determine whether it lies in the range of the index (sub)type.

         --  Last must satisfy this relation:
         --    First + Length - 1 <= Last
         --  We regroup terms:
         --    First - 1 <= Last - Length
         --  Which can rewrite as:
         --    No_Index <= Last - Length

         if Index_Type'Base'Last - Index_Type'Base (Length) < No_Index then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We now know that the computed value of Last is within the base
         --  range of the type, so it is safe to compute its value:

         Last := No_Index + Index_Type'Base (Length);

         --  Finally we test whether the value is within the range of the
         --  generic actual index subtype:

         if Last > Index_Type'Last then
            raise Constraint_Error with "Length is out of range";
         end if;

      elsif Index_Type'First <= 0 then

         --  Here we can compute Last directly, in the normal way. We know that
         --  No_Index is less than 0, so there is no danger of overflow when
         --  adding the (positive) value of Length.

         Index := Count_Type'Base (No_Index) + Length;  -- Last

         if Index > Count_Type'Base (Index_Type'Last) then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We know that the computed value (having type Count_Type) of Last
         --  is within the range of the generic actual index subtype, so it is
         --  safe to convert to Index_Type:

         Last := Index_Type'Base (Index);

      else
         --  Here Index_Type'First (and Index_Type'Last) is positive, so we
         --  must test the length indirectly (by working backwards from the
         --  largest possible value of Last), in order to prevent overflow.

         Index := Count_Type'Base (Index_Type'Last) - Length;  -- No_Index

         if Index < Count_Type'Base (No_Index) then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We have determined that the value of Length would not create a
         --  Last index value outside of the range of Index_Type, so we can now
         --  safely compute its value.

         Last := Index_Type'Base (Count_Type'Base (No_Index) + Length);
      end if;

      Elements := new Elements_Type (Last);

      return Vector'(Controlled with Elements, Last, 0, 0);
   end To_Vector;

   function To_Vector
     (New_Item : Element_Type;
      Length   : Count_Type) return Vector
   is
      Index    : Count_Type'Base;
      Last     : Index_Type'Base;
      Elements : Elements_Access;

   begin
      if Length = 0 then
         return Empty_Vector;
      end if;

      --  We create a vector object with a capacity that matches the specified
      --  Length, but we do not allow the vector capacity (the length of the
      --  internal array) to exceed the number of values in Index_Type'Range
      --  (otherwise, there would be no way to refer to those components via an
      --  index). We must therefore check whether the specified Length would
      --  create a Last index value greater than Index_Type'Last.

      if Index_Type'Base'Last >= Count_Type'Pos (Count_Type'Last) then

         --  We perform a two-part test. First we determine whether the
         --  computed Last value lies in the base range of the type, and then
         --  determine whether it lies in the range of the index (sub)type.

         --  Last must satisfy this relation:
         --    First + Length - 1 <= Last
         --  We regroup terms:
         --    First - 1 <= Last - Length
         --  Which can rewrite as:
         --    No_Index <= Last - Length

         if Index_Type'Base'Last - Index_Type'Base (Length) < No_Index then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We now know that the computed value of Last is within the base
         --  range of the type, so it is safe to compute its value:

         Last := No_Index + Index_Type'Base (Length);

         --  Finally we test whether the value is within the range of the
         --  generic actual index subtype:

         if Last > Index_Type'Last then
            raise Constraint_Error with "Length is out of range";
         end if;

      elsif Index_Type'First <= 0 then
         --  Here we can compute Last directly, in the normal way. We know that
         --  No_Index is less than 0, so there is no danger of overflow when
         --  adding the (positive) value of Length.

         Index := Count_Type'Base (No_Index) + Length;  -- same value as V.Last

         if Index > Count_Type'Base (Index_Type'Last) then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We know that the computed value (having type Count_Type) of Last
         --  is within the range of the generic actual index subtype, so it is
         --  safe to convert to Index_Type:

         Last := Index_Type'Base (Index);

      else
         --  Here Index_Type'First (and Index_Type'Last) is positive, so we
         --  must test the length indirectly (by working backwards from the
         --  largest possible value of Last), in order to prevent overflow.

         Index := Count_Type'Base (Index_Type'Last) - Length;  -- No_Index

         if Index < Count_Type'Base (No_Index) then
            raise Constraint_Error with "Length is out of range";
         end if;

         --  We have determined that the value of Length would not create a
         --  Last index value outside of the range of Index_Type, so we can now
         --  safely compute its value.

         Last := Index_Type'Base (Count_Type'Base (No_Index) + Length);
      end if;

      Elements := new Elements_Type'(Last, EA => (others => New_Item));

      return Vector'(Controlled with Elements, Last, 0, 0);
   end To_Vector;

   --------------------
   -- Update_Element --
   --------------------

   procedure Update_Element
     (Container : in out Vector;
      Index     : Index_Type;
      Process   : not null access procedure (Element : in out Element_Type))
   is
      B : Natural renames Container.Busy;
      L : Natural renames Container.Lock;

   begin
      if Index > Container.Last then
         raise Constraint_Error with "Index is out of range";
      end if;

      B := B + 1;
      L := L + 1;

      begin
         Process (Container.Elements.EA (Index));
      exception
         when others =>
            L := L - 1;
            B := B - 1;
            raise;
      end;

      L := L - 1;
      B := B - 1;
   end Update_Element;

   procedure Update_Element
     (Container : in out Vector;
      Position  : Cursor;
      Process   : not null access procedure (Element : in out Element_Type))
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor denotes wrong container";
      end if;

      Update_Element (Container, Position.Index, Process);
   end Update_Element;

   -----------
   -- Write --
   -----------

   procedure Write
     (Stream    : not null access Root_Stream_Type'Class;
      Container : Vector)
   is
   begin
      Count_Type'Base'Write (Stream, Length (Container));

      for J in Index_Type'First .. Container.Last loop
         Element_Type'Write (Stream, Container.Elements.EA (J));
      end loop;
   end Write;

   procedure Write
     (Stream   : not null access Root_Stream_Type'Class;
      Position : Cursor)
   is
   begin
      raise Program_Error with "attempt to stream vector cursor";
   end Write;

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : Reference_Type)
   is
   begin
      raise Program_Error with "attempt to stream reference";
   end Write;

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : Constant_Reference_Type)
   is
   begin
      raise Program_Error with "attempt to stream reference";
   end Write;

end Ada.Containers.Vectors;
