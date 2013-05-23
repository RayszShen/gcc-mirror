/* Check that Neon is used to handle 64-bits scalar operations.  */

/* { dg-do compile } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-O2 -mneon-for-64bits" } */
/* { dg-add-options arm_neon } */

typedef long long i64;
typedef unsigned long long u64;
typedef unsigned int u32;
typedef int i32;

/* Unary operators */
#define UNARY_OP(name, op) \
  void unary_##name(u64 *a, u64 *b) { *a = op (*b + 0x1234567812345678ULL) ; }

/* Binary operators */
#define BINARY_OP(name, op) \
  void binary_##name(u64 *a, u64 *b, u64 *c) { *a = *b op *c ; }

/* Unsigned shift */
#define SHIFT_U(name, op, amount) \
  void ushift_##name(u64 *a, u64 *b, int c) { *a = *b op amount; }

/* Signed shift */
#define SHIFT_S(name, op, amount) \
  void sshift_##name(i64 *a, i64 *b, int c) { *a = *b op amount; }

UNARY_OP(not, ~)

BINARY_OP(add, +)
BINARY_OP(sub, -)
BINARY_OP(and, &)
BINARY_OP(or, |)
BINARY_OP(xor, ^)

SHIFT_U(right1, >>, 1)
SHIFT_U(right2, >>, 2)
SHIFT_U(right5, >>, 5)
SHIFT_U(rightn, >>, c)

SHIFT_S(right1, >>, 1)
SHIFT_S(right2, >>, 2)
SHIFT_S(right5, >>, 5)
SHIFT_S(rightn, >>, c)

/* { dg-final {scan-assembler-times "vmvn" 1} }  */
/* Two vadd: 1 in unary_not, 1 in binary_add */
/* { dg-final {scan-assembler-times "vadd" 2} }  */
/* { dg-final {scan-assembler-times "vsub" 1} }  */
/* { dg-final {scan-assembler-times "vand" 1} }  */
/* { dg-final {scan-assembler-times "vorr" 1} }  */
/* { dg-final {scan-assembler-times "veor" 1} }  */
/* 6 vshr for right shifts by constant, and variable right shift uses
   vshl with a negative amount in register.  */
/* { dg-final {scan-assembler-times "vshr" 6} }  */
/* { dg-final {scan-assembler-times "vshl" 2} }  */
