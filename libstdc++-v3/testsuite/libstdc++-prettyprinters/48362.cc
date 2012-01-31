// { dg-do run }
// { dg-options "-g -std=gnu++11 -O0" }

// Copyright (C) 2011, 2012 Free Software Foundation, Inc.
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

#include <tuple>
#include <string>

int
main()
{
  std::tuple<> t1;
// { dg-final { regexp-test t1 {empty std::(__7::)?tuple} } }

  std::tuple<std::string, int, std::tuple<>> t2{ "Johnny", 5, {} };
// { dg-final { regexp-test t2 {std::(__7::)?tuple containing = {\[1\] = "Johnny", \[2\] = 5, \[3\] = {<std::(__7::)?tuple<>> = empty std::(__7::)?tuple, <No data fields>}}} } }

  return 0; // Mark SPOT
}

// { dg-final { gdb-test SPOT } }
