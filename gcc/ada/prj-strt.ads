------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             P R J . S T R T                              --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                            $Revision: 1.1 $
--                                                                          --
--             Copyright (C) 2001 Free Software Foundation, Inc.            --
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
--
--  This package implements parsing of string expressions in project files.

with Prj.Tree;  use Prj.Tree;

private package Prj.Strt is

   procedure Parse_String_Type_List (First_String : out Project_Node_Id);
   --  Get the list of literal strings that are allowed for a typed string.
   --  On entry, the current token is the first literal string following
   --  a left parenthesis in a string type declaration such as:
   --    type Toto is ("string_1", "string_2", "string_3");
   --  On exit, the current token is the right parenthesis.
   --  The parameter First_String is a node that contained the first
   --  literal string of the string type, linked with the following
   --  literal strings.
   --
   --  Report an error if
   --    - a literal string is not found at the beginning of the list
   --      or after a comma
   --    - two literal strings in the list are equal

   procedure Start_New_Case_Construction (String_Type : Project_Node_Id);
   --  This procedure is called at the beginning of a case construction
   --  The parameter String_Type is the node for the string type
   --  of the case label variable.
   --  The different literal strings of the string type are stored
   --  into a table to be checked against the case labels of the
   --  case construction.

   procedure End_Case_Construction;
   --  This procedure is called at the end of a case construction
   --  to remove the case labels and to restore the previous state.
   --  In particular, in the case of nested case constructions,
   --  the case labels of the enclosing case construction are restored.

   procedure Parse_Choice_List
     (First_Choice : out Project_Node_Id);
   --  Get the label for a choice list.
   --  Report an error if
   --    - a case label is not a literal string
   --    - a case label is not in the typed string list
   --    - the same case label is repeated in the same case construction

   procedure Parse_Expression
     (Expression      : out Project_Node_Id;
      Current_Project : Project_Node_Id;
      Current_Package : Project_Node_Id);
   --  Parse a simple string expression or a string list expression.
   --  Current_Project is the node of the project file being parsed.
   --  Current_Package is the node of the package being parsed,
   --  or Empty_Node when we are at the project level (not in a package).
   --  On exit, Expression is the node of the expression that has
   --  been parsed.

   procedure Parse_Variable_Reference
     (Variable        : out Project_Node_Id;
      Current_Project : Project_Node_Id;
      Current_Package : Project_Node_Id);
   --  Parse a variable or attribute reference.
   --  Used internally (in expressions) and for case variables (in Prj.Dect).
   --  Current_Package is the node of the package being parsed,
   --  or Empty_Node when we are at the project level (not in a package).
   --  On exit, Variable is the node of the variable or attribute reference.
   --  A variable reference is made of one to three simple names.
   --  An attribute reference is made of one or two simple names,
   --  followed by an apostroph, followed by the attribute simple name.

end Prj.Strt;
