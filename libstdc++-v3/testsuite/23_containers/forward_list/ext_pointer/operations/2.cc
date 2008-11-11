// { dg-options "-std=gnu++0x" }

// Copyright (C) 2008 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without Pred the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

// 23.2.3.n forward_list xxx [lib.forward_list.xxx]

#include <forward_list>
#include <testsuite_hooks.h>
#include <ext/extptr_allocator.h>

using __gnu_cxx::_ExtPtr_allocator;

bool test __attribute__((unused)) = true;

// This test verifies the following:
//   remove
void
test01()
{
  std::forward_list<int, _ExtPtr_allocator<int> > fl =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  fl.remove(7);

  std::forward_list<int, _ExtPtr_allocator<int> >::const_iterator 
    pos = fl.cbefore_begin();

  for (std::size_t i = 0; i < 7; ++i)
    ++pos;
  VERIFY(*pos == 6);

  ++pos;
  VERIFY(*pos == 8);
}

int
main()
{
  test01();
  return 0;
}
