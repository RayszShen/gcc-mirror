// 2001-08-15 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2002, 2003 Free Software Foundation
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

// 22.2.4.1.1 collate members

#include <locale>
#include <testsuite_hooks.h>

// Check "C" locale.
void test01()
{
  using namespace std;
  typedef std::collate<char>::string_type string_type;

  bool test = true;

  // basic construction
  locale loc_c = locale::classic();
  locale loc_us = __gnu_cxx_test::try_named_locale("en_US");
  locale loc_fr = __gnu_cxx_test::try_named_locale("fr_FR");
  locale loc_de = __gnu_cxx_test::try_named_locale("de_DE");
  VERIFY( loc_c != loc_de );
  VERIFY( loc_us != loc_fr );
  VERIFY( loc_us != loc_de );
  VERIFY( loc_de != loc_fr );

  // cache the collate facets
  const collate<char>& coll_c = use_facet<collate<char> >(loc_c); 
  const collate<char>& coll_us = use_facet<collate<char> >(loc_us); 
  const collate<char>& coll_fr = use_facet<collate<char> >(loc_fr); 
  const collate<char>& coll_de = use_facet<collate<char> >(loc_de); 

  // int compare(const charT*, const charT*, const charT*, const charT*) const

  const char* strlit1 = "monkey picked tikuanyin oolong";
  const char* strlit2 = "imperial tea court green oolong";

  int i1;
  int i2;
  int size1 = char_traits<char>::length(strlit1) - 1;
  int size2 = char_traits<char>::length(strlit2) - 1;

  i1 = coll_c.compare(strlit1, strlit1 + size1, strlit1, strlit1 + 7);
  VERIFY ( i1 == 1 );
  i1 = coll_c.compare(strlit1, strlit1 + 7, strlit1, strlit1 + size1);
  VERIFY ( i1 == -1 );
  i1 = coll_c.compare(strlit1, strlit1 + 7, strlit1, strlit1 + 7);
  VERIFY ( i1 == 0 );

  i2 = coll_c.compare(strlit2, strlit2 + size2, strlit2, strlit2 + 13);
  VERIFY ( i2 == 1 );
  i2 = coll_c.compare(strlit2, strlit2 + 13, strlit2, strlit2 + size2);
  VERIFY ( i2 == -1 );
  i2 = coll_c.compare(strlit2, strlit2 + size2, strlit2, strlit2 + size2);
  VERIFY ( i2 == 0 );
}

int main()
{
  test01();
  return 0;
}
