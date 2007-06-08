// 2007-02-04  Edward Smith-Rowland <3dw4rd@verizon.net>
//
// Copyright (C) 2007 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

//  comp_ellint_3


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
testcase_comp_ellint_3<double> data001[] = {
  { 2.2805491384227703, -0.90000000000000002, 0.0000000000000000 },
  { 2.1537868513875287, -0.90000000000000002, 0.10000000000000001 },
  { 2.0443194576468890, -0.90000000000000002, 0.20000000000000001 },
  { 1.9486280260314426, -0.90000000000000002, 0.29999999999999999 },
  { 1.8641114227238349, -0.90000000000000002, 0.40000000000000002 },
  { 1.7888013241937861, -0.90000000000000002, 0.50000000000000000 },
  { 1.7211781128919523, -0.90000000000000002, 0.59999999999999998 },
  { 1.6600480747670938, -0.90000000000000002, 0.69999999999999996 },
  { 1.6044591960982204, -0.90000000000000002, 0.80000000000000004 },
  { 1.5536420236310946, -0.90000000000000002, 0.90000000000000002 },
};

// Test function for k=-0.90000000000000002.
template <typename Tp>
void test001()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data001)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data001[i].k), Tp(data001[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.80000000000000004.
testcase_comp_ellint_3<double> data002[] = {
  { 1.9953027776647296, -0.80000000000000004, 0.0000000000000000 },
  { 1.8910755418379521, -0.80000000000000004, 0.10000000000000001 },
  { 1.8007226661734588, -0.80000000000000004, 0.20000000000000001 },
  { 1.7214611048717301, -0.80000000000000004, 0.29999999999999999 },
  { 1.6512267838651289, -0.80000000000000004, 0.40000000000000002 },
  { 1.5884528947755532, -0.80000000000000004, 0.50000000000000000 },
  { 1.5319262547427865, -0.80000000000000004, 0.59999999999999998 },
  { 1.4806912324625332, -0.80000000000000004, 0.69999999999999996 },
  { 1.4339837018309474, -0.80000000000000004, 0.80000000000000004 },
  { 1.3911845406776222, -0.80000000000000004, 0.90000000000000002 },
};

// Test function for k=-0.80000000000000004.
template <typename Tp>
void test002()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data002)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data002[i].k), Tp(data002[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.69999999999999996.
testcase_comp_ellint_3<double> data003[] = {
  { 1.8456939983747236, -0.69999999999999996, 0.0000000000000000 },
  { 1.7528050171757608, -0.69999999999999996, 0.10000000000000001 },
  { 1.6721098780092147, -0.69999999999999996, 0.20000000000000001 },
  { 1.6011813647733213, -0.69999999999999996, 0.29999999999999999 },
  { 1.5382162002954762, -0.69999999999999996, 0.40000000000000002 },
  { 1.4818433192178544, -0.69999999999999996, 0.50000000000000000 },
  { 1.4309994736080540, -0.69999999999999996, 0.59999999999999998 },
  { 1.3848459188329196, -0.69999999999999996, 0.69999999999999996 },
  { 1.3427110650397533, -0.69999999999999996, 0.80000000000000004 },
  { 1.3040500499695911, -0.69999999999999996, 0.90000000000000002 },
};

// Test function for k=-0.69999999999999996.
template <typename Tp>
void test003()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data003)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data003[i].k), Tp(data003[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.59999999999999998.
testcase_comp_ellint_3<double> data004[] = {
  { 1.7507538029157526, -0.59999999999999998, 0.0000000000000000 },
  { 1.6648615773343014, -0.59999999999999998, 0.10000000000000001 },
  { 1.5901418016279374, -0.59999999999999998, 0.20000000000000001 },
  { 1.5243814243493585, -0.59999999999999998, 0.29999999999999999 },
  { 1.4659345278069984, -0.59999999999999998, 0.40000000000000002 },
  { 1.4135484285693078, -0.59999999999999998, 0.50000000000000000 },
  { 1.3662507535812816, -0.59999999999999998, 0.59999999999999998 },
  { 1.3232737468822811, -0.59999999999999998, 0.69999999999999996 },
  { 1.2840021261752192, -0.59999999999999998, 0.80000000000000004 },
  { 1.2479362973851875, -0.59999999999999998, 0.90000000000000002 },
};

// Test function for k=-0.59999999999999998.
template <typename Tp>
void test004()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data004)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data004[i].k), Tp(data004[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=-0.50000000000000000.
testcase_comp_ellint_3<double> data005[] = {
  { 1.6857503548125963, -0.50000000000000000, 0.0000000000000000 },
  { 1.6045524936084892, -0.50000000000000000, 0.10000000000000001 },
  { 1.5338490483665983, -0.50000000000000000, 0.20000000000000001 },
  { 1.4715681939859637, -0.50000000000000000, 0.29999999999999999 },
  { 1.4161679518465340, -0.50000000000000000, 0.40000000000000002 },
  { 1.3664739530045971, -0.50000000000000000, 0.50000000000000000 },
  { 1.3215740290190876, -0.50000000000000000, 0.59999999999999998 },
  { 1.2807475181182502, -0.50000000000000000, 0.69999999999999996 },
  { 1.2434165408189539, -0.50000000000000000, 0.80000000000000004 },
  { 1.2091116095504744, -0.50000000000000000, 0.90000000000000002 },
};

// Test function for k=-0.50000000000000000.
template <typename Tp>
void test005()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data005)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data005[i].k), Tp(data005[i].nu));
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
testcase_comp_ellint_3<double> data006[] = {
  { 1.6399998658645112, -0.40000000000000002, 0.0000000000000000 },
  { 1.5620566886683604, -0.40000000000000002, 0.10000000000000001 },
  { 1.4941414344266770, -0.40000000000000002, 0.20000000000000001 },
  { 1.4342789859950078, -0.40000000000000002, 0.29999999999999999 },
  { 1.3809986210732901, -0.40000000000000002, 0.40000000000000002 },
  { 1.3331797176377398, -0.40000000000000002, 0.50000000000000000 },
  { 1.2899514672527024, -0.40000000000000002, 0.59999999999999998 },
  { 1.2506255923253344, -0.40000000000000002, 0.69999999999999996 },
  { 1.2146499565727209, -0.40000000000000002, 0.80000000000000004 },
  { 1.1815758115929846, -0.40000000000000002, 0.90000000000000002 },
};

