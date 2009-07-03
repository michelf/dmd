
// Compiler implementation of the D programming language
// Copyright (c) 1999-2007 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#if __DMC__
#include <complex.h>
#endif

#include "lexer.h"
#include "mtype.h"
#include "expression.h"
#include "declaration.h"
#include "aggregate.h"
#include "init.h"


#ifdef IN_GCC
#include "d-gcc-real.h"

/* %% fix? */
extern "C" bool real_isnan (const real_t *);
#endif

static real_t zero;	// work around DMC bug for now


/*************************************
 * If expression is a variable with a const initializer,
 * return that initializer.
 */

Expression *fromConstInitializer(Expression *e1)
{
    //printf("fromConstInitializer(%s)\n", e1->toChars());
    if (e1->op == TOKvar)
    {	VarExp *ve = (VarExp *)e1;
	VarDeclaration *v = ve->var->isVarDeclaration();
	if (v && v->isConst() && v->init)
	{   Expression *ei = v->init->toExpression();
	    if (ei)
	    {	e1 = ei;
		if (!e1->type)
		    e1->type = v->type;
	    }
	}
    }
    return e1;
}


Expression *Expression::optimize(int result)
{
    //printf("Expression::optimize(result = x%x) %s\n", result, toChars());
    return this;
}

Expression *TupleExp::optimize(int result)
{
    for (size_t i = 0; i < exps->dim; i++)
    {   Expression *e = (Expression *)exps->data[i];

	e = e->optimize(WANTvalue | (result & WANTinterpret));
	exps->data[i] = (void *)e;
    }
    return this;
}

Expression *ArrayLiteralExp::optimize(int result)
{
    if (elements)
    {
	for (size_t i = 0; i < elements->dim; i++)
	{   Expression *e = (Expression *)elements->data[i];

	    e = e->optimize(WANTvalue | (result & WANTinterpret));
	    elements->data[i] = (void *)e;
	}
    }
    return this;
}

Expression *TypeExp::optimize(int result)
{
    return this;
}

Expression *UnaExp::optimize(int result)
{
    e1 = e1->optimize(result);
    return this;
}

Expression *NegExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    if (e1->isConst() == 1)
    {
	e = Neg(type, e1);
    }
    else
	e = this;
    return e;
}

Expression *ComExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    if (e1->isConst() == 1)
    {
	e = Com(type, e1);
    }
    else
	e = this;
    return e;
}

Expression *NotExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    if (e1->isConst() == 1)
    {
	e = Not(type, e1);
    }
    else
	e = this;
    return e;
}

Expression *BoolExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    if (e1->isConst() == 1)
    {
	e = Bool(type, e1);
    }
    else
	e = this;
    return e;
}

Expression *AddrExp::optimize(int result)
{   Expression *e;

    //printf("AddrExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(result);
    // Convert &*ex to ex
    if (e1->op == TOKstar)
    {	Expression *ex;

	ex = ((PtrExp *)e1)->e1;
	if (type->equals(ex->type))
	    e = ex;
	else
	{
	    e = ex->copy();
	    e->type = type;
	}
	return e;
    }
    if (e1->op == TOKvar)
    {	VarExp *ve = (VarExp *)e1;
	if (!ve->var->isOut() && !ve->var->isImportedSymbol())
	{
	    e = new SymOffExp(loc, ve->var, 0);
	    e->type = type;
	    return e;
	}
    }
    if (e1->op == TOKindex)
    {	// Convert &array[n] to &array+n
	IndexExp *ae = (IndexExp *)e1;

	if (ae->e2->op == TOKint64 && ae->e1->op == TOKvar)
	{
	    integer_t index = ae->e2->toInteger();
	    VarExp *ve = (VarExp *)ae->e1;
	    if (ve->type->ty == Tsarray && ve->type->next->ty != Tbit
		&& !ve->var->isImportedSymbol())
	    {
		TypeSArray *ts = (TypeSArray *)ve->type;
		integer_t dim = ts->dim->toInteger();
		if (index < 0 || index >= dim)
		    error("array index %jd is out of bounds [0..%jd]", index, dim);
		e = new SymOffExp(loc, ve->var, index * ts->next->size());
		e->type = type;
		return e;
	    }
	}
    }
    return this;
}

Expression *PtrExp::optimize(int result)
{
    e1 = e1->optimize(result);
    // Convert *&ex to ex
    if (e1->op == TOKaddress)
    {	Expression *e;
	Expression *ex;

	ex = ((AddrExp *)e1)->e1;
	if (type->equals(ex->type))
	    e = ex;
	else
	{
	    e = ex->copy();
	    e->type = type;
	}
	return e;
    }
    return this;
}

Expression *CallExp::optimize(int result)
{   Expression *e = this;

    e1 = e1->optimize(result);
    if (e1->op == TOKvar && result & WANTinterpret)
    {
	FuncDeclaration *fd = ((VarExp *)e1)->var->isFuncDeclaration();
	if (fd)
	{
	    Expression *eresult = fd->interpret(NULL, arguments);
	    if (eresult)
		e = eresult;
	    else if (result & WANTinterpret)
		error("cannot evaluate %s at compile time", toChars());
	}
    }
    return e;
}


