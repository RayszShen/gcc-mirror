// Copyright (C) 1994, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2007
// Free Software Foundation
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.

// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING.  If not, write to
// the Free Software Foundation, 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA. 

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline enums from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#include "tinfo.h"

namespace __cxxabiv1 {

__si_class_type_info::
~__si_class_type_info ()
{}

__class_type_info::__sub_kind __si_class_type_info::
__do_find_public_src (ptrdiff_t src2dst,
                      const void *obj_ptr,
                      const __class_type_info *src_type,
                      const void *src_ptr) const
{
  if (src_ptr == obj_ptr && *this == *src_type)
    return __contained_public;
  return __base_type->__do_find_public_src (src2dst, obj_ptr, src_type, src_ptr);
}

bool __si_class_type_info::
__do_dyncast (ptrdiff_t src2dst,
              __sub_kind access_path,
              const __class_type_info *dst_type,
              const void *obj_ptr,
              const __class_type_info *src_type,
              const void *src_ptr,
              __dyncast_result &__restrict result) const
{
  if (*this == *dst_type)
    {
      result.dst_ptr = obj_ptr;
      result.whole2dst = access_path;
      if (src2dst >= 0)
        result.dst2src = adjust_pointer <void> (obj_ptr, src2dst) == src_ptr
              ? __contained_public : __not_contained;
      else if (src2dst == -2)
        result.dst2src = __not_contained;
      return false;
    }
  if (obj_ptr == src_ptr && *this == *src_type)
    {
      // The src object we started from. Indicate how we are accessible from
      // the most derived object.
      result.whole2src = access_path;
      return false;
    }
  return __base_type->__do_dyncast (src2dst, access_path, dst_type, obj_ptr,
                             src_type, src_ptr, result);
}

bool __si_class_type_info::
__do_upcast (const __class_type_info *dst, const void *obj_ptr,
             __upcast_result &__restrict result) const
{
  if (__class_type_info::__do_upcast (dst, obj_ptr, result))
    return true;
  
  return __base_type->__do_upcast (dst, obj_ptr, result);
}

}
