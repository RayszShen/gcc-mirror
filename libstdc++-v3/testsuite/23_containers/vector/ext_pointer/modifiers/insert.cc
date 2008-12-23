// Test for Container using non-standard pointer types.

// Copyright (C) 2008
// Free Software Foundation, Inc.
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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#include <vector>
#include <testsuite_hooks.h>
#include <ext/extptr_allocator.h>
#include <stdexcept>

void test01() 
{ 
  bool test __attribute__((unused)) = true;

  __gnu_cxx::_ExtPtr_allocator<int> alloc;
  std::vector<int, __gnu_cxx::_ExtPtr_allocator<int> > iv(alloc);
  VERIFY( iv.get_allocator() == alloc );
  VERIFY( iv.size() == 0 );
  
  int A[] = { 0, 1, 2, 3, 4 };
  int B[] = { 5, 5, 5, 5, 5 };
  int C[] = { 6, 7 };
  iv.insert(iv.end(), A, A+5 );
  VERIFY( iv.size() == 5 );
  iv.insert(iv.begin(), 5, 5 );
  iv.insert(iv.begin()+5, 7);
  iv.insert(iv.begin()+5, 6);
  VERIFY( std::equal(iv.begin(), iv.begin()+5, B ));
  VERIFY( std::equal(iv.begin()+5, iv.begin()+7, C));
  VERIFY( std::equal(iv.begin()+7, iv.end(), A));
  VERIFY( iv.size() == 12 );

  try
    {
      iv.insert(iv.end(), iv.max_size() + 1, 1);
    }
  catch(std::length_error&)
    {
      VERIFY( true );
    }
  catch(...)
    {
      VERIFY( false );
    }
}

int main()
{
  test01();
  return 0;
}
