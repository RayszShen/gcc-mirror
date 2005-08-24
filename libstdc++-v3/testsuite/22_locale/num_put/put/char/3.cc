// { dg-require-namedlocale "" }

// 2001-11-19 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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

// 22.2.2.2.1  num_put members

#include <locale>
#include <sstream>
#include <testsuite_hooks.h>

void test03()
{
  using namespace std;
  typedef ostreambuf_iterator<char> iterator_type;

  bool test __attribute__((unused)) = true;

  // basic construction
  locale loc_c = locale::classic();
  locale loc_hk = locale("en_HK");
  VERIFY( loc_c != loc_hk );

  // sanity check the data is correct.
  const string empty;
  string result1;
  string result2;

  long l1 = 2147483647;
  long l2 = -2147483647;

  // cache the num_put facet
  ostringstream oss;
  oss.imbue(loc_hk);
  const num_put<char>& np = use_facet<num_put<char> >(oss.getloc()); 

  // HK
  // long, in a locale that expects grouping
  oss.str(empty);
  oss.clear();
  np.put(oss.rdbuf(), oss, '+', l1);
  result1 = oss.str();
  VERIFY( result1 == "2,147,483,647" );

  oss.str(empty);
  oss.clear();
  oss.width(20);
  oss.setf(ios_base::left, ios_base::adjustfield);
  np.put(oss.rdbuf(), oss, '+', l2);
  result1 = oss.str();
  VERIFY( result1 == "-2,147,483,647++++++" );
}

int main()
{
  test03();
  return 0;
}


