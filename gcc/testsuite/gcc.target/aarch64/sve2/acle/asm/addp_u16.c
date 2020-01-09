/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** addp_u16_m_tied1:
**	addp	z0\.h, p0/m, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_m_tied1, svuint16_t,
		z0 = svaddp_u16_m (p0, z0, z1),
		z0 = svaddp_m (p0, z0, z1))

/*
** addp_u16_m_tied2:
**	mov	(z[0-9]+)\.d, z0\.d
**	movprfx	z0, z1
**	addp	z0\.h, p0/m, z0\.h, \1\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_m_tied2, svuint16_t,
		z0 = svaddp_u16_m (p0, z1, z0),
		z0 = svaddp_m (p0, z1, z0))

/*
** addp_u16_m_untied:
**	movprfx	z0, z1
**	addp	z0\.h, p0/m, z0\.h, z2\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_m_untied, svuint16_t,
		z0 = svaddp_u16_m (p0, z1, z2),
		z0 = svaddp_m (p0, z1, z2))

/*
** addp_u16_x_tied1:
**	addp	z0\.h, p0/m, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_x_tied1, svuint16_t,
		z0 = svaddp_u16_x (p0, z0, z1),
		z0 = svaddp_x (p0, z0, z1))

/*
** addp_u16_x_tied2:
**	mov	(z[0-9]+)\.d, z0\.d
**	movprfx	z0, z1
**	addp	z0\.h, p0/m, z0\.h, \1\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_x_tied2, svuint16_t,
		z0 = svaddp_u16_x (p0, z1, z0),
		z0 = svaddp_x (p0, z1, z0))

/*
** addp_u16_x_untied:
**	movprfx	z0, z1
**	addp	z0\.h, p0/m, z0\.h, z2\.h
**	ret
*/
TEST_UNIFORM_Z (addp_u16_x_untied, svuint16_t,
		z0 = svaddp_u16_x (p0, z1, z2),
		z0 = svaddp_x (p0, z1, z2))
