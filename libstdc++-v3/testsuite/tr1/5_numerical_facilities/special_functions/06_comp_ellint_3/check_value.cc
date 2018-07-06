// { dg-do run { target c++11 } }
// { dg-options "-D__STDCPP_WANT_MATH_SPEC_FUNCS__" }
//
// Copyright (C) 2016-2018 Free Software Foundation, Inc.
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

//  comp_ellint_3
//  Compare against values generated by the GNU Scientific Library.
//  The GSL can be found on the web: http://www.gnu.org/software/gsl/
#include <limits>
#include <tr1/cmath>
#if defined(__TEST_DEBUG)
#  include <iostream>
#  define VERIFY(A) \
  if (!(A)) \
    { \
      std::cout << "line " << __LINE__ \
	<< "  max_abs_frac = " << max_abs_frac \
	<< std::endl; \
    }
#else
#  include <testsuite_hooks.h>
#endif
#include <specfun_testcase.h>

// Test data for k=-0.90000000000000002.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 5
// max(|f - f_Boost| / |f_Boost|): 1.2838262090802751e-16
// mean(f - f_Boost): 4.4408920985006264e-17
// variance(f - f_Boost): 2.4347558803117648e-34
// stddev(f - f_Boost): 1.5603704304785339e-17
const testcase_comp_ellint_3<double>
data001[10] =
{
  { 2.2805491384227703, -0.90000000000000002, 0.0000000000000000, 0.0 },
  { 2.4295011187834885, -0.90000000000000002, 0.10000000000000001, 0.0 },
  { 2.6076835743348412, -0.90000000000000002, 0.20000000000000001, 0.0 },
  { 2.8256506968858512, -0.90000000000000002, 0.30000000000000004, 0.0 },
  { 3.1000689868578619, -0.90000000000000002, 0.40000000000000002, 0.0 },
  { 3.4591069002104677, -0.90000000000000002, 0.50000000000000000, 0.0 },
  { 3.9549939883570229, -0.90000000000000002, 0.60000000000000009, 0.0 },
  { 4.6985482312992435, -0.90000000000000002, 0.70000000000000007, 0.0 },
  { 5.9820740813645710, -0.90000000000000002, 0.80000000000000004, 0.0 },
  { 8.9942562031858699, -0.90000000000000002, 0.90000000000000002, 0.0 },
};
const double toler001 = 2.5000000000000020e-13;

// Test data for k=-0.80000000000000004.
// max(|f - f_Boost|): 1.7763568394002505e-15 at index 8
// max(|f - f_Boost| / |f_Boost|): 4.1949393471095187e-16
// mean(f - f_Boost): 9.5479180117763459e-16
// variance(f - f_Boost): 5.4782007307014711e-34
// stddev(f - f_Boost): 2.3405556457178006e-17
const testcase_comp_ellint_3<double>
data002[10] =
{
  { 1.9953027776647294, -0.80000000000000004, 0.0000000000000000, 0.0 },
  { 2.1172616484005085, -0.80000000000000004, 0.10000000000000001, 0.0 },
  { 2.2624789434186798, -0.80000000000000004, 0.20000000000000001, 0.0 },
  { 2.4392042002725698, -0.80000000000000004, 0.30000000000000004, 0.0 },
  { 2.6604037035529728, -0.80000000000000004, 0.40000000000000002, 0.0 },
  { 2.9478781158239751, -0.80000000000000004, 0.50000000000000000, 0.0 },
  { 3.3418121892288055, -0.80000000000000004, 0.60000000000000009, 0.0 },
  { 3.9268876980046397, -0.80000000000000004, 0.70000000000000007, 0.0 },
  { 4.9246422058196071, -0.80000000000000004, 0.80000000000000004, 0.0 },
  { 7.2263259298637132, -0.80000000000000004, 0.90000000000000002, 0.0 },
};
const double toler002 = 2.5000000000000020e-13;

