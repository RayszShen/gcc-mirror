// 2001-11-21 Benjamin Kosnik  <bkoz@redhat.com>

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

// 22.2.2.1.1  num_get members

#include <locale>
#include <sstream>
#include <testsuite_hooks.h>

void test01()
{
  using namespace std;
  typedef istreambuf_iterator<char> iterator_type;

  bool test = true;

  // basic construction
  locale loc_c = locale::classic();
  locale loc_hk = __gnu_cxx_test::try_named_locale("en_HK");
  locale loc_fr = __gnu_cxx_test::try_named_locale("fr_FR@euro");
  locale loc_de = __gnu_cxx_test::try_named_locale("de_DE");
  VERIFY( loc_c != loc_de );
  VERIFY( loc_hk != loc_fr );
  VERIFY( loc_hk != loc_de );
  VERIFY( loc_de != loc_fr );

  // cache the numpunct facets
  const numpunct<char>& numpunct_c = use_facet<numpunct<char> >(loc_c); 
  const numpunct<char>& numpunct_de = use_facet<numpunct<char> >(loc_de); 
  const numpunct<char>& numpunct_hk = use_facet<numpunct<char> >(loc_hk); 

  // sanity check the data is correct.
  const string empty;
  char c;

  bool b1 = true;
  bool b0 = false;
  long l1 = 2147483647;
  long l2 = -2147483647;
  long l;
  unsigned long ul1 = 1294967294;
  unsigned long ul2 = 0;
  unsigned long ul;
  double d1 =  1.02345e+308;
  double d2 = 3.15e-308;
  double d;
  long double ld1 = 6.630025e+4;
  long double ld2 = 0.0;
  long double ld;
  void* v;
  const void* cv = &ul2;

  // cache the num_get facet
  istringstream iss;
  iss.imbue(loc_de);
  const num_get<char>& ng = use_facet<num_get<char> >(iss.getloc()); 
  const ios_base::iostate goodbit = ios_base::goodbit;
  const ios_base::iostate eofbit = ios_base::eofbit;
  ios_base::iostate err = ios_base::goodbit;

  // bool, simple
  iss.str("1");
  iterator_type os_it00 = iss.rdbuf();
  iterator_type os_it01 = ng.get(os_it00, 0, iss, err, b1);
  VERIFY( b1 == true );
  VERIFY( err & ios_base::eofbit );

  iss.str("0");
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, b0);
  VERIFY( b0 == false );
  VERIFY( err & eofbit );

  // ... and one that does
  iss.imbue(loc_de);
  iss.str("1.294.967.294+++++++");
  iss.clear();
  iss.width(20);
  iss.setf(ios_base::left, ios_base::adjustfield);
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, ul);
  VERIFY( ul == ul1 );
  VERIFY( err == goodbit );

  iss.str("+1,02345e+308");
  iss.clear();
  iss.width(20);
  iss.setf(ios_base::right, ios_base::adjustfield);
  iss.setf(ios_base::scientific, ios_base::floatfield);
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, d);
  VERIFY( d == d1 );
  VERIFY( err == eofbit );

  iss.str("3,15E-308 ");
  iss.clear();
  iss.width(20);
  iss.precision(10);
  iss.setf(ios_base::right, ios_base::adjustfield);
  iss.setf(ios_base::scientific, ios_base::floatfield);
  iss.setf(ios_base::uppercase);
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, d);
  VERIFY( d == d2 );
  VERIFY( err == goodbit );

  // long double
  iss.str("6,630025e+4");
  iss.clear();
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, ld);
  VERIFY( ld == ld1 );
  VERIFY( err == eofbit );

  iss.str("0 ");
  iss.clear();
  iss.precision(0);
  iss.setf(ios_base::fixed, ios_base::floatfield);
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, ld);
  VERIFY( ld == 0 );
  VERIFY( err == goodbit );

  // const void
  iss.str("0xbffff74c,");
  iss.clear();
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, v);
  VERIFY( &v != &cv );
  VERIFY( err == goodbit );

#ifdef _GLIBCPP_USE_LONG_LONG
  long long ll1 = 9223372036854775807LL;
  long long ll2 = -9223372036854775807LL;
  long long ll;

  iss.str("9.223.372.036.854.775.807");
  iss.clear();
  err = goodbit;
  ng.get(iss.rdbuf(), 0, iss, err, ll);
  VERIFY( ll == ll1 );
  VERIFY( err == eofbit );
#endif
}

int main()
{
  test01();
  return 0;
}


// Kathleen Hannah, humanitarian, woman, art-thief
