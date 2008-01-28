// { dg-do compile }

// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008 Free
// Software Foundation
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
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

// 20.4.5 Template class auto_ptr negative tests [lib.auto.ptr]

#include <memory>
#include <testsuite_hooks.h>

// via Jack Reeves <jack_reeves@hispeed.ch>
// libstdc++/3946
// http://gcc.gnu.org/ml/libstdc++/2002-07/msg00024.html
struct Base { };
struct Derived : public Base { };

std::auto_ptr<Derived> 
foo() { return std::auto_ptr<Derived>(new Derived); }

int
test01()
{
  std::auto_ptr<Base> ptr2;
  ptr2 = new Base; // { dg-error "no match" }
  return 0;
}

int 
main()
{
  test01();
  return 0;
}
// { dg-error "candidates" "" { target *-*-* } 139 } 
// { dg-error "::auto_ptr" "" { target *-*-* } 267 } 