// Test data for k=-0.69999999999999996.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 3
// max(|f - f_Boost| / |f_Boost|): 1.9832236886714888e-16
// mean(f - f_Boost): -1.5543122344752191e-16
// variance(f - f_Boost): 2.9825759533819119e-33
// stddev(f - f_Boost): 5.4612965066748680e-17
const testcase_comp_ellint_3<double>
data003[10] =
{
  { 1.8456939983747234, -0.69999999999999996, 0.0000000000000000, 0.0 },
  { 1.9541347343119564, -0.69999999999999996, 0.10000000000000001, 0.0 },
  { 2.0829290325820202, -0.69999999999999996, 0.20000000000000001, 0.0 },
  { 2.2392290510988535, -0.69999999999999996, 0.30000000000000004, 0.0 },
  { 2.4342502915307880, -0.69999999999999996, 0.40000000000000002, 0.0 },
  { 2.6868019968236996, -0.69999999999999996, 0.50000000000000000, 0.0 },
  { 3.0314573496746742, -0.69999999999999996, 0.60000000000000009, 0.0 },
  { 3.5408408771788564, -0.69999999999999996, 0.70000000000000007, 0.0 },
  { 4.4042405729076961, -0.69999999999999996, 0.80000000000000004, 0.0 },
  { 6.3796094177887754, -0.69999999999999996, 0.90000000000000002, 0.0 },
};
const double toler003 = 2.5000000000000020e-13;

// Test data for k=-0.59999999999999998.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 2
// max(|f - f_Boost| / |f_Boost|): 2.2547200163366559e-16
// mean(f - f_Boost): -1.9984014443252818e-16
// variance(f - f_Boost): 4.9303806576313241e-33
// stddev(f - f_Boost): 7.0216669371534022e-17
const testcase_comp_ellint_3<double>
data004[10] =
{
  { 1.7507538029157526, -0.59999999999999998, 0.0000000000000000, 0.0 },
  { 1.8508766487100685, -0.59999999999999998, 0.10000000000000001, 0.0 },
  { 1.9695980282802217, -0.59999999999999998, 0.20000000000000001, 0.0 },
  { 2.1134154405060599, -0.59999999999999998, 0.30000000000000004, 0.0 },
  { 2.2925036420985130, -0.59999999999999998, 0.40000000000000002, 0.0 },
  { 2.5239007084492711, -0.59999999999999998, 0.50000000000000000, 0.0 },
  { 2.8388723099514972, -0.59999999999999998, 0.60000000000000009, 0.0 },
  { 3.3029735898397159, -0.59999999999999998, 0.70000000000000007, 0.0 },
  { 4.0867036409261832, -0.59999999999999998, 0.80000000000000004, 0.0 },
  { 5.8709993116265604, -0.59999999999999998, 0.90000000000000002, 0.0 },
};
const double toler004 = 2.5000000000000020e-13;

// Test data for k=-0.50000000000000000.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 3
// max(|f - f_Boost| / |f_Boost|): 2.1900131385114407e-16
// mean(f - f_Boost): 2.4424906541753446e-16
// variance(f - f_Boost): 7.3651365379430888e-33
// stddev(f - f_Boost): 8.5820373676319358e-17
const testcase_comp_ellint_3<double>
data005[10] =
{
  { 1.6857503548125961, -0.50000000000000000, 0.0000000000000000, 0.0 },
  { 1.7803034946545482, -0.50000000000000000, 0.10000000000000001, 0.0 },
  { 1.8922947612264021, -0.50000000000000000, 0.20000000000000001, 0.0 },
  { 2.0277924458111314, -0.50000000000000000, 0.30000000000000004, 0.0 },
  { 2.1962905366178065, -0.50000000000000000, 0.40000000000000002, 0.0 },
  { 2.4136715042011945, -0.50000000000000000, 0.50000000000000000, 0.0 },
  { 2.7090491861753558, -0.50000000000000000, 0.60000000000000009, 0.0 },
  { 3.1433945297859229, -0.50000000000000000, 0.70000000000000007, 0.0 },
  { 3.8750701888108070, -0.50000000000000000, 0.80000000000000004, 0.0 },
  { 5.5355132096026463, -0.50000000000000000, 0.90000000000000002, 0.0 },
};
const double toler005 = 2.5000000000000020e-13;