// Test function for k=-0.40000000000000002.
template <typename Tp>
void test006()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data006)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data006[i].k), Tp(data006[i].nu));
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
testcase_comp_ellint_3<double> data007[] = {
  { 1.6080486199305126, -0.30000000000000004, 0.0000000000000000 },
  { 1.5323534693557526, -0.30000000000000004, 0.10000000000000001 },
  { 1.4663658145259875, -0.30000000000000004, 0.20000000000000001 },
  { 1.4081767433479089, -0.30000000000000004, 0.29999999999999999 },
  { 1.3563643538969761, -0.30000000000000004, 0.40000000000000002 },
  { 1.3098448759814960, -0.30000000000000004, 0.50000000000000000 },
  { 1.2677758800420666, -0.30000000000000004, 0.59999999999999998 },
  { 1.2294913236274980, -0.30000000000000004, 0.69999999999999996 },
  { 1.1944567571590046, -0.30000000000000004, 0.80000000000000004 },
  { 1.1622376896064912, -0.30000000000000004, 0.90000000000000002 },
};

// Test function for k=-0.30000000000000004.
template <typename Tp>
void test007()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data007)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data007[i].k), Tp(data007[i].nu));
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
testcase_comp_ellint_3<double> data008[] = {
  { 1.5868678474541664, -0.19999999999999996, 0.0000000000000000 },
  { 1.5126513474261092, -0.19999999999999996, 0.10000000000000001 },
  { 1.4479323932249568, -0.19999999999999996, 0.20000000000000001 },
  { 1.3908453514752481, -0.19999999999999996, 0.29999999999999999 },
  { 1.3400002519661010, -0.19999999999999996, 0.40000000000000002 },
  { 1.2943374404397376, -0.19999999999999996, 0.50000000000000000 },
  { 1.2530330675914561, -0.19999999999999996, 0.59999999999999998 },
  { 1.2154356555075867, -0.19999999999999996, 0.69999999999999996 },
  { 1.1810223448909913, -0.19999999999999996, 0.80000000000000004 },
  { 1.1493679916141863, -0.19999999999999996, 0.90000000000000002 },
};

