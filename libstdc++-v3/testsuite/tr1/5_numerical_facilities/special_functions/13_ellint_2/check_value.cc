// 2007-02-04  Edward Smith-Rowland <3dw4rd@verizon.net>
//
// Copyright (C) 2007-2013 Free Software Foundation, Inc.
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

//  ellint_2


//  Compare against values generated by the GNU Scientific Library.
//  The GSL can be found on the web: http://www.gnu.org/software/gsl/

#include <tr1/cmath>
#if defined(__TEST_DEBUG)
#include <iostream>
#define VERIFY(A) \
if (!(A)) \
  { \
    std::cout << "line " << __LINE__ \
      << "  max_abs_frac = " << max_abs_frac \
      << std::endl; \
  }
#else
#include <testsuite_hooks.h>
#endif
#include "../testcase.h"


// Test data for k=-0.90000000000000002.
testcase_ellint_2<double> data001[] = {
  { -0.0000000000000000, -0.90000000000000002, 0.0000000000000000 },
  { 0.17381690606167963, -0.90000000000000002, 0.17453292519943295 },
  { 0.34337919186972055, -0.90000000000000002, 0.34906585039886590 },
  { 0.50464268659856337, -0.90000000000000002, 0.52359877559829882 },
  { 0.65400003842368593, -0.90000000000000002, 0.69813170079773179 },
  { 0.78854928419904657, -0.90000000000000002, 0.87266462599716477 },
  { 0.90645698626315407, -0.90000000000000002, 1.0471975511965976 },
  { 1.0075154899135925, -0.90000000000000002, 1.2217304763960306 },
  { 1.0940135583194071, -0.90000000000000002, 1.3962634015954636 },
  { 1.1716970527816140, -0.90000000000000002, 1.5707963267948966 },
};

// Test function for k=-0.90000000000000002.
template <typename Tp>
void test001()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data001)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data001[i].k), Tp(data001[i].phi));
      const Tp f0 = data001[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000029e-12));
}

// Test data for k=-0.80000000000000004.
testcase_ellint_2<double> data002[] = {
  { -0.0000000000000000, -0.80000000000000004, 0.0000000000000000 },
  { 0.17396762274534808, -0.80000000000000004, 0.17453292519943295 },
  { 0.34458685226969316, -0.80000000000000004, 0.34906585039886590 },
  { 0.50872923654502444, -0.80000000000000004, 0.52359877559829882 },
  { 0.66372016539176237, -0.80000000000000004, 0.69813170079773179 },
  { 0.80760344410167406, -0.80000000000000004, 0.87266462599716477 },
  { 0.93945480372495072, -0.80000000000000004, 1.0471975511965976 },
  { 1.0597473310395036, -0.80000000000000004, 1.2217304763960306 },
  { 1.1706981862452361, -0.80000000000000004, 1.3962634015954636 },
  { 1.2763499431699066, -0.80000000000000004, 1.5707963267948966 },
};

// Test function for k=-0.80000000000000004.
template <typename Tp>
void test002()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data002)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data002[i].k), Tp(data002[i].phi));
      const Tp f0 = data002[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(1.0000000000000008e-12));
}

// Test data for k=-0.69999999999999996.
testcase_ellint_2<double> data003[] = {
  { -0.0000000000000000, -0.69999999999999996, 0.0000000000000000 },
  { 0.17410041242702540, -0.69999999999999996, 0.17453292519943295 },
  { 0.34564605085764760, -0.69999999999999996, 0.34906585039886590 },
  { 0.51228495693314657, -0.69999999999999996, 0.52359877559829882 },
  { 0.67207654098799530, -0.69999999999999996, 0.69813170079773179 },
  { 0.82370932631556515, -0.69999999999999996, 0.87266462599716477 },
  { 0.96672313309452795, -0.69999999999999996, 1.0471975511965976 },
  { 1.1017090644949503, -0.69999999999999996, 1.2217304763960306 },
  { 1.2304180097292916, -0.69999999999999996, 1.3962634015954636 },
  { 1.3556611355719557, -0.69999999999999996, 1.5707963267948966 },
};

// Test function for k=-0.69999999999999996.
template <typename Tp>
void test003()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data003)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data003[i].k), Tp(data003[i].phi));
      const Tp f0 = data003[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000039e-13));
}