// Test data for k=-0.39999999999999991.
// max(|f - f_Boost|): 1.7763568394002505e-15 at index 9
// max(|f - f_Boost| / |f_Boost|): 4.1718164615986397e-16
// mean(f - f_Boost): 6.2172489379008762e-16
// variance(f - f_Boost): 1.6458949750907531e-31
// stddev(f - f_Boost): 4.0569631192441877e-16
const testcase_comp_ellint_3<double>
data006[10] =
{
  { 1.6399998658645112, -0.39999999999999991, 0.0000000000000000, 0.0 },
  { 1.7306968836847190, -0.39999999999999991, 0.10000000000000001, 0.0 },
  { 1.8380358826317627, -0.39999999999999991, 0.20000000000000001, 0.0 },
  { 1.9677924132520139, -0.39999999999999991, 0.30000000000000004, 0.0 },
  { 2.1289968719280026, -0.39999999999999991, 0.40000000000000002, 0.0 },
  { 2.3367461373176512, -0.39999999999999991, 0.50000000000000000, 0.0 },
  { 2.6186940209850191, -0.39999999999999991, 0.60000000000000009, 0.0 },
  { 3.0327078743873246, -0.39999999999999991, 0.70000000000000007, 0.0 },
  { 3.7289548002199902, -0.39999999999999991, 0.80000000000000004, 0.0 },
  { 5.3055535102872513, -0.39999999999999991, 0.90000000000000002, 0.0 },
};
const double toler006 = 2.5000000000000020e-13;

// Test data for k=-0.29999999999999993.
// max(|f - f_Boost|): 1.3322676295501878e-15 at index 8
// max(|f - f_Boost| / |f_Boost|): 3.9274792319434433e-16
// mean(f - f_Boost): 6.2172489379008762e-16
// variance(f - f_Boost): 8.7651211691223537e-33
// stddev(f - f_Boost): 9.3622225828712025e-17
const testcase_comp_ellint_3<double>
data007[10] =
{
  { 1.6080486199305128, -0.29999999999999993, 0.0000000000000000, 0.0 },
  { 1.6960848815118226, -0.29999999999999993, 0.10000000000000001, 0.0 },
  { 1.8002173372290500, -0.29999999999999993, 0.20000000000000001, 0.0 },
  { 1.9260216862473254, -0.29999999999999993, 0.30000000000000004, 0.0 },
  { 2.0822121773175533, -0.29999999999999993, 0.40000000000000002, 0.0 },
  { 2.2833505881933971, -0.29999999999999993, 0.50000000000000000, 0.0 },
  { 2.5560975528589065, -0.29999999999999993, 0.60000000000000009, 0.0 },
  { 2.9562123549913877, -0.29999999999999993, 0.70000000000000007, 0.0 },
  { 3.6283050484567170, -0.29999999999999993, 0.80000000000000004, 0.0 },
  { 5.1479514944016795, -0.29999999999999993, 0.90000000000000002, 0.0 },
};
const double toler007 = 2.5000000000000020e-13;

// Test data for k=-0.19999999999999996.
// max(|f - f_Boost|): 8.8817841970012523e-16 at index 9
// max(|f - f_Boost| / |f_Boost|): 1.9753938705764407e-16
// mean(f - f_Boost): 3.1086244689504381e-16
// variance(f - f_Boost): 4.1147374377268827e-32
// stddev(f - f_Boost): 2.0284815596220939e-16
const testcase_comp_ellint_3<double>
data008[10] =
{
  { 1.5868678474541662, -0.19999999999999996, 0.0000000000000000, 0.0 },
  { 1.6731552050562593, -0.19999999999999996, 0.10000000000000001, 0.0 },
  { 1.7751816279738935, -0.19999999999999996, 0.20000000000000001, 0.0 },
  { 1.8983924169967101, -0.19999999999999996, 0.30000000000000004, 0.0 },
  { 2.0512956926676806, -0.19999999999999996, 0.40000000000000002, 0.0 },
  { 2.2481046259421302, -0.19999999999999996, 0.50000000000000000, 0.0 },
  { 2.5148333891629315, -0.19999999999999996, 0.60000000000000009, 0.0 },
  { 2.9058704854500967, -0.19999999999999996, 0.70000000000000007, 0.0 },
  { 3.5622166386422633, -0.19999999999999996, 0.80000000000000004, 0.0 },
  { 5.0448269356200370, -0.19999999999999996, 0.90000000000000002, 0.0 },
};
const double toler008 = 2.5000000000000020e-13;

