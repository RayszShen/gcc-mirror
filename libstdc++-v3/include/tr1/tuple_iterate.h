// class template tuple -*- C++ -*-

// Copyright (C) 2004, 2005 Free Software Foundation, Inc.
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

/** @file
 *  This is an internal header file, included by other library headers.
 *  You should not attempt to use it directly.
 */

// Chris Jefferson <chris@bubblescope.net>

/// @brief class tuple_size
template<_GLIBCXX_TEMPLATE_PARAMS>
  struct tuple_size<tuple<_GLIBCXX_TEMPLATE_ARGS> >
  { static const int value = _GLIBCXX_NUM_ARGS; };

template<_GLIBCXX_TEMPLATE_PARAMS>
#ifdef _GLIBCXX_LAST_INCLUDE
  class tuple
#else
  class tuple<_GLIBCXX_TEMPLATE_ARGS>
#endif
  {
    _GLIBCXX_BIND_MEMBERS

  public:
    tuple() 
    { }

#if _GLIBCXX_NUM_ARGS == 2
    template<typename _U1, typename _U2>
      tuple(const std::pair<_U1, _U2>& __u) :
      _M_arg1(__u.first), _M_arg2(__u.second)
      { }

    template<typename _U1, typename _U2>
      tuple&
      operator=(const std::pair<_U1, _U2>& __u)
      { 
	_M_arg1 = __u.first;
	_M_arg2 = __u.second;
	return *this;
      }
#endif

#if _GLIBCXX_NUM_ARGS > 0
    explicit tuple(_GLIBCXX_TUPLE_ADD_CREF) :
      _GLIBCXX_BIND_MEMBERS_INIT
    { }

    template<_GLIBCXX_TEMPLATE_PARAMS_U>
      tuple(const tuple<_GLIBCXX_TEMPLATE_ARGS_U>& __in) :
      _GLIBCXX_TUPLE_COPY_INIT
    { }


    template<_GLIBCXX_TEMPLATE_PARAMS_U>
      tuple&
      operator=(const tuple<_GLIBCXX_TEMPLATE_ARGS_U>& __in)
      {
	_GLIBCXX_TUPLE_ASSIGN
	return *this;
      }

    tuple(const tuple& __in) :
      _GLIBCXX_TUPLE_COPY_INIT
    { }

#else

    tuple(const tuple& __in)
    { }

#endif

    tuple&
    operator=(const tuple& __in)
    {
      _GLIBCXX_TUPLE_ASSIGN
	return *this;
    }

    template<int __i, typename __Type>
      friend class __get_helper;

    template<typename, typename, typename, typename, typename,
             typename, typename, typename, typename, typename>
      friend class tuple;
  };

#ifndef _GLIBCXX_LAST_INCLUDE

template<typename _Tp>
    struct __get_helper<_GLIBCXX_NUM_ARGS, _Tp>
    {
      static typename __add_ref<typename tuple_element<_GLIBCXX_NUM_ARGS, 
	                                               _Tp>::type>::type
      get_value(_Tp& __in)
      { return __in._GLIBCXX_CAT(_M_arg,_GLIBCXX_NUM_ARGS_PLUS_1); }

      static typename __add_c_ref<typename tuple_element<_GLIBCXX_NUM_ARGS,
	                                                 _Tp>::type>::type
      get_value(const _Tp& __in)
      { return __in._GLIBCXX_CAT(_M_arg,_GLIBCXX_NUM_ARGS_PLUS_1); }
    };

/// @brief class tuple_element 
template<typename _T1, typename _T2, typename _T3, typename _T4, 
         typename _T5, typename _T6, typename _T7, typename _T8, 
         typename _T9, typename _T10>
   struct tuple_element<_GLIBCXX_NUM_ARGS, tuple<_T1, _T2, _T3, _T4, 
                                                _T5, _T6, _T7, _T8, _T9, 
                                                _T10> >
  { typedef _GLIBCXX_T_NUM_ARGS_PLUS_1 type; };

#endif
#if _GLIBCXX_NUM_ARGS == 0

tuple<>
inline make_tuple()
{ return tuple<>(); }

tuple<>
inline tie()
{ return tuple<>(); }
#else

template<_GLIBCXX_TEMPLATE_PARAMS>
  typename __stripped_tuple_type<_GLIBCXX_TEMPLATE_ARGS>::__type
  inline make_tuple(_GLIBCXX_PARAMS)
  { 
    return typename __stripped_tuple_type<_GLIBCXX_TEMPLATE_ARGS>::
      __type(_GLIBCXX_ARGS);
  }

template<_GLIBCXX_TEMPLATE_PARAMS>
  tuple<_GLIBCXX_REF_TEMPLATE_ARGS>
  inline tie(_GLIBCXX_REF_PARAMS)
  { return make_tuple(_GLIBCXX_REF_WRAP_PARAMS); }
#endif

