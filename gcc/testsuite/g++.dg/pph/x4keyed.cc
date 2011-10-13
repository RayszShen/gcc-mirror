// { dg-xfail-if "BOGUS MERGE AUXVAR" { "*-*-*" } { "-fpph-map=pph.map" } }
// { dg-bogus "x4keyed.cc:14:1: error: redefinition of 'const char _ZTS5keyed ..'" "" { xfail *-*-* } 0 }
// The variable for the typeinfo name for 'keyed' is duplicated.

#include "x0keyed1.h"
#include "x0keyed2.h"

int keyed::key( int arg ) { return mix( field & arg ); }

int main()
{
    keyed variable;
    return variable.key( 3 );
}