// Test data for k=-0.099999999999999978.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 5
// max(|f - f_Boost| / |f_Boost|): 1.9932308021417639e-16
// mean(f - f_Boost): 0.0000000000000000
// variance(f - f_Boost): 6.8368087769470551e-64
// stddev(f - f_Boost): 2.6147291976315738e-32
const testcase_comp_ellint_3<double>
data009[10] =
{
  { 1.5747455615173560, -0.099999999999999978, 0.0000000000000000, 0.0 },
  { 1.6600374067558428, -0.099999999999999978, 0.10000000000000001, 0.0 },
  { 1.7608656115083421, -0.099999999999999978, 0.20000000000000001, 0.0 },
  { 1.8826015946315438, -0.099999999999999978, 0.30000000000000004, 0.0 },
  { 2.0336367403076760, -0.099999999999999978, 0.40000000000000002, 0.0 },
  { 2.2279868912966849, -0.099999999999999978, 0.50000000000000000, 0.0 },
  { 2.4913004919173827, -0.099999999999999978, 0.60000000000000009, 0.0 },
  { 2.8771910188009744, -0.099999999999999978, 0.70000000000000007, 0.0 },
  { 3.5246199613295617, -0.099999999999999978, 0.80000000000000004, 0.0 },
  { 4.9862890417305508, -0.099999999999999978, 0.90000000000000002, 0.0 },
};
const double toler009 = 2.5000000000000020e-13;

// Test data for k=0.0000000000000000.
// max(|f - f_Boost|): 8.8817841970012523e-16 at index 9
// max(|f - f_Boost| / |f_Boost|): 2.1899085000907084e-16
// mean(f - f_Boost): -2.2204460492503131e-16
// variance(f - f_Boost): 5.4782007307014711e-32
// stddev(f - f_Boost): 2.3405556457178008e-16
const testcase_comp_ellint_3<double>
data010[10] =
{
  { 1.5707963267948966, 0.0000000000000000, 0.0000000000000000, 0.0 },
  { 1.6557647109660170, 0.0000000000000000, 0.10000000000000001, 0.0 },
  { 1.7562036827601817, 0.0000000000000000, 0.20000000000000001, 0.0 },
  { 1.8774607092226381, 0.0000000000000000, 0.30000000000000004, 0.0 },
  { 2.0278893379868062, 0.0000000000000000, 0.40000000000000002, 0.0 },
  { 2.2214414690791831, 0.0000000000000000, 0.50000000000000000, 0.0 },
  { 2.4836470664490258, 0.0000000000000000, 0.60000000000000009, 0.0 },
  { 2.8678686047727386, 0.0000000000000000, 0.70000000000000007, 0.0 },
  { 3.5124073655203634, 0.0000000000000000, 0.80000000000000004, 0.0 },
  { 4.9672941328980516, 0.0000000000000000, 0.90000000000000002, 0.0 },
};
const double toler010 = 2.5000000000000020e-13;

// Test data for k=0.10000000000000009.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 5
// max(|f - f_Boost| / |f_Boost|): 1.9932308021417639e-16
// mean(f - f_Boost): -2.2204460492503132e-17
// variance(f - f_Boost): 6.0868897007794120e-35
// stddev(f - f_Boost): 7.8018521523926693e-18
const testcase_comp_ellint_3<double>
data011[10] =
{
  { 1.5747455615173560, 0.10000000000000009, 0.0000000000000000, 0.0 },
  { 1.6600374067558428, 0.10000000000000009, 0.10000000000000001, 0.0 },
  { 1.7608656115083421, 0.10000000000000009, 0.20000000000000001, 0.0 },
  { 1.8826015946315440, 0.10000000000000009, 0.30000000000000004, 0.0 },
  { 2.0336367403076760, 0.10000000000000009, 0.40000000000000002, 0.0 },
  { 2.2279868912966849, 0.10000000000000009, 0.50000000000000000, 0.0 },
  { 2.4913004919173827, 0.10000000000000009, 0.60000000000000009, 0.0 },
  { 2.8771910188009744, 0.10000000000000009, 0.70000000000000007, 0.0 },
  { 3.5246199613295617, 0.10000000000000009, 0.80000000000000004, 0.0 },
  { 4.9862890417305508, 0.10000000000000009, 0.90000000000000002, 0.0 },
};
const double toler011 = 2.5000000000000020e-13;