// Test data for k=-0.59999999999999998.
testcase_ellint_2<double> data004[] = {
  { -0.0000000000000000, -0.59999999999999998, 0.0000000000000000 },
  { 0.17421534919599130, -0.59999999999999998, 0.17453292519943295 },
  { 0.34655927787174101, -0.59999999999999998, 0.34906585039886590 },
  { 0.51533034538432165, -0.59999999999999998, 0.52359877559829882 },
  { 0.67916550597453029, -0.59999999999999998, 0.69813170079773179 },
  { 0.83720218180349870, -0.59999999999999998, 0.87266462599716477 },
  { 0.98922159354937755, -0.59999999999999998, 1.0471975511965976 },
  { 1.1357478470419360, -0.59999999999999998, 1.2217304763960306 },
  { 1.2780617372844056, -0.59999999999999998, 1.3962634015954636 },
  { 1.4180833944487243, -0.59999999999999998, 1.5707963267948966 },
};

// Test function for k=-0.59999999999999998.
template <typename Tp>
void test004()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data004)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data004[i].k), Tp(data004[i].phi));
      const Tp f0 = data004[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000039e-13));
}

// Test data for k=-0.50000000000000000.
testcase_ellint_2<double> data005[] = {
  { -0.0000000000000000, -0.50000000000000000, 0.0000000000000000 },
  { 0.17431249677315910, -0.50000000000000000, 0.17453292519943295 },
  { 0.34732862537770803, -0.50000000000000000, 0.34906585039886590 },
  { 0.51788193485993805, -0.50000000000000000, 0.52359877559829882 },
  { 0.68506022954164536, -0.50000000000000000, 0.69813170079773179 },
  { 0.84831662803347196, -0.50000000000000000, 0.87266462599716477 },
  { 1.0075555551444717, -0.50000000000000000, 1.0471975511965976 },
  { 1.1631768599287302, -0.50000000000000000, 1.2217304763960306 },
  { 1.3160584048772543, -0.50000000000000000, 1.3962634015954636 },
  { 1.4674622093394274, -0.50000000000000000, 1.5707963267948966 },
};

