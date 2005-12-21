// String utility -*- C++ -*-

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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

/** @file ext/string_util.h
 *  This file is a GNU extension to the Standard C++ Library.
 */

#ifndef _STRING_UTIL_H
#define _STRING_UTIL_H 1

#include <bits/functexcept.h>
#include <bits/stringfwd.h>
#include <cstddef>
#include <bits/stl_iterator_base_types.h>
#include <bits/stl_iterator.h>
#include <bits/cpp_type_traits.h>
#include <algorithm> // For std::distance, srd::search.

namespace __gnu_cxx
{
  template<typename _CharT, typename _Traits, typename _Alloc>
    struct __string_utility
    {
      typedef typename _Alloc::template rebind<_CharT>::other _CharT_alloc_type;

      typedef _Traits					    traits_type;      
      typedef typename _Traits::char_type		    value_type;
      typedef typename _CharT_alloc_type::size_type	    size_type;
      typedef typename _CharT_alloc_type::pointer	    pointer;
      typedef typename _CharT_alloc_type::const_pointer	    const_pointer;
      typedef __gnu_cxx::
      __normal_iterator<pointer,
			std::basic_string<_CharT, _Traits, _Alloc> >
        iterator;
      typedef __gnu_cxx::
      __normal_iterator<const_pointer,
			std::basic_string<_CharT, _Traits, _Alloc> >
        const_iterator;

      // NB:  When the allocator is empty, deriving from it saves space 
      // (http://www.cantrip.org/emptyopt.html).
      template<typename _Alloc1>
        struct _Alloc_hider
	: public _Alloc1
	{
	  _Alloc_hider(const _Alloc1& __a, _CharT* __ptr)
	  : _Alloc1(__a), _M_p(__ptr) { }

	  _CharT*  _M_p; // The actual data.
	};

      // For use in _M_construct (_S_construct) forward_iterator_tag.
      template<typename _Type>
        static bool
        _S_is_null_pointer(_Type* __ptr)
        { return __ptr == 0; }

      template<typename _Type>
        static bool
        _S_is_null_pointer(_Type)
        { return false; }

      // When __n = 1 way faster than the general multichar
      // traits_type::copy/move/assign.
      static void
      _S_copy(_CharT* __d, const _CharT* __s, size_type __n)
      {
	if (__n == 1)
	  traits_type::assign(*__d, *__s);
	else
	  traits_type::copy(__d, __s, __n);
      }

      static void
      _S_move(_CharT* __d, const _CharT* __s, size_type __n)
      {
	if (__n == 1)
	  traits_type::assign(*__d, *__s);
	else
	  traits_type::move(__d, __s, __n);	  
      }

      static void
      _S_assign(_CharT* __d, size_type __n, _CharT __c)
      {
	if (__n == 1)
	  traits_type::assign(*__d, __c);
	else
	  traits_type::assign(__d, __n, __c);	  
      }

      // _S_copy_chars is a separate template to permit specialization
      // to optimize for the common case of pointers as iterators.
      template<typename _Iterator>
        static void
        _S_copy_chars(_CharT* __p, _Iterator __k1, _Iterator __k2)
        {
	  for (; __k1 != __k2; ++__k1, ++__p)
	    traits_type::assign(*__p, *__k1); // These types are off.
	}

      static void
      _S_copy_chars(_CharT* __p, iterator __k1, iterator __k2)
      { _S_copy_chars(__p, __k1.base(), __k2.base()); }

      static void
      _S_copy_chars(_CharT* __p, const_iterator __k1, const_iterator __k2)
      { _S_copy_chars(__p, __k1.base(), __k2.base()); }

      static void
      _S_copy_chars(_CharT* __p, _CharT* __k1, _CharT* __k2)
      { _S_copy(__p, __k1, __k2 - __k1); }

      static void
      _S_copy_chars(_CharT* __p, const _CharT* __k1, const _CharT* __k2)
      { _S_copy(__p, __k1, __k2 - __k1); }
    };
} // namespace __gnu_cxx

#endif /* _STRING_UTIL_H */