// Test data for k=0.20000000000000018.
// max(|f - f_Boost|): 8.8817841970012523e-16 at index 9
// max(|f - f_Boost| / |f_Boost|): 1.9753938705764407e-16
// mean(f - f_Boost): 3.1086244689504381e-16
// variance(f - f_Boost): 4.1147374377268827e-32
// stddev(f - f_Boost): 2.0284815596220939e-16
const testcase_comp_ellint_3<double>
data012[10] =
{
  { 1.5868678474541662, 0.20000000000000018, 0.0000000000000000, 0.0 },
  { 1.6731552050562593, 0.20000000000000018, 0.10000000000000001, 0.0 },
  { 1.7751816279738935, 0.20000000000000018, 0.20000000000000001, 0.0 },
  { 1.8983924169967101, 0.20000000000000018, 0.30000000000000004, 0.0 },
  { 2.0512956926676806, 0.20000000000000018, 0.40000000000000002, 0.0 },
  { 2.2481046259421302, 0.20000000000000018, 0.50000000000000000, 0.0 },
  { 2.5148333891629315, 0.20000000000000018, 0.60000000000000009, 0.0 },
  { 2.9058704854500967, 0.20000000000000018, 0.70000000000000007, 0.0 },
  { 3.5622166386422633, 0.20000000000000018, 0.80000000000000004, 0.0 },
  { 5.0448269356200370, 0.20000000000000018, 0.90000000000000002, 0.0 },
};
const double toler012 = 2.5000000000000020e-13;

// Test data for k=0.30000000000000004.
// max(|f - f_Boost|): 8.8817841970012523e-16 at index 8
// max(|f - f_Boost| / |f_Boost|): 3.4585997630846713e-16
// mean(f - f_Boost): 5.1070259132757197e-16
// variance(f - f_Boost): 1.7591111235252501e-32
// stddev(f - f_Boost): 1.3263148659067538e-16
const testcase_comp_ellint_3<double>
data013[10] =
{
  { 1.6080486199305128, 0.30000000000000004, 0.0000000000000000, 0.0 },
  { 1.6960848815118228, 0.30000000000000004, 0.10000000000000001, 0.0 },
  { 1.8002173372290500, 0.30000000000000004, 0.20000000000000001, 0.0 },
  { 1.9260216862473254, 0.30000000000000004, 0.30000000000000004, 0.0 },
  { 2.0822121773175533, 0.30000000000000004, 0.40000000000000002, 0.0 },
  { 2.2833505881933975, 0.30000000000000004, 0.50000000000000000, 0.0 },
  { 2.5560975528589065, 0.30000000000000004, 0.60000000000000009, 0.0 },
  { 2.9562123549913877, 0.30000000000000004, 0.70000000000000007, 0.0 },
  { 3.6283050484567174, 0.30000000000000004, 0.80000000000000004, 0.0 },
  { 5.1479514944016795, 0.30000000000000004, 0.90000000000000002, 0.0 },
};
const double toler013 = 2.5000000000000020e-13;

// Test data for k=0.40000000000000013.
// max(|f - f_Boost|): 2.6645352591003757e-15 at index 9
// max(|f - f_Boost| / |f_Boost|): 6.7696531428672557e-16
// mean(f - f_Boost): 1.1990408665951691e-15
// variance(f - f_Boost): 2.6514491536595121e-31
// stddev(f - f_Boost): 5.1492224205791612e-16
const testcase_comp_ellint_3<double>
data014[10] =
{
  { 1.6399998658645112, 0.40000000000000013, 0.0000000000000000, 0.0 },
  { 1.7306968836847190, 0.40000000000000013, 0.10000000000000001, 0.0 },
  { 1.8380358826317629, 0.40000000000000013, 0.20000000000000001, 0.0 },
  { 1.9677924132520141, 0.40000000000000013, 0.30000000000000004, 0.0 },
  { 2.1289968719280030, 0.40000000000000013, 0.40000000000000002, 0.0 },
  { 2.3367461373176512, 0.40000000000000013, 0.50000000000000000, 0.0 },
  { 2.6186940209850196, 0.40000000000000013, 0.60000000000000009, 0.0 },
  { 3.0327078743873246, 0.40000000000000013, 0.70000000000000007, 0.0 },
  { 3.7289548002199906, 0.40000000000000013, 0.80000000000000004, 0.0 },
  { 5.3055535102872522, 0.40000000000000013, 0.90000000000000002, 0.0 },
};
const double toler014 = 2.5000000000000020e-13;

