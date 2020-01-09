/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** sri_1_u16_tied1:
**	sri	z0\.h, z1\.h, #1
**	ret
*/
TEST_UNIFORM_Z (sri_1_u16_tied1, svuint16_t,
		z0 = svsri_n_u16 (z0, z1, 1),
		z0 = svsri (z0, z1, 1))

/* Bad RA choice: no preferred output sequence.  */
TEST_UNIFORM_Z (sri_1_u16_tied2, svuint16_t,
		z0 = svsri_n_u16 (z1, z0, 1),
		z0 = svsri (z1, z0, 1))

/*
** sri_1_u16_untied:
**	mov	z0\.d, z1\.d
**	sri	z0\.h, z2\.h, #1
**	ret
*/
TEST_UNIFORM_Z (sri_1_u16_untied, svuint16_t,
		z0 = svsri_n_u16 (z1, z2, 1),
		z0 = svsri (z1, z2, 1))

/*
** sri_2_u16_tied1:
**	sri	z0\.h, z1\.h, #2
**	ret
*/
TEST_UNIFORM_Z (sri_2_u16_tied1, svuint16_t,
		z0 = svsri_n_u16 (z0, z1, 2),
		z0 = svsri (z0, z1, 2))

/* Bad RA choice: no preferred output sequence.  */
TEST_UNIFORM_Z (sri_2_u16_tied2, svuint16_t,
		z0 = svsri_n_u16 (z1, z0, 2),
		z0 = svsri (z1, z0, 2))

/*
** sri_2_u16_untied:
**	mov	z0\.d, z1\.d
**	sri	z0\.h, z2\.h, #2
**	ret
*/
TEST_UNIFORM_Z (sri_2_u16_untied, svuint16_t,
		z0 = svsri_n_u16 (z1, z2, 2),
		z0 = svsri (z1, z2, 2))

/*
** sri_16_u16_tied1:
**	sri	z0\.h, z1\.h, #16
**	ret
*/
TEST_UNIFORM_Z (sri_16_u16_tied1, svuint16_t,
		z0 = svsri_n_u16 (z0, z1, 16),
		z0 = svsri (z0, z1, 16))

/* Bad RA choice: no preferred output sequence.  */
TEST_UNIFORM_Z (sri_16_u16_tied2, svuint16_t,
		z0 = svsri_n_u16 (z1, z0, 16),
		z0 = svsri (z1, z0, 16))

/*
** sri_16_u16_untied:
**	mov	z0\.d, z1\.d
**	sri	z0\.h, z2\.h, #16
**	ret
*/
TEST_UNIFORM_Z (sri_16_u16_untied, svuint16_t,
		z0 = svsri_n_u16 (z1, z2, 16),
		z0 = svsri (z1, z2, 16))
