// Copyright (C) 2018 Free Software Foundation, Inc.
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

// { dg-do run { target c++11 } }

// { dg-options "-std=gnu++2a" }
// { dg-do compile { target c++2a } }

#include <bit>

template<typename UInt>
constexpr auto
test(UInt x)
-> decltype(std::countr_zero(x))
{
  static_assert( noexcept(std::countr_zero(x)) );

  constexpr unsigned digits = std::numeric_limits<UInt>::digits;

  static_assert( std::countr_zero((UInt)0) == digits );
  static_assert( std::countr_zero((UInt)-1) == 0 );
  static_assert( std::countr_zero((UInt)127) == 0 );

  static_assert( std::countr_zero((UInt)1) == 0 );
  static_assert( std::countr_zero((UInt)2) == 1 );
  static_assert( std::countr_zero((UInt)0x70) == 4 );

  if constexpr (std::numeric_limits<UInt>::digits > 8)
  {
    static_assert( std::countr_zero((UInt)(1u << 8)) == 8 );
    static_assert( std::countr_zero((UInt)(4u << 9)) == 11 );
  }

  if constexpr (std::numeric_limits<UInt>::digits > 64)
  {
    static_assert( std::countr_zero((UInt)2 << 70) == 71 );
  }

  return true;
}

static_assert( test( (unsigned char)0 ) );
static_assert( test( (unsigned short)0 ) );
static_assert( test( (unsigned int)0 ) );
static_assert( test( (unsigned long)0 ) );
static_assert( test( (unsigned long long)0 ) );

// std::countr_zero(T) shall not participate in overload resolution
// unless T is an unsigned integer type.
struct X { constexpr bool did_not_match() { return true; } };
constexpr X test(...) { return X{}; }
static_assert( test( (bool)0 ).did_not_match() );
static_assert( test( (char)0 ).did_not_match() );
static_assert( test( (int)0 ).did_not_match() );
static_assert( test( (char16_t)0 ).did_not_match() );
static_assert( test( (float)0 ).did_not_match() );
static_assert( test( (void*)0 ).did_not_match() );
static_assert( test( X{} ).did_not_match() );
enum E : unsigned { e };
static_assert( test( e ).did_not_match() );

#ifndef __STRICT_ANSI__
#include <cstddef>
constexpr int bits = std::numeric_limits<unsigned char>::digits;
static_assert( std::countr_zero(std::byte{0}) == bits );
static_assert( std::countr_zero(std::byte{0x01}) == 0 );
static_assert( std::countr_zero(std::byte{0x02}) == 1 );
static_assert( std::countr_zero(std::byte{0x03}) == 0 );
static_assert( std::countr_zero(std::byte{0x30}) == 4 );
static_assert( std::countr_zero(std::byte{0x40}) == 6 );
static_assert( std::countr_zero(std::byte{0x41}) == 0 );
#else
static_assert( test( (std::byte)0 ).did_not_match() );
#endif

#if !defined(__STRICT_ANSI__) && defined _GLIBCXX_USE_INT128
static_assert( test( (unsigned __int128)0 ) );
static_assert( test( (__int128)0 ).did_not_match() );
#endif
#if defined(__GLIBCXX_TYPE_INT_N_0)
static_assert( test( (unsigned __GLIBCXX_TYPE_INT_N_0)0 ) );
static_assert( test( (__GLIBCXX_TYPE_INT_N_0)0 ).did_not_match() );
#endif
#if defined(__GLIBCXX_TYPE_INT_N_1)
static_assert( test( (unsigned __GLIBCXX_TYPE_INT_N_1)0 ) );
static_assert( test( (__GLIBCXX_TYPE_INT_N_1)0 ).did_not_match() );
#endif
#if defined(__GLIBCXX_TYPE_INT_N_2)
static_assert( test( (unsigned __GLIBCXX_TYPE_INT_N_2)0 ) );
static_assert( test( (__GLIBCXX_TYPE_INT_N_2)0 ).did_not_match() );
#endif