// Test data for k=0.50000000000000000.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 3
// max(|f - f_Boost| / |f_Boost|): 2.1900131385114407e-16
// mean(f - f_Boost): 2.4424906541753446e-16
// variance(f - f_Boost): 7.3651365379430888e-33
// stddev(f - f_Boost): 8.5820373676319358e-17
const testcase_comp_ellint_3<double>
data015[10] =
{
  { 1.6857503548125961, 0.50000000000000000, 0.0000000000000000, 0.0 },
  { 1.7803034946545482, 0.50000000000000000, 0.10000000000000001, 0.0 },
  { 1.8922947612264021, 0.50000000000000000, 0.20000000000000001, 0.0 },
  { 2.0277924458111314, 0.50000000000000000, 0.30000000000000004, 0.0 },
  { 2.1962905366178065, 0.50000000000000000, 0.40000000000000002, 0.0 },
  { 2.4136715042011945, 0.50000000000000000, 0.50000000000000000, 0.0 },
  { 2.7090491861753558, 0.50000000000000000, 0.60000000000000009, 0.0 },
  { 3.1433945297859229, 0.50000000000000000, 0.70000000000000007, 0.0 },
  { 3.8750701888108070, 0.50000000000000000, 0.80000000000000004, 0.0 },
  { 5.5355132096026463, 0.50000000000000000, 0.90000000000000002, 0.0 },
};
const double toler015 = 2.5000000000000020e-13;

// Test data for k=0.60000000000000009.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 2
// max(|f - f_Boost| / |f_Boost|): 2.2547200163366559e-16
// mean(f - f_Boost): -2.2204460492503131e-16
// variance(f - f_Boost): 6.0868897007794117e-33
// stddev(f - f_Boost): 7.8018521523926690e-17
const testcase_comp_ellint_3<double>
data016[10] =
{
  { 1.7507538029157526, 0.60000000000000009, 0.0000000000000000, 0.0 },
  { 1.8508766487100687, 0.60000000000000009, 0.10000000000000001, 0.0 },
  { 1.9695980282802217, 0.60000000000000009, 0.20000000000000001, 0.0 },
  { 2.1134154405060599, 0.60000000000000009, 0.30000000000000004, 0.0 },
  { 2.2925036420985130, 0.60000000000000009, 0.40000000000000002, 0.0 },
  { 2.5239007084492711, 0.60000000000000009, 0.50000000000000000, 0.0 },
  { 2.8388723099514976, 0.60000000000000009, 0.60000000000000009, 0.0 },
  { 3.3029735898397159, 0.60000000000000009, 0.70000000000000007, 0.0 },
  { 4.0867036409261832, 0.60000000000000009, 0.80000000000000004, 0.0 },
  { 5.8709993116265613, 0.60000000000000009, 0.90000000000000002, 0.0 },
};
const double toler016 = 2.5000000000000020e-13;

// Test data for k=0.70000000000000018.
// max(|f - f_Boost|): 1.7763568394002505e-15 at index 9
// max(|f - f_Boost| / |f_Boost|): 2.9298727220933567e-16
// mean(f - f_Boost): 4.8849813083506892e-16
// variance(f - f_Boost): 2.0476296953421943e-31
// stddev(f - f_Boost): 4.5250742483877478e-16
const testcase_comp_ellint_3<double>
data017[10] =
{
  { 1.8456939983747238, 0.70000000000000018, 0.0000000000000000, 0.0 },
  { 1.9541347343119566, 0.70000000000000018, 0.10000000000000001, 0.0 },
  { 2.0829290325820207, 0.70000000000000018, 0.20000000000000001, 0.0 },
  { 2.2392290510988540, 0.70000000000000018, 0.30000000000000004, 0.0 },
  { 2.4342502915307880, 0.70000000000000018, 0.40000000000000002, 0.0 },
  { 2.6868019968237000, 0.70000000000000018, 0.50000000000000000, 0.0 },
  { 3.0314573496746746, 0.70000000000000018, 0.60000000000000009, 0.0 },
  { 3.5408408771788569, 0.70000000000000018, 0.70000000000000007, 0.0 },
  { 4.4042405729076970, 0.70000000000000018, 0.80000000000000004, 0.0 },
  { 6.3796094177887763, 0.70000000000000018, 0.90000000000000002, 0.0 },
};
const double toler017 = 2.5000000000000020e-13;

