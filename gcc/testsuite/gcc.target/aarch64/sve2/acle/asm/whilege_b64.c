/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** whilege_rr_b64_s32:
**	whilege	p0\.d, w0, w1
**	ret
*/
TEST_COMPARE_S (whilege_rr_b64_s32, int32_t,
		p0 = svwhilege_b64_s32 (x0, x1),
		p0 = svwhilege_b64 (x0, x1))

/*
** whilege_0r_b64_s32:
**	whilege	p0\.d, wzr, w1
**	ret
*/
TEST_COMPARE_S (whilege_0r_b64_s32, int32_t,
		p0 = svwhilege_b64_s32 (0, x1),
		p0 = svwhilege_b64 (0, x1))

/*
** whilege_5r_b64_s32:
**	mov	(w[0-9]+), #?5
**	whilege	p0\.d, \1, w1
**	ret
*/
TEST_COMPARE_S (whilege_5r_b64_s32, int32_t,
		p0 = svwhilege_b64_s32 (5, x1),
		p0 = svwhilege_b64 (5, x1))

/*
** whilege_r0_b64_s32:
**	whilege	p0\.d, w0, wzr
**	ret
*/
TEST_COMPARE_S (whilege_r0_b64_s32, int32_t,
		p0 = svwhilege_b64_s32 (x0, 0),
		p0 = svwhilege_b64 (x0, 0))

/*
** whilege_r5_b64_s32:
**	mov	(w[0-9]+), #?5
**	whilege	p0\.d, w0, \1
**	ret
*/
TEST_COMPARE_S (whilege_r5_b64_s32, int32_t,
		p0 = svwhilege_b64_s32 (x0, 5),
		p0 = svwhilege_b64 (x0, 5))

/*
** whilege_rr_b64_s64:
**	whilege	p0\.d, x0, x1
**	ret
*/
TEST_COMPARE_S (whilege_rr_b64_s64, int64_t,
		p0 = svwhilege_b64_s64 (x0, x1),
		p0 = svwhilege_b64 (x0, x1))

/*
** whilege_0r_b64_s64:
**	whilege	p0\.d, xzr, x1
**	ret
*/
TEST_COMPARE_S (whilege_0r_b64_s64, int64_t,
		p0 = svwhilege_b64_s64 (0, x1),
		p0 = svwhilege_b64 ((int64_t) 0, x1))

/*
** whilege_5r_b64_s64:
**	mov	(x[0-9]+), #?5
**	whilege	p0\.d, \1, x1
**	ret
*/
TEST_COMPARE_S (whilege_5r_b64_s64, int64_t,
		p0 = svwhilege_b64_s64 (5, x1),
		p0 = svwhilege_b64 ((int64_t) 5, x1))

/*
** whilege_r0_b64_s64:
**	whilege	p0\.d, x0, xzr
**	ret
*/
TEST_COMPARE_S (whilege_r0_b64_s64, int64_t,
		p0 = svwhilege_b64_s64 (x0, 0),
		p0 = svwhilege_b64 (x0, (int64_t) 0))

/*
** whilege_r5_b64_s64:
**	mov	(x[0-9]+), #?5
**	whilege	p0\.d, x0, \1
**	ret
*/
TEST_COMPARE_S (whilege_r5_b64_s64, int64_t,
		p0 = svwhilege_b64_s64 (x0, 5),
		p0 = svwhilege_b64 (x0, (int64_t) 5))

/*
** whilege_rr_b64_u32:
**	whilehs	p0\.d, w0, w1
**	ret
*/
TEST_COMPARE_S (whilege_rr_b64_u32, uint32_t,
		p0 = svwhilege_b64_u32 (x0, x1),
		p0 = svwhilege_b64 (x0, x1))

/*
** whilege_0r_b64_u32:
**	whilehs	p0\.d, wzr, w1
**	ret
*/
TEST_COMPARE_S (whilege_0r_b64_u32, uint32_t,
		p0 = svwhilege_b64_u32 (0, x1),
		p0 = svwhilege_b64 ((uint32_t) 0, x1))

/*
** whilege_5r_b64_u32:
**	mov	(w[0-9]+), #?5
**	whilehs	p0\.d, \1, w1
**	ret
*/
TEST_COMPARE_S (whilege_5r_b64_u32, uint32_t,
		p0 = svwhilege_b64_u32 (5, x1),
		p0 = svwhilege_b64 ((uint32_t) 5, x1))

/*
** whilege_r5_b64_u32:
**	mov	(w[0-9]+), #?5
**	whilehs	p0\.d, w0, \1
**	ret
*/
TEST_COMPARE_S (whilege_r5_b64_u32, uint32_t,
		p0 = svwhilege_b64_u32 (x0, 5),
		p0 = svwhilege_b64 (x0, (uint32_t) 5))

/*
** whilege_rr_b64_u64:
**	whilehs	p0\.d, x0, x1
**	ret
*/
TEST_COMPARE_S (whilege_rr_b64_u64, uint64_t,
		p0 = svwhilege_b64_u64 (x0, x1),
		p0 = svwhilege_b64 (x0, x1))

/*
** whilege_0r_b64_u64:
**	whilehs	p0\.d, xzr, x1
**	ret
*/
TEST_COMPARE_S (whilege_0r_b64_u64, uint64_t,
		p0 = svwhilege_b64_u64 (0, x1),
		p0 = svwhilege_b64 ((uint64_t) 0, x1))

/*
** whilege_5r_b64_u64:
**	mov	(x[0-9]+), #?5
**	whilehs	p0\.d, \1, x1
**	ret
*/
TEST_COMPARE_S (whilege_5r_b64_u64, uint64_t,
		p0 = svwhilege_b64_u64 (5, x1),
		p0 = svwhilege_b64 ((uint64_t) 5, x1))

/*
** whilege_r5_b64_u64:
**	mov	(x[0-9]+), #?5
**	whilehs	p0\.d, x0, \1
**	ret
*/
TEST_COMPARE_S (whilege_r5_b64_u64, uint64_t,
		p0 = svwhilege_b64_u64 (x0, 5),
		p0 = svwhilege_b64 (x0, (uint64_t) 5))
