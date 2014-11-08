#define INSN_NAME vcle
#define TEST_MSG "VCLE/VCLEQ"

#include "cmp_op.inc"

/* Expected results.  */
VECT_VAR_DECL(expected,int,8,8) [] = { 0x33, 0x33, 0x33, 0x33,
				       0x33, 0x33, 0x33, 0x33 };
VECT_VAR_DECL(expected,int,16,4) [] = { 0x333, 0x3333, 0x3333, 0x3333 };
VECT_VAR_DECL(expected,int,32,2) [] = { 0x33333333, 0x33333333 };
VECT_VAR_DECL(expected,int,64,1) [] = { 0x3333333333333333 };
VECT_VAR_DECL(expected,uint,8,8) [] = { 0xff, 0xff, 0xff, 0xff,
					0xff, 0xff, 0xff, 0x0 };
VECT_VAR_DECL(expected,uint,16,4) [] = { 0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected,uint,32,2) [] = { 0xffffffff, 0x0 };
VECT_VAR_DECL(expected,uint,64,1) [] = { 0x3333333333333333 };
VECT_VAR_DECL(expected,poly,8,8) [] = { 0x33, 0x33, 0x33, 0x33,
					0x33, 0x33, 0x33, 0x33 };
VECT_VAR_DECL(expected,poly,16,4) [] = { 0x3333, 0x3333, 0x3333, 0x3333 };
VECT_VAR_DECL(expected,hfloat,32,2) [] = { 0x33333333, 0x33333333 };
VECT_VAR_DECL(expected,int,8,16) [] = { 0x33, 0x33, 0x33, 0x33,
					0x33, 0x33, 0x33, 0x33,
					0x33, 0x33, 0x33, 0x33,
					0x33, 0x33, 0x33, 0x33 };
VECT_VAR_DECL(expected,int,16,8) [] = { 0x333, 0x3333, 0x3333, 0x3333,
					0x333, 0x3333, 0x3333, 0x3333 };
VECT_VAR_DECL(expected,int,32,4) [] = { 0x33333333, 0x33333333,
					0x33333333, 0x33333333 };
VECT_VAR_DECL(expected,int,64,2) [] = { 0x3333333333333333,
					0x3333333333333333 };
VECT_VAR_DECL(expected,uint,8,16) [] = { 0xff, 0xff, 0xff, 0xff,
					 0xff, 0xff, 0xff, 0xff,
					 0xff, 0xff, 0xff, 0xff,
					 0xff, 0x0, 0x0, 0x0 };
VECT_VAR_DECL(expected,uint,16,8) [] = { 0xffff, 0xffff, 0xffff, 0xffff,
					 0xffff, 0xffff, 0xffff, 0x0 };
VECT_VAR_DECL(expected,uint,32,4) [] = { 0xffffffff, 0xffffffff,
					 0xffffffff, 0x0 };
VECT_VAR_DECL(expected,uint,64,2) [] = { 0x3333333333333333,
					 0x3333333333333333 };
VECT_VAR_DECL(expected,poly,8,16) [] = { 0x33, 0x33, 0x33, 0x33,
					 0x33, 0x33, 0x33, 0x33,
					 0x33, 0x33, 0x33, 0x33,
					 0x33, 0x33, 0x33, 0x33 };
VECT_VAR_DECL(expected,poly,16,8) [] = { 0x3333, 0x3333, 0x3333, 0x3333,
					 0x3333, 0x3333, 0x3333, 0x3333 };
VECT_VAR_DECL(expected,hfloat,32,4) [] = { 0x33333333, 0x33333333,
					   0x33333333, 0x33333333 };

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

VECT_VAR_DECL(expected_float,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_q_float,uint,32,4) [] = { 0xffffffff, 0xffffffff,
						 0xffffffff, 0x0 };

VECT_VAR_DECL(expected_uint2,uint,32,2) [] = { 0xffffffff, 0x0 };
VECT_VAR_DECL(expected_uint3,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_uint4,uint,32,2) [] = { 0xffffffff, 0x0 };

VECT_VAR_DECL(expected_nan,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_mnan,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_nan2,uint,32,2) [] = { 0x0, 0x0 };

VECT_VAR_DECL(expected_inf,uint,32,2) [] = { 0xffffffff, 0xffffffff };
VECT_VAR_DECL(expected_minf,uint,32,2) [] = { 0x0, 0x0 };
VECT_VAR_DECL(expected_inf2,uint,32,2) [] = { 0x0, 0x0 };

VECT_VAR_DECL(expected_mzero,uint,32,2) [] = { 0xffffffff, 0xffffffff };
