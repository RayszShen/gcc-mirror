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

// { dg-options "-std=gnu++2a -D_GLIBCXX_DEBUG" }
// { dg-do compile { xfail c++2a } }

#include <iterator>
#include <testsuite_iterators.h>

void
test01()
{
  int a[2] = { };
  __gnu_test::test_container<int, __gnu_test::forward_iterator_wrapper> c(a);
  auto iter = c.begin();
  std::ranges::advance(iter, -1);
}
