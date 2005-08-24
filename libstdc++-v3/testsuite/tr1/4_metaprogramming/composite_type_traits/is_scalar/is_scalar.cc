// 2004-12-25  Paolo Carlini  <pcarlini@suse.de>
//
// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 4.5.2 Composite type traits

#include <tr1/type_traits>
#include <testsuite_hooks.h>
#include <testsuite_tr1.h>

void test01()
{
  bool test __attribute__((unused)) = true;
  using std::tr1::is_scalar;
  using namespace __gnu_test;

  VERIFY( (test_category<is_scalar, int>(true)) );
  VERIFY( (test_category<is_scalar, float>(true)) );
  VERIFY( (test_category<is_scalar, EnumType>(true)) );
  VERIFY( (test_category<is_scalar, int*>(true)) );
  VERIFY( (test_category<is_scalar, int(*)(int)>(true)) );
  VERIFY( (test_category<is_scalar, int (ClassType::*)>(true)) );
  VERIFY( (test_category<is_scalar, int (ClassType::*) (int)>(true)) );

  // Sanity check.
  VERIFY( (test_category<is_scalar, ClassType>(false)) );
}

int main()
{
  test01();
  return 0;
}
