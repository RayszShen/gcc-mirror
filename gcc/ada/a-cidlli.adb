------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--               ADA.CONTAINERS.INDEFINITE_DOUBLY_LINKED_LISTS              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2004-2012, Free Software Foundation, Inc.         --
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

with Ada.Unchecked_Deallocation;

with System; use type System.Address;

package body Ada.Containers.Indefinite_Doubly_Linked_Lists is

   procedure Free is
     new Ada.Unchecked_Deallocation (Element_Type, Element_Access);

   type Iterator is new Limited_Controlled and
     List_Iterator_Interfaces.Reversible_Iterator with
   record
      Container : List_Access;
      Node      : Node_Access;
   end record;

   overriding procedure Finalize (Object : in out Iterator);

   overriding function First (Object : Iterator) return Cursor;
   overriding function Last  (Object : Iterator) return Cursor;

   overriding function Next
     (Object   : Iterator;
      Position : Cursor) return Cursor;

   overriding function Previous
     (Object   : Iterator;
      Position : Cursor) return Cursor;

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Free (X : in out Node_Access);

   procedure Insert_Internal
     (Container : in out List;
      Before    : Node_Access;
      New_Node  : Node_Access);

   function Vet (Position : Cursor) return Boolean;
   --  Checks invariants of the cursor and its designated container, as a
   --  simple way of detecting dangling references (see operation Free for a
   --  description of the detection mechanism), returning True if all checks
   --  pass. Invocations of Vet are used here as the argument of pragma Assert,
   --  so the checks are performed only when assertions are enabled.

   ---------
   -- "=" --
   ---------

   function "=" (Left, Right : List) return Boolean is
      L : Node_Access;
      R : Node_Access;

   begin
      if Left'Address = Right'Address then
         return True;
      end if;

      if Left.Length /= Right.Length then
         return False;
      end if;

      L := Left.First;
      R := Right.First;
      for J in 1 .. Left.Length loop
         if L.Element.all /= R.Element.all then
            return False;
         end if;

         L := L.Next;
         R := R.Next;
      end loop;

      return True;
   end "=";

   ------------
   -- Adjust --
   ------------

   procedure Adjust (Container : in out List) is
      Src : Node_Access := Container.First;
      Dst : Node_Access;

   begin
      if Src = null then
         pragma Assert (Container.Last = null);
         pragma Assert (Container.Length = 0);
         pragma Assert (Container.Busy = 0);
         pragma Assert (Container.Lock = 0);
         return;
      end if;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);
      pragma Assert (Container.Length > 0);

      Container.First := null;
      Container.Last := null;
      Container.Length := 0;
      Container.Busy := 0;
      Container.Lock := 0;

      declare
         Element : Element_Access := new Element_Type'(Src.Element.all);
      begin
         Dst := new Node_Type'(Element, null, null);
      exception
         when others =>
            Free (Element);
            raise;
      end;

      Container.First := Dst;
      Container.Last := Dst;
      Container.Length := 1;

      Src := Src.Next;
      while Src /= null loop
         declare
            Element : Element_Access := new Element_Type'(Src.Element.all);
         begin
            Dst := new Node_Type'(Element, null, Prev => Container.Last);
         exception
            when others =>
               Free (Element);
               raise;
         end;

         Container.Last.Next := Dst;
         Container.Last := Dst;
         Container.Length := Container.Length + 1;

         Src := Src.Next;
      end loop;
   end Adjust;

   procedure Adjust (Control : in out Reference_Control_Type) is
   begin
      if Control.Container /= null then
         declare
            C : List renames Control.Container.all;
            B : Natural renames C.Busy;
            L : Natural renames C.Lock;
         begin
            B := B + 1;
            L := L + 1;
         end;
      end if;
   end Adjust;

   ------------
   -- Append --
   ------------

   procedure Append
     (Container : in out List;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
   begin
      Insert (Container, No_Element, New_Item, Count);
   end Append;

   ------------
   -- Assign --
   ------------

   procedure Assign (Target : in out List; Source : List) is
      Node : Node_Access;

   begin
      if Target'Address = Source'Address then
         return;
      end if;

      Target.Clear;

      Node := Source.First;
      while Node /= null loop
         Target.Append (Node.Element.all);
         Node := Node.Next;
      end loop;
   end Assign;

   -----------
   -- Clear --
   -----------

   procedure Clear (Container : in out List) is
      X : Node_Access;
      pragma Warnings (Off, X);

   begin
      if Container.Length = 0 then
         pragma Assert (Container.First = null);
         pragma Assert (Container.Last = null);
         pragma Assert (Container.Busy = 0);
         pragma Assert (Container.Lock = 0);
         return;
      end if;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      while Container.Length > 1 loop
         X := Container.First;
         pragma Assert (X.Next.Prev = Container.First);

         Container.First := X.Next;
         Container.First.Prev := null;

         Container.Length := Container.Length - 1;

         Free (X);
      end loop;

      X := Container.First;
      pragma Assert (X = Container.Last);

      Container.First := null;
      Container.Last := null;
      Container.Length := 0;

      Free (X);
   end Clear;

   ------------------------
   -- Constant_Reference --
   ------------------------

   function Constant_Reference
     (Container : aliased List;
      Position  : Cursor) return Constant_Reference_Type
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Node has no element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Constant_Reference");

      declare
         C : List renames Position.Container.all;
         B : Natural renames C.Busy;
         L : Natural renames C.Lock;
      begin
         return R : constant Constant_Reference_Type :=
                      (Element => Position.Node.Element.all'Access,
                       Control => (Controlled with Position.Container))
         do
            B := B + 1;
            L := L + 1;
         end return;
      end;
   end Constant_Reference;

   --------------
   -- Contains --
   --------------

   function Contains
     (Container : List;
      Item      : Element_Type) return Boolean
   is
   begin
      return Find (Container, Item) /= No_Element;
   end Contains;

   ----------
   -- Copy --
   ----------

   function Copy (Source : List) return List is
   begin
      return Target : List do
         Target.Assign (Source);
      end return;
   end Copy;

   ------------
   -- Delete --
   ------------

   procedure Delete
     (Container : in out List;
      Position  : in out Cursor;
      Count     : Count_Type := 1)
   is
      X : Node_Access;

   begin
      if Position.Node = null then
         raise Constraint_Error with
           "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Delete");

      if Position.Node = Container.First then
         Delete_First (Container, Count);
         Position := No_Element;  --  Post-York behavior
         return;
      end if;

      if Count = 0 then
         Position := No_Element;  --  Post-York behavior
         return;
      end if;

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      for Index in 1 .. Count loop
         X := Position.Node;
         Container.Length := Container.Length - 1;

         if X = Container.Last then
            Position := No_Element;

            Container.Last := X.Prev;
            Container.Last.Next := null;

            Free (X);
            return;
         end if;

         Position.Node := X.Next;

         X.Next.Prev := X.Prev;
         X.Prev.Next := X.Next;

         Free (X);
      end loop;

      Position := No_Element;  --  Post-York behavior
   end Delete;

   ------------------
   -- Delete_First --
   ------------------

   procedure Delete_First
     (Container : in out List;
      Count     : Count_Type := 1)
   is
      X : Node_Access;

   begin
      if Count >= Container.Length then
         Clear (Container);
         return;
      end if;

      if Count = 0 then
         return;
      end if;

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      for I in 1 .. Count loop
         X := Container.First;
         pragma Assert (X.Next.Prev = Container.First);

         Container.First := X.Next;
         Container.First.Prev := null;

         Container.Length := Container.Length - 1;

         Free (X);
      end loop;
   end Delete_First;

   -----------------
   -- Delete_Last --
   -----------------

   procedure Delete_Last
     (Container : in out List;
      Count     : Count_Type := 1)
   is
      X : Node_Access;

   begin
      if Count >= Container.Length then
         Clear (Container);
         return;
      end if;

      if Count = 0 then
         return;
      end if;

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      for I in 1 .. Count loop
         X := Container.Last;
         pragma Assert (X.Prev.Next = Container.Last);

         Container.Last := X.Prev;
         Container.Last.Next := null;

         Container.Length := Container.Length - 1;

         Free (X);
      end loop;
   end Delete_Last;

   -------------
   -- Element --
   -------------

   function Element (Position : Cursor) return Element_Type is
   begin
      if Position.Node = null then
         raise Constraint_Error with
           "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Element");

      return Position.Node.Element.all;
   end Element;

   --------------
   -- Finalize --
   --------------

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

   procedure Finalize (Control : in out Reference_Control_Type) is
   begin
      if Control.Container /= null then
         declare
            C : List renames Control.Container.all;
            B : Natural renames C.Busy;
            L : Natural renames C.Lock;
         begin
            B := B - 1;
            L := L - 1;
         end;

         Control.Container := null;
      end if;
   end Finalize;

   ----------
   -- Find --
   ----------

   function Find
     (Container : List;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor
   is
      Node : Node_Access := Position.Node;

   begin
      if Node = null then
         Node := Container.First;

      else
         if Node.Element = null then
            raise Program_Error;
         end if;

         if Position.Container /= Container'Unrestricted_Access then
            raise Program_Error with
              "Position cursor designates wrong container";
         end if;

         pragma Assert (Vet (Position), "bad cursor in Find");
      end if;

      while Node /= null loop
         if Node.Element.all = Item then
            return Cursor'(Container'Unrestricted_Access, Node);
         end if;

         Node := Node.Next;
      end loop;

      return No_Element;
   end Find;

   -----------
   -- First --
   -----------

   function First (Container : List) return Cursor is
   begin
      if Container.First = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Container.First);
   end First;

   function First (Object : Iterator) return Cursor is
   begin
      --  The value of the iterator object's Node component influences the
      --  behavior of the First (and Last) selector function.

      --  When the Node component is null, this means the iterator object was
      --  constructed without a start expression, in which case the (forward)
      --  iteration starts from the (logical) beginning of the entire sequence
      --  of items (corresponding to Container.First, for a forward iterator).

      --  Otherwise, this is iteration over a partial sequence of items. When
      --  the Node component is non-null, the iterator object was constructed
      --  with a start expression, that specifies the position from which the
      --  (forward) partial iteration begins.

      if Object.Node = null then
         return Indefinite_Doubly_Linked_Lists.First (Object.Container.all);
      else
         return Cursor'(Object.Container, Object.Node);
      end if;
   end First;

   -------------------
   -- First_Element --
   -------------------

   function First_Element (Container : List) return Element_Type is
   begin
      if Container.First = null then
         raise Constraint_Error with "list is empty";
      end if;

      return Container.First.Element.all;
   end First_Element;

   ----------
   -- Free --
   ----------

   procedure Free (X : in out Node_Access) is
      procedure Deallocate is
         new Ada.Unchecked_Deallocation (Node_Type, Node_Access);

   begin
      --  While a node is in use, as an active link in a list, its Previous and
      --  Next components must be null, or designate a different node; this is
      --  a node invariant. For this indefinite list, there is an additional
      --  invariant: that the element access value be non-null. Before actually
      --  deallocating the node, we set the node access value components of the
      --  node to point to the node itself, and set the element access value to
      --  null (by deallocating the node's element), thus falsifying the node
      --  invariant. Subprogram Vet inspects the value of the node components
      --  when interrogating the node, in order to detect whether the cursor's
      --  node access value is dangling.

      --  Note that we have no guarantee that the storage for the node isn't
      --  modified when it is deallocated, but there are other tests that Vet
      --  does if node invariants appear to be satisifed. However, in practice
      --  this simple test works well enough, detecting dangling references
      --  immediately, without needing further interrogation.

      X.Next := X;
      X.Prev := X;

      begin
         Free (X.Element);
      exception
         when others =>
            X.Element := null;
            Deallocate (X);
            raise;
      end;

      Deallocate (X);
   end Free;

   ---------------------
   -- Generic_Sorting --
   ---------------------

   package body Generic_Sorting is

      ---------------
      -- Is_Sorted --
      ---------------

      function Is_Sorted (Container : List) return Boolean is
         Node : Node_Access := Container.First;

      begin
         for I in 2 .. Container.Length loop
            if Node.Next.Element.all < Node.Element.all then
               return False;
            end if;

            Node := Node.Next;
         end loop;

         return True;
      end Is_Sorted;

      -----------
      -- Merge --
      -----------

      procedure Merge
        (Target : in out List;
         Source : in out List)
      is
         LI, RI : Cursor;

      begin

         --  The semantics of Merge changed slightly per AI05-0021. It was
         --  originally the case that if Target and Source denoted the same
         --  container object, then the GNAT implementation of Merge did
         --  nothing. However, it was argued that RM05 did not precisely
         --  specify the semantics for this corner case. The decision of the
         --  ARG was that if Target and Source denote the same non-empty
         --  container object, then Program_Error is raised.

         if Source.Is_Empty then
            return;
         end if;

         if Target'Address = Source'Address then
            raise Program_Error with
              "Target and Source denote same non-empty container";
         end if;

         if Target.Busy > 0 then
            raise Program_Error with
              "attempt to tamper with cursors of Target (list is busy)";
         end if;

         if Source.Busy > 0 then
            raise Program_Error with
              "attempt to tamper with cursors of Source (list is busy)";
         end if;

         LI := First (Target);
         RI := First (Source);
         while RI.Node /= null loop
            pragma Assert (RI.Node.Next = null
                             or else not (RI.Node.Next.Element.all <
                                          RI.Node.Element.all));

            if LI.Node = null then
               Splice (Target, No_Element, Source);
               return;
            end if;

            pragma Assert (LI.Node.Next = null
                             or else not (LI.Node.Next.Element.all <
                                          LI.Node.Element.all));

            if RI.Node.Element.all < LI.Node.Element.all then
               declare
                  RJ : Cursor := RI;
                  pragma Warnings (Off, RJ);
               begin
                  RI.Node := RI.Node.Next;
                  Splice (Target, LI, Source, RJ);
               end;

            else
               LI.Node := LI.Node.Next;
            end if;
         end loop;
      end Merge;

      ----------
      -- Sort --
      ----------

      procedure Sort (Container : in out List) is
         procedure Partition (Pivot : Node_Access; Back  : Node_Access);

         procedure Sort (Front, Back : Node_Access);

         ---------------
         -- Partition --
         ---------------

         procedure Partition (Pivot : Node_Access; Back : Node_Access) is
            Node : Node_Access := Pivot.Next;

         begin
            while Node /= Back loop
               if Node.Element.all < Pivot.Element.all then
                  declare
                     Prev : constant Node_Access := Node.Prev;
                     Next : constant Node_Access := Node.Next;
                  begin
                     Prev.Next := Next;

                     if Next = null then
                        Container.Last := Prev;
                     else
                        Next.Prev := Prev;
                     end if;

                     Node.Next := Pivot;
                     Node.Prev := Pivot.Prev;

                     Pivot.Prev := Node;

                     if Node.Prev = null then
                        Container.First := Node;
                     else
                        Node.Prev.Next := Node;
                     end if;

                     Node := Next;
                  end;

               else
                  Node := Node.Next;
               end if;
            end loop;
         end Partition;

         ----------
         -- Sort --
         ----------

         procedure Sort (Front, Back : Node_Access) is
            Pivot : constant Node_Access :=
                      (if Front = null then Container.First else Front.Next);
         begin
            if Pivot /= Back then
               Partition (Pivot, Back);
               Sort (Front, Pivot);
               Sort (Pivot, Back);
            end if;
         end Sort;

      --  Start of processing for Sort

      begin
         if Container.Length <= 1 then
            return;
         end if;

         pragma Assert (Container.First.Prev = null);
         pragma Assert (Container.Last.Next = null);

         if Container.Busy > 0 then
            raise Program_Error with
              "attempt to tamper with cursors (list is busy)";
         end if;

         Sort (Front => null, Back => null);

         pragma Assert (Container.First.Prev = null);
         pragma Assert (Container.Last.Next = null);
      end Sort;

   end Generic_Sorting;

   -----------------
   -- Has_Element --
   -----------------

   function Has_Element (Position : Cursor) return Boolean is
   begin
      pragma Assert (Vet (Position), "bad cursor in Has_Element");
      return Position.Node /= null;
   end Has_Element;

   ------------
   -- Insert --
   ------------

   procedure Insert
     (Container : in out List;
      Before    : Cursor;
      New_Item  : Element_Type;
      Position  : out Cursor;
      Count     : Count_Type := 1)
   is
      New_Node : Node_Access;

   begin
      if Before.Container /= null then
         if Before.Container /= Container'Unrestricted_Access then
            raise Program_Error with
              "attempt to tamper with cursors (list is busy)";
         end if;

         if Before.Node = null
           or else Before.Node.Element = null
         then
            raise Program_Error with
              "Before cursor has no element";
         end if;

         pragma Assert (Vet (Before), "bad cursor in Insert");
      end if;

      if Count = 0 then
         Position := Before;
         return;
      end if;

      if Container.Length > Count_Type'Last - Count then
         raise Constraint_Error with "new length exceeds maximum";
      end if;

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      declare
         Element : Element_Access := new Element_Type'(New_Item);
      begin
         New_Node := new Node_Type'(Element, null, null);
      exception
         when others =>
            Free (Element);
            raise;
      end;

      Insert_Internal (Container, Before.Node, New_Node);
      Position := Cursor'(Container'Unchecked_Access, New_Node);

      for J in Count_Type'(2) .. Count loop

         declare
            Element : Element_Access := new Element_Type'(New_Item);
         begin
            New_Node := new Node_Type'(Element, null, null);
         exception
            when others =>
               Free (Element);
               raise;
         end;

         Insert_Internal (Container, Before.Node, New_Node);
      end loop;
   end Insert;

   procedure Insert
     (Container : in out List;
      Before    : Cursor;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
      Position : Cursor;
      pragma Unreferenced (Position);
   begin
      Insert (Container, Before, New_Item, Position, Count);
   end Insert;

   ---------------------
   -- Insert_Internal --
   ---------------------

   procedure Insert_Internal
     (Container : in out List;
      Before    : Node_Access;
      New_Node  : Node_Access)
   is
   begin
      if Container.Length = 0 then
         pragma Assert (Before = null);
         pragma Assert (Container.First = null);
         pragma Assert (Container.Last = null);

         Container.First := New_Node;
         Container.Last := New_Node;

      elsif Before = null then
         pragma Assert (Container.Last.Next = null);

         Container.Last.Next := New_Node;
         New_Node.Prev := Container.Last;

         Container.Last := New_Node;

      elsif Before = Container.First then
         pragma Assert (Container.First.Prev = null);

         Container.First.Prev := New_Node;
         New_Node.Next := Container.First;

         Container.First := New_Node;

      else
         pragma Assert (Container.First.Prev = null);
         pragma Assert (Container.Last.Next = null);

         New_Node.Next := Before;
         New_Node.Prev := Before.Prev;

         Before.Prev.Next := New_Node;
         Before.Prev := New_Node;
      end if;

      Container.Length := Container.Length + 1;
   end Insert_Internal;

   --------------
   -- Is_Empty --
   --------------

   function Is_Empty (Container : List) return Boolean is
   begin
      return Container.Length = 0;
   end Is_Empty;

   -------------
   -- Iterate --
   -------------

   procedure Iterate
     (Container : List;
      Process   : not null access procedure (Position : Cursor))
   is
      B    : Natural renames Container'Unrestricted_Access.all.Busy;
      Node : Node_Access := Container.First;

   begin
      B := B + 1;

      begin
         while Node /= null loop
            Process (Cursor'(Container'Unrestricted_Access, Node));
            Node := Node.Next;
         end loop;
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Iterate;

   function Iterate
     (Container : List)
      return List_Iterator_Interfaces.Reversible_Iterator'class
   is
      B : Natural renames Container'Unrestricted_Access.all.Busy;

   begin
      --  The value of the Node component influences the behavior of the First
      --  and Last selector functions of the iterator object. When the Node
      --  component is null (as is the case here), this means the iterator
      --  object was constructed without a start expression. This is a
      --  complete iterator, meaning that the iteration starts from the
      --  (logical) beginning of the sequence of items.

      --  Note: For a forward iterator, Container.First is the beginning, and
      --  for a reverse iterator, Container.Last is the beginning.

      return It : constant Iterator :=
                    Iterator'(Limited_Controlled with
                                Container => Container'Unrestricted_Access,
                                Node      => null)
      do
         B := B + 1;
      end return;
   end Iterate;

   function Iterate
     (Container : List;
      Start     : Cursor)
      return List_Iterator_Interfaces.Reversible_Iterator'Class
   is
      B  : Natural renames Container'Unrestricted_Access.all.Busy;

   begin
      --  It was formerly the case that when Start = No_Element, the partial
      --  iterator was defined to behave the same as for a complete iterator,
      --  and iterate over the entire sequence of items. However, those
      --  semantics were unintuitive and arguably error-prone (it is too easy
      --  to accidentally create an endless loop), and so they were changed,
      --  per the ARG meeting in Denver on 2011/11. However, there was no
      --  consensus about what positive meaning this corner case should have,
      --  and so it was decided to simply raise an exception. This does imply,
      --  however, that it is not possible to use a partial iterator to specify
      --  an empty sequence of items.

      if Start = No_Element then
         raise Constraint_Error with
           "Start position for iterator equals No_Element";
      end if;

      if Start.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Start cursor of Iterate designates wrong list";
      end if;

      pragma Assert (Vet (Start), "Start cursor of Iterate is bad");

      --  The value of the Node component influences the behavior of the First
      --  and Last selector functions of the iterator object. When the Node
      --  component is non-null (as is the case here), it means that this
      --  is a partial iteration, over a subset of the complete sequence of
      --  items. The iterator object was constructed with a start expression,
      --  indicating the position from which the iteration begins. Note that
      --  the start position has the same value irrespective of whether this
      --  is a forward or reverse iteration.

      return It : constant Iterator :=
                    Iterator'(Limited_Controlled with
                                Container => Container'Unrestricted_Access,
                                Node      => Start.Node)
      do
         B := B + 1;
      end return;
   end Iterate;

   ----------
   -- Last --
   ----------

   function Last (Container : List) return Cursor is
   begin
      if Container.Last = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Container.Last);
   end Last;

   function Last (Object : Iterator) return Cursor is
   begin
      --  The value of the iterator object's Node component influences the
      --  behavior of the Last (and First) selector function.

      --  When the Node component is null, this means the iterator object was
      --  constructed without a start expression, in which case the (reverse)
      --  iteration starts from the (logical) beginning of the entire sequence
      --  (corresponding to Container.Last, for a reverse iterator).

      --  Otherwise, this is iteration over a partial sequence of items. When
      --  the Node component is non-null, the iterator object was constructed
      --  with a start expression, that specifies the position from which the
      --  (reverse) partial iteration begins.

      if Object.Node = null then
         return Indefinite_Doubly_Linked_Lists.Last (Object.Container.all);
      else
         return Cursor'(Object.Container, Object.Node);
      end if;
   end Last;

   ------------------
   -- Last_Element --
   ------------------

   function Last_Element (Container : List) return Element_Type is
   begin
      if Container.Last = null then
         raise Constraint_Error with "list is empty";
      end if;

      return Container.Last.Element.all;
   end Last_Element;

   ------------
   -- Length --
   ------------

   function Length (Container : List) return Count_Type is
   begin
      return Container.Length;
   end Length;

   ----------
   -- Move --
   ----------

   procedure Move (Target : in out List; Source : in out List) is
   begin
      if Target'Address = Source'Address then
         return;
      end if;

      if Source.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors of Source (list is busy)";
      end if;

      Clear (Target);

      Target.First := Source.First;
      Source.First := null;

      Target.Last := Source.Last;
      Source.Last := null;

      Target.Length := Source.Length;
      Source.Length := 0;
   end Move;

   ----------
   -- Next --
   ----------

   procedure Next (Position : in out Cursor) is
   begin
      Position := Next (Position);
   end Next;

   function Next (Position : Cursor) return Cursor is
   begin
      if Position.Node = null then
         return No_Element;
      end if;

      pragma Assert (Vet (Position), "bad cursor in Next");

      declare
         Next_Node : constant Node_Access := Position.Node.Next;
      begin
         if Next_Node = null then
            return No_Element;
         end if;

         return Cursor'(Position.Container, Next_Node);
      end;
   end Next;

   function Next (Object : Iterator; Position : Cursor) return Cursor is
   begin
      if Position.Container = null then
         return No_Element;
      end if;

      if Position.Container /= Object.Container then
         raise Program_Error with
           "Position cursor of Next designates wrong list";
      end if;

      return Next (Position);
   end Next;

   -------------
   -- Prepend --
   -------------

   procedure Prepend
     (Container : in out List;
      New_Item  : Element_Type;
      Count     : Count_Type := 1)
   is
   begin
      Insert (Container, First (Container), New_Item, Count);
   end Prepend;

   --------------
   -- Previous --
   --------------

   procedure Previous (Position : in out Cursor) is
   begin
      Position := Previous (Position);
   end Previous;

   function Previous (Position : Cursor) return Cursor is
   begin
      if Position.Node = null then
         return No_Element;
      end if;

      pragma Assert (Vet (Position), "bad cursor in Previous");

      declare
         Prev_Node : constant Node_Access := Position.Node.Prev;
      begin
         if Prev_Node = null then
            return No_Element;
         end if;

         return Cursor'(Position.Container, Prev_Node);
      end;
   end Previous;

   function Previous (Object : Iterator; Position : Cursor) return Cursor is
   begin
      if Position.Container = null then
         return No_Element;
      end if;

      if Position.Container /= Object.Container then
         raise Program_Error with
           "Position cursor of Previous designates wrong list";
      end if;

      return Previous (Position);
   end Previous;

   -------------------
   -- Query_Element --
   -------------------

   procedure Query_Element
     (Position : Cursor;
      Process  : not null access procedure (Element : Element_Type))
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with
           "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Query_Element");

      declare
         C : List renames Position.Container.all'Unrestricted_Access.all;
         B : Natural renames C.Busy;
         L : Natural renames C.Lock;

      begin
         B := B + 1;
         L := L + 1;

         begin
            Process (Position.Node.Element.all);
         exception
            when others =>
               L := L - 1;
               B := B - 1;
               raise;
         end;

         L := L - 1;
         B := B - 1;
      end;
   end Query_Element;

   ----------
   -- Read --
   ----------

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out List)
   is
      N   : Count_Type'Base;
      Dst : Node_Access;

   begin
      Clear (Item);

      Count_Type'Base'Read (Stream, N);

      if N = 0 then
         return;
      end if;

      declare
         Element : Element_Access :=
                     new Element_Type'(Element_Type'Input (Stream));
      begin
         Dst := new Node_Type'(Element, null, null);
      exception
         when others =>
            Free (Element);
            raise;
      end;

      Item.First := Dst;
      Item.Last := Dst;
      Item.Length := 1;

      while Item.Length < N loop
         declare
            Element : Element_Access :=
                        new Element_Type'(Element_Type'Input (Stream));
         begin
            Dst := new Node_Type'(Element, Next => null, Prev => Item.Last);
         exception
            when others =>
               Free (Element);
               raise;
         end;

         Item.Last.Next := Dst;
         Item.Last := Dst;
         Item.Length := Item.Length + 1;
      end loop;
   end Read;

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out Cursor)
   is
   begin
      raise Program_Error with "attempt to stream list cursor";
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

   function Reference
     (Container : aliased in out List;
      Position  : Cursor) return Reference_Type
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Node has no element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in function Reference");

      declare
         C : List renames Position.Container.all;
         B : Natural renames C.Busy;
         L : Natural renames C.Lock;
      begin
         return R : constant Reference_Type :=
                      (Element => Position.Node.Element.all'Access,
                       Control => (Controlled with Position.Container))
         do
            B := B + 1;
            L := L + 1;
         end return;
      end;
   end Reference;

   ---------------------
   -- Replace_Element --
   ---------------------

   procedure Replace_Element
     (Container : in out List;
      Position  : Cursor;
      New_Item  : Element_Type)
   is
   begin
      if Position.Container = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unchecked_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (list is locked)";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Replace_Element");

      declare
         X : Element_Access := Position.Node.Element;

      begin
         Position.Node.Element := new Element_Type'(New_Item);
         Free (X);
      end;
   end Replace_Element;

   ----------------------
   -- Reverse_Elements --
   ----------------------

   procedure Reverse_Elements (Container : in out List) is
      I : Node_Access := Container.First;
      J : Node_Access := Container.Last;

      procedure Swap (L, R : Node_Access);

      ----------
      -- Swap --
      ----------

      procedure Swap (L, R : Node_Access) is
         LN : constant Node_Access := L.Next;
         LP : constant Node_Access := L.Prev;

         RN : constant Node_Access := R.Next;
         RP : constant Node_Access := R.Prev;

      begin
         if LP /= null then
            LP.Next := R;
         end if;

         if RN /= null then
            RN.Prev := L;
         end if;

         L.Next := RN;
         R.Prev := LP;

         if LN = R then
            pragma Assert (RP = L);

            L.Prev := R;
            R.Next := L;

         else
            L.Prev := RP;
            RP.Next := L;

            R.Next := LN;
            LN.Prev := R;
         end if;
      end Swap;

   --  Start of processing for Reverse_Elements

   begin
      if Container.Length <= 1 then
         return;
      end if;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      Container.First := J;
      Container.Last := I;
      loop
         Swap (L => I, R => J);

         J := J.Next;
         exit when I = J;

         I := I.Prev;
         exit when I = J;

         Swap (L => J, R => I);

         I := I.Next;
         exit when I = J;

         J := J.Prev;
         exit when I = J;
      end loop;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);
   end Reverse_Elements;

   ------------------
   -- Reverse_Find --
   ------------------

   function Reverse_Find
     (Container : List;
      Item      : Element_Type;
      Position  : Cursor := No_Element) return Cursor
   is
      Node : Node_Access := Position.Node;

   begin
      if Node = null then
         Node := Container.Last;

      else
         if Node.Element = null then
            raise Program_Error with "Position cursor has no element";
         end if;

         if Position.Container /= Container'Unrestricted_Access then
            raise Program_Error with
              "Position cursor designates wrong container";
         end if;

         pragma Assert (Vet (Position), "bad cursor in Reverse_Find");
      end if;

      while Node /= null loop
         if Node.Element.all = Item then
            return Cursor'(Container'Unrestricted_Access, Node);
         end if;

         Node := Node.Prev;
      end loop;

      return No_Element;
   end Reverse_Find;

   ---------------------
   -- Reverse_Iterate --
   ---------------------

   procedure Reverse_Iterate
     (Container : List;
      Process   : not null access procedure (Position : Cursor))
   is
      C : List renames Container'Unrestricted_Access.all;
      B : Natural renames C.Busy;

      Node : Node_Access := Container.Last;

   begin
      B := B + 1;

      begin
         while Node /= null loop
            Process (Cursor'(Container'Unrestricted_Access, Node));
            Node := Node.Prev;
         end loop;
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Reverse_Iterate;

   ------------
   -- Splice --
   ------------

   procedure Splice
     (Target : in out List;
      Before : Cursor;
      Source : in out List)
   is
   begin
      if Before.Container /= null then
         if Before.Container /= Target'Unrestricted_Access then
            raise Program_Error with
              "Before cursor designates wrong container";
         end if;

         if Before.Node = null
           or else Before.Node.Element = null
         then
            raise Program_Error with
              "Before cursor has no element";
         end if;

         pragma Assert (Vet (Before), "bad cursor in Splice");
      end if;

      if Target'Address = Source'Address
        or else Source.Length = 0
      then
         return;
      end if;

      pragma Assert (Source.First.Prev = null);
      pragma Assert (Source.Last.Next = null);

      if Target.Length > Count_Type'Last - Source.Length then
         raise Constraint_Error with "new length exceeds maximum";
      end if;

      if Target.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors of Target (list is busy)";
      end if;

      if Source.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors of Source (list is busy)";
      end if;

      if Target.Length = 0 then
         pragma Assert (Before = No_Element);
         pragma Assert (Target.First = null);
         pragma Assert (Target.Last = null);

         Target.First := Source.First;
         Target.Last := Source.Last;

      elsif Before.Node = null then
         pragma Assert (Target.Last.Next = null);

         Target.Last.Next := Source.First;
         Source.First.Prev := Target.Last;

         Target.Last := Source.Last;

      elsif Before.Node = Target.First then
         pragma Assert (Target.First.Prev = null);

         Source.Last.Next := Target.First;
         Target.First.Prev := Source.Last;

         Target.First := Source.First;

      else
         pragma Assert (Target.Length >= 2);
         Before.Node.Prev.Next := Source.First;
         Source.First.Prev := Before.Node.Prev;

         Before.Node.Prev := Source.Last;
         Source.Last.Next := Before.Node;
      end if;

      Source.First := null;
      Source.Last := null;

      Target.Length := Target.Length + Source.Length;
      Source.Length := 0;
   end Splice;

   procedure Splice
     (Container : in out List;
      Before    : Cursor;
      Position  : Cursor)
   is
   begin
      if Before.Container /= null then
         if Before.Container /= Container'Unchecked_Access then
            raise Program_Error with
              "Before cursor designates wrong container";
         end if;

         if Before.Node = null
           or else Before.Node.Element = null
         then
            raise Program_Error with
              "Before cursor has no element";
         end if;

         pragma Assert (Vet (Before), "bad Before cursor in Splice");
      end if;

      if Position.Node = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      pragma Assert (Vet (Position), "bad Position cursor in Splice");

      if Position.Node = Before.Node
        or else Position.Node.Next = Before.Node
      then
         return;
      end if;

      pragma Assert (Container.Length >= 2);

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      if Before.Node = null then
         pragma Assert (Position.Node /= Container.Last);

         if Position.Node = Container.First then
            Container.First := Position.Node.Next;
            Container.First.Prev := null;
         else
            Position.Node.Prev.Next := Position.Node.Next;
            Position.Node.Next.Prev := Position.Node.Prev;
         end if;

         Container.Last.Next := Position.Node;
         Position.Node.Prev := Container.Last;

         Container.Last := Position.Node;
         Container.Last.Next := null;

         return;
      end if;

      if Before.Node = Container.First then
         pragma Assert (Position.Node /= Container.First);

         if Position.Node = Container.Last then
            Container.Last := Position.Node.Prev;
            Container.Last.Next := null;
         else
            Position.Node.Prev.Next := Position.Node.Next;
            Position.Node.Next.Prev := Position.Node.Prev;
         end if;

         Container.First.Prev := Position.Node;
         Position.Node.Next := Container.First;

         Container.First := Position.Node;
         Container.First.Prev := null;

         return;
      end if;

      if Position.Node = Container.First then
         Container.First := Position.Node.Next;
         Container.First.Prev := null;

      elsif Position.Node = Container.Last then
         Container.Last := Position.Node.Prev;
         Container.Last.Next := null;

      else
         Position.Node.Prev.Next := Position.Node.Next;
         Position.Node.Next.Prev := Position.Node.Prev;
      end if;

      Before.Node.Prev.Next := Position.Node;
      Position.Node.Prev := Before.Node.Prev;

      Before.Node.Prev := Position.Node;
      Position.Node.Next := Before.Node;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);
   end Splice;

   procedure Splice
     (Target   : in out List;
      Before   : Cursor;
      Source   : in out List;
      Position : in out Cursor)
   is
   begin
      if Target'Address = Source'Address then
         Splice (Target, Before, Position);
         return;
      end if;

      if Before.Container /= null then
         if Before.Container /= Target'Unrestricted_Access then
            raise Program_Error with
              "Before cursor designates wrong container";
         end if;

         if Before.Node = null
           or else Before.Node.Element = null
         then
            raise Program_Error with
              "Before cursor has no element";
         end if;

         pragma Assert (Vet (Before), "bad Before cursor in Splice");
      end if;

      if Position.Node = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      if Position.Container /= Source'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      pragma Assert (Vet (Position), "bad Position cursor in Splice");

      if Target.Length = Count_Type'Last then
         raise Constraint_Error with "Target is full";
      end if;

      if Target.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors of Target (list is busy)";
      end if;

      if Source.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors of Source (list is busy)";
      end if;

      if Position.Node = Source.First then
         Source.First := Position.Node.Next;

         if Position.Node = Source.Last then
            pragma Assert (Source.First = null);
            pragma Assert (Source.Length = 1);
            Source.Last := null;

         else
            Source.First.Prev := null;
         end if;

      elsif Position.Node = Source.Last then
         pragma Assert (Source.Length >= 2);
         Source.Last := Position.Node.Prev;
         Source.Last.Next := null;

      else
         pragma Assert (Source.Length >= 3);
         Position.Node.Prev.Next := Position.Node.Next;
         Position.Node.Next.Prev := Position.Node.Prev;
      end if;

      if Target.Length = 0 then
         pragma Assert (Before = No_Element);
         pragma Assert (Target.First = null);
         pragma Assert (Target.Last = null);

         Target.First := Position.Node;
         Target.Last := Position.Node;

         Target.First.Prev := null;
         Target.Last.Next := null;

      elsif Before.Node = null then
         pragma Assert (Target.Last.Next = null);
         Target.Last.Next := Position.Node;
         Position.Node.Prev := Target.Last;

         Target.Last := Position.Node;
         Target.Last.Next := null;

      elsif Before.Node = Target.First then
         pragma Assert (Target.First.Prev = null);
         Target.First.Prev := Position.Node;
         Position.Node.Next := Target.First;

         Target.First := Position.Node;
         Target.First.Prev := null;

      else
         pragma Assert (Target.Length >= 2);
         Before.Node.Prev.Next := Position.Node;
         Position.Node.Prev := Before.Node.Prev;

         Before.Node.Prev := Position.Node;
         Position.Node.Next := Before.Node;
      end if;

      Target.Length := Target.Length + 1;
      Source.Length := Source.Length - 1;

      Position.Container := Target'Unchecked_Access;
   end Splice;

   ----------
   -- Swap --
   ----------

   procedure Swap
     (Container : in out List;
      I, J      : Cursor)
   is
   begin
      if I.Node = null then
         raise Constraint_Error with "I cursor has no element";
      end if;

      if J.Node = null then
         raise Constraint_Error with "J cursor has no element";
      end if;

      if I.Container /= Container'Unchecked_Access then
         raise Program_Error with "I cursor designates wrong container";
      end if;

      if J.Container /= Container'Unchecked_Access then
         raise Program_Error with "J cursor designates wrong container";
      end if;

      if I.Node = J.Node then
         return;
      end if;

      if Container.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with elements (list is locked)";
      end if;

      pragma Assert (Vet (I), "bad I cursor in Swap");
      pragma Assert (Vet (J), "bad J cursor in Swap");

      declare
         EI_Copy : constant Element_Access := I.Node.Element;

      begin
         I.Node.Element := J.Node.Element;
         J.Node.Element := EI_Copy;
      end;
   end Swap;

   ----------------
   -- Swap_Links --
   ----------------

   procedure Swap_Links
     (Container : in out List;
      I, J      : Cursor)
   is
   begin
      if I.Node = null then
         raise Constraint_Error with "I cursor has no element";
      end if;

      if J.Node = null then
         raise Constraint_Error with "J cursor has no element";
      end if;

      if I.Container /= Container'Unrestricted_Access then
         raise Program_Error with "I cursor designates wrong container";
      end if;

      if J.Container /= Container'Unrestricted_Access then
         raise Program_Error with "J cursor designates wrong container";
      end if;

      if I.Node = J.Node then
         return;
      end if;

      if Container.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (list is busy)";
      end if;

      pragma Assert (Vet (I), "bad I cursor in Swap_Links");
      pragma Assert (Vet (J), "bad J cursor in Swap_Links");

      declare
         I_Next : constant Cursor := Next (I);

      begin
         if I_Next = J then
            Splice (Container, Before => I, Position => J);

         else
            declare
               J_Next : constant Cursor := Next (J);

            begin
               if J_Next = I then
                  Splice (Container, Before => J, Position => I);

               else
                  pragma Assert (Container.Length >= 3);

                  Splice (Container, Before => I_Next, Position => J);
                  Splice (Container, Before => J_Next, Position => I);
               end if;
            end;
         end if;
      end;

      pragma Assert (Container.First.Prev = null);
      pragma Assert (Container.Last.Next = null);
   end Swap_Links;

   --------------------
   -- Update_Element --
   --------------------

   procedure Update_Element
     (Container : in out List;
      Position  : Cursor;
      Process   : not null access procedure (Element : in out Element_Type))
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor has no element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with
           "Position cursor has no element";
      end if;

      if Position.Container /= Container'Unchecked_Access then
         raise Program_Error with
           "Position cursor designates wrong container";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Update_Element");

      declare
         B : Natural renames Container.Busy;
         L : Natural renames Container.Lock;

      begin
         B := B + 1;
         L := L + 1;

         begin
            Process (Position.Node.Element.all);
         exception
            when others =>
               L := L - 1;
               B := B - 1;
               raise;
         end;

         L := L - 1;
         B := B - 1;
      end;
   end Update_Element;

   ---------
   -- Vet --
   ---------

   function Vet (Position : Cursor) return Boolean is
   begin
      if Position.Node = null then
         return Position.Container = null;
      end if;

      if Position.Container = null then
         return False;
      end if;

      --  An invariant of a node is that its Previous and Next components can
      --  be null, or designate a different node. Also, its element access
      --  value must be non-null. Operation Free sets the node access value
      --  components of the node to designate the node itself, and the element
      --  access value to null, before actually deallocating the node, thus
      --  deliberately violating the node invariant. This gives us a simple way
      --  to detect a dangling reference to a node.

      if Position.Node.Next = Position.Node then
         return False;
      end if;

      if Position.Node.Prev = Position.Node then
         return False;
      end if;

      if Position.Node.Element = null then
         return False;
      end if;

      --  In practice the tests above will detect most instances of a dangling
      --  reference. If we get here, it means that the invariants of the
      --  designated node are satisfied (they at least appear to be satisfied),
      --  so we perform some more tests, to determine whether invariants of the
      --  designated list are satisfied too.

      declare
         L : List renames Position.Container.all;

      begin
         if L.Length = 0 then
            return False;
         end if;

         if L.First = null then
            return False;
         end if;

         if L.Last = null then
            return False;
         end if;

         if L.First.Prev /= null then
            return False;
         end if;

         if L.Last.Next /= null then
            return False;
         end if;

         if Position.Node.Prev = null and then Position.Node /= L.First then
            return False;
         end if;

         if Position.Node.Next = null and then Position.Node /= L.Last then
            return False;
         end if;

         if L.Length = 1 then
            return L.First = L.Last;
         end if;

         if L.First = L.Last then
            return False;
         end if;

         if L.First.Next = null then
            return False;
         end if;

         if L.Last.Prev = null then
            return False;
         end if;

         if L.First.Next.Prev /= L.First then
            return False;
         end if;

         if L.Last.Prev.Next /= L.Last then
            return False;
         end if;

         if L.Length = 2 then
            if L.First.Next /= L.Last then
               return False;
            end if;

            if L.Last.Prev /= L.First then
               return False;
            end if;

            return True;
         end if;

         if L.First.Next = L.Last then
            return False;
         end if;

         if L.Last.Prev = L.First then
            return False;
         end if;

         if Position.Node = L.First then
            return True;
         end if;

         if Position.Node = L.Last then
            return True;
         end if;

         if Position.Node.Next = null then
            return False;
         end if;

         if Position.Node.Prev = null then
            return False;
         end if;

         if Position.Node.Next.Prev /= Position.Node then
            return False;
         end if;

         if Position.Node.Prev.Next /= Position.Node then
            return False;
         end if;

         if L.Length = 3 then
            if L.First.Next /= Position.Node then
               return False;
            end if;

            if L.Last.Prev /= Position.Node then
               return False;
            end if;
         end if;

         return True;
      end;
   end Vet;

   -----------
   -- Write --
   -----------

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : List)
   is
      Node : Node_Access := Item.First;

   begin
      Count_Type'Base'Write (Stream, Item.Length);

      while Node /= null loop
         Element_Type'Output (Stream, Node.Element.all);
         Node := Node.Next;
      end loop;
   end Write;

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : Cursor)
   is
   begin
      raise Program_Error with "attempt to stream list cursor";
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

end Ada.Containers.Indefinite_Doubly_Linked_Lists;
