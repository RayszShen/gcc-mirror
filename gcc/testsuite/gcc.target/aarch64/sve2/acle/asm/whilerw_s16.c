/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** whilerw_rr_s16:
**	whilerw	p0\.h, x0, x1
**	ret
*/
TEST_COMPARE_S (whilerw_rr_s16, const int16_t *,
		p0 = svwhilerw_s16 (x0, x1),
		p0 = svwhilerw (x0, x1))

/*
** whilerw_0r_s16:
**	whilerw	p0\.h, xzr, x1
**	ret
*/
TEST_COMPARE_S (whilerw_0r_s16, const int16_t *,
		p0 = svwhilerw_s16 ((const int16_t *) 0, x1),
		p0 = svwhilerw ((const int16_t *) 0, x1))

/*
** whilerw_cr_s16:
**	mov	(x[0-9]+), #?1073741824
**	whilerw	p0\.h, \1, x1
**	ret
*/
TEST_COMPARE_S (whilerw_cr_s16, const int16_t *,
		p0 = svwhilerw_s16 ((const int16_t *) 1073741824, x1),
		p0 = svwhilerw ((const int16_t *) 1073741824, x1))

/*
** whilerw_r0_s16:
**	whilerw	p0\.h, x0, xzr
**	ret
*/
TEST_COMPARE_S (whilerw_r0_s16, const int16_t *,
		p0 = svwhilerw_s16 (x0, (const int16_t *) 0),
		p0 = svwhilerw (x0, (const int16_t *) 0))

/*
** whilerw_rc_s16:
**	mov	(x[0-9]+), #?1073741824
**	whilerw	p0\.h, x0, \1
**	ret
*/
TEST_COMPARE_S (whilerw_rc_s16, const int16_t *,
		p0 = svwhilerw_s16 (x0, (const int16_t *) 1073741824),
		p0 = svwhilerw (x0, (const int16_t *) 1073741824))
