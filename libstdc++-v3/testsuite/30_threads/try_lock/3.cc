// { dg-do run { target *-*-freebsd* *-*-netbsd* *-*-linux* *-*-solaris* *-*-cygwin *-*-darwin* alpha*-*-osf* mips-sgi-irix6* } }
// { dg-options " -std=gnu++0x -pthread" { target *-*-freebsd* *-*-netbsd* *-*-linux* alpha*-*-osf* mips-sgi-irix6* } }
// { dg-options " -std=gnu++0x -pthreads" { target *-*-solaris* } }
// { dg-options " -std=gnu++0x " { target *-*-cygwin *-*-darwin* } }
// { dg-require-cstdint "" }
// { dg-require-gthreads "" }

// Copyright (C) 2008 Free Software Foundation, Inc.
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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#include <mutex>
#include <system_error>
#include <testsuite_hooks.h>

struct user_lock
{
  user_lock() : is_locked(false) { }
  ~user_lock() = default;
  user_lock(const user_lock&) = default;

  void lock()
  {
    bool test __attribute__((unused)) = true;
    VERIFY( !is_locked );
    is_locked = true;
  }

  bool try_lock() 
  { return is_locked ? false : (is_locked = true); }

  void unlock()
  {
    bool test __attribute__((unused)) = true;
    VERIFY( is_locked );
    is_locked = false;
  }

private:
  bool is_locked;
};

int main()
{
  bool test __attribute__((unused)) = true;

  try
    {
      std::mutex m1;
      std::recursive_mutex m2;
      user_lock m3;

      try
	{
	  //heterogeneous types
	  int result = std::try_lock(m1, m2, m3);
	  VERIFY( result == -1 );
	  m1.unlock();
	  m2.unlock();
	  m3.unlock();
	}
      catch (const std::system_error& e)
	{
	  VERIFY( false );
	}
    }
  catch (const std::system_error& e)
    {
      VERIFY( false );
    }
  catch (...)
    {
      VERIFY( false );
    }

  return 0;
}
