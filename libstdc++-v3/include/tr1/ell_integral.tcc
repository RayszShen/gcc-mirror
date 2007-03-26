// Special functions -*- C++ -*-

// Copyright (C) 2006-2007
// Free Software Foundation, Inc.
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
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.
//
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

/** @file tr1/ell_integral.tcc
 *  This is an internal header file, included by other library headers.
 *  You should not attempt to use it directly.
 */

//
// ISO C++ 14882 TR1: 5.2  Special functions
//

// Written by Edward Smith-Rowland based on:
//   (1)  B. C. Carlson Numer. Math. 33, 1 (1979)
//   (2)  B. C. Carlson, Special Functions of Applied Mathematics (1977)
//   (3)  The Gnu Scientific Library, http://www.gnu.org/software/gsl
//   (4)  Numerical Recipes in C, 2nd ed, by W. H. Press, S. A. Teukolsky,
//        W. T. Vetterling, B. P. Flannery, Cambridge University Press
//        (1992), pp. 261-269

#ifndef _TR1_ELL_INTEGRAL_TCC
#define _TR1_ELL_INTEGRAL_TCC 1

namespace std
{
_GLIBCXX_BEGIN_NAMESPACE(_GLIBCXX_TR1)

  // [5.2] Special functions

  /**
   * @ingroup tr1_math_spec_func
   * @{
   */

  //
  // Implementation-space details.
  //
  namespace __detail
  {

    /**
     *   @brief Return the Carlson elliptic function @f$ R_F(x,y,z) @f$
     *          of the first kind.
     * 
     *   The Carlson elliptic function of the first kind is defined by:
     *   @f[
     *       R_F(x,y,z) = \frac{1}{2} \int_0^\infty
     *                 \frac{dt}{(t + x)^{1/2}(t + y)^{1/2}(t + z)^{1/2}}
     *   @f]
     *
     *   @param  __x  The first of three symmetric arguments.
     *   @param  __y  The second of three symmetric arguments.
     *   @param  __z  The third of three symmetric arguments.
     *   @return  The Carlson elliptic function of the first kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_rf(const _Tp __x, const _Tp __y, const _Tp __z)
    {
      const _Tp __min = std::numeric_limits<_Tp>::min();
      const _Tp __max = std::numeric_limits<_Tp>::max();
      const _Tp __lolim = _Tp(5) * __min;
      const _Tp __uplim = __max / _Tp(5);

      if (__x < _Tp(0) || __y < _Tp(0) || __z < _Tp(0))
        std::__throw_domain_error(__N("Argument less than zero "
                                      "in __ellint_rf."));
      else if (__x + __y < __lolim || __x + __z < __lolim
            || __y + __z < __lolim)
        std::__throw_domain_error(__N("Argument too small in __ellint_rf"));
      else
        {
          const _Tp __c0 = _Tp(1) / _Tp(4);
          const _Tp __c1 = _Tp(1) / _Tp(24);
          const _Tp __c2 = _Tp(1) / _Tp(10);
          const _Tp __c3 = _Tp(3) / _Tp(44);
          const _Tp __c4 = _Tp(1) / _Tp(14);

          _Tp __xn = __x;
          _Tp __yn = __y;
          _Tp __zn = __z;

          const _Tp __eps = std::numeric_limits<_Tp>::epsilon();
          const _Tp __errtol = std::pow(__eps, _Tp(1) / _Tp(6));
          _Tp __mu;
          _Tp __xndev, __yndev, __zndev;

          const unsigned int __max_iter = 100;
          for (unsigned int __iter = 0; __iter < __max_iter; ++__iter)
            {
              __mu = (__xn + __yn + __zn) / _Tp(3);
              __xndev = 2 - (__mu + __xn) / __mu;
              __yndev = 2 - (__mu + __yn) / __mu;
              __zndev = 2 - (__mu + __zn) / __mu;
              _Tp __epsilon = std::max(std::abs(__xndev), std::abs(__yndev));
              __epsilon = std::max(__epsilon, std::abs(__zndev));
              if (__epsilon < __errtol)
                break;
              const _Tp __xnroot = std::sqrt(__xn);
              const _Tp __ynroot = std::sqrt(__yn);
              const _Tp __znroot = std::sqrt(__zn);
              const _Tp __lambda = __xnroot * (__ynroot + __znroot)
                                 + __ynroot * __znroot;
              __xn = __c0 * (__xn + __lambda);
              __yn = __c0 * (__yn + __lambda);
              __zn = __c0 * (__zn + __lambda);
            }

          const _Tp __e2 = __xndev * __yndev - __zndev * __zndev;
          const _Tp __e3 = __xndev * __yndev * __zndev;
          const _Tp __s  = _Tp(1) + (__c1 * __e2 - __c2 - __c3 * __e3) * __e2
                   + __c4 * __e3;

          return __s / std::sqrt(__mu);
        }
    }


    /**
     *   @brief Return the complete elliptic integral of the first kind
     *          @f$ K(k) @f$ by series expansion.
     * 
     *   The complete elliptic integral of the first kind is defined as
     *   @f[
     *     K(k) = F(k,\pi/2) = \int_0^{\pi/2}\frac{d\theta}
     *                              {\sqrt{1 - k^2sin^2\theta}}
     *   @f]
     * 
     *   This routine is not bad as long as |k| is somewhat smaller than 1
     *   but is not is good as the Carlson elliptic integral formulation.
     * 
     *   @param  __k  The argument of the complete elliptic function.
     *   @return  The complete elliptic function of the first kind.
     */
    template<typename _Tp>
    _Tp
    __comp_ellint_1_series(const _Tp __k)
    {

      const _Tp __kk = __k * __k;

      _Tp __term = __kk / _Tp(4);
      _Tp __sum = _Tp(1) + __term;

      const unsigned int __max_iter = 1000;
      for (unsigned int __i = 2; __i < __max_iter; ++__i)
        {
          __term *= (2 * __i - 1) * __kk / (2 * __i);
          if (__term < std::numeric_limits<_Tp>::epsilon())
            break;
          __sum += __term;
        }

      return __numeric_constants<_Tp>::__pi_2() * __sum;
    }


