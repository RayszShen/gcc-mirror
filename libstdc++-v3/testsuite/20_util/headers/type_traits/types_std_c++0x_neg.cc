// { dg-options "-std=gnu++0x" }
// { dg-do compile }

// Copyright (C) 2007 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

#include <type_traits>

namespace gnu
{
  // C++0x changes from TR1.
  using std::has_trivial_constructor;
  using std::has_nothrow_constructor;
  using std::has_trivial_copy;
  using std::has_nothrow_copy;
}

// { dg-error "has not been declared" "" { xfail *-*-* } 27 } 
// { dg-error "has not been declared" "" { xfail *-*-* } 28 } 
// { dg-error "has not been declared" "" { xfail *-*-* } 29 } 
// { dg-error "has not been declared" "" { xfail *-*-* } 30 } 

