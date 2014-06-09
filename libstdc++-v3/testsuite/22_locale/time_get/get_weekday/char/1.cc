// 2001-09-21 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001-2013 Free Software Foundation, Inc.
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

// 22.2.5.1.1 time_get members

#include <locale>
#include <sstream>
#include <testsuite_hooks.h>

void test01()
{
  using namespace std;
  bool test __attribute__((unused)) = true;

  typedef istreambuf_iterator<char> iterator_type;

  // basic construction
  locale loc_c = locale::classic();

  const string empty;

  // create an ostream-derived object, cache the time_get facet
  iterator_type end;

  istringstream iss;
  iss.imbue(loc_c);
  const time_get<char>& tim_get = use_facet<time_get<char> >(iss.getloc()); 

  const ios_base::iostate good = ios_base::goodbit;
  ios_base::iostate errorstate = good;

  // create "C" time objects
  const tm time_bday = __gnu_test::test_tm(0, 0, 12, 4, 3, 71, 0, 93, 0);

  // iter_type 
  // get_weekday(iter_type, iter_type, ios_base&, 
  //             ios_base::iostate&, tm*) const

  // sanity checks for "C" locale
  iss.str("Sunday");
  iterator_type is_it01(iss);
  tm time01;
  errorstate = good;
  tim_get.get_weekday(is_it01, end, iss, errorstate, &time01);
  VERIFY( time01.tm_wday == time_bday.tm_wday );
  VERIFY( errorstate == ios_base::eofbit );

  iss.str("Sun");
  iterator_type is_it02(iss);
  tm time02;
  errorstate = good;
  tim_get.get_weekday(is_it02, end, iss, errorstate, &time02);
  VERIFY( time02.tm_wday == time_bday.tm_wday );
  VERIFY( errorstate == ios_base::eofbit );

  iss.str("Sun ");
  iterator_type is_it03(iss);
  tm time03;
  errorstate = good;
  iterator_type ret03 = tim_get.get_weekday(is_it03, end, iss, errorstate,
					    &time03);
  VERIFY( time03.tm_wday == time_bday.tm_wday );
  VERIFY( errorstate == good );
  VERIFY( *ret03 == ' ' );

  iss.str("San");
  iterator_type is_it04(iss);
  tm time04;
  time04.tm_wday = 4;
  errorstate = good;
  iterator_type ret04 = tim_get.get_weekday(is_it04, end, iss, errorstate,
					    &time04);
  VERIFY( time04.tm_wday == 4 );
  VERIFY( *ret04 == 'n' );
  VERIFY( errorstate == ios_base::failbit );

  iss.str("Tuesday ");
  iterator_type is_it05(iss);
  tm time05;
  errorstate = good;
  iterator_type ret05 = tim_get.get_weekday(is_it05, end, iss, errorstate,
					    &time05);
  VERIFY( time05.tm_wday == 2 );
  VERIFY( errorstate == good );
  VERIFY( *ret05 == ' ' );

  iss.str("Tuesducky "); // Kind of like Fryday, without the swirls.
  iterator_type is_it06(iss);
  tm time06;
  time06.tm_wday = 4;
  errorstate = good;
  iterator_type ret06 = tim_get.get_weekday(is_it06, end, iss, errorstate,
					    &time06);
  VERIFY( time06.tm_wday == 4 );
  VERIFY( errorstate == ios_base::failbit );
  VERIFY( *ret06 == 'u' );
}

int main()
{
  test01();
  return 0;
}