    /**
     *   @brief  Return the complete elliptic integral of the first kind
     *           @f$ K(k) @f$ using the Carlson formulation.
     * 
     *   The complete elliptic integral of the first kind is defined as
     *   @f[
     *     K(k) = F(k,\pi/2) = \int_0^{\pi/2}\frac{d\theta}
     *                                           {\sqrt{1 - k^2 sin^2\theta}}
     *   @f]
     *   where @f$ F(k,\phi) @f$ is the incomplete elliptic integral of the
     *   first kind.
     * 
     *   @param  __k  The argument of the complete elliptic function.
     *   @return  The complete elliptic function of the first kind.
     */
    template<typename _Tp>
    _Tp
    __comp_ellint_1(const _Tp __k)
    {

      if (__isnan(__k))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (std::abs(__k) >= _Tp(1))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else
        return __ellint_rf(_Tp(0), _Tp(1) - __k * __k, _Tp(1));
    }


    /**
     *   @brief  Return the incomplete elliptic integral of the first kind
     *           @f$ F(k,\phi) @f$ using the Carlson formulation.
     * 
     *   The incomplete elliptic integral of the first kind is defined as
     *   @f[
     *     F(k,\phi) = \int_0^{\phi}\frac{d\theta}
     *                                   {\sqrt{1 - k^2 sin^2\theta}}
     *   @f]
     * 
     *   @param  __k  The argument of the elliptic function.
     *   @param  __phi  The integral limit argument of the elliptic function.
     *   @return  The elliptic function of the first kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_1(const _Tp __k, const _Tp __phi)
    {

      if (__isnan(__k) || __isnan(__phi))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (std::abs(__k) > _Tp(1))
        std::__throw_domain_error(__N("Bad argument in __ellint_1."));
      else
        {
          //  Reduce phi to -pi/2 < phi < +pi/2.
          const int __n = std::floor(__phi / __numeric_constants<_Tp>::__pi()
                                   + _Tp(0.5L));
          const _Tp __phi_red = __phi
                              - __n * __numeric_constants<_Tp>::__pi();

          const _Tp __s = std::sin(__phi_red);
          const _Tp __c = std::cos(__phi_red);

          const _Tp __F = __s
                        * __ellint_rf(__c * __c,
                                _Tp(1) - __k * __k * __s * __s, _Tp(1));

          if (__n == 0)
            return __F;
          else
            return __F + _Tp(2) * __n * __comp_ellint_1(__k);
        }
    }


    /**
     *   @brief Return the complete elliptic integral of the second kind
     *          @f$ E(k) @f$ by series expansion.
     * 
     *   The complete elliptic integral of the second kind is defined as
     *   @f[
     *     E(k,\pi/2) = \int_0^{\pi/2}\sqrt{1 - k^2 sin^2\theta}
     *   @f]
     * 
     *   This routine is not bad as long as |k| is somewhat smaller than 1
     *   but is not is good as the Carlson elliptic integral formulation.
     * 
     *   @param  __k  The argument of the complete elliptic function.
     *   @return  The complete elliptic function of the second kind.
     */
    template<typename _Tp>
    _Tp
    __comp_ellint_2_series(const _Tp __k)
    {

      const _Tp __kk = __k * __k;

      _Tp __term = __kk;
      _Tp __sum = __term;

      const unsigned int __max_iter = 1000;
      for (unsigned int __i = 2; __i < __max_iter; ++__i)
        {
          const _Tp __i2m = 2 * __i - 1;
          const _Tp __i2 = 2 * __i;
          __term *= __i2m * __i2m * __kk / (__i2 * __i2);
          if (__term < std::numeric_limits<_Tp>::epsilon())
            break;
          __sum += __term / __i2m;
        }

      return __numeric_constants<_Tp>::__pi_2() * (_Tp(1) - __sum);
    }


