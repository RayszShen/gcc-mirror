// Copyright (C) 2004 Free Software Foundation, Inc.
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

// 27.6.1.2.2 arithmetic extractors

#include <istream>
#include <sstream>
#include <locale>
#include <cwchar>
#include <testsuite_hooks.h>

// In the presence of no fmtflags, the input operator should behave
// like strtol(x, y, 0)
// libstdc++/90
bool test11()
{
  bool test __attribute__((unused)) = true;
  const wchar_t* cstrlit = L"0x2a";

  // sanity check via 'C' library call
  wchar_t* err;
  long l = std::wcstol(cstrlit, &err, 0);

  std::wistringstream iss(cstrlit);
  iss.setf(std::wios::fmtflags(0), std::ios::basefield);
  int i;
  iss >> i;

  VERIFY( !iss.fail() );
  VERIFY( l == i );

  return test;
}

int main()
{
  test11();
  return 0;
}
