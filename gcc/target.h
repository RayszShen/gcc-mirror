/* Data structure definitions for a generic GCC target.
   Copyright (C) 2001 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

/* This file contains a data structure that describes a GCC target.
   At present, it is incomplete, but in future it should grow to
   contain most or all target machine and target O/S specific
   information.

   This structure has its initializer declared in target-def.h in the
   form of large macro TARGET_INITIALIZER that expands to many smaller
   macros.

   The smaller macros each initialize one component of the structure,
   and each has a default.  Each target should have a file that
   includes target.h and target-def.h, and overrides any inappropriate
   defaults by undefining the relevant macro and defining a suitable
   replacement.  That file should then contain the definition of
   "target" like so:

   struct gcc_target target = TARGET_INITIALIZER;

   Doing things this way allows us to bring together everything that
   defines a target to GCC.  By supplying a default that is
   appropriate to most targets, we can easily add new items without
   needing to edit dozens of target configuration files.  It should
   also allow us to gradually reduce the amount of conditional
   compilation that is scattered throughout GCC.  */

struct gcc_target
{
  /* Given two decls, merge their attributes and return the result.  */
  tree (* merge_decl_attributes) PARAMS ((tree, tree));

  /* Given two types, merge their attributes and return the result.  */
  tree (* merge_type_attributes) PARAMS ((tree, tree));

  /* Return nonzero if IDENTIFIER with arguments ARGS is a valid machine
     specific attribute for DECL.  The attributes in ATTRIBUTES have
     previously been assigned to DECL.  */
  int (* valid_decl_attribute) PARAMS ((tree decl, tree attributes,
					tree identifier, tree args));

  /* Return nonzero if IDENTIFIER with arguments ARGS is a valid machine
     specific attribute for TYPE.  The attributes in ATTRIBUTES have
     previously been assigned to TYPE.  */
  int (* valid_type_attribute) PARAMS ((tree type, tree attributes,
					tree identifier, tree args));
};

extern struct gcc_target target;
