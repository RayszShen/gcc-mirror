// { dg-options "-std=gnu++0x" }
// { dg-do compile }

// Copyright (C) 2011 Free Software Foundation
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

// You should have received a copy of the GNU General Public License
// along with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <memory>

struct D;

struct B
{
 B& operator=(D&) = delete; // { dg-error "declared here" }

 template<class T>
   void operator()(T*) const {}
};

struct D : B { };

// libstdc++/48635
void f()
{
  B b;
  D d;

  std::unique_ptr<int, B&> ub(nullptr, b);
  std::unique_ptr<int, D&> ud(nullptr, d);
  ub = std::move(ud);
// { dg-error "use of deleted function" "" { target *-*-* } 189 }

  std::unique_ptr<int[], B&> uba(nullptr, b);
  std::unique_ptr<int[], D&> uda(nullptr, d);
  uba = std::move(uda);
// { dg-error "use of deleted function" "" { target *-*-* } 329 }
}
