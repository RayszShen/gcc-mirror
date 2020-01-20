// Copyright (C) 2020 Free Software Foundation, Inc.
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
// { dg-do run { target c++2a } }

#include <algorithm>
#include <testsuite_hooks.h>
#include <testsuite_iterators.h>

using __gnu_test::test_container;
using __gnu_test::test_range;
using __gnu_test::input_iterator_wrapper;
using __gnu_test::forward_iterator_wrapper;
using __gnu_test::bidirectional_iterator_wrapper;

namespace ranges = std::ranges;

void
test01()
{
    {
      int x[7] = { 1, 2, 3, 4, 5, 6, 7 };
      int y[7] = { 0 };
      int z[7] = { 1, 2, 3, 4, 5, 6, 7 };
      auto [in, out] = ranges::copy(x, y);
      VERIFY( ranges::equal(x, y) && in == x+7 && out == y+7 );
      VERIFY( ranges::equal(x, z) );
    }

    {
      int x[3] = { 1, 2, 3 };
      char y[4] = { 0 };
      int z[3] = { 1, 2, 3 };
      test_container<int, forward_iterator_wrapper> cx(x);
      test_container<char, forward_iterator_wrapper> cy(y);
      auto [in, out] = ranges::copy(x, y);
      VERIFY( ranges::equal(x, x+3, y, y+3) && in == x+3 && out == y+3 );
      VERIFY( ranges::equal(x, z) );
    }

    {
      char x[3] = { 1, 2, 3 };
      int y[4] = { 0 };
      int z[3] = { 1, 2, 3 };
      test_range<char, forward_iterator_wrapper> cx(x);
      test_range<int, forward_iterator_wrapper> cy(y);
      auto [in, out] = ranges::copy(x, y);
      VERIFY( ranges::equal(x, x+3, y, y+3) && in == x+3 && out == y+3 );
      VERIFY( ranges::equal(x, z) );
    }
}

struct X
{
  int i;
  constexpr X (int a) : i(a) { }
};

void
test02()
{
  int x[] = { {2}, {2}, {6}, {8}, {10} };
  X y[] = { {2}, {6}, {8}, {10}, {11}, {2} };
  int z[] = { {2}, {2}, {6}, {8}, {10} };
  auto [in, out] = ranges::copy(x, y);
  VERIFY( ranges::equal(x, x+5, y, y+5, {}, {}, &X::i) );
  VERIFY( in == x+5 );
  VERIFY( out == y+5 );
  VERIFY( y[5].i == 2 );
  VERIFY( ranges::equal(x, z) );
}

constexpr bool
test03()
{
  bool ok = true;
  int x[] = { {2}, {2}, {6}, {8}, {10} };
  X y[] = { {2}, {6}, {8}, {10}, {11}, {2} };
  int z[] = { {2}, {2}, {6}, {8}, {10} };
  auto [in, out] = ranges::copy(x, y);
  ok &= ranges::equal(x, x+5, y, y+5, {}, {}, &X::i);
  ok &= (in == x+5);
  ok &= (out == y+5);
  ok &= (y[5].i == 2);
  ok &= ranges::equal(x, z);
  return ok;
}

struct Y
{
  int i;
  int moved = 0;

  constexpr Y(int a) : i(a) { }

  constexpr Y(const Y&) = delete;
  constexpr Y& operator=(const Y&) = delete;

  constexpr Y(Y&& other)
  {
    *this = std::move(other);
  }

  constexpr Y&
  operator=(Y&& other)
  {
    other.moved++;
    i = other.i;
    return *this;
  }

  friend constexpr bool
  operator==(const Y& a, const Y& b)
  { return a.i == b.i; }
};

void
test04()
{
  Y x[7] = { 1, 2, 3, 4, 5, 6, 7 };
  Y y[7] = { 0, 0, 0, 0, 0, 0, 0 };
  Y z[7] = { 1, 2, 3, 4, 5, 6, 7 };
  auto [in, out] = ranges::copy(std::move_iterator{ranges::begin(x)},
				std::move_sentinel{ranges::end(x)},
				ranges::begin(y));
  VERIFY( ranges::equal(x, y) && in.base() == x+7 && out == y+7 );
  VERIFY( ranges::equal(x, z) );
  for (const auto& v : x)
    VERIFY( v.moved == 1 );
  for (const auto& v : y)
    VERIFY( v.moved == 0 );
}

constexpr bool
test05()
{
  bool ok = true;
  Y x[7] = { 1, 2, 3, 4, 5, 6, 7 };
  Y y[7] = { 0, 0, 0, 0, 0, 0, 0 };
  Y z[7] = { 1, 2, 3, 4, 5, 6, 7 };
  auto [in, out] = ranges::copy(std::move_iterator{ranges::begin(x)},
				std::move_sentinel{ranges::end(x)},
				ranges::begin(y));
  ok &= ranges::equal(x, y);
  ok &= in.base() == x+7;
  ok &= out == y+7;
  ok &= ranges::equal(x, z);
  for (const auto& v : x)
    ok &= v.moved == 1;
  for (const auto& v : y)
    ok &= v.moved == 0;
  return ok;
}

int
main()
{
  test01();
  test02();
  static_assert(test03());
  test04();
  static_assert(test05());
}