    /**
     *   @brief  Return the Carlson elliptic function of the second kind
     *           @f$ R_D(x,y,z) = R_J(x,y,z,z) @f$ where
     *           @f$ R_J(x,y,z,p) @f$ is the Carlson elliptic function
     *           of the third kind.
     * 
     *   The Carlson elliptic function of the second kind is defined by:
     *   @f[
     *       R_D(x,y,z) = \frac{3}{2} \int_0^\infty
     *                 \frac{dt}{(t + x)^{1/2}(t + y)^{1/2}(t + z)^{3/2}}
     *   @f]
     *
     *   Based on Carlson's algorithms:
     *   -  B. C. Carlson Numer. Math. 33, 1 (1979)
     *   -  B. C. Carlson, Special Functions of Applied Mathematics (1977)
     *   -  Nunerical Recipes in C, 2nd ed, pp. 261-269,
     *      by Press, Teukolsky, Vetterling, Flannery (1992)
     *
     *   @param  __x  The first of two symmetric arguments.
     *   @param  __y  The second of two symmetric arguments.
     *   @param  __z  The third argument.
     *   @return  The Carlson elliptic function of the second kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_rd(const _Tp __x, const _Tp __y, const _Tp __z)
    {
      const _Tp __eps = std::numeric_limits<_Tp>::epsilon();
      const _Tp __errtol = std::pow(__eps / _Tp(8), _Tp(1) / _Tp(6));
      const _Tp __min = std::numeric_limits<_Tp>::min();
      const _Tp __max = std::numeric_limits<_Tp>::max();
      const _Tp __lolim = _Tp(2) / std::pow(__max, _Tp(2) / _Tp(3));
      const _Tp __uplim = std::pow(_Tp(0.1L) * __errtol / __min, _Tp(2) / _Tp(3));

      if (__x < _Tp(0) || __y < _Tp(0))
        std::__throw_domain_error(__N("Argument less than zero "
                                      "in __ellint_rd."));
      else if (__x + __y < __lolim || __z < __lolim)
        std::__throw_domain_error(__N("Argument too small "
                                      "in __ellint_rd."));
      else
        {
          const _Tp __c0 = _Tp(1) / _Tp(4);
          const _Tp __c1 = _Tp(3) / _Tp(14);
          const _Tp __c2 = _Tp(1) / _Tp(6);
          const _Tp __c3 = _Tp(9) / _Tp(22);
          const _Tp __c4 = _Tp(3) / _Tp(26);

          _Tp __xn = __x;
          _Tp __yn = __y;
          _Tp __zn = __z;
          _Tp __sigma = _Tp(0);
          _Tp __power4 = _Tp(1);

          _Tp __mu;
          _Tp __xndev, __yndev, __zndev;

          const unsigned int __max_iter = 100;
          for (unsigned int __iter = 0; __iter < __max_iter; ++__iter)
            {
              __mu = (__xn + __yn + _Tp(3) * __zn) / _Tp(5);
              __xndev = (__mu - __xn) / __mu;
              __yndev = (__mu - __yn) / __mu;
              __zndev = (__mu - __zn) / __mu;
              _Tp __epsilon = std::max(std::abs(__xndev), std::abs(__yndev));
              __epsilon = std::max(__epsilon, std::abs(__zndev));
              if (__epsilon < __errtol)
                break;
              _Tp __xnroot = std::sqrt(__xn);
              _Tp __ynroot = std::sqrt(__yn);
              _Tp __znroot = std::sqrt(__zn);
              _Tp __lambda = __xnroot * (__ynroot + __znroot)
                           + __ynroot * __znroot;
              __sigma += __power4 / (__znroot * (__zn + __lambda));
              __power4 *= __c0;
              __xn = __c0 * (__xn + __lambda);
              __yn = __c0 * (__yn + __lambda);
              __zn = __c0 * (__zn + __lambda);
            }

          _Tp __ea = __xndev * __yndev;
          _Tp __eb = __zndev * __zndev;
          _Tp __ec = __ea - __eb;
          _Tp __ed = __ea - _Tp(6) * __eb;
          _Tp __ef = __ed + __ec + __ec;
          _Tp __s1 = __ed * (-__c1 + __c3 * __ed
                                   / _Tp(3) - _Tp(3) * __c4 * __zndev * __ef
                                   / _Tp(2));
          _Tp __s2 = __zndev
                   * (__c2 * __ef
                    + __zndev * (-__c3 * __ec - __zndev * __c4 - __ea));

          return _Tp(3) * __sigma + __power4 * (_Tp(1) + __s1 + __s2)
                                        / (__mu * std::sqrt(__mu));
        }
    }


    /**
     *   @brief  Return the complete elliptic integral of the second kind
     *           @f$ E(k) @f$ using the Carlson formulation.
     * 
     *   The complete elliptic integral of the second kind is defined as
     *   @f[
     *     E(k,\pi/2) = \int_0^{\pi/2}\sqrt{1 - k^2 sin^2\theta}
     *   @f]
     * 
     *   @param  __k  The argument of the complete elliptic function.
     *   @return  The complete elliptic function of the second kind.
     */
    template<typename _Tp>
    _Tp
    __comp_ellint_2(const _Tp __k)
    {

      if (__isnan(__k))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (std::abs(__k) == 1)
        return _Tp(1);
      else if (std::abs(__k) > _Tp(1))
        std::__throw_domain_error(__N("Bad argument in __comp_ellint_2."));
      else
        {
          const _Tp __kk = __k * __k;

          return __ellint_rf(_Tp(0), _Tp(1) - __kk, _Tp(1))
               - __kk * __ellint_rd(_Tp(0), _Tp(1) - __kk, _Tp(1)) / _Tp(3);
        }
    }


