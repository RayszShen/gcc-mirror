// Copyright (C) 2019 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// { dg-options "-std=gnu++2a" }
// { dg-do compile { target c++2a } }

#include <algorithm>
#include <array>

#ifndef __cpp_lib_constexpr_algorithms
# error "Feature-test macro for constexpr algorithms missing"
#elif __cpp_lib_constexpr_algorithms < 201711L
# error "Feature-test macro for constexpr algorithms has wrong value"
#endif

constexpr std::array<int, 12> ca0{{0, 1, 2, 3, 4, 5,  6, 7, 8,  9, 10, 11}};

constexpr auto oute = std::find(ca0.begin(), ca0.end(), 5);

constexpr bool
test()
{
  return oute == ca0.begin() + 5;
}

static_assert(test());