// Test function for k=-0.19999999999999996.
template <typename Tp>
void test008()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data008)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data008[i].k), Tp(data008[i].nu));
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
testcase_comp_ellint_3<double> data009[] = {
  { 1.5747455615173562, -0.099999999999999978, 0.0000000000000000 },
  { 1.5013711111199950, -0.099999999999999978, 0.10000000000000001 },
  { 1.4373749386463430, -0.099999999999999978, 0.20000000000000001 },
  { 1.3809159606704959, -0.099999999999999978, 0.29999999999999999 },
  { 1.3306223265207477, -0.099999999999999978, 0.40000000000000002 },
  { 1.2854480708580160, -0.099999999999999978, 0.50000000000000000 },
  { 1.2445798942989255, -0.099999999999999978, 0.59999999999999998 },
  { 1.2073745911083187, -0.099999999999999978, 0.69999999999999996 },
  { 1.1733158866987732, -0.099999999999999978, 0.80000000000000004 },
  { 1.1419839485283374, -0.099999999999999978, 0.90000000000000002 },
};

// Test function for k=-0.099999999999999978.
template <typename Tp>
void test009()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data009)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data009[i].k), Tp(data009[i].nu));
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
testcase_comp_ellint_3<double> data010[] = {
  { 1.5707963267948966, 0.0000000000000000, 0.0000000000000000 },
  { 1.4976955329233277, 0.0000000000000000, 0.10000000000000001 },
  { 1.4339343023863691, 0.0000000000000000, 0.20000000000000001 },
  { 1.3776795151134889, 0.0000000000000000, 0.29999999999999999 },
  { 1.3275651989026322, 0.0000000000000000, 0.40000000000000002 },
  { 1.2825498301618641, 0.0000000000000000, 0.50000000000000000 },
  { 1.2418235332245127, 0.0000000000000000, 0.59999999999999998 },
  { 1.2047457872617382, 0.0000000000000000, 0.69999999999999996 },
  { 1.1708024551734544, 0.0000000000000000, 0.80000000000000004 },
  { 1.1395754288497419, 0.0000000000000000, 0.90000000000000002 },
};

// Test function for k=0.0000000000000000.
template <typename Tp>
void test010()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data010)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data010[i].k), Tp(data010[i].nu));
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
testcase_comp_ellint_3<double> data011[] = {
  { 1.5747455615173562, 0.10000000000000009, 0.0000000000000000 },
  { 1.5013711111199950, 0.10000000000000009, 0.10000000000000001 },
  { 1.4373749386463430, 0.10000000000000009, 0.20000000000000001 },
  { 1.3809159606704959, 0.10000000000000009, 0.29999999999999999 },
  { 1.3306223265207477, 0.10000000000000009, 0.40000000000000002 },
  { 1.2854480708580160, 0.10000000000000009, 0.50000000000000000 },
  { 1.2445798942989255, 0.10000000000000009, 0.59999999999999998 },
  { 1.2073745911083187, 0.10000000000000009, 0.69999999999999996 },
  { 1.1733158866987732, 0.10000000000000009, 0.80000000000000004 },
  { 1.1419839485283374, 0.10000000000000009, 0.90000000000000002 },
};

// Test function for k=0.10000000000000009.
template <typename Tp>
void test011()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data011)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data011[i].k), Tp(data011[i].nu));
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
testcase_comp_ellint_3<double> data012[] = {
  { 1.5868678474541664, 0.19999999999999996, 0.0000000000000000 },
  { 1.5126513474261092, 0.19999999999999996, 0.10000000000000001 },
  { 1.4479323932249568, 0.19999999999999996, 0.20000000000000001 },
  { 1.3908453514752481, 0.19999999999999996, 0.29999999999999999 },
  { 1.3400002519661010, 0.19999999999999996, 0.40000000000000002 },
  { 1.2943374404397376, 0.19999999999999996, 0.50000000000000000 },
  { 1.2530330675914561, 0.19999999999999996, 0.59999999999999998 },
  { 1.2154356555075867, 0.19999999999999996, 0.69999999999999996 },
  { 1.1810223448909913, 0.19999999999999996, 0.80000000000000004 },
  { 1.1493679916141863, 0.19999999999999996, 0.90000000000000002 },
};

