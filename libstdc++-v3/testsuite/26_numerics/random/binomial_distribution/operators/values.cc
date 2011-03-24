// { dg-options "-std=gnu++0x" }
// { dg-require-cstdint "" }
// { dg-require-cmath "" }
//
// Copyright (C) 2011 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// 26.5.8.2.2 Class template binomial_distribution [rand.dist.bern.bin]

#include <random>
#include <functional>
#include <testsuite_random.h>

void test01()
{
  using namespace __gnu_test;

  std::mt19937 eng;

  std::binomial_distribution<> bd1(5, 0.3);
  auto bbd1 = std::bind(bd1, eng);
  testDiscreteDist(bbd1, [](int n) { return binomial_pdf(n, 0.3, 5); } );

  std::binomial_distribution<> bd2(55, 0.3);
  auto bbd2 = std::bind(bd2, eng);
  testDiscreteDist(bbd2, [](int n) { return binomial_pdf(n, 0.3, 55); } );

  // libstdc++/48114
  std::binomial_distribution<> bd3(10, 0.75);
  auto bbd3 = std::bind(bd3, eng);
  testDiscreteDist(bbd3, [](int n) { return binomial_pdf(n, 0.75, 10); } );
}

int main()
{
  test01();
  return 0;
}
