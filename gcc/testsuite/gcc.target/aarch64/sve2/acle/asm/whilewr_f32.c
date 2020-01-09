/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** whilewr_rr_f32:
**	whilewr	p0\.s, x0, x1
**	ret
*/
TEST_COMPARE_S (whilewr_rr_f32, const float32_t *,
		p0 = svwhilewr_f32 (x0, x1),
		p0 = svwhilewr (x0, x1))

/*
** whilewr_0r_f32:
**	whilewr	p0\.s, xzr, x1
**	ret
*/
TEST_COMPARE_S (whilewr_0r_f32, const float32_t *,
		p0 = svwhilewr_f32 ((const float32_t *) 0, x1),
		p0 = svwhilewr ((const float32_t *) 0, x1))

/*
** whilewr_cr_f32:
**	mov	(x[0-9]+), #?1073741824
**	whilewr	p0\.s, \1, x1
**	ret
*/
TEST_COMPARE_S (whilewr_cr_f32, const float32_t *,
		p0 = svwhilewr_f32 ((const float32_t *) 1073741824, x1),
		p0 = svwhilewr ((const float32_t *) 1073741824, x1))

/*
** whilewr_r0_f32:
**	whilewr	p0\.s, x0, xzr
**	ret
*/
TEST_COMPARE_S (whilewr_r0_f32, const float32_t *,
		p0 = svwhilewr_f32 (x0, (const float32_t *) 0),
		p0 = svwhilewr (x0, (const float32_t *) 0))

/*
** whilewr_rc_f32:
**	mov	(x[0-9]+), #?1073741824
**	whilewr	p0\.s, x0, \1
**	ret
*/
TEST_COMPARE_S (whilewr_rc_f32, const float32_t *,
		p0 = svwhilewr_f32 (x0, (const float32_t *) 1073741824),
		p0 = svwhilewr (x0, (const float32_t *) 1073741824))