    /**
     *   @brief  Return the incomplete elliptic integral of the second kind
     *           @f$ E(k,\phi) @f$ using the Carlson formulation.
     * 
     *   The incomplete elliptic integral of the second kind is defined as
     *   @f[
     *     E(k,\phi) = \int_0^{\phi} \sqrt{1 - k^2 sin^2\theta}
     *   @f]
     * 
     *   @param  __k  The argument of the elliptic function.
     *   @param  __phi  The integral limit argument of the elliptic function.
     *   @return  The elliptic function of the second kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_2(const _Tp __k, const _Tp __phi)
    {

      if (__isnan(__k) || __isnan(__phi))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (std::abs(__k) > _Tp(1))
        std::__throw_domain_error(__N("Bad argument in __ellint_2."));
      else
        {
          //  Reduce phi to -pi/2 < phi < +pi/2.
          const int __n = std::floor(__phi / __numeric_constants<_Tp>::__pi()
                                   + _Tp(0.5L));
          const _Tp __phi_red = __phi
                              - __n * __numeric_constants<_Tp>::__pi();

          const _Tp __kk = __k * __k;
          const _Tp __s = std::sin(__phi_red);
          const _Tp __ss = __s * __s;
          const _Tp __sss = __ss * __s;
          const _Tp __c = std::cos(__phi_red);
          const _Tp __cc = __c * __c;

          const _Tp __E = __s
                        * __ellint_rf(__cc, _Tp(1) - __kk * __ss, _Tp(1))
                        - __kk * __sss
                        * __ellint_rd(__cc, _Tp(1) - __kk * __ss, _Tp(1))
                        / _Tp(3);

          if (__n == 0)
            return __E;
          else
            return __E + _Tp(2) * __n * __comp_ellint_2(__k);
        }
    }


    /**
     *   @brief  Return the Carlson elliptic function
     *           @f$ R_C(x,y) = R_F(x,y,y) @f$ where @f$ R_F(x,y,z) @f$
     *           is the Carlson elliptic function of the first kind.
     * 
     *   The Carlson elliptic function is defined by:
     *   @f[
     *       R_C(x,y) = \frac{1}{2} \int_0^\infty
     *                 \frac{dt}{(t + x)^{1/2}(t + y)}
     *   @f]
     *
     *   Based on Carlson's algorithms:
     *   -  B. C. Carlson Numer. Math. 33, 1 (1979)
     *   -  B. C. Carlson, Special Functions of Applied Mathematics (1977)
     *   -  Nunerical Recipes in C, 2nd ed, pp. 261-269,
     *      by Press, Teukolsky, Vetterling, Flannery (1992)
     *
     *   @param  __x  The first argument.
     *   @param  __y  The second argument.
     *   @return  The Carlson elliptic function.
     */
    template<typename _Tp>
    _Tp
    __ellint_rc(const _Tp __x, const _Tp __y)
    {
      const _Tp __min = std::numeric_limits<_Tp>::min();
      const _Tp __max = std::numeric_limits<_Tp>::max();
      const _Tp __lolim = _Tp(5) * __min;
      const _Tp __uplim = __max / _Tp(5);

      if (__x < _Tp(0) || __y < _Tp(0) || __x + __y < __lolim)
        std::__throw_domain_error(__N("Argument less than zero "
                                      "in __ellint_rc."));
      else
        {
          const _Tp __c0 = _Tp(1) / _Tp(4);
          const _Tp __c1 = _Tp(1) / _Tp(7);
          const _Tp __c2 = _Tp(9) / _Tp(22);
          const _Tp __c3 = _Tp(3) / _Tp(10);
          const _Tp __c4 = _Tp(3) / _Tp(8);

          _Tp __xn = __x;
          _Tp __yn = __y;

          const _Tp __eps = std::numeric_limits<_Tp>::epsilon();
          const _Tp __errtol = std::pow(__eps / _Tp(30), _Tp(1) / _Tp(6));
          _Tp __mu;
          _Tp __sn;

          const unsigned int __max_iter = 100;
          for (unsigned int __iter = 0; __iter < __max_iter; ++__iter)
            {
              __mu = (__xn + _Tp(2) * __yn) / _Tp(3);
              __sn = (__yn + __mu) / __mu - _Tp(2);
              if (std::abs(__sn) < __errtol)
                break;
              const _Tp __lambda = _Tp(2) * std::sqrt(__xn) * std::sqrt(__yn)
                             + __yn;
              __xn = __c0 * (__xn + __lambda);
              __yn = __c0 * (__yn + __lambda);
            }

          _Tp __s = __sn * __sn
                  * (__c3 + __sn*(__c1 + __sn * (__c4 + __sn * __c2)));

          return (_Tp(1) + __s) / std::sqrt(__mu);
        }
    }


