// 2003-04-24 P�tur Run�lfsson <peturr02@ru.is>
// Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <fstream>
#include <locale>
#include <stdexcept>
#include <testsuite_hooks.h>

class Cvt : public std::codecvt<wchar_t, char, std::mbstate_t>
{
protected:
  virtual std::codecvt_base::result
  do_out(std::mbstate_t&, const wchar_t*, const wchar_t*, const wchar_t*&, 
	 char*, char*, char*&) const
  { throw std::runtime_error("codecvt failed"); }
};

int main()
{
  using namespace std;
  bool test = true;

  locale loc = locale(locale::classic(), new Cvt);
  wfilebuf* fb = new wfilebuf;
  fb->pubimbue(loc);
  fb->open("tmp_10132", ios_base::out);
  fb->sputc(L'a');
  
  try
    {
      delete fb;
    }
  catch(exception& obj)
    {
      VERIFY( false ); 
    }
  return 0;
}
