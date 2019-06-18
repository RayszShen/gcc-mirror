// Copyright (C) 2019 Free Software Foundation, Inc.
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

// { dg-options "-std=gnu++17" }
// { dg-do preprocess { target c++17 } }

#include <algorithm>
#ifndef __cpp_lib_parallel_algorithm
# error "Feature-test macro for parallel algorithms missing"
#elif __cpp_lib_parallel_algorithm != 201603L
# error "Feature-test macro for parallel algorithms has wrong value in <algorithm>"
#endif

#include <numeric>
#if __cpp_lib_parallel_algorithm != 201603L
# error "Feature-test macro for parallel algorithms has wrong value in <numeric>"
#endif

#include <version>
#if __cpp_lib_parallel_algorithm != 201603L
# error "Feature-test macro for parallel algorithms has wrong value in <version>"
#endif

// The N4810 draft does not require the macro to be defined in <execution>.
#include <memory>
#if __cpp_lib_parallel_algorithm != 201603L
# error "Feature-test macro for parallel algorithms has wrong value in <memory>"
#endif

// The N4810 draft does not require the macro to be defined in <execution>.
// Include this last, because it will trigger the inclusion of TBB headers,
// which then include <memory>, so we need to have already checked <memory>.
#include <execution>
#if __cpp_lib_parallel_algorithm != 201603L
# error "Feature-test macro for parallel algorithms has wrong value in <execution>"
#endif