// Test function for k=-0.50000000000000000.
template <typename Tp>
void test005()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data005)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data005[i].k), Tp(data005[i].phi));
      const Tp f0 = data005[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.40000000000000002.
testcase_ellint_2<double> data006[] = {
  { -0.0000000000000000, -0.40000000000000002, 0.0000000000000000 },
  { 0.17439190872481269, -0.40000000000000002, 0.17453292519943295 },
  { 0.34795581767099210, -0.40000000000000002, 0.34906585039886590 },
  { 0.51995290683804474, -0.40000000000000002, 0.52359877559829882 },
  { 0.68981638464431549, -0.40000000000000002, 0.69813170079773179 },
  { 0.85722088859936041, -0.40000000000000002, 0.87266462599716477 },
  { 1.0221301327876993, -0.40000000000000002, 1.0471975511965976 },
  { 1.1848138019818371, -0.40000000000000002, 1.2217304763960306 },
  { 1.3458259266501531, -0.40000000000000002, 1.3962634015954636 },
  { 1.5059416123600402, -0.40000000000000002, 1.5707963267948966 },
};

// Test function for k=-0.40000000000000002.
template <typename Tp>
void test006()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data006)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data006[i].k), Tp(data006[i].phi));
      const Tp f0 = data006[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.30000000000000004.
testcase_ellint_2<double> data007[] = {
  { -0.0000000000000000, -0.30000000000000004, 0.0000000000000000 },
  { 0.17445362864048916, -0.30000000000000004, 0.17453292519943295 },
  { 0.34844223535713464, -0.30000000000000004, 0.34906585039886590 },
  { 0.52155353877411770, -0.30000000000000004, 0.52359877559829882 },
  { 0.69347584418369879, -0.30000000000000004, 0.69813170079773179 },
  { 0.86403609928237668, -0.30000000000000004, 0.87266462599716477 },
  { 1.0332234514065410, -0.30000000000000004, 1.0471975511965976 },
  { 1.2011943182068923, -0.30000000000000004, 1.2217304763960306 },
  { 1.3682566113689620, -0.30000000000000004, 1.3962634015954636 },
  { 1.5348334649232489, -0.30000000000000004, 1.5707963267948966 },
};

// Test function for k=-0.30000000000000004.
template <typename Tp>
void test007()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data007)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data007[i].k), Tp(data007[i].phi));
      const Tp f0 = data007[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.19999999999999996.
testcase_ellint_2<double> data008[] = {
  { -0.0000000000000000, -0.19999999999999996, 0.0000000000000000 },
  { 0.17449769027652814, -0.19999999999999996, 0.17453292519943295 },
  { 0.34878893400762095, -0.19999999999999996, 0.34906585039886590 },
  { 0.52269152856057410, -0.19999999999999996, 0.52359877559829882 },
  { 0.69606913360157596, -0.19999999999999996, 0.69813170079773179 },
  { 0.86884782374863356, -0.19999999999999996, 0.87266462599716477 },
  { 1.0410255369689567, -0.19999999999999996, 1.0471975511965976 },
  { 1.2126730391631360, -0.19999999999999996, 1.2217304763960306 },
  { 1.3839259540325153, -0.19999999999999996, 1.3962634015954636 },
  { 1.5549685462425296, -0.19999999999999996, 1.5707963267948966 },
};

// Test function for k=-0.19999999999999996.
template <typename Tp>
void test008()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data008)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data008[i].k), Tp(data008[i].phi));
      const Tp f0 = data008[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.099999999999999978.
testcase_ellint_2<double> data009[] = {
  { -0.0000000000000000, -0.099999999999999978, 0.0000000000000000 },
  { 0.17452411766649945, -0.099999999999999978, 0.17453292519943295 },
  { 0.34899665805442398, -0.099999999999999978, 0.34906585039886590 },
  { 0.52337222400508787, -0.099999999999999978, 0.52359877559829882 },
  { 0.69761705217284875, -0.099999999999999978, 0.69813170079773179 },
  { 0.87171309273007491, -0.099999999999999978, 0.87266462599716477 },
  { 1.0456602197056328, -0.099999999999999978, 1.0471975511965976 },
  { 1.2194762899272025, -0.099999999999999978, 1.2217304763960306 },
  { 1.3931950229892744, -0.099999999999999978, 1.3962634015954636 },
  { 1.5668619420216685, -0.099999999999999978, 1.5707963267948966 },
};

// Test function for k=-0.099999999999999978.
template <typename Tp>
void test009()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data009)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data009[i].k), Tp(data009[i].phi));
      const Tp f0 = data009[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.0000000000000000.
testcase_ellint_2<double> data010[] = {
  { -0.0000000000000000, 0.0000000000000000, 0.0000000000000000 },
  { 0.17453292519943295, 0.0000000000000000, 0.17453292519943295 },
  { 0.34906585039886584, 0.0000000000000000, 0.34906585039886590 },
  { 0.52359877559829882, 0.0000000000000000, 0.52359877559829882 },
  { 0.69813170079773179, 0.0000000000000000, 0.69813170079773179 },
  { 0.87266462599716477, 0.0000000000000000, 0.87266462599716477 },
  { 1.0471975511965976, 0.0000000000000000, 1.0471975511965976 },
  { 1.2217304763960304, 0.0000000000000000, 1.2217304763960306 },
  { 1.3962634015954631, 0.0000000000000000, 1.3962634015954636 },
  { 1.5707963267948966, 0.0000000000000000, 1.5707963267948966 },
};

// Test function for k=0.0000000000000000.
template <typename Tp>
void test010()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data010)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data010[i].k), Tp(data010[i].phi));
      const Tp f0 = data010[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.10000000000000009.
testcase_ellint_2<double> data011[] = {
  { -0.0000000000000000, 0.10000000000000009, 0.0000000000000000 },
  { 0.17452411766649945, 0.10000000000000009, 0.17453292519943295 },
  { 0.34899665805442398, 0.10000000000000009, 0.34906585039886590 },
  { 0.52337222400508787, 0.10000000000000009, 0.52359877559829882 },
  { 0.69761705217284875, 0.10000000000000009, 0.69813170079773179 },
  { 0.87171309273007491, 0.10000000000000009, 0.87266462599716477 },
  { 1.0456602197056328, 0.10000000000000009, 1.0471975511965976 },
  { 1.2194762899272025, 0.10000000000000009, 1.2217304763960306 },
  { 1.3931950229892744, 0.10000000000000009, 1.3962634015954636 },
  { 1.5668619420216685, 0.10000000000000009, 1.5707963267948966 },
};

// Test function for k=0.10000000000000009.
template <typename Tp>
void test011()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data011)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data011[i].k), Tp(data011[i].phi));
      const Tp f0 = data011[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.19999999999999996.
testcase_ellint_2<double> data012[] = {
  { -0.0000000000000000, 0.19999999999999996, 0.0000000000000000 },
  { 0.17449769027652814, 0.19999999999999996, 0.17453292519943295 },
  { 0.34878893400762095, 0.19999999999999996, 0.34906585039886590 },
  { 0.52269152856057410, 0.19999999999999996, 0.52359877559829882 },
  { 0.69606913360157596, 0.19999999999999996, 0.69813170079773179 },
  { 0.86884782374863356, 0.19999999999999996, 0.87266462599716477 },
  { 1.0410255369689567, 0.19999999999999996, 1.0471975511965976 },
  { 1.2126730391631360, 0.19999999999999996, 1.2217304763960306 },
  { 1.3839259540325153, 0.19999999999999996, 1.3962634015954636 },
  { 1.5549685462425296, 0.19999999999999996, 1.5707963267948966 },
};

// Test function for k=0.19999999999999996.
template <typename Tp>
void test012()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data012)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data012[i].k), Tp(data012[i].phi));
      const Tp f0 = data012[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.30000000000000004.
testcase_ellint_2<double> data013[] = {
  { -0.0000000000000000, 0.30000000000000004, 0.0000000000000000 },
  { 0.17445362864048916, 0.30000000000000004, 0.17453292519943295 },
  { 0.34844223535713464, 0.30000000000000004, 0.34906585039886590 },
  { 0.52155353877411770, 0.30000000000000004, 0.52359877559829882 },
  { 0.69347584418369879, 0.30000000000000004, 0.69813170079773179 },
  { 0.86403609928237668, 0.30000000000000004, 0.87266462599716477 },
  { 1.0332234514065410, 0.30000000000000004, 1.0471975511965976 },
  { 1.2011943182068923, 0.30000000000000004, 1.2217304763960306 },
  { 1.3682566113689620, 0.30000000000000004, 1.3962634015954636 },
  { 1.5348334649232489, 0.30000000000000004, 1.5707963267948966 },
};

// Test function for k=0.30000000000000004.
template <typename Tp>
void test013()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data013)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data013[i].k), Tp(data013[i].phi));
      const Tp f0 = data013[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.39999999999999991.
testcase_ellint_2<double> data014[] = {
  { -0.0000000000000000, 0.39999999999999991, 0.0000000000000000 },
  { 0.17439190872481269, 0.39999999999999991, 0.17453292519943295 },
  { 0.34795581767099210, 0.39999999999999991, 0.34906585039886590 },
  { 0.51995290683804474, 0.39999999999999991, 0.52359877559829882 },
  { 0.68981638464431549, 0.39999999999999991, 0.69813170079773179 },
  { 0.85722088859936041, 0.39999999999999991, 0.87266462599716477 },
  { 1.0221301327876993, 0.39999999999999991, 1.0471975511965976 },
  { 1.1848138019818373, 0.39999999999999991, 1.2217304763960306 },
  { 1.3458259266501531, 0.39999999999999991, 1.3962634015954636 },
  { 1.5059416123600404, 0.39999999999999991, 1.5707963267948966 },
};

// Test function for k=0.39999999999999991.
template <typename Tp>
void test014()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data014)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data014[i].k), Tp(data014[i].phi));
      const Tp f0 = data014[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.50000000000000000.
testcase_ellint_2<double> data015[] = {
  { -0.0000000000000000, 0.50000000000000000, 0.0000000000000000 },
  { 0.17431249677315910, 0.50000000000000000, 0.17453292519943295 },
  { 0.34732862537770803, 0.50000000000000000, 0.34906585039886590 },
  { 0.51788193485993805, 0.50000000000000000, 0.52359877559829882 },
  { 0.68506022954164536, 0.50000000000000000, 0.69813170079773179 },
  { 0.84831662803347196, 0.50000000000000000, 0.87266462599716477 },
  { 1.0075555551444717, 0.50000000000000000, 1.0471975511965976 },
  { 1.1631768599287302, 0.50000000000000000, 1.2217304763960306 },
  { 1.3160584048772543, 0.50000000000000000, 1.3962634015954636 },
  { 1.4674622093394274, 0.50000000000000000, 1.5707963267948966 },
};

// Test function for k=0.50000000000000000.
template <typename Tp>
void test015()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data015)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data015[i].k), Tp(data015[i].phi));
      const Tp f0 = data015[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.60000000000000009.
testcase_ellint_2<double> data016[] = {
  { -0.0000000000000000, 0.60000000000000009, 0.0000000000000000 },
  { 0.17421534919599130, 0.60000000000000009, 0.17453292519943295 },
  { 0.34655927787174101, 0.60000000000000009, 0.34906585039886590 },
  { 0.51533034538432165, 0.60000000000000009, 0.52359877559829882 },
  { 0.67916550597453029, 0.60000000000000009, 0.69813170079773179 },
  { 0.83720218180349870, 0.60000000000000009, 0.87266462599716477 },
  { 0.98922159354937789, 0.60000000000000009, 1.0471975511965976 },
  { 1.1357478470419360, 0.60000000000000009, 1.2217304763960306 },
  { 1.2780617372844056, 0.60000000000000009, 1.3962634015954636 },
  { 1.4180833944487241, 0.60000000000000009, 1.5707963267948966 },
};

// Test function for k=0.60000000000000009.
template <typename Tp>
void test016()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data016)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data016[i].k), Tp(data016[i].phi));
      const Tp f0 = data016[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000039e-13));
}

