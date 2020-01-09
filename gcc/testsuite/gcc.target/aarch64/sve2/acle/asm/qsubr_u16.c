/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** qsubr_u16_m_tied1:
**	uqsubr	z0\.h, p0/m, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_m_tied1, svuint16_t,
		z0 = svqsubr_u16_m (p0, z0, z1),
		z0 = svqsubr_m (p0, z0, z1))

/*
** qsubr_u16_m_tied2:
**	mov	(z[0-9]+)\.d, z0\.d
**	movprfx	z0, z1
**	uqsubr	z0\.h, p0/m, z0\.h, \1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_m_tied2, svuint16_t,
		z0 = svqsubr_u16_m (p0, z1, z0),
		z0 = svqsubr_m (p0, z1, z0))

/*
** qsubr_u16_m_untied:
**	movprfx	z0, z1
**	uqsubr	z0\.h, p0/m, z0\.h, z2\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_m_untied, svuint16_t,
		z0 = svqsubr_u16_m (p0, z1, z2),
		z0 = svqsubr_m (p0, z1, z2))

/*
** qsubr_w0_u16_m_tied1:
**	mov	(z[0-9]+\.h), w0
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_m_tied1, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_m (p0, z0, x0),
		 z0 = svqsubr_m (p0, z0, x0))

/*
** qsubr_w0_u16_m_untied: { xfail *-*-* }
**	mov	(z[0-9]+\.h), w0
**	movprfx	z0, z1
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_m_untied, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_m (p0, z1, x0),
		 z0 = svqsubr_m (p0, z1, x0))

/*
** qsubr_1_u16_m_tied1:
**	mov	(z[0-9]+\.h), #1
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_m_tied1, svuint16_t,
		z0 = svqsubr_n_u16_m (p0, z0, 1),
		z0 = svqsubr_m (p0, z0, 1))

/*
** qsubr_1_u16_m_untied: { xfail *-*-* }
**	mov	(z[0-9]+\.h), #1
**	movprfx	z0, z1
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_m_untied, svuint16_t,
		z0 = svqsubr_n_u16_m (p0, z1, 1),
		z0 = svqsubr_m (p0, z1, 1))

/*
** qsubr_m2_u16_m:
**	mov	(z[0-9]+\.h), #-2
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_Z (qsubr_m2_u16_m, svuint16_t,
		z0 = svqsubr_n_u16_m (p0, z0, -2),
		z0 = svqsubr_m (p0, z0, -2))

/*
** qsubr_u16_z_tied1:
**	movprfx	z0\.h, p0/z, z0\.h
**	uqsubr	z0\.h, p0/m, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_z_tied1, svuint16_t,
		z0 = svqsubr_u16_z (p0, z0, z1),
		z0 = svqsubr_z (p0, z0, z1))

/*
** qsubr_u16_z_tied2:
**	movprfx	z0\.h, p0/z, z0\.h
**	uqsub	z0\.h, p0/m, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_z_tied2, svuint16_t,
		z0 = svqsubr_u16_z (p0, z1, z0),
		z0 = svqsubr_z (p0, z1, z0))

/*
** qsubr_u16_z_untied:
** (
**	movprfx	z0\.h, p0/z, z1\.h
**	uqsubr	z0\.h, p0/m, z0\.h, z2\.h
** |
**	movprfx	z0\.h, p0/z, z2\.h
**	uqsub	z0\.h, p0/m, z0\.h, z1\.h
** )
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_z_untied, svuint16_t,
		z0 = svqsubr_u16_z (p0, z1, z2),
		z0 = svqsubr_z (p0, z1, z2))

/*
** qsubr_w0_u16_z_tied1:
**	mov	(z[0-9]+\.h), w0
**	movprfx	z0\.h, p0/z, z0\.h
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_z_tied1, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_z (p0, z0, x0),
		 z0 = svqsubr_z (p0, z0, x0))

/*
** qsubr_w0_u16_z_untied:
**	mov	(z[0-9]+\.h), w0
** (
**	movprfx	z0\.h, p0/z, z1\.h
**	uqsubr	z0\.h, p0/m, z0\.h, \1
** |
**	movprfx	z0\.h, p0/z, \1
**	uqsub	z0\.h, p0/m, z0\.h, z1\.h
** )
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_z_untied, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_z (p0, z1, x0),
		 z0 = svqsubr_z (p0, z1, x0))

/*
** qsubr_1_u16_z_tied1:
**	mov	(z[0-9]+\.h), #1
**	movprfx	z0\.h, p0/z, z0\.h
**	uqsubr	z0\.h, p0/m, z0\.h, \1
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_z_tied1, svuint16_t,
		z0 = svqsubr_n_u16_z (p0, z0, 1),
		z0 = svqsubr_z (p0, z0, 1))

/*
** qsubr_1_u16_z_untied:
**	mov	(z[0-9]+\.h), #1
** (
**	movprfx	z0\.h, p0/z, z1\.h
**	uqsubr	z0\.h, p0/m, z0\.h, \1
** |
**	movprfx	z0\.h, p0/z, \1
**	uqsub	z0\.h, p0/m, z0\.h, z1\.h
** )
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_z_untied, svuint16_t,
		z0 = svqsubr_n_u16_z (p0, z1, 1),
		z0 = svqsubr_z (p0, z1, 1))

/*
** qsubr_u16_x_tied1:
**	uqsub	z0\.h, z1\.h, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_x_tied1, svuint16_t,
		z0 = svqsubr_u16_x (p0, z0, z1),
		z0 = svqsubr_x (p0, z0, z1))

/*
** qsubr_u16_x_tied2:
**	uqsub	z0\.h, z0\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_x_tied2, svuint16_t,
		z0 = svqsubr_u16_x (p0, z1, z0),
		z0 = svqsubr_x (p0, z1, z0))

/*
** qsubr_u16_x_untied:
**	uqsub	z0\.h, z2\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_u16_x_untied, svuint16_t,
		z0 = svqsubr_u16_x (p0, z1, z2),
		z0 = svqsubr_x (p0, z1, z2))

/*
** qsubr_w0_u16_x_tied1:
**	mov	(z[0-9]+\.h), w0
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_x_tied1, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_x (p0, z0, x0),
		 z0 = svqsubr_x (p0, z0, x0))

/*
** qsubr_w0_u16_x_untied:
**	mov	(z[0-9]+\.h), w0
**	uqsub	z0\.h, \1, z1\.h
**	ret
*/
TEST_UNIFORM_ZX (qsubr_w0_u16_x_untied, svuint16_t, uint16_t,
		 z0 = svqsubr_n_u16_x (p0, z1, x0),
		 z0 = svqsubr_x (p0, z1, x0))