Expression *CastExp::optimize(int result)
{
    //printf("CastExp::optimize(result = %d) %s\n", result, toChars());
    //printf("from %s to %s\n", type->toChars(), to->toChars());
    //printf("type = %p\n", type);
    assert(type);

    e1 = e1->optimize(result);
    if ((e1->op == TOKstring || e1->op == TOKarrayliteral) &&
	(type->ty == Tpointer || type->ty == Tarray) &&
	type->next->equals(e1->type->next)
       )
    {
	e1->type = type;
	return e1;
    }
    if (e1->op == TOKnull &&
	(type->ty == Tpointer || type->ty == Tclass))
    {
	e1->type = type;
	return e1;
    }

    if (result & WANTflags && type->ty == Tclass && e1->type->ty == Tclass)
    {
	// See if we can remove an unnecessary cast
	ClassDeclaration *cdfrom;
	ClassDeclaration *cdto;
	int offset;

	cdfrom = e1->type->isClassHandle();
	cdto   = type->isClassHandle();
	if (cdto->isBaseOf(cdfrom, &offset) && offset == 0)
	{
	    e1->type = type;
	    return e1;
	}
    }

    Expression *e;

    if (e1->isConst())
    {
	if (e1->op == TOKsymoff)
	{
	    if (type->size() == e1->type->size() &&
		type->toBasetype()->ty != Tsarray)
	    {
		e1->type = type;
		return e1;
	    }
	    return this;
	}
	if (to->toBasetype()->ty == Tvoid)
	    e = this;
	else
	    e = Cast(type, to, e1);
    }
    else
	e = this;
    return e;
}

Expression *BinExp::optimize(int result)
{
    //printf("BinExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    return this;
}