// Test data for k=0.69999999999999996.
testcase_ellint_2<double> data017[] = {
  { -0.0000000000000000, 0.69999999999999996, 0.0000000000000000 },
  { 0.17410041242702540, 0.69999999999999996, 0.17453292519943295 },
  { 0.34564605085764760, 0.69999999999999996, 0.34906585039886590 },
  { 0.51228495693314657, 0.69999999999999996, 0.52359877559829882 },
  { 0.67207654098799530, 0.69999999999999996, 0.69813170079773179 },
  { 0.82370932631556515, 0.69999999999999996, 0.87266462599716477 },
  { 0.96672313309452795, 0.69999999999999996, 1.0471975511965976 },
  { 1.1017090644949503, 0.69999999999999996, 1.2217304763960306 },
  { 1.2304180097292916, 0.69999999999999996, 1.3962634015954636 },
  { 1.3556611355719557, 0.69999999999999996, 1.5707963267948966 },
};

// Test function for k=0.69999999999999996.
template <typename Tp>
void test017()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data017)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data017[i].k), Tp(data017[i].phi));
      const Tp f0 = data017[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000039e-13));
}

// Test data for k=0.80000000000000004.
testcase_ellint_2<double> data018[] = {
  { -0.0000000000000000, 0.80000000000000004, 0.0000000000000000 },
  { 0.17396762274534808, 0.80000000000000004, 0.17453292519943295 },
  { 0.34458685226969316, 0.80000000000000004, 0.34906585039886590 },
  { 0.50872923654502444, 0.80000000000000004, 0.52359877559829882 },
  { 0.66372016539176237, 0.80000000000000004, 0.69813170079773179 },
  { 0.80760344410167406, 0.80000000000000004, 0.87266462599716477 },
  { 0.93945480372495072, 0.80000000000000004, 1.0471975511965976 },
  { 1.0597473310395036, 0.80000000000000004, 1.2217304763960306 },
  { 1.1706981862452361, 0.80000000000000004, 1.3962634015954636 },
  { 1.2763499431699066, 0.80000000000000004, 1.5707963267948966 },
};