// Test data for k=0.80000000000000004.
// max(|f - f_Boost|): 1.7763568394002505e-15 at index 8
// max(|f - f_Boost| / |f_Boost|): 4.1949393471095187e-16
// mean(f - f_Boost): 9.5479180117763459e-16
// variance(f - f_Boost): 5.4782007307014711e-34
// stddev(f - f_Boost): 2.3405556457178006e-17
const testcase_comp_ellint_3<double>
data018[10] =
{
  { 1.9953027776647294, 0.80000000000000004, 0.0000000000000000, 0.0 },
  { 2.1172616484005085, 0.80000000000000004, 0.10000000000000001, 0.0 },
  { 2.2624789434186798, 0.80000000000000004, 0.20000000000000001, 0.0 },
  { 2.4392042002725698, 0.80000000000000004, 0.30000000000000004, 0.0 },
  { 2.6604037035529728, 0.80000000000000004, 0.40000000000000002, 0.0 },
  { 2.9478781158239751, 0.80000000000000004, 0.50000000000000000, 0.0 },
  { 3.3418121892288055, 0.80000000000000004, 0.60000000000000009, 0.0 },
  { 3.9268876980046397, 0.80000000000000004, 0.70000000000000007, 0.0 },
  { 4.9246422058196071, 0.80000000000000004, 0.80000000000000004, 0.0 },
  { 7.2263259298637132, 0.80000000000000004, 0.90000000000000002, 0.0 },
};
const double toler018 = 2.5000000000000020e-13;

// Test data for k=0.90000000000000013.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 3
// max(|f - f_Boost| / |f_Boost|): 1.5716352001310461e-16
// mean(f - f_Boost): 4.4408920985006264e-17
// variance(f - f_Boost): 2.4347558803117648e-34
// stddev(f - f_Boost): 1.5603704304785339e-17
const testcase_comp_ellint_3<double>
data019[10] =
{
  { 2.2805491384227707, 0.90000000000000013, 0.0000000000000000, 0.0 },
  { 2.4295011187834890, 0.90000000000000013, 0.10000000000000001, 0.0 },
  { 2.6076835743348421, 0.90000000000000013, 0.20000000000000001, 0.0 },
  { 2.8256506968858521, 0.90000000000000013, 0.30000000000000004, 0.0 },
  { 3.1000689868578628, 0.90000000000000013, 0.40000000000000002, 0.0 },
  { 3.4591069002104686, 0.90000000000000013, 0.50000000000000000, 0.0 },
  { 3.9549939883570242, 0.90000000000000013, 0.60000000000000009, 0.0 },
  { 4.6985482312992453, 0.90000000000000013, 0.70000000000000007, 0.0 },
  { 5.9820740813645727, 0.90000000000000013, 0.80000000000000004, 0.0 },
  { 8.9942562031858735, 0.90000000000000013, 0.90000000000000002, 0.0 },
};
const double toler019 = 2.5000000000000020e-13;

template<typename Ret, unsigned int Num>
  void
  test(const testcase_comp_ellint_3<Ret> (&data)[Num], Ret toler)
  {
    bool test __attribute__((unused)) = true;
    const Ret eps = std::numeric_limits<Ret>::epsilon();
    Ret max_abs_diff = -Ret(1);
    Ret max_abs_frac = -Ret(1);
    unsigned int num_datum = Num;
    for (unsigned int i = 0; i < num_datum; ++i)
      {
	const Ret f = std::tr1::comp_ellint_3(data[i].k, data[i].nu);
	const Ret f0 = data[i].f0;
	const Ret diff = f - f0;
	if (std::abs(diff) > max_abs_diff)
	  max_abs_diff = std::abs(diff);
	if (std::abs(f0) > Ret(10) * eps
	 && std::abs(f) > Ret(10) * eps)
	  {
	    const Ret frac = diff / f0;
	    if (std::abs(frac) > max_abs_frac)
	      max_abs_frac = std::abs(frac);
	  }
      }
    VERIFY(max_abs_frac < toler);
  }

int
main()
{
  test(data001, toler001);
  test(data002, toler002);
  test(data003, toler003);
  test(data004, toler004);
  test(data005, toler005);
  test(data006, toler006);
  test(data007, toler007);
  test(data008, toler008);
  test(data009, toler009);
  test(data010, toler010);
  test(data011, toler011);
  test(data012, toler012);
  test(data013, toler013);
  test(data014, toler014);
  test(data015, toler015);
  test(data016, toler016);
  test(data017, toler017);
  test(data018, toler018);
  test(data019, toler019);
  return 0;
}
