/* { dg-do assemble }  */
/* { dg-options "-flto-single -funsigned-char" }  */ 
typedef unsigned int size_t;
foo (const char *src, unsigned char *dst, size_t size)
{
  int ch;
  while ((ch = *src++) != '\0') {
  }
}
