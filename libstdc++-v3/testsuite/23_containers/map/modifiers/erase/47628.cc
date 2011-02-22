// Copyright (C) 2011 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.
//

// { dg-do compile }

#include <map>

struct Key
{
  Key() { }

  Key(const Key&) { }

  template<typename T>
    Key(const T&)
    { }

  bool operator<(const Key&) const;
};

#ifndef __GXX_EXPERIMENTAL_CXX0X__
// libstdc++/47628
void f()
{
  typedef std::map<Key, int> Map;
  Map m;
  m.insert(Map::value_type());
  Map::iterator i = m.begin();
  m.erase(i);
}
#endif