// Test function for k=0.19999999999999996.
template <typename Tp>
void test012()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data012)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data012[i].k), Tp(data012[i].nu));
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
testcase_comp_ellint_3<double> data013[] = {
  { 1.6080486199305126, 0.30000000000000004, 0.0000000000000000 },
  { 1.5323534693557526, 0.30000000000000004, 0.10000000000000001 },
  { 1.4663658145259875, 0.30000000000000004, 0.20000000000000001 },
  { 1.4081767433479089, 0.30000000000000004, 0.29999999999999999 },
  { 1.3563643538969761, 0.30000000000000004, 0.40000000000000002 },
  { 1.3098448759814960, 0.30000000000000004, 0.50000000000000000 },
  { 1.2677758800420666, 0.30000000000000004, 0.59999999999999998 },
  { 1.2294913236274980, 0.30000000000000004, 0.69999999999999996 },
  { 1.1944567571590046, 0.30000000000000004, 0.80000000000000004 },
  { 1.1622376896064912, 0.30000000000000004, 0.90000000000000002 },
};

// Test function for k=0.30000000000000004.
template <typename Tp>
void test013()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data013)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data013[i].k), Tp(data013[i].nu));
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
testcase_comp_ellint_3<double> data014[] = {
  { 1.6399998658645112, 0.39999999999999991, 0.0000000000000000 },
  { 1.5620566886683604, 0.39999999999999991, 0.10000000000000001 },
  { 1.4941414344266770, 0.39999999999999991, 0.20000000000000001 },
  { 1.4342789859950078, 0.39999999999999991, 0.29999999999999999 },
  { 1.3809986210732901, 0.39999999999999991, 0.40000000000000002 },
  { 1.3331797176377398, 0.39999999999999991, 0.50000000000000000 },
  { 1.2899514672527024, 0.39999999999999991, 0.59999999999999998 },
  { 1.2506255923253344, 0.39999999999999991, 0.69999999999999996 },
  { 1.2146499565727209, 0.39999999999999991, 0.80000000000000004 },
  { 1.1815758115929846, 0.39999999999999991, 0.90000000000000002 },
};

// Test function for k=0.39999999999999991.
template <typename Tp>
void test014()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data014)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data014[i].k), Tp(data014[i].nu));
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
testcase_comp_ellint_3<double> data015[] = {
  { 1.6857503548125963, 0.50000000000000000, 0.0000000000000000 },
  { 1.6045524936084892, 0.50000000000000000, 0.10000000000000001 },
  { 1.5338490483665983, 0.50000000000000000, 0.20000000000000001 },
  { 1.4715681939859637, 0.50000000000000000, 0.29999999999999999 },
  { 1.4161679518465340, 0.50000000000000000, 0.40000000000000002 },
  { 1.3664739530045971, 0.50000000000000000, 0.50000000000000000 },
  { 1.3215740290190876, 0.50000000000000000, 0.59999999999999998 },
  { 1.2807475181182502, 0.50000000000000000, 0.69999999999999996 },
  { 1.2434165408189539, 0.50000000000000000, 0.80000000000000004 },
  { 1.2091116095504744, 0.50000000000000000, 0.90000000000000002 },
};

// Test function for k=0.50000000000000000.
template <typename Tp>
void test015()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data015)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data015[i].k), Tp(data015[i].nu));
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
testcase_comp_ellint_3<double> data016[] = {
  { 1.7507538029157526, 0.60000000000000009, 0.0000000000000000 },
  { 1.6648615773343014, 0.60000000000000009, 0.10000000000000001 },
  { 1.5901418016279374, 0.60000000000000009, 0.20000000000000001 },
  { 1.5243814243493585, 0.60000000000000009, 0.29999999999999999 },
  { 1.4659345278069984, 0.60000000000000009, 0.40000000000000002 },
  { 1.4135484285693078, 0.60000000000000009, 0.50000000000000000 },
  { 1.3662507535812816, 0.60000000000000009, 0.59999999999999998 },
  { 1.3232737468822811, 0.60000000000000009, 0.69999999999999996 },
  { 1.2840021261752192, 0.60000000000000009, 0.80000000000000004 },
  { 1.2479362973851875, 0.60000000000000009, 0.90000000000000002 },
};

