
// Copyright (c) 1999-2003 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "mars.h"
#include "mtype.h"
#include "declaration.h"
#include "expression.h"
#include "template.h"

static void indent(int indent)
{
    int i;

    for (i = 0; i < indent; i++)
	printf(" ");
}

static char *type_print(Type *type)
{
    return type ? type->toChars() : (char *) "null";
}

void Expression::dump(int i)
{
    indent(i);
    printf("%p %s type=%s\n", this, Token::toChars(op), type_print(type));
}

void IntegerExp::dump(int i)
{
    indent(i);
    printf("%p %lld type=%s\n", this, (d_int64)value, type_print(type));
}

void IdentifierExp::dump(int i)
{
    indent(i);
    printf("%p ident '%s' type=%s\n", this, ident->toChars(), type_print(type));
}

void DsymbolExp::dump(int i)
{
    indent(i);
    printf("%p %s type=%s\n", this, s->toChars(), type_print(type));
}

void VarExp::dump(int i)
{
    indent(i);
    printf("%p %s var=%s type=%s\n", this, Token::toChars(op), var->toChars(), type_print(type));
}

void UnaExp::dump(int i)
{
    indent(i);
    printf("%p %s type=%s e1=%p\n", this, Token::toChars(op), type_print(type), e1);
    if (e1)
	e1->dump(i + 2);
}

void DotVarExp::dump(int i)
{
    indent(i);
    printf("%p %s type=%s var='%s' e1=%p\n", this, Token::toChars(op), type_print(type), var->toChars(), e1);
    if (e1)
	e1->dump(i + 2);
}

void DotTemplateInstanceExp::dump(int i)
{
    indent(i);
    printf("%p %s type=%s ti='%s' e1=%p\n", this, Token::toChars(op), type_print(type), ti->toChars(), e1);
    if (e1)
	e1->dump(i + 2);
}

void DelegateExp::dump(int i)
{
    indent(i);
    printf("%p %s func=%s type=%s e1=%p\n", this, Token::toChars(op), func->toChars(), type_print(type), e1);
    if (e1)
	e1->dump(i + 2);
}

void BinExp::dump(int i)
{
    indent(i);
    printf("%p %s type=%s e1=%p e2=%p\n", this, Token::toChars(op), type_print(type), e1, e2);
    if (e1)
	e1->dump(i + 2);
    if (e2)
	e2->dump(i + 2);
}


