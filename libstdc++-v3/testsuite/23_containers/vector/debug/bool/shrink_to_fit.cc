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
//
// { dg-require-debug-mode "" }
// { dg-options "-std=gnu++0x" }
// { dg-do run { xfail *-*-* } }

#include <vector>

void test01()
{
  using std::vector;

  vector<bool> vb(__CHAR_BIT__ * sizeof(unsigned long) + 1);
  vb.pop_back();

  vector<bool>::iterator it = vb.begin();
  vb.shrink_to_fit();

  // Following line should assert
  *it = true;
}

int main()
{
  test01();
  return 0;
}
