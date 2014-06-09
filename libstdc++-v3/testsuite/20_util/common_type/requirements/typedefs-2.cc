// { dg-options "-std=gnu++0x" }
// 2009-11-12  Paolo Carlini  <paolo.carlini@oracle.com>
//
// Copyright (C) 2009-2013 Free Software Foundation, Inc.
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

#include <type_traits>
#include <testsuite_hooks.h>

// DR 1255.
void test01()
{
  bool test __attribute__((unused)) = true;
  using std::common_type;
  using std::is_same;

  VERIFY( (is_same<common_type<void>::type, void>::value) );
  VERIFY( (is_same<common_type<const void>::type, void>::value) );
  VERIFY( (is_same<common_type<volatile void>::type, void>::value) );
  VERIFY( (is_same<common_type<const volatile void>::type, void>::value) );

  VERIFY( (is_same<common_type<void, void>::type, void>::value) );
  VERIFY( (is_same<common_type<void, const void>::type, void>::value) );
  VERIFY( (is_same<common_type<void, volatile void>::type, void>::value) );
  VERIFY( (is_same<common_type<void, const volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const void, void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const void, const void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const void, volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const void, const volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<volatile void, void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<volatile void, volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<volatile void, const void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<volatile void, const volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const volatile void, void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const volatile void, const void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const volatile void, volatile void>::type,
	           void>::value) );
  VERIFY( (is_same<common_type<const volatile void, const volatile void>::type,
	           void>::value) );
 }

int main()
{
  test01();
  return 0;
}
