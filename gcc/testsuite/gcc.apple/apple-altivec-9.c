/* APPLE LOCAL file AltiVec */
/* { dg-do compile { target powerpc*-*-* } } */
/* { dg-options "-faltivec" } */

int vConvert_PlanarFtoPlanar16F( )
{
    vector float twoP10 = (vector float) (0x1.0p+10f, 0x1.0p+24f, 0x1.0p+102f, 1.0f/0.0f ); /* { dg-error "vector literal contains an invalid constant expression" } */
    return 0;
}
