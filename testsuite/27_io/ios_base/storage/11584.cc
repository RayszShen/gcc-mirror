// 2004-01-25 jlquinn@gcc.gnu.org

// Copyright (C) 2004 Free Software Foundation
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

// 27.4.2.5 ios_base storage functions

#include <cstdlib>
#include <new>
#include <testsuite_hooks.h>

int new_fails;

void* operator new (size_t n)
{
    if (new_fails)
        throw std::bad_alloc();

    return malloc(n);
}

void operator delete (void *p) { free(p); }
void* operator new[] (size_t n) { return operator new(n); }
void operator delete[] (void *p) { operator delete(p); }

int main ()
{
    const int i = std::ios::xalloc ();

    new_fails = 1;

    // Successive accesses to failure storage clears to zero.
    std::cout.iword(100) = 0xdeadbeef;
    VERIFY(std::cout.iword(100) == 0);

    // Access to pword failure storage shouldn't clear iword pword storage.
    long& lr = std::cout.iword(100);
    lr = 0xdeadbeef;

    void* pv = std::cout.pword(100);
    VERIFY(pv == 0);
    VERIFY(lr == 0xdeadbeef);
    
    return 0;
}

