
// Copyright (c) 1999-2003 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#ifndef DMD_STATICASSERT_H
#define DMD_STATICASSERT_H

#ifdef __DMC__
#pragma once
#endif /* __DMC__ */

#include "dsymbol.h"

struct Expression;
#ifdef _DH
struct HdrGenState;
#endif

struct StaticAssert : Dsymbol
{
    Expression *exp;

    StaticAssert(Loc loc, Expression *exp);

    Dsymbol *syntaxCopy(Dsymbol *s);
    void addMember(Scope *sc, ScopeDsymbol *sd);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    void inlineScan();
    void toObjFile();
    char *kind();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
};

#endif
