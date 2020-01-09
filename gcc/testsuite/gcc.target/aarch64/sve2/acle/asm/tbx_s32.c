/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** tbx_s32_tied1:
**	tbx	z0\.s, z1\.s, z4\.s
**	ret
*/
TEST_DUAL_Z (tbx_s32_tied1, svint32_t, svuint32_t,
	     z0 = svtbx_s32 (z0, z1, z4),
	     z0 = svtbx (z0, z1, z4))

/* Bad RA choice: no preferred output sequence.  */
TEST_DUAL_Z (tbx_s32_tied2, svint32_t, svuint32_t,
	     z0 = svtbx_s32 (z1, z0, z4),
	     z0 = svtbx (z1, z0, z4))

/* Bad RA choice: no preferred output sequence.  */
TEST_DUAL_Z_REV (tbx_s32_tied3, svint32_t, svuint32_t,
		 z0_res = svtbx_s32 (z4, z5, z0),
		 z0_res = svtbx (z4, z5, z0))

/*
** tbx_s32_untied:
** (
**	mov	z0\.d, z1\.d
**	tbx	z0\.s, z2\.s, z4\.s
** |
**	tbx	z1\.s, z2\.s, z4\.s
**	mov	z0\.d, z1\.d
** )
**	ret
*/
TEST_DUAL_Z (tbx_s32_untied, svint32_t, svuint32_t,
	     z0 = svtbx_s32 (z1, z2, z4),
	     z0 = svtbx (z1, z2, z4))