Expression *AddExp::optimize(int result)
{   Expression *e;

    //printf("AddExp::optimize(%s)\n", toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() && e2->isConst())
    {
	if (e1->op == TOKsymoff && e2->op == TOKsymoff)
	    return this;
	e = Add(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *MinExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() && e2->isConst())
    {
	if (e2->op == TOKsymoff)
	    return this;
	e = Min(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *MulExp::optimize(int result)
{   Expression *e;

    //printf("MulExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Mul(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *DivExp::optimize(int result)
{   Expression *e;

    //printf("DivExp::optimize(%s)\n", toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Div(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *ModExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Mod(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *ShlExp::optimize(int result)
{   Expression *e;

    //printf("ShlExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    e = this;
    if (e2->isConst() == 1)
    {
	integer_t i2 = e2->toInteger();
	if (i2 < 0 || i2 > e1->type->size() * 8)
	{   error("shift left by %jd exceeds %zu", i2, e2->type->size() * 8);
	    e2 = new IntegerExp(0);
	}
	if (e1->isConst() == 1)
	    e = new IntegerExp(loc, e1->toInteger() << e2->toInteger(), type);
    }
    return e;
}

Expression *ShrExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);

    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Shr(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *UshrExp::optimize(int result)
{   Expression *e;

    //printf("UshrExp::optimize() %s\n", toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);

    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Ushr(type, e1, e2);
    }
    else
	e = this;
    return e;
}

Expression *AndExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
	e = And(type, e1, e2);
    else
	e = this;
    return e;
}

Expression *OrExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
	e = Or(type, e1, e2);
    else
	e = this;
    return e;
}

Expression *XorExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
	e = Xor(type, e1, e2);
    else
	e = this;
    return e;
}

Expression *CommaExp::optimize(int result)
{   Expression *e;

    //printf("CommaExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(result & WANTinterpret);
    e2 = e2->optimize(result);
    if (!e1 || e1->op == TOKint64 || e1->op == TOKfloat64 || !e1->checkSideEffect(2))
    {
	e = e2;
	if (e)
	    e->type = type;
    }
    else
	e = this;
    //printf("-CommaExp::optimize(result = %d) %s\n", result, e->toChars());
    return e;
}

Expression *ArrayLengthExp::optimize(int result)
{   Expression *e;

    //printf("ArrayLengthExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(WANTvalue | (result & WANTinterpret));
    e = this;
    if (e1->op == TOKstring || e1->op == TOKarrayliteral)
    {
	e = ArrayLength(type, e1);
    }
    return e;
}

Expression *EqualExp::optimize(int result)
{   Expression *e;

    //printf("EqualExp::optimize(result = %x) %s\n", result, toChars());
    e1 = e1->optimize(WANTvalue | (result & WANTinterpret));
    e2 = e2->optimize(WANTvalue | (result & WANTinterpret));
    e = this;

    Expression *e1 = fromConstInitializer(this->e1);
    Expression *e2 = fromConstInitializer(this->e2);

    e = Equal(op, type, e1, e2);
    if (e == EXP_CANT_INTERPRET)
	e = this;
    return e;
}

Expression *IdentityExp::optimize(int result)
{   Expression *e;

    //printf("IdentityExp::optimize(result = %d) %s\n", result, toChars());
    e1 = e1->optimize(WANTvalue | (result & WANTinterpret));
    e2 = e2->optimize(WANTvalue | (result & WANTinterpret));
    e = this;

    if (this->e1->isConst() && this->e2->isConst())
    {
	e = Identity(op, type, this->e1, this->e2);
    }
    return e;
}

Expression *IndexExp::optimize(int result)
{   Expression *e;

    //printf("IndexExp::optimize(result = %d) %s\n", result, toChars());
    Expression *e1 = this->e1->optimize(WANTvalue | (result & WANTinterpret));
    if (result & WANTinterpret)
	e1 = fromConstInitializer(e1);
    e2 = e2->optimize(WANTvalue | (result & WANTinterpret));
    e = Index(type, e1, e2);
    if (e == EXP_CANT_INTERPRET)
	e = this;
    return e;
}

Expression *SliceExp::optimize(int result)
{   Expression *e;

    //printf("SliceExp::optimize(result = %d) %s\n", result, toChars());
    e = this;
    e1 = e1->optimize(WANTvalue | (result & WANTinterpret));
    if (!lwr)
    {	if (e1->op == TOKstring)
	{   // Convert slice of string literal into dynamic array
	    Type *t = e1->type->toBasetype();
	    if (t->next)
		e = e1->castTo(NULL, t->next->arrayOf());
	}
	return e;
    }
    if (result & WANTinterpret)
	e1 = fromConstInitializer(e1);
    lwr = lwr->optimize(WANTvalue | (result & WANTinterpret));
    upr = upr->optimize(WANTvalue | (result & WANTinterpret));
    e = Slice(type, e1, lwr, upr);
    if (e == EXP_CANT_INTERPRET)
	e = this;
    return e;
}

Expression *AndAndExp::optimize(int result)
{   Expression *e;

    //printf("AndAndExp::optimize(%d) %s\n", result, toChars());
    e1 = e1->optimize(WANTflags | (result & WANTinterpret));
    e = this;
    if (e1->isBool(FALSE))
    {
	e = new CommaExp(loc, e1, new IntegerExp(loc, 0, type));
	e->type = type;
	e = e->optimize(result);
    }
    else
    {
	e2 = e2->optimize(WANTflags | (result & WANTinterpret));
	if (result && e2->type->toBasetype()->ty == Tvoid && !global.errors)
	    error("void has no value");
	if (e1->isConst())
	{
	    if (e2->isConst())
	    {	int n1 = e1->isBool(1);
		int n2 = e2->isBool(1);

		e = new IntegerExp(loc, n1 && n2, type);
	    }
	    else if (e1->isBool(TRUE))
		e = new BoolExp(loc, e2, type);
	}
    }
    return e;
}

Expression *OrOrExp::optimize(int result)
{   Expression *e;

    e1 = e1->optimize(WANTflags | (result & WANTinterpret));
    e = this;
    if (e1->isBool(TRUE))
    {	// Replace with (e1, 1)
	e = new CommaExp(loc, e1, new IntegerExp(loc, 1, type));
	e->type = type;
	e = e->optimize(result);
    }
    else
    {
	e2 = e2->optimize(WANTflags | (result & WANTinterpret));
	if (result && e2->type->toBasetype()->ty == Tvoid && !global.errors)
	    error("void has no value");
	if (e1->isConst())
	{
	    if (e2->isConst())
	    {	int n1 = e1->isBool(1);
		int n2 = e2->isBool(1);

		e = new IntegerExp(loc, n1 || n2, type);
	    }
	    else if (e1->isBool(FALSE))
		e = new BoolExp(loc, e2, type);
	}
    }
    return e;
}

Expression *CmpExp::optimize(int result)
{   Expression *e;

    //printf("CmpExp::optimize() %s\n", toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    if (e1->isConst() == 1 && e2->isConst() == 1)
    {
	e = Cmp(op, type, this->e1, this->e2);
    }
    else
	e = this;
    return e;
}

Expression *CatExp::optimize(int result)
{   Expression *e;

    //printf("CatExp::optimize(%d) %s\n", result, toChars());
    e1 = e1->optimize(result);
    e2 = e2->optimize(result);
    e = Cat(type, e1, e2);
    if (e == EXP_CANT_INTERPRET)
	e = this;
    return e;
}


Expression *CondExp::optimize(int result)
{   Expression *e;

    econd = econd->optimize(WANTflags | (result & WANTinterpret));
    if (econd->isBool(TRUE))
	e = e1->optimize(result);
    else if (econd->isBool(FALSE))
	e = e2->optimize(result);
    else
    {	e1 = e1->optimize(result);
	e2 = e2->optimize(result);
	e = this;
    }
    return e;
}


