// 2001-05-21 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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

// 27.8.1.4 Overridden virtual functions

#include <fstream>
#include <testsuite_hooks.h>
#include <testsuite_io.h>

// @require@ %-*.tst %-*.txt
// @diff@ %-*.tst %*.txt

const char name_02[] = "tmp_sgetn_2out.tst"; // empty file, need to create

// Test overloaded virtual functions.
void test05() 
{
  using namespace std;
  using namespace __gnu_test;
  typedef filebuf::int_type 	int_type;
  typedef filebuf::traits_type 	traits_type;

  bool 					test = true;

  streamsize 			strmsz_1, strmsz_2, strmsz_3;
  int 					i = 0, j = 0, k = 0;
  char carray1[13] = "";
  char carray2[8192] = "";
  int_type 			c1, c2, c3, c4;
  
  // streamsize sgetn(char_type *s, streamsize n)
  // streamsize xsgetn(char_type *s, streamsize n)
  // assign up to n chars to s from input sequence, indexing in_cur as
  // approp and returning the number of chars assigned

  // out
  {
    constraint_filebuf fb_02; 
    fb_02.pubsetbuf(0, 0);
    fb_02.open(name_02, ios_base::out | ios_base::trunc);
    VERIFY( fb_02.unbuffered() );
    VERIFY( !fb_02.read_position() );
    strmsz_2 = fb_02.in_avail();
    strmsz_2 = fb_02.sgetn(carray2, 10);
    VERIFY( strmsz_2 == 0 );
    c2 = fb_02.sgetc();
    VERIFY( c2 == traits_type::eof() );
    strmsz_1 = fb_02.in_avail();
    strmsz_2 = fb_02.sgetn(carray2, strmsz_1 + 5);
    VERIFY( strmsz_1 == -1 );
    VERIFY( strmsz_2 == 0 );
    c4 = fb_02.sgetc(); 
    VERIFY( fb_02.unbuffered() );
    VERIFY( !fb_02.read_position() );
  }
}

main() 
{
  test05();
  return 0;
}