// Test function for k=0.60000000000000009.
template <typename Tp>
void test016()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data016)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data016[i].k), Tp(data016[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.69999999999999996.
testcase_comp_ellint_3<double> data017[] = {
  { 1.8456939983747236, 0.69999999999999996, 0.0000000000000000 },
  { 1.7528050171757608, 0.69999999999999996, 0.10000000000000001 },
  { 1.6721098780092147, 0.69999999999999996, 0.20000000000000001 },
  { 1.6011813647733213, 0.69999999999999996, 0.29999999999999999 },
  { 1.5382162002954762, 0.69999999999999996, 0.40000000000000002 },
  { 1.4818433192178544, 0.69999999999999996, 0.50000000000000000 },
  { 1.4309994736080540, 0.69999999999999996, 0.59999999999999998 },
  { 1.3848459188329196, 0.69999999999999996, 0.69999999999999996 },
  { 1.3427110650397533, 0.69999999999999996, 0.80000000000000004 },
  { 1.3040500499695911, 0.69999999999999996, 0.90000000000000002 },
};

// Test function for k=0.69999999999999996.
template <typename Tp>
void test017()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data017)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data017[i].k), Tp(data017[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.80000000000000004.
testcase_comp_ellint_3<double> data018[] = {
  { 1.9953027776647296, 0.80000000000000004, 0.0000000000000000 },
  { 1.8910755418379521, 0.80000000000000004, 0.10000000000000001 },
  { 1.8007226661734588, 0.80000000000000004, 0.20000000000000001 },
  { 1.7214611048717301, 0.80000000000000004, 0.29999999999999999 },
  { 1.6512267838651289, 0.80000000000000004, 0.40000000000000002 },
  { 1.5884528947755532, 0.80000000000000004, 0.50000000000000000 },
  { 1.5319262547427865, 0.80000000000000004, 0.59999999999999998 },
  { 1.4806912324625332, 0.80000000000000004, 0.69999999999999996 },
  { 1.4339837018309474, 0.80000000000000004, 0.80000000000000004 },
  { 1.3911845406776222, 0.80000000000000004, 0.90000000000000002 },
};

// Test function for k=0.80000000000000004.
template <typename Tp>
void test018()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data018)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data018[i].k), Tp(data018[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
}

// Test data for k=0.89999999999999991.
testcase_comp_ellint_3<double> data019[] = {
  { 2.2805491384227699, 0.89999999999999991, 0.0000000000000000 },
  { 2.1537868513875282, 0.89999999999999991, 0.10000000000000001 },
  { 2.0443194576468890, 0.89999999999999991, 0.20000000000000001 },
  { 1.9486280260314424, 0.89999999999999991, 0.29999999999999999 },
  { 1.8641114227238347, 0.89999999999999991, 0.40000000000000002 },
  { 1.7888013241937859, 0.89999999999999991, 0.50000000000000000 },
  { 1.7211781128919521, 0.89999999999999991, 0.59999999999999998 },
  { 1.6600480747670936, 0.89999999999999991, 0.69999999999999996 },
  { 1.6044591960982200, 0.89999999999999991, 0.80000000000000004 },
  { 1.5536420236310944, 0.89999999999999991, 0.90000000000000002 },
};

// Test function for k=0.89999999999999991.
template <typename Tp>
void test019()
{
  const Tp eps = std::numeric_limits<Tp>::epsilon();
  Tp max_abs_diff = -Tp(1);
  Tp max_abs_frac = -Tp(1);
  unsigned int num_datum = sizeof(data019)
                         / sizeof(testcase_comp_ellint_3<double>);
  for (unsigned int i = 0; i < num_datum; ++i)
    {
      const Tp f = std::tr1::comp_ellint_3(Tp(data019[i].k), Tp(data019[i].nu));
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
  VERIFY(max_abs_frac < Tp(2.5000000000000020e-13));
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
