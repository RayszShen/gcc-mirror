// -*- C++ -*- ARM specific Exception handling support routines.
// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING.  If not, write to
// the Free Software Foundation, 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.


#include "unwind-cxx.h"

using namespace __cxxabiv1;


// Given the thrown type THROW_TYPE, pointer to a variable containing a
// pointer to the exception object THROWN_PTR_P and a type CATCH_TYPE to
// compare against, return whether or not there is a match and if so,
// update *THROWN_PTR_P.

extern "C" bool
__cxa_type_match(_Unwind_Exception* ue_header,
		 const std::type_info* catch_type,
		 void** thrown_ptr_p)
{
  if (!__is_gxx_exception_class(ue_header->exception_class))
    return false;

  __cxa_exception* xh = __get_exception_header_from_ue(ue_header);
  const std::type_info* throw_type = xh->exceptionType;
  void* thrown_ptr = *thrown_ptr_p;

  // Pointer types need to adjust the actual pointer, not
  // the pointer to pointer that is the exception object.
  // This also has the effect of passing pointer types
  // "by value" through the __cxa_begin_catch return value.
  if (throw_type->__is_pointer_p())
    thrown_ptr = *(void**) thrown_ptr;

  if (catch_type->__do_catch(throw_type, &thrown_ptr, 1))
    {
      *thrown_ptr_p = thrown_ptr;
      return true;
    }

  return false;
}

extern "C" void
__cxa_begin_cleanup(_Unwind_Exception* ue_header __attribute__((unused)))
{
}

extern "C" void
__cxa_end_cleanup(_Unwind_Exception* ue_header)
{
  /* TODO: This should really be an assembly stub which doesn't corrupt any
     registers.  */
  _Unwind_Resume(ue_header);
}

