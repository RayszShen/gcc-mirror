//
// Copyright (C) 2004-2017 Free Software Foundation, Inc.
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

// 20.4.1.1 allocator members

#include <ext/mt_allocator.h>
#include <testsuite_allocator.h>

int main()
{ 
  typedef int value_type;
  using __gnu_cxx::__pool;
  typedef __gnu_cxx::__common_pool_policy<__pool, true> policy_type;
  typedef __gnu_cxx::__mt_alloc<value_type, policy_type> allocator_type;
  __gnu_test::check_deallocate_null<allocator_type>(); 
  return 0;
}

