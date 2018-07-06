/* { dg-do compile } */
/* { dg-require-effective-target arm_fp16fml_neon_ok } */
/* { dg-add-options arm_fp16fml_neon }  */

#include "arm_neon.h"

float32x2_t
test_vfmlal_high_u32 (float32x2_t r, float16x4_t a, float16x4_t b)
{
  return vfmlal_high_u32 (r, a, b);
}

float32x4_t
test_vfmlalq_high_u32 (float32x4_t r, float16x8_t a, float16x8_t b)
{
  return vfmlalq_high_u32 (r, a, b);
}

float32x2_t
test_vfmlsl_high_u32 (float32x2_t r, float16x4_t a, float16x4_t b)
{
  return vfmlsl_high_u32 (r, a, b);
}

float32x4_t
test_vfmlslq_high_u32 (float32x4_t r, float16x8_t a, float16x8_t b)
{
  return vfmlslq_high_u32 (r, a, b);
}

/* { dg-final { scan-assembler-times {vfmal.f16\td[0-9]+, s[123]?[13579], s[123]?[13579]} 1 } } */
/* { dg-final { scan-assembler-times {vfmal.f16\tq[0-9]+, d[123]?[13579], d[123]?[13579]} 1 } } */
/* { dg-final { scan-assembler-times {vfmsl.f16\td[0-9]+, s[123]?[13579], s[123]?[13579]} 1 } } */
/* { dg-final { scan-assembler-times {vfmsl.f16\tq[0-9]+, d[123]?[13579], d[123]?[13579]} 1 } } */
