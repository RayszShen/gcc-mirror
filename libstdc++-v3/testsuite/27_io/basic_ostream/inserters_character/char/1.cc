// 1999-08-16 bkoz

// Copyright (C) 1999, 2000, 2002, 2003 Free Software Foundation
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

// 27.6.2.5.4 basic_ostream character inserters

#include <string>
#include <ostream>
#include <sstream>
#include <fstream>
#include <testsuite_hooks.h>

// ofstream
void test01()
{
  std::string str01;
  const int size = 1000;
  const char name_02[] = "ostream_inserter_char-1.txt";

  // initialize string
  for(int i=0 ; i < size; i++) {
    str01 += '1';
    str01 += '2';
    str01 += '3';
    str01 += '4';
    str01 += '5';
    str01 += '6';
    str01 += '7';
    str01 += '8';
    str01 += '9';
    str01 += '\n';
  }
  std::ofstream f(name_02);

  f << str01;
  f.close();
}

int main()
{
  test01();
  return 0;
}
