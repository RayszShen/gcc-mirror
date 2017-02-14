#define INSN_NAME vcle
#define TEST_MSG "VCLE/VCLEQ"

#include "cmp_op.inc"

/* Expected results.  */
VECT_VAR_DECL(expected,uint,8,8) [] = { 0xff, 0xff, 0xff, 0xff,
					0xff, 0xff, 0xff, 0x0 };
VECT_VAR_DECL(expected,uint,16,4) [] = { 0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected,uint,32,2) [] = { 0xffffffff, 0x0 };
VECT_VAR_DECL(expected,uint,8,16) [] = { 0xff, 0xff, 0xff, 0xff,
					 0xff, 0xff, 0xff, 0xff,
					 0xff, 0xff, 0xff, 0xff,
					 0xff, 0x0, 0x0, 0x0 };
VECT_VAR_DECL(expected,uint,16,8) [] = { 0xffff, 0xffff, 0xffff, 0xffff,
					 0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected,uint,32,4) [] = { 0xffffffff, 0xffffffff,
					 0xffffffff, 0x0 };

VECT_VAR_DECL(expected_uint,uint,8,8) [] = { 0xff, 0xff, 0xff, 0xff,
					     0x0, 0x0, 0x0, 0x0 };
VECT_VAR_DECL(expected_uint,uint,16,4) [] = { 0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected_uint,uint,32,2) [] = { 0xffffffff, 0xffffffff };

VECT_VAR_DECL(expected_q_uint,uint,8,16) [] = { 0xff, 0xff, 0xff, 0xff,
						0xff, 0x0, 0x0, 0x0,
						0x0, 0x0, 0x0, 0x0,
						0x0, 0x0, 0x0, 0x0 };
VECT_VAR_DECL(expected_q_uint,uint,16,8) [] = { 0xffff, 0xffff, 0xffff, 0xffff,
						0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected_q_uint,uint,32,4) [] = { 0xffffffff, 0xffffffff,
						0xffffffff, 0x0 };

#if defined (__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
VECT_VAR_DECL (expected_float, uint, 16, 4) [] = { 0xffff, 0xffff, 0x0, 0x0 };
VECT_VAR_DECL (expected_q_float, uint, 16, 8) [] = { 0xffff, 0xffff,
						     0xffff, 0x0,
						     0x0, 0x0,
						     0x0, 0x0 };
#endif

VECT_VAR_DECL(expected_float,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_q_float,uint,32,4) [] = { 0xffffffff, 0xffffffff,
						 0xffffffff, 0x0 };

VECT_VAR_DECL(expected_uint2,uint,32,2) [] = { 0xffffffff, 0x0 };
VECT_VAR_DECL(expected_uint3,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_uint4,uint,32,2) [] = { 0xffffffff, 0x0 };

#if defined (__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
VECT_VAR_DECL (expected_nan, uint, 16, 4) [] = { 0x0, 0x0, 0x0, 0x0  };
VECT_VAR_DECL (expected_mnan, uint, 16, 4) [] = { 0x0, 0x0, 0x0, 0x0 };
VECT_VAR_DECL (expected_nan2, uint, 16, 4) [] = { 0x0, 0x0, 0x0, 0x0 };

VECT_VAR_DECL (expected_inf, uint, 16, 4) [] = { 0xffff, 0xffff,
						 0xffff, 0xffff };
VECT_VAR_DECL (expected_minf, uint, 16, 4) [] = { 0x0, 0x0, 0x0, 0x0 };
VECT_VAR_DECL (expected_inf2, uint, 16, 4) [] = { 0x0, 0x0, 0x0, 0x0 };

VECT_VAR_DECL (expected_mzero, uint, 16, 4) [] = { 0xffff, 0xffff,
						   0xffff, 0xffff };
#endif

VECT_VAR_DECL(expected_nan,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_mnan,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_nan2,uint,32,2) [] = { 0x0, 0x0 };

VECT_VAR_DECL(expected_inf,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_minf,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_inf2,uint,32,2) [] = { 0x0, 0x0 };

VECT_VAR_DECL(expected_mzero,uint,32,2) [] = { 0xffffffff, 0xffffffff };