/*
** qsubr_1_u16_x_tied1:
**	mov	(z[0-9]+\.h), #1
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_x_tied1, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 1),
		z0 = svqsubr_x (p0, z0, 1))

/*
** qsubr_1_u16_x_untied:
**	mov	(z[0-9]+\.h), #1
**	uqsub	z0\.h, \1, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_1_u16_x_untied, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z1, 1),
		z0 = svqsubr_x (p0, z1, 1))

/*
** qsubr_127_u16_x:
**	mov	(z[0-9]+\.h), #127
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_127_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 127),
		z0 = svqsubr_x (p0, z0, 127))

/*
** qsubr_128_u16_x:
**	mov	(z[0-9]+\.h), #128
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_128_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 128),
		z0 = svqsubr_x (p0, z0, 128))

/*
** qsubr_255_u16_x:
**	mov	(z[0-9]+\.h), #255
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_255_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 255),
		z0 = svqsubr_x (p0, z0, 255))

/*
** qsubr_256_u16_x:
**	mov	(z[0-9]+\.h), #256
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_256_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 256),
		z0 = svqsubr_x (p0, z0, 256))

/*
** qsubr_257_u16_x:
**	mov	(z[0-9]+)\.b, #1
**	uqsub	z0\.h, \1\.h, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_257_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 257),
		z0 = svqsubr_x (p0, z0, 257))

/*
** qsubr_512_u16_x:
**	mov	(z[0-9]+\.h), #512
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_512_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 512),
		z0 = svqsubr_x (p0, z0, 512))

/*
** qsubr_65280_u16_x:
**	mov	(z[0-9]+\.h), #-256
**	uqsub	z0\.h, \1, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_65280_u16_x, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, 0xff00),
		z0 = svqsubr_x (p0, z0, 0xff00))

/*
** qsubr_m1_u16_x_tied1:
**	mov	(z[0-9]+)\.b, #-1
**	uqsub	z0\.h, \1\.h, z0\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_m1_u16_x_tied1, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z0, -1),
		z0 = svqsubr_x (p0, z0, -1))

/*
** qsubr_m1_u16_x_untied:
**	mov	(z[0-9]+)\.b, #-1
**	uqsub	z0\.h, \1\.h, z1\.h
**	ret
*/
TEST_UNIFORM_Z (qsubr_m1_u16_x_untied, svuint16_t,
		z0 = svqsubr_n_u16_x (p0, z1, -1),
		z0 = svqsubr_x (p0, z1, -1))
