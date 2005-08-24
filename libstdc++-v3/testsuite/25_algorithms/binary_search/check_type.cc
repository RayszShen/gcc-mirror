// Copyright (C) 2005 Free Software Foundation, Inc.
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

// 25.3.3.4 [lib.binary.search]

// { dg-do compile }

#include <algorithm>
#include <testsuite_iterators.h>

using __gnu_test::forward_iterator_wrapper;

struct S { };

bool operator<(const S&, const S&) {return true;}

struct X { };

bool predicate(const X&, const X&) {return true;}

bool
test1(forward_iterator_wrapper<S>& s)
{ return std::binary_search(s, s, *s); }

bool
test2(forward_iterator_wrapper<X>& x)
{ return std::binary_search(x, x, *x, predicate); }
