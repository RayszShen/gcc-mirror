// 2001-06-18  Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2004 Free Software Foundation, Inc.
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
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 20.2.2 Pairs

#include <utility>
#include <testsuite_hooks.h>

class gnu_obj
{
  int i;
public:
  gnu_obj(int arg = 0): i(arg) { }
  bool operator==(const gnu_obj& rhs) const { return i == rhs.i; }
  bool operator<(const gnu_obj& rhs) const { return i < rhs.i; }
};

template<typename T>
  struct gnu_t
  {
    bool b;
  public:
    gnu_t(bool arg = 0): b(arg) { }
    bool operator==(const gnu_t& rhs) const { return b == rhs.b; }
    bool operator<(const gnu_t& rhs) const { return int(b) < int(rhs.b); }
  };

// homogeneous
void test02()
{
  bool test __attribute__((unused)) = true;

  std::pair<bool, bool> p_bb_1(true, false);
  std::pair<bool, bool> p_bb_2 = std::make_pair(true, false);
  VERIFY( p_bb_1 == p_bb_2 );
  VERIFY( !(p_bb_1 < p_bb_2) );
}

int main() 
{ 
  test02();
  return 0;
}
