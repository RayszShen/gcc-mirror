/* { dg-options { -nostartfiles below100.o -Tbelow100.ld -O2 } } */
/* { dg-final { scan-assembler "clr1 32532,#7" } } */

typedef struct
{
  unsigned char b0:1;
  unsigned char b1:1;
  unsigned char b2:1;
  unsigned char b3:1;
  unsigned char b4:1;
  unsigned char b5:1;
  unsigned char b6:1;
  unsigned char b7:1;
} BitField;

#define SFR (*((volatile BitField*)0x7f14))
unsigned char *p = (unsigned char *) 0x7f14;

void
Do (void)
{
  SFR.b7 = 0;
}

int
main (void)
{
  *p = 0xcb;
  Do ();
  return (*p == 0x4b) ? 0 : 1;
}
