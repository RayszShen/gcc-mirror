/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** minp_s64_m_tied1:
**	sminp	z0\.d, p0/m, z0\.d, z1\.d
**	ret
*/
TEST_UNIFORM_Z (minp_s64_m_tied1, svint64_t,
		z0 = svminp_s64_m (p0, z0, z1),
		z0 = svminp_m (p0, z0, z1))

/*
** minp_s64_m_tied2:
**	mov	(z[0-9]+\.d), z0\.d
**	movprfx	z0, z1
**	sminp	z0\.d, p0/m, z0\.d, \1
**	ret
*/
TEST_UNIFORM_Z (minp_s64_m_tied2, svint64_t,
		z0 = svminp_s64_m (p0, z1, z0),
		z0 = svminp_m (p0, z1, z0))

/*
** minp_s64_m_untied:
**	movprfx	z0, z1
**	sminp	z0\.d, p0/m, z0\.d, z2\.d
**	ret
*/
TEST_UNIFORM_Z (minp_s64_m_untied, svint64_t,
		z0 = svminp_s64_m (p0, z1, z2),
		z0 = svminp_m (p0, z1, z2))

/*
** minp_s64_x_tied1:
**	sminp	z0\.d, p0/m, z0\.d, z1\.d
**	ret
*/
TEST_UNIFORM_Z (minp_s64_x_tied1, svint64_t,
		z0 = svminp_s64_x (p0, z0, z1),
		z0 = svminp_x (p0, z0, z1))

/*
** minp_s64_x_tied2:
**	mov	(z[0-9]+\.d), z0\.d
**	movprfx	z0, z1
**	sminp	z0\.d, p0/m, z0\.d, \1
**	ret
*/
TEST_UNIFORM_Z (minp_s64_x_tied2, svint64_t,
		z0 = svminp_s64_x (p0, z1, z0),
		z0 = svminp_x (p0, z1, z0))

/*
** minp_s64_x_untied:
**	movprfx	z0, z1
**	sminp	z0\.d, p0/m, z0\.d, z2\.d
**	ret
*/
TEST_UNIFORM_Z (minp_s64_x_untied, svint64_t,
		z0 = svminp_s64_x (p0, z1, z2),
		z0 = svminp_x (p0, z1, z2))
