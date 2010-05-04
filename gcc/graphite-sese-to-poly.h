/* Conversion of SESE regions to Polyhedra.
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.
   Contributed by Sebastian Pop <sebastian.pop@amd.com>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_GRAPHITE_SESE_TO_POLY_H
#define GCC_GRAPHITE_SESE_TO_POLY_H

typedef struct base_alias_pair base_alias_pair;
struct base_alias_pair
{
  int base_obj_set;
  int *alias_set;
};

void build_poly_scop (scop_p);
void check_poly_representation (scop_p);

#endif