    /**
     *   @brief  Return the Carlson elliptic function @f$ R_J(x,y,z,p) @f$
     *           of the third kind.
     * 
     *   The Carlson elliptic function of the third kind is defined by:
     *   @f[
     *       R_J(x,y,z,p) = \frac{3}{2} \int_0^\infty
     *       \frac{dt}{(t + x)^{1/2}(t + y)^{1/2}(t + z)^{1/2}(t + p)}
     *   @f]
     *
     *   Based on Carlson's algorithms:
     *   -  B. C. Carlson Numer. Math. 33, 1 (1979)
     *   -  B. C. Carlson, Special Functions of Applied Mathematics (1977)
     *   -  Nunerical Recipes in C, 2nd ed, pp. 261-269,
     *      by Press, Teukolsky, Vetterling, Flannery (1992)
     *
     *   @param  __x  The first of three symmetric arguments.
     *   @param  __y  The second of three symmetric arguments.
     *   @param  __z  The third of three symmetric arguments.
     *   @param  __p  The fourth argument.
     *   @return  The Carlson elliptic function of the fourth kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_rj(const _Tp __x, const _Tp __y, const _Tp __z, const _Tp __p)
    {
      const _Tp __min = std::numeric_limits<_Tp>::min();
      const _Tp __max = std::numeric_limits<_Tp>::max();
      const _Tp __lolim = std::pow(_Tp(5) * __min, _Tp(1)/_Tp(3));
      const _Tp __uplim = _Tp(0.3L)
                        * std::pow(_Tp(0.2L) * __max, _Tp(1)/_Tp(3));

      if (__x < _Tp(0) || __y < _Tp(0) || __z < _Tp(0))
        std::__throw_domain_error(__N("Argument less than zero "
                                      "in __ellint_rj."));
      else if (__x + __y < __lolim || __x + __z < __lolim
            || __y + __z < __lolim || __p < __lolim)
        std::__throw_domain_error(__N("Argument too small "
                                      "in __ellint_rj"));
      else
        {
          const _Tp __c0 = _Tp(1) / _Tp(4);
          const _Tp __c1 = _Tp(3) / _Tp(14);
          const _Tp __c2 = _Tp(1) / _Tp(3);
          const _Tp __c3 = _Tp(3) / _Tp(22);
          const _Tp __c4 = _Tp(3) / _Tp(26);

          _Tp __xn = __x;
          _Tp __yn = __y;
          _Tp __zn = __z;
          _Tp __pn = __p;
          _Tp __sigma = _Tp(0);
          _Tp __power4 = _Tp(1);

          const _Tp __eps = std::numeric_limits<_Tp>::epsilon();
          const _Tp __errtol = std::pow(__eps / _Tp(8), _Tp(1) / _Tp(6));

          _Tp __lambda, __mu;
          _Tp __xndev, __yndev, __zndev, __pndev;

          const unsigned int __max_iter = 100;
          for (unsigned int __iter = 0; __iter < __max_iter; ++__iter)
            {
              __mu = (__xn + __yn + __zn + _Tp(2) * __pn) / _Tp(5);
              __xndev = (__mu - __xn) / __mu;
              __yndev = (__mu - __yn) / __mu;
              __zndev = (__mu - __zn) / __mu;
              __pndev = (__mu - __pn) / __mu;
              _Tp __epsilon = std::max(std::abs(__xndev), std::abs(__yndev));
              __epsilon = std::max(__epsilon, std::abs(__zndev));
              __epsilon = std::max(__epsilon, std::abs(__pndev));
              if (__epsilon < __errtol)
                break;
              const _Tp __xnroot = std::sqrt(__xn);
              const _Tp __ynroot = std::sqrt(__yn);
              const _Tp __znroot = std::sqrt(__zn);
              const _Tp __lambda = __xnroot * (__ynroot + __znroot)
                                 + __ynroot * __znroot;
              const _Tp __alpha = __pn * (__xnroot + __ynroot + __znroot)
                                + __xnroot * __ynroot * __znroot;
              const _Tp __alpha2 = __alpha * __alpha;
              const _Tp __beta = __pn * (__pn + __lambda)
                                      * (__pn + __lambda);
              __sigma += __power4 * __ellint_rc(__alpha2, __beta);
              __power4 *= __c0;
              __xn = __c0 * (__xn + __lambda);
              __yn = __c0 * (__yn + __lambda);
              __zn = __c0 * (__zn + __lambda);
              __pn = __c0 * (__pn + __lambda);
            }

          _Tp __ea = __xndev * (__yndev + __zndev) + __yndev * __zndev;
          _Tp __eb = __xndev * __yndev * __zndev;
          _Tp __ec = __pndev * __pndev;
          _Tp __e2 = __ea - _Tp(3) * __ec;
          _Tp __e3 = __eb + _Tp(2) * __pndev * (__ea - __ec);
          _Tp __s1 = _Tp(1) + __e2 * (-__c1 + _Tp(3) * __c3 * __e2 / _Tp(4)
                            - _Tp(3) * __c4 * __e3 / _Tp(2));
          _Tp __s2 = __eb * (__c2 / _Tp(2)
                   + __pndev * (-__c3 - __c3 + __pndev * __c4));
          _Tp __s3 = __pndev * __ea * (__c2 - __pndev * __c3)
                   - __c2 * __pndev * __ec;

          return _Tp(3) * __sigma + __power4 * (__s1 + __s2 + __s3)
                                             / (__mu * std::sqrt(__mu));
        }
    }


    /**
     *   @brief Return the complete elliptic integral of the third kind
     *          @f$ \Pi(k,\nu) = \Pi(k,\nu,\pi/2) @f$ using the
     *          Carlson formulation.
     * 
     *   The complete elliptic integral of the third kind is defined as
     *   @f[
     *     \Pi(k,\nu) = \int_0^{\pi/2}
     *                   \frac{d\theta}
     *                 {(1 - \nu \sin^2\theta)\sqrt{1 - k^2 \sin^2\theta}}
     *   @f]
     * 
     *   @param  __k  The argument of the elliptic function.
     *   @param  __nu  The second argument of the elliptic function.
     *   @return  The complete elliptic function of the third kind.
     */
    template<typename _Tp>
    _Tp
    __comp_ellint_3(const _Tp __k, const _Tp __nu)
    {

      if (__isnan(__k) || __isnan(__nu))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (__nu == _Tp(1))
        return std::numeric_limits<_Tp>::infinity();
      else if (std::abs(__k) > _Tp(1))
        std::__throw_domain_error(__N("Bad argument in __comp_ellint_3."));
      else
        {
          const _Tp __kk = __k * __k;

          return __ellint_rf(_Tp(0), _Tp(1) - __kk, _Tp(1))
               - __nu
               * __ellint_rj(_Tp(0), _Tp(1) - __kk, _Tp(1), _Tp(1) + __nu)
               / _Tp(3);
        }
    }


