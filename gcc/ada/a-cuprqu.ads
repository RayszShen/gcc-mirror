------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--                 ADA.CONTAINERS.UNBOUNDED_PRIORITY_QUEUES                 --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2011-2015, Free Software Foundation, Inc.         --
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
-- This unit was originally developed by Matthew J Heaney.                  --
------------------------------------------------------------------------------

with System;
with Ada.Containers.Synchronized_Queue_Interfaces;
with Ada.Finalization;

generic
   with package Queue_Interfaces is
     new Ada.Containers.Synchronized_Queue_Interfaces (<>);

   type Queue_Priority is private;

   with function Get_Priority
     (Element : Queue_Interfaces.Element_Type) return Queue_Priority is <>;

   with function Before
     (Left, Right : Queue_Priority) return Boolean is <>;

   Default_Ceiling : System.Any_Priority := System.Priority'Last;

package Ada.Containers.Unbounded_Priority_Queues is
   pragma Annotate (CodePeer, Skip_Analysis);
   pragma Preelaborate;

   package Implementation is

      --  All identifiers in this unit are implementation defined

      pragma Implementation_Defined;

      type List_Type is tagged limited private;

      procedure Enqueue
        (List     : in out List_Type;
         New_Item : Queue_Interfaces.Element_Type);

      procedure Dequeue
        (List    : in out List_Type;
         Element : out Queue_Interfaces.Element_Type);

      procedure Dequeue
        (List     : in out List_Type;
         At_Least : Queue_Priority;
         Element  : in out Queue_Interfaces.Element_Type;
         Success  : out Boolean);

      function Length (List : List_Type) return Count_Type;

      function Max_Length (List : List_Type) return Count_Type;

   private

      --  List_Type is implemented as a circular doubly-linked list with a
      --  dummy header node; Prev and Next are the links. The list is in
      --  decreasing priority order, so the highest-priority item is always
      --  first. (If there are multiple items with the highest priority, the
      --  oldest one is first.) Header.Element is undefined and not used.
      --
      --  In addition, Next_Unequal points to the next item with a different
      --  (i.e. strictly lower) priority. This is used to speed up the search
      --  for the next lower-priority item, in cases where there are many items
      --  with the same priority.
      --
      --  An empty list has Header.Prev, Header.Next, and Header.Next_Unequal
      --  all pointing to Header. A nonempty list has Header.Next_Unequal
      --  pointing to the first "real" item, and the last item has Next_Unequal
      --  pointing back to Header.

      type Node_Type;
      type Node_Access is access all Node_Type;

      type Node_Type is limited record
         Element      : Queue_Interfaces.Element_Type;
         Prev, Next   : Node_Access := Node_Type'Unchecked_Access;
         Next_Unequal : Node_Access := Node_Type'Unchecked_Access;
      end record;

      type List_Type is new Ada.Finalization.Limited_Controlled with record
         Header     : aliased Node_Type;
         Length     : Count_Type := 0;
         Max_Length : Count_Type := 0;
      end record;

      overriding procedure Finalize (List : in out List_Type);

   end Implementation;

   protected type Queue (Ceiling : System.Any_Priority := Default_Ceiling)
   with
     Priority => Ceiling
   is new Queue_Interfaces.Queue with

      overriding entry Enqueue (New_Item : Queue_Interfaces.Element_Type);

      overriding entry Dequeue (Element : out Queue_Interfaces.Element_Type);

      --  The priority queue operation Dequeue_Only_High_Priority had been a
      --  protected entry in early drafts of AI05-0159, but it was discovered
      --  that that operation as specified was not in fact implementable. The
      --  operation was changed from an entry to a protected procedure per the
      --  ARG meeting in Edinburgh (June 2011), with a different signature and
      --  semantics.

      procedure Dequeue_Only_High_Priority
        (At_Least : Queue_Priority;
         Element  : in out Queue_Interfaces.Element_Type;
         Success  : out Boolean);

      overriding function Current_Use return Count_Type;

      overriding function Peak_Use return Count_Type;

   private
      List : Implementation.List_Type;
   end Queue;

end Ada.Containers.Unbounded_Priority_Queues;
