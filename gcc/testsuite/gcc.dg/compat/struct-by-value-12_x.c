#include "compat-common.h"

#include "fp-struct-defs.h"
#include "fp-struct-check.h"
#include "fp-struct-test-by-value-x.h"

DEFS(cs, _Complex short)
CHECKS(cs, _Complex short)

TEST(Scs1, _Complex short)
TEST(Scs2, _Complex short)
TEST(Scs3, _Complex short)
TEST(Scs4, _Complex short)
TEST(Scs5, _Complex short)
TEST(Scs6, _Complex short)
TEST(Scs7, _Complex short)
TEST(Scs8, _Complex short)
TEST(Scs9, _Complex short)
TEST(Scs10, _Complex short)
TEST(Scs11, _Complex short)
TEST(Scs12, _Complex short)
TEST(Scs13, _Complex short)
TEST(Scs14, _Complex short)
TEST(Scs15, _Complex short)
TEST(Scs16, _Complex short)

#undef T

void
struct_by_value_12_x ()
{
#define T(TYPE, MTYPE) testit##TYPE ();

T(Scs1, _Complex short)
T(Scs2, _Complex short)
T(Scs3, _Complex short)
T(Scs4, _Complex short)
T(Scs5, _Complex short)
T(Scs6, _Complex short)
T(Scs7, _Complex short)
T(Scs8, _Complex short)
T(Scs9, _Complex short)
T(Scs10, _Complex short)
T(Scs11, _Complex short)
T(Scs12, _Complex short)
T(Scs13, _Complex short)
T(Scs14, _Complex short)
T(Scs15, _Complex short)
T(Scs16, _Complex short)

if (fails != 0)
  abort ();

#undef T
}