// Test function for k=0.80000000000000004.
template <typename Tp>
void test018()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data018)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data018[i].k), Tp(data018[i].phi));
      const Tp f0 = data018[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(1.0000000000000008e-12));
}

// Test data for k=0.89999999999999991.
testcase_ellint_2<double> data019[] = {
  { -0.0000000000000000, 0.89999999999999991, 0.0000000000000000 },
  { 0.17381690606167963, 0.89999999999999991, 0.17453292519943295 },
  { 0.34337919186972055, 0.89999999999999991, 0.34906585039886590 },
  { 0.50464268659856337, 0.89999999999999991, 0.52359877559829882 },
  { 0.65400003842368581, 0.89999999999999991, 0.69813170079773179 },
  { 0.78854928419904657, 0.89999999999999991, 0.87266462599716477 },
  { 0.90645698626315385, 0.89999999999999991, 1.0471975511965976 },
  { 1.0075154899135930, 0.89999999999999991, 1.2217304763960306 },
  { 1.0940135583194071, 0.89999999999999991, 1.3962634015954636 },
  { 1.1716970527816142, 0.89999999999999991, 1.5707963267948966 },
};

// Test function for k=0.89999999999999991.
template <typename Tp>
void test019()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data019)
                         / sizeof(testcase_ellint_2<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::ellint_2(Tp(data019[i].k), Tp(data019[i].phi));
      const Tp f0 = data019[i].f0;
      const Tp diff = f - f0;
      if (std::abs(diff) > max_abs_diff)
        max_abs_diff = std::abs(diff);
      if (std::abs(f0) > Tp(10) * eps
       && std::abs(f) > Tp(10) * eps)
        {
          const Tp frac = diff / f0;
          if (std::abs(frac) > max_abs_frac)
            max_abs_frac = std::abs(frac);
        }
    }
  VERIFY(max_abs_frac < Tp(5.0000000000000029e-12));
}

int main(int, char**)
{
  test001<double>();
  test002<double>();
  test003<double>();
  test004<double>();
  test005<double>();
  test006<double>();
  test007<double>();
  test008<double>();
  test009<double>();
  test010<double>();
  test011<double>();
  test012<double>();
  test013<double>();
  test014<double>();
  test015<double>();
  test016<double>();
  test017<double>();
  test018<double>();
  test019<double>();
  return 0;
}
