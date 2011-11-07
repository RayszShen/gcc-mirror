// Copyright (C) 2011 Free Software Foundation, Inc.
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

// 21.4.6.5 basic_string::pop_back
// { dg-options "-std=gnu++0x" }

#include <ext/vstring.h>
#include <testsuite_hooks.h>

template<typename StrT>
int test01()
{
  bool test __attribute__((unused)) = true;

  const StrT cstr("Badger");
  StrT str = cstr;
  str.pop_back();
  VERIFY( str.size() == cstr.size() - 1 );

  str += cstr.back();
  VERIFY( str == cstr );

  return test;
}

int main()
{ 
  test01<__gnu_cxx::__sso_string>();
  test01<__gnu_cxx::__rc_string>();
  return 0;
}