    /**
     *   @brief Return the incomplete elliptic integral of the third kind
     *          @f$ \Pi(k,\nu,\phi) @f$ using the Carlson formulation.
     * 
     *   The incomplete elliptic integral of the third kind is defined as
     *   @f[
     *     \Pi(k,\nu,\phi) = \int_0^{\phi}
     *                       \frac{d\theta}
     *                            {(1 - \nu \sin^2\theta)
     *                             \sqrt{1 - k^2 \sin^2\theta}}
     *   @f]
     * 
     *   @param  __k  The argument of the elliptic function.
     *   @param  __nu  The second argument of the elliptic function.
     *   @param  __phi  The integral limit argument of the elliptic function.
     *   @return  The elliptic function of the third kind.
     */
    template<typename _Tp>
    _Tp
    __ellint_3(const _Tp __k, const _Tp __nu, const _Tp __phi)
    {

      if (__isnan(__k) || __isnan(__nu) || __isnan(__phi))
        return std::numeric_limits<_Tp>::quiet_NaN();
      else if (std::abs(__k) > _Tp(1))
        std::__throw_domain_error(__N("Bad argument in __ellint_3."));
      else
        {
          //  Reduce phi to -pi/2 < phi < +pi/2.
          const int __n = std::floor(__phi / __numeric_constants<_Tp>::__pi()
                                   + _Tp(0.5L));
          const _Tp __phi_red = __phi
                              - __n * __numeric_constants<_Tp>::__pi();

          const _Tp __kk = __k * __k;
          const _Tp __s = std::sin(__phi_red);
          const _Tp __ss = __s * __s;
          const _Tp __sss = __ss * __s;
          const _Tp __c = std::cos(__phi_red);
          const _Tp __cc = __c * __c;

          const _Tp __Pi = __s
                         * __ellint_rf(__cc, _Tp(1) - __kk * __ss, _Tp(1))
                         - __nu * __sss
                         * __ellint_rj(__cc, _Tp(1) - __kk * __ss, _Tp(1),
                                       _Tp(1) + __nu * __ss) / _Tp(3);

          if (__n == 0)
            return __Pi;
          else
            return __Pi + _Tp(2) * __n * __comp_ellint_3(__k, __nu);
        }
    }

  } // namespace std::tr1::__detail

  /* @} */ // group tr1_math_spec_func

_GLIBCXX_END_NAMESPACE
}

#endif // _TR1_ELL_INTEGRAL_TCC

