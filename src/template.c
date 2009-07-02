
// Copyright (c) 1999-2004 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

// Handle template implementation

#include <stdio.h>
#include <assert.h>

#include "root.h"
#include "mem.h"
#include "stringtable.h"

#include "template.h"
#include "mtype.h"
#include "init.h"
#include "expression.h"
#include "scope.h"
#include "module.h"
#include "aggregate.h"
#include "declaration.h"
#include "dsymbol.h"
#include "mars.h"
#include "dsymbol.h"
#include "identifier.h"

#define LOG	0

/********************************************
 * These functions substitute for dynamic_cast. dynamic_cast does not work
 * on earlier versions of gcc.
 */

static Expression *isExpression(Object *o)
{
    //return dynamic_cast<Expression *>(o);
    if (!o || o->dyncast() != DYNCAST_EXPRESSION)
	return NULL;
    return (Expression *)o;
}

static Dsymbol *isDsymbol(Object *o)
{
    //return dynamic_cast<Dsymbol *>(o);
    if (!o || o->dyncast() != DYNCAST_DSYMBOL)
	return NULL;
    return (Dsymbol *)o;
}

static Type *isType(Object *o)
{
    //return dynamic_cast<Type *>(o);
    if (!o || o->dyncast() != DYNCAST_TYPE)
	return NULL;
    return (Type *)o;
}

/* ======================== TemplateDeclaration ============================= */

TemplateDeclaration::TemplateDeclaration(Loc loc, Identifier *id, Array *parameters, Array *decldefs)
    : ScopeDsymbol(id)
{
#if LOG
    printf("TemplateDeclaration(this = %p, id = '%s')\n", this, id->toChars());
#endif
    this->loc = loc;
    this->parameters = parameters;
    this->members = decldefs;
    this->overnext = NULL;
    this->scope = NULL;
}

Dsymbol *TemplateDeclaration::syntaxCopy(Dsymbol *)
{
    TemplateDeclaration *td;
    Array *p;
    Array *d;

    p = NULL;
    if (parameters)
    {
	p = new Array();
	p->setDim(parameters->dim);
	for (int i = 0; i < p->dim; i++)
	{   TemplateParameter *tp = (TemplateParameter *)parameters->data[i];
	    p->data[i] = (void *)tp->syntaxCopy();
	}
    }
    d = Dsymbol::arraySyntaxCopy(members);
    td = new TemplateDeclaration(loc, ident, p, d);
    return td;
}

void TemplateDeclaration::semantic(Scope *sc)
{
#if LOG
    printf("TemplateDeclaration::semantic(this = %p, id = '%s')\n", this, ident->toChars());
#endif
    if (scope)
	return;		// semantic() already run

    if (sc->func)
    {
	error("cannot declare template at function scope %s", sc->func->toChars());
    }

    if (global.params.useArrayBounds && sc->module)
    {
	// Generate this function as it may be used
	// when template is instantiated in other modules
	sc->module->toModuleArray();
    }

    if (global.params.useAssert && sc->module)
    {
	// Generate this function as it may be used
	// when template is instantiated in other modules
	sc->module->toModuleAssert();
    }

    /* Remember Scope for later instantiations, but make
     * a copy since attributes can change.
     */
    this->scope = new Scope(*sc);
    this->scope->setNoFree();

    // Set up scope for parameters
    ScopeDsymbol *paramsym = new ScopeDsymbol();
    paramsym->parent = sc->parent;
    Scope *paramscope = sc->push(paramsym);

    for (int i = 0; i < parameters->dim; i++)
    {
	TemplateParameter *tp = (TemplateParameter *)parameters->data[i];

	tp->semantic(paramscope);
    }

    paramscope->pop();

    /* BUG: should check:
     *	o no virtual functions or non-static data members of classes
     */
}

char *TemplateDeclaration::kind()
{
    return "template";
}

/**********************************
 * Overload existing TemplateDeclaration 'this' with the new one 's'.
 * Return !=0 if successful; i.e. no conflict.
 */

int TemplateDeclaration::overloadInsert(Dsymbol *s)
{
    TemplateDeclaration **pf;
    TemplateDeclaration *f;

#if LOG
    printf("TemplateDeclaration::overloadInsert('%s')\n", s->toChars());
#endif
    f = s->isTemplateDeclaration();
    if (!f)
	return FALSE;
    TemplateDeclaration *pthis = this;
    for (pf = &pthis; *pf; pf = &(*pf)->overnext)
    {
	// Conflict if TemplateParameter's match
	// Will get caught anyway later with TemplateInstance, but
	// should check it now.
	TemplateDeclaration *f2 = *pf;

	if (f->parameters->dim != f2->parameters->dim)
	    goto Lcontinue;

	for (int i = 0; i < f->parameters->dim; i++)
	{   TemplateParameter *p1 = (TemplateParameter *)f->parameters->data[i];
	    TemplateParameter *p2 = (TemplateParameter *)f2->parameters->data[i];

	    if (!p1->overloadMatch(p2))
		goto Lcontinue;
	}

#if LOG
	printf("\tfalse: conflict\n");
#endif
	return FALSE;

     Lcontinue:
	;
    }


    *pf = f;
#if LOG
    printf("\ttrue: no conflict\n");
#endif
    return TRUE;
}

/***************************************
 * Given that ti is an instance of this TemplateDeclaration,
 * deduce the types of the parameters to this, and store
 * those deduced types in dedtypes[].
 * Input:
 *	flag	1: don't do semantic() because of dummy types
 * Return match level.
 */

MATCH TemplateDeclaration::matchWithInstance(TemplateInstance *ti,
	Array *dedtypes, int flag)
{   MATCH m;
    int dim = dedtypes->dim;

#if LOG
    printf("+TemplateDeclaration::matchWithInstance(this = %p, ti = %p)\n", this, ti);
#endif
    dedtypes->zero();


//printf("TemplateDeclaration::matchWithInstance(this = %p, ti = %p)\n", this, ti);
//printf("dim = %d, parameters->dim = %d\n", dim, parameters->dim);
//if (ti->tiargs.dim)
//printf("ti->tiargs.dim = %d, [0] = %p\n", ti->tiargs.dim, ti->tiargs.data[0]);

    assert(dim == parameters->dim);
    assert(dim >= ti->tiargs.dim);

    // Set up scope for parameters
    ScopeDsymbol *paramsym = new ScopeDsymbol();
    paramsym->parent = scope->parent;
    Scope *paramscope = scope->push(paramsym);

    // Attempt type deduction
    m = MATCHexact;
    for (int i = 0; i < dim; i++)
    {	MATCH m2;
	TemplateParameter *tp = (TemplateParameter *)parameters->data[i];
	Object *oarg;
	Declaration *sparam;

	if (i < ti->tiargs.dim)
	    oarg = (Object *)ti->tiargs.data[i];
	else
	{   // Look for default argument instead
	    oarg = tp->defaultArg(paramscope);
	    if (!oarg)
		goto Lnomatch;
	}

	m2 = tp->matchArg(paramscope, oarg, i, parameters, dedtypes, &sparam);
	if (m2 == MATCHnomatch)
	    goto Lnomatch;

	if (m2 < m)
	    m = m2;

	if (!flag)
	    sparam->semantic(paramscope);
	if (!paramscope->insert(sparam))
	    goto Lnomatch;
    }

    // Any parameter left without a type gets the type of its corresponding arg
    for (int i = 0; i < dim; i++)
    {
	if (!dedtypes->data[i])
	{   assert(i < ti->tiargs.dim);
	    dedtypes->data[i] = ti->tiargs.data[i];
	}
    }

#if 0
    // Print out the results
    printf("--------------------------\n");
    printf("template %s\n", toChars());
    printf("instance %s\n", ti->toChars());
    if (m)
    {
	for (int i = 0; i < dim; i++)
	{
	    TemplateParameter *tp = (TemplateParameter *)parameters->data[i];
	    Object *oarg;

	    printf(" [%d]", i);

	    if (i < ti->tiargs.dim)
		oarg = (Object *)ti->tiargs.data[i];
	    else
		oarg = NULL;
	    tp->print(oarg, (Object *)dedtypes->data[i]);
	}
    }
    else
	goto Lnomatch;
#endif

#if LOG
    printf(" match = %d\n", m);
#endif
    goto Lret;

Lnomatch:
#if LOG
    printf(" no match\n");
#endif
    m = MATCHnomatch;

Lret:
    paramscope->pop();
#if LOG
    printf("-TemplateDeclaration::matchWithInstance(this = %p, ti = %p) = %d\n", this, ti, m);
#endif
    return m;
}

/********************************************
 * Determine partial specialization order of 'this' vs td2.
 * Returns:
 *	1	this is at least as specialized as td2
 *	0	td2 is more specialized than this
 */

int TemplateDeclaration::leastAsSpecialized(TemplateDeclaration *td2)
{
    /* This works by taking the template parameters to this template
     * declaration and feeding them to td2 as if it were a template
     * instance.
     * If it works, then this template is at least as specialized
     * as td2.
     */

    TemplateInstance ti(ident);		// create dummy template instance
    Array dedtypes;

#define LOG_LEASTAS	0

#if LOG_LEASTAS
    printf("%s.leastAsSpecialized(%s)\n", toChars(), td2->toChars());
#endif

    // Set type arguments to dummy template instance to be types
    // generated from the parameters to this template declaration
    ti.tiargs.setDim(parameters->dim);
    for (int i = 0; i < ti.tiargs.dim; i++)
    {
	TemplateParameter *tp = (TemplateParameter *)parameters->data[i];

	ti.tiargs.data[i] = tp->dummyArg();
    }

    // Temporary Array to hold deduced types
    dedtypes.setDim(parameters->dim);

    // Attempt a type deduction
    if (td2->matchWithInstance(&ti, &dedtypes, 1))
    {
#if LOG_LEASTAS
	printf("  is least as specialized\n");
#endif
	return 1;
    }
#if LOG_LEASTAS
    printf("  is not as specialized\n");
#endif
    return 0;
}

void TemplateDeclaration::toCBuffer(OutBuffer *buf)
{
    int i;

    buf->writestring("template ");
    buf->writestring(ident->toChars());
    buf->writeByte('(');
    for (i = 0; i < parameters->dim; i++)
    {
	TemplateParameter *tp = (TemplateParameter *)parameters->data[i];
	if (i)
	    buf->writeByte(',');
	tp->toCBuffer(buf);
    }
    buf->writeByte(')');
}


char *TemplateDeclaration::toChars()
{
    OutBuffer buf;
    char *s;

    toCBuffer(&buf);
    s = buf.toChars();
    buf.data = NULL;
    return s + 9;	// kludge to skip over 'template '
}

/* ======================== Type ============================================ */

/* These form the heart of template argument deduction.
 * Given 'this' being the argument to the template instance,
 * it is matched against the template declaration parameter specialization
 * 'tparam' to determine the type to be used for the parameter.
 */

MATCH Type::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    //printf("Type::deduceType()\n");
    //printf("\tthis   = %d, ", ty); print();
    //printf("\ttparam = %d, ", tparam->ty); tparam->print();
    if (!tparam)
	goto Lnomatch;

    if (this == tparam)
	goto Lexact;

    if (tparam->ty == Tident)
    {
	TypeIdentifier *tident = (TypeIdentifier *)tparam;

	//printf("\ttident = '%s'\n", tident->toChars());
	if (tident->idents.dim > 0)
	    goto Lnomatch;

	// Determine which parameter tparam is
	Identifier *id = tident->ident;
	int i;
	for (i = 0; 1; i++)
	{
	    if (i == parameters->dim)
		goto Lnomatch;
	    TemplateParameter *tp = (TemplateParameter *)parameters->data[i];

	    if (tp->ident->equals(id))
	    {	// Found the corresponding parameter
		Type *at = (Type *)dedtypes->data[i];
		if (!at)
		{
		    dedtypes->data[i] = (void *)this;
		    goto Lexact;
		}
		if (equals(at))
		    goto Lexact;
		else
		    goto Lnomatch;
	    }
	}
    }

    if (ty != tparam->ty)
	goto Lnomatch;

    if (next)
	return next->deduceType(tparam->next, parameters, dedtypes);

Lexact:
    return MATCHexact;

Lnomatch:
    return MATCHnomatch;
}

MATCH TypeSArray::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check that array dimensions must match
    if (tparam && tparam->ty == Tsarray)
    {
	TypeSArray *tp = (TypeSArray *)tparam;
	if (dim->toInteger() != tp->dim->toInteger())
	    return MATCHnomatch;
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeAArray::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check that index type must match
    if (tparam && tparam->ty == Taarray)
    {
	TypeAArray *tp = (TypeAArray *)tparam;
	if (!index->deduceType(tp->index, parameters, dedtypes))
	    return MATCHnomatch;
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeFunction::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check that function characteristics must match
    if (tparam && tparam->ty == Tfunction)
    {
	TypeFunction *tp = (TypeFunction *)tparam;
	if (varargs != tp->varargs ||
	    linkage != tp->linkage ||
	    arguments->dim != tp->arguments->dim)
	    return MATCHnomatch;
	for (int i = 0; i < arguments->dim; i++)
	{
	    Argument *a = (Argument *)arguments->data[i];
	    Argument *ap = (Argument *)tp->arguments->data[i];
	    if (a->inout != ap->inout ||
		!a->type->deduceType(ap->type, parameters, dedtypes))
		return MATCHnomatch;
	}
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeIdentifier::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check
    if (tparam && tparam->ty == Tident)
    {
	TypeIdentifier *tp = (TypeIdentifier *)tparam;

	for (int i = 0; i < idents.dim; i++)
	{
	    Identifier *id1 = (Identifier *)idents.data[i];
	    Identifier *id2 = (Identifier *)tp->idents.data[i];

	    if (!id1->equals(id2))
		return MATCHnomatch;
	}
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeInstance::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check
    if (tparam && tparam->ty == Tinstance)
    {
	TypeInstance *tp = (TypeInstance *)tparam;

	for (int i = 0; i < idents.dim; i++)
	{
	    Identifier *id1 = (Identifier *)idents.data[i];
	    Identifier *id2 = (Identifier *)tp->idents.data[i];

	    if (!id1->equals(id2))
		return MATCHnomatch;
	}

	for (int i = 0; i < tempinst->tiargs.dim; i++)
	{
	    Type *t1 = (Type *)tempinst->tiargs.data[i];
	    Type *t2 = (Type *)tp->tempinst->tiargs.data[i];

	    if (!t1->deduceType(t2, parameters, dedtypes))
		return MATCHnomatch;
	}
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeStruct::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check
    if (tparam && tparam->ty == Tstruct)
    {
	TypeStruct *tp = (TypeStruct *)tparam;

	if (sym != tp->sym)
	    return MATCHnomatch;
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeEnum::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check
    if (tparam && tparam->ty == Tenum)
    {
	TypeEnum *tp = (TypeEnum *)tparam;

	if (sym != tp->sym)
	    return MATCHnomatch;
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeTypedef::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    // Extra check
    if (tparam && tparam->ty == Ttypedef)
    {
	TypeTypedef *tp = (TypeTypedef *)tparam;

	if (sym != tp->sym)
	    return MATCHnomatch;
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

MATCH TypeClass::deduceType(Type *tparam, Array *parameters, Array *dedtypes)
{
    //printf("TypeClass::deduceType()\n");

    // Extra check
    if (tparam && tparam->ty == Tclass)
    {
	TypeClass *tp = (TypeClass *)tparam;

#if 1
	//printf("\t%d\n", (MATCH) implicitConvTo(tp));
	return (MATCH) implicitConvTo(tp);
#else
	if (sym != tp->sym)
	    return MATCHnomatch;
#endif
    }
    return Type::deduceType(tparam, parameters, dedtypes);
}

/* ======================== TemplateParameter =============================== */

TemplateParameter::TemplateParameter(Loc loc, Identifier *ident)
{
    this->loc = loc;
    this->ident = ident;
}

TemplateTypeParameter  *TemplateParameter::isTemplateTypeParameter()
{
    return NULL;
}

TemplateValueParameter *TemplateParameter::isTemplateValueParameter()
{
    return NULL;
}

TemplateAliasParameter *TemplateParameter::isTemplateAliasParameter()
{
    return NULL;
}

/* ======================== TemplateTypeParameter =========================== */

// type-parameter

TemplateTypeParameter::TemplateTypeParameter(Loc loc, Identifier *ident, Type *specType,
	Type *defaultType)
    : TemplateParameter(loc, ident)
{
    this->ident = ident;
    this->specType = specType;
    this->defaultType = defaultType;
}

TemplateTypeParameter  *TemplateTypeParameter::isTemplateTypeParameter()
{
    return this;
}

TemplateParameter *TemplateTypeParameter::syntaxCopy()
{
    TemplateTypeParameter *tp = new TemplateTypeParameter(loc, ident, specType, defaultType);
    if (tp->specType)
	tp->specType = specType->syntaxCopy();
    if (defaultType)
	tp->defaultType = defaultType->syntaxCopy();
    return tp;
}

void TemplateTypeParameter::semantic(Scope *sc)
{
    //printf("TemplateTypeParameter::semantic()\n");
    TypeIdentifier *ti = new TypeIdentifier(loc, ident);
    Declaration *sparam = new AliasDeclaration(loc, ident, ti);
    if (!sc->insert(sparam))
	error(loc, "parameter '%s' multiply defined", ident->toChars());

    if (specType)
	specType = specType->semantic(loc, sc);
    if (defaultType)
    {
	defaultType = defaultType->semantic(loc, sc);
	//defaultType->print();
    }
}

int TemplateTypeParameter::overloadMatch(TemplateParameter *tp)
{
    TemplateTypeParameter *ttp = tp->isTemplateTypeParameter();

    if (ttp)
    {
	if (specType != ttp->specType)
	    goto Lnomatch;

	if (specType && !specType->equals(ttp->specType))
	    goto Lnomatch;

	return 1;			// match
    }

Lnomatch:
    return 0;
}


MATCH TemplateTypeParameter::matchArg(Scope *sc, Object *oarg,
	int i, Array *parameters, Array *dedtypes, Declaration **psparam)
{
    Type *t;
    MATCH m = MATCHexact;
    Type *ta = isType(oarg);
    if (!ta)
	goto Lnomatch;

    t = (Type *)dedtypes->data[i];

    if (specType)
    {
	//printf("calling deduceType()\n");
	MATCH m2 = ta->deduceType(specType, parameters, dedtypes);
	if (m2 == MATCHnomatch)
	    goto Lnomatch;

	if (m2 < m)
	    m = m2;
	t = (Type *)dedtypes->data[i];
    }
    else if (t)
    {   // Must match already deduced type

	if (!t->equals(ta))
	    goto Lnomatch;
    }

    if (!t)
    {
	dedtypes->data[i] = ta;
	t = ta;
    }
    *psparam = new AliasDeclaration(loc, ident, t);
    return m;

Lnomatch:
    *psparam = NULL;
    return MATCHnomatch;
}


void TemplateTypeParameter::print(Object *oarg, Object *oded)
{
    printf(" %s\n", ident->toChars());

    Type *t  = isType(oarg);
    Type *ta = isType(oded);

    assert(ta);

    if (specType)
	printf("\tSpecialization: %s\n", specType->toChars());
    if (defaultType)
	printf("\tDefault:        %s\n", defaultType->toChars());
    printf("\tArgument:       %s\n", t ? t->toChars() : "NULL");
    printf("\tDeduced Type:   %s\n", ta->toChars());
}


void TemplateTypeParameter::toCBuffer(OutBuffer *buf)
{
    buf->writestring(ident->toChars());
    if (specType)
    {
	buf->writestring(" : ");
	specType->toCBuffer(buf, NULL);
    }
    if (defaultType)
    {
	buf->writestring(" = ");
	defaultType->toCBuffer(buf, NULL);
    }
}


void *TemplateTypeParameter::dummyArg()
{   Type *t;

    if (specType)
	t = specType;
    else
    {   // Use this for alias-parameter's too (?)
	t = new TypeIdentifier(loc, ident);
    }
    return (void *)t;
}


Object *TemplateTypeParameter::defaultArg(Scope *sc)
{
    Type *t;

    t = defaultType;
    if (t)
    {
	t = t->syntaxCopy();
	t = t->semantic(loc, sc);
    }
    return t;
}

/* ======================== TemplateAliasParameter ========================== */

// alias-parameter

TemplateAliasParameter::TemplateAliasParameter(Loc loc, Identifier *ident, Type *defaultAlias)
    : TemplateParameter(loc, ident)
{
    this->ident = ident;
    this->defaultAlias = defaultAlias;
}

TemplateAliasParameter *TemplateAliasParameter::isTemplateAliasParameter()
{
    return this;
}

TemplateParameter *TemplateAliasParameter::syntaxCopy()
{
    TemplateAliasParameter *tp = new TemplateAliasParameter(loc, ident, defaultAlias);
    if (defaultAlias)
	tp->defaultAlias = defaultAlias->syntaxCopy();
    return tp;
}

void TemplateAliasParameter::semantic(Scope *sc)
{
    TypeIdentifier *ti = new TypeIdentifier(loc, ident);
    Declaration *sparam = new AliasDeclaration(loc, ident, ti);
    if (!sc->insert(sparam))
	error(loc, "parameter '%s' multiply defined", ident->toChars());

    if (defaultAlias)
	defaultAlias = defaultAlias->semantic(loc, sc);
}

int TemplateAliasParameter::overloadMatch(TemplateParameter *tp)
{
    TemplateAliasParameter *tap = tp->isTemplateAliasParameter();

    if (tap)
    {
	return 1;			// match
    }

Lnomatch:
    return 0;
}

MATCH TemplateAliasParameter::matchArg(Scope *sc,
	Object *oarg, int i, Array *parameters, Array *dedtypes, Declaration **psparam)
{
    Dsymbol *sa;

    //printf("TemplateAliasParameter::matchArg()\n");
    Expression *ea = isExpression(oarg);
    if (ea)
    {   // Try to convert Expression to symbol
	if (ea->op == TOKvar)
	    sa = ((VarExp *)ea)->var;
	else
	    goto Lnomatch;
    }
    else
    {   // Try to convert Type to symbol
	Type *ta = isType(oarg);
	if (ta)
	    sa = ta->toDsymbol(NULL);
	else
	    sa = isDsymbol(oarg);	// if already a symbol
    }

    if (!sa)
	goto Lnomatch;

    dedtypes->data[i] = sa;

    *psparam = new AliasDeclaration(loc, ident, sa);
    return MATCHexact;

Lnomatch:
    *psparam = NULL;
    return MATCHnomatch;
}


void TemplateAliasParameter::print(Object *oarg, Object *oded)
{
    printf(" %s\n", ident->toChars());

    Dsymbol *sa = isDsymbol(oded);
    assert(sa);

    printf("\tArgument alias: %s\n", sa->toChars());
}

void TemplateAliasParameter::toCBuffer(OutBuffer *buf)
{
    buf->writestring("alias ");
    buf->writestring(ident->toChars());
    if (defaultAlias)
    {
	buf->writestring(" = ");
	defaultAlias->toCBuffer(buf, NULL);
    }
}


void *TemplateAliasParameter::dummyArg()
{
    return NULL;		// not sure what to put here
}


Object *TemplateAliasParameter::defaultArg(Scope *sc)
{
    Type *t;

    t = defaultAlias;
    if (t)
    {
	t = t->syntaxCopy();
	t = t->semantic(loc, sc);
    }
    return t;
}

/* ======================== TemplateValueParameter ========================== */

// value-parameter

TemplateValueParameter::TemplateValueParameter(Loc loc, Identifier *ident, Type *valType,
	Expression *specValue, Expression *defaultValue)
    : TemplateParameter(loc, ident)
{
    this->ident = ident;
    this->valType = valType;
    this->specValue = specValue;
    this->defaultValue = defaultValue;
}

TemplateValueParameter *TemplateValueParameter::isTemplateValueParameter()
{
    return this;
}

TemplateParameter *TemplateValueParameter::syntaxCopy()
{
    TemplateValueParameter *tp =
	new TemplateValueParameter(loc, ident, valType, specValue, defaultValue);
    tp->valType = valType->syntaxCopy();
    if (specValue)
	tp->specValue = specValue->syntaxCopy();
    if (defaultValue)
	tp->defaultValue = defaultValue->syntaxCopy();
    return tp;
}

void TemplateValueParameter::semantic(Scope *sc)
{
    Declaration *sparam = new VarDeclaration(loc, valType, ident, NULL);
    if (!sc->insert(sparam))
	error(loc, "parameter '%s' multiply defined", ident->toChars());

    sparam->semantic(sc);
    valType = valType->semantic(loc, sc);
    if (!valType->isintegral())
	error(loc, "integral type expected for value-parameter, not %s", valType->toChars());

    if (specValue)
    {   Expression *e = specValue;

	e = e->semantic(sc);
	e = e->implicitCastTo(valType);
	e = e->constFold();
	if (e->op == TOKint64)
	    specValue = e;
	//e->toInteger();
    }

    if (defaultValue)
    {   Expression *e = defaultValue;

	e = e->semantic(sc);
	e = e->implicitCastTo(valType);
	e = e->constFold();
	if (e->op == TOKint64)
	    defaultValue = e;
	//e->toInteger();
    }
}

int TemplateValueParameter::overloadMatch(TemplateParameter *tp)
{
    TemplateValueParameter *tvp = tp->isTemplateValueParameter();

    if (tvp)
    {
	if (valType != tvp->valType)
	    goto Lnomatch;

	if (valType && !valType->equals(tvp->valType))
	    goto Lnomatch;

	if (specValue != tvp->specValue)
	    goto Lnomatch;

	return 1;			// match
    }

Lnomatch:
    return 0;
}


MATCH TemplateValueParameter::matchArg(Scope *sc,
	Object *oarg, int i, Array *parameters, Array *dedtypes, Declaration **psparam)
{
    Initializer *init;
    Declaration *sparam;
    Expression *ei = isExpression(oarg);
    if (!ei && oarg)
	goto Lnomatch;

    if (specValue)
    {
	if (!ei)
	    goto Lnomatch;

	Expression *e = specValue;
	e = e->semantic(sc);
	e = e->implicitCastTo(valType);
	e = e->constFold();
	if (!ei->equals(e))
	    goto Lnomatch;
    }
    else if (dedtypes->data[i])
    {   // Must match already deduced value
	Expression *e = (Expression *)dedtypes->data[i];

	if (!ei || !ei->equals(e))
	    goto Lnomatch;
    }
    dedtypes->data[i] = ei;

    init = new ExpInitializer(loc, ei);
    sparam = new VarDeclaration(loc, valType, ident, init);
    sparam->storage_class = STCconst;
    *psparam = sparam;
    return MATCHexact;

Lnomatch:
    *psparam = NULL;
    return MATCHnomatch;
}


void TemplateValueParameter::print(Object *oarg, Object *oded)
{
    printf(" %s\n", ident->toChars());

    Expression *ea = isExpression(oded);

    if (specValue)
	printf("\tSpecialization: %s\n", specValue->toChars());
    printf("\tArgument Value: %s\n", ea ? ea->toChars() : "NULL");
}


void TemplateValueParameter::toCBuffer(OutBuffer *buf)
{
    valType->toCBuffer(buf, ident);
    if (specValue)
    {
	buf->writestring(" : ");
	specValue->toCBuffer(buf);
    }
    if (defaultValue)
    {
	buf->writestring(" = ");
	defaultValue->toCBuffer(buf);
    }
}


void *TemplateValueParameter::dummyArg()
{
    return (void *)specValue;
}


Object *TemplateValueParameter::defaultArg(Scope *sc)
{
    Expression *e;

    e = defaultValue;
    if (e)
    {
	e = e->syntaxCopy();
	e = e->semantic(sc);
    }
    return e;
}

/* ======================== TemplateInstance ================================ */

TemplateInstance::TemplateInstance(Identifier *ident)
    : ScopeDsymbol(NULL)
{
#if LOG
    printf("TemplateInstance(this = %p, ident = '%s')\n", this, ident ? ident->toChars() : "null");
#endif
    this->idents.push(ident);
    this->tempdecl = NULL;
    this->inst = NULL;
    this->argsym = NULL;
    this->aliasdecl = NULL;
    this->semanticdone = 0;
}


Dsymbol *TemplateInstance::syntaxCopy(Dsymbol *s)
{
    TemplateInstance *ti;
    int i;

    assert(!s);
    ti = new TemplateInstance((Identifier *)idents.data[0]);

    ti->idents.setDim(idents.dim);
    for (i = 1; i < idents.dim; i++)
	ti->idents.data[i] = idents.data[i];

    ti->tiargs.setDim(tiargs.dim);
    for (i = 0; i < tiargs.dim; i++)
    {
	Type *ta = isType((Object *)tiargs.data[i]);
	if (ta)
	    ti->tiargs.data[i] = ta->syntaxCopy();
	else
	{
	    Expression *ea = isExpression((Object *)tiargs.data[i]);
	    assert(ea);
	    ti->tiargs.data[i] = ea->syntaxCopy();
	}
    }

    ScopeDsymbol::syntaxCopy(ti);
    return ti;
}


void TemplateInstance::addIdent(Identifier *ident)
{
    idents.push(ident);
}

void TemplateInstance::semantic(Scope *sc)
{
#if LOG
    printf("+TemplateInstance::semantic('%s', this=%p)\n", toChars(), this);
#endif
    if (inst)		// if semantic() was already run
    {
#if LOG
    printf("-TemplateInstance::semantic('%s', this=%p)\n", inst->toChars(), inst);
#endif
	return;
    }

    assert(semanticdone == 0);
    semanticdone = 1;

#if LOG
    printf("\tdo semantic\n");
#endif

    // Run semantic on each argument, place results in tiargs[]
    for (int j = 0; j < tiargs.dim; j++)
    {   Type *ta = isType((Object *)tiargs.data[j]);
	Expression *ea;
	Dsymbol *sa;

	if (ta)
	{
	    // It might really be an Expression or an Alias
	    ta->resolve(loc, sc, &ea, &ta, &sa);
	    if (ea)
		tiargs.data[j] = ea;
	    else if (sa)
		tiargs.data[j] = sa;
	    else
	    {	assert(ta);
		tiargs.data[j] = ta;
	    }
	}
	else
	{
	    ea = isExpression((Object *)tiargs.data[j]);
	    assert(ea);
	    ea = ea->semantic(sc);
	    ea = ea->constFold();
	    tiargs.data[j] = ea;
	}
	//printf("1: tiargs.data[%d] = %p\n", j, tiargs.data[j]);
    }

    if (!tempdecl)
    {
	/* Given:
	 *    instance foo.bar.abc( ... )
	 * figure out which TemplateDeclaration foo.bar.abc refers to.
	 */
	Dsymbol *s;
	Dsymbol *scopesym;
	Identifier *id;
	int i;

	id = (Identifier *)idents.data[0];
	s = sc->search(id, &scopesym);
	if (s)
	{
#if LOG
	    printf("It's an instance of '%s'\n", s->toChars());
#endif
	    s = s->toAlias();
	    for (i = 1; i < idents.dim; i++)
	    {   Dsymbol *sm;

		id = (Identifier *)idents.data[i];
		sm = s->search(id, 0);
		if (!sm)
		{
		    s = NULL;
		    break;
		}
		s = sm->toAlias();
	    }
	}
	if (!s)
	{   error("identifier '%s' is not defined", id->toChars());
	    return;
	}

	/* It should be a TemplateDeclaration, not some other symbol
	 */
	tempdecl = s->isTemplateDeclaration();
	if (!tempdecl)
	{
	    error("%s is not a template declaration", id->toChars());
	    return;
	}
    }
    else
	assert(tempdecl->isTemplateDeclaration());

    if (!tempdecl->scope)
    {
	error("forward reference to template");
	return;
    }

    /* Since there can be multiple TemplateDeclaration's with the same
     * name, look for the best match.
     */
    TemplateDeclaration *td_ambig = NULL;
    TemplateDeclaration *td_best = NULL;
    MATCH m_best = MATCHnomatch;
    Array dedtypes;

    for (TemplateDeclaration *td = tempdecl; td; td = td->overnext)
    {
	MATCH m;

//if (tiargs.dim) printf("2: tiargs.dim = %d, data[0] = %p\n", tiargs.dim, tiargs.data[0]);

	// If more arguments than parameters,
	// then this is no match.
	if (td->parameters->dim < tiargs.dim)
	    continue;

	dedtypes.setDim(td->parameters->dim);
	m = td->matchWithInstance(this, &dedtypes, 0);
	if (!m)			// no match at all
	    continue;

#if 0
	if (m < m_best)
	    goto Ltd_best;
	if (m > m_best)
	    goto Ltd;
#else
	if (!m_best)
	    goto Ltd;
#endif
	{
	// Disambiguate by picking the most specialized TemplateDeclaration
	int c1 = td->leastAsSpecialized(td_best);
	int c2 = td_best->leastAsSpecialized(td);

	if (c1 && !c2)
	    goto Ltd;
	else if (!c1 && c2)
	    goto Ltd_best;
	else
	    goto Lambig;
	}

      Lambig:		// td_best and td are ambiguous
	td_ambig = td;
	continue;

      Ltd_best:		// td_best is the best match so far
	continue;

      Ltd:		// td is the new best match
	td_best = td;
	m_best = m;
	tdtypes.setDim(dedtypes.dim);
	memcpy(tdtypes.data, dedtypes.data, tdtypes.dim * sizeof(void *));
	continue;
    }

    if (!td_best)
    {
	error("does not match any template declaration");
	return;
    }
    if (td_ambig)
    {
	error("matches more than one template declaration");
    }

    /* The best match is td_best
     */
    tempdecl = td_best;
#if LOG
    printf("\tIt's a match with template declaration '%s'\n", tempdecl->toChars());
#endif

    /* See if there is an existing TemplateInstantiation that already
     * implements the typeargs. If so, just refer to that one instead.
     */

    for (int i = 0; i < tempdecl->instances.dim; i++)
    {
	TemplateInstance *ti = (TemplateInstance *)tempdecl->instances.data[i];
#if LOG
	printf("\tchecking for match with instance %d (%p): '%s'\n", i, ti, ti->toChars());
#endif
	assert(tdtypes.dim == ti->tdtypes.dim);

	for (int j = 0; j < tdtypes.dim; j++)
	{   Object *o1 = (Object *)tdtypes.data[j];
	    Object *o2 = (Object *)ti->tdtypes.data[j];
	    Type *t1 = isType(o1);
	    Type *t2 = isType(o2);
	    Expression *e1 = isExpression(o1);
	    Expression *e2 = isExpression(o2);
	    Dsymbol *s1 = isDsymbol(o1);
	    Dsymbol *s2 = isDsymbol(o2);

	    /* A proper implementation of the various equals() overrides
	     * should make it possible to just do o1->equals(o2), but
	     * we'll do that another day.
	     */

	    if (t1)
	    {
		if (!t2 || !t1->equals(t2))
		    goto L1;
	    }
	    else if (e1)
	    {
		if (!e2 || !e1->equals(e2))
		    goto L1;
	    }
	    else if (s1)
	    {
		if (!s2 || !s1->equals(s2))
		    goto L1;
	    }
	}

	// It's a match
	inst = ti;
	parent = ti->parent;
#if LOG
	printf("\tit's a match with instance %p\n", inst);
#endif
	return;

     L1:
	;
    }

    /* So, we need to implement 'this' instance.
     */
#if LOG
    printf("\timplement template instance '%s'\n", toChars());
#endif
    inst = this;
    tempdecl->instances.push(this);
    parent = tempdecl->parent;
    //printf("parent = '%s'\n", parent->kind());

    // This instance needs an identifier for name mangling purposes.
    // Create one by taking the template declaration name and adding
    // the type signature for it.
    {	OutBuffer buf;
	char *id;

	buf.writestring(tempdecl->ident->toChars());
	buf.writeByte('_');
	for (int i = 0; i < tiargs.dim; i++)
	{   Object *o = (Object *)tiargs.data[i];
	    //Object *o = (Object *)tdtypes.data[i];
	    Type *ta = isType(o);
	    Expression *ea = isExpression(o);
	    Dsymbol *sa = isDsymbol(o);
	    if (ta)
	    {
		assert(ta->deco);
		buf.writestring(ta->deco);
	    }
	    else if (ea)
	    {
		if (ea->op == TOKvar)
		{
		    sa = ((VarExp *)ea)->var;
		    ea = NULL;
		    goto Lsa;
		}
		buf.writeByte('_');
		buf.printf("%u", ea->toInteger());
	    }
	    else if (sa)
	    {
	      Lsa:
		Declaration *d = sa->isDeclaration();
		if (d && !d->isDataseg())
		{
		    error("cannot use local '%s' as template parameter", d->toChars());
		}
		char *p = sa->mangle();
		buf.printf("__%u_%s", strlen(p) + 1, p);
	    }
	    else
		assert(0);
	}
	id = buf.toChars();
	buf.data = NULL;
	ident = new Identifier(id, TOKidentifier);
    }

    // Add 'this' to the enclosing scope's members[] so the semantic routines
    // will get called on the instance members
#if 1
    {	Array *a;
	int i;

	if (sc->scopesym->members)
	{
	    a = sc->scopesym->members;
	}
	else
	{
	    a = sc->module->members;
	}
	for (i = 0; 1; i++)
	{
	    if (i == a->dim)
	    {
		a->push(this);
		break;
	    }
	    if (this == (Dsymbol *)a->data[i])	// if already in Array
		break;
	}
    }
#endif

    // Copy the syntax trees from the TemplateDeclaration
    members = Dsymbol::arraySyntaxCopy(tempdecl->members);

    // Create our own scope for the template parameters
    Scope *scope = tempdecl->scope;
    if (!scope)
    {
	error("forward reference to template declaration %s\n", tempdecl->toChars());
    }

#if LOG
    printf("\tcreate scope for template parameters '%s'\n", toChars());
#endif
    argsym = new ScopeDsymbol();
    argsym->parent = scope->parent;
    scope = scope->push(argsym);

    // Declare each template parameter as an alias for the argument type
    for (int i = 0; i < tdtypes.dim; i++)
    {
	TemplateParameter *tp = (TemplateParameter *)tempdecl->parameters->data[i];
	//Object *o = (Object *)tiargs.data[i];
	Object *o = (Object *)tdtypes.data[i];
	Type *targ = isType(o);
	Expression *ea = isExpression(o);
	Dsymbol *sa = isDsymbol(o);
	Dsymbol *s;

	if (targ)
	{
	    Type *tded = isType((Object *)tdtypes.data[i]);

	    assert(tded);
	    s = new AliasDeclaration(0, tp->ident, tded);
	}
	else if (sa)
	{
	    s = new AliasDeclaration(0, tp->ident, sa);
	}
	else if (ea)
	{
	    // tdtypes.data[i] always matches ea here
	    Initializer *init = new ExpInitializer(loc, ea);
	    TemplateValueParameter *tvp = tp->isTemplateValueParameter();
	    assert(tvp);

	    VarDeclaration *v = new VarDeclaration(0, tvp->valType, tp->ident, init);
	    v->storage_class = STCconst;
	    s = v;
	}
	else
	    assert(0);
	if (!scope->insert(s))
	    error("declaration %s is already defined", tp->ident->toChars());
	s->semantic(scope);
    }

    // Add members of template instance to template instance symbol table
//    parent = scope->scopesym;
    symtab = new DsymbolTable();
    for (int i = 0; i < members->dim; i++)
    {
	Dsymbol *s = (Dsymbol *)members->data[i];
#if LOG
	printf("\tadding member '%s' %p to '%s'\n", s->toChars(), s, this->toChars());
#endif
	s->addMember(this);
    }

    /* See if there is only one member of template instance, and that
     * member has the same name as the template instance.
     * If so, this template instance becomes an alias for that member.
     */
    //printf("members->dim = %d\n", members->dim);
    if (members->dim)
    {
	Dsymbol *s = (Dsymbol *)members->data[0];
	s = s->oneMember();

	// Ignore any additional template instance symbols
	for (int j = 1; j < members->dim; j++)
	{   Dsymbol *sx = (Dsymbol *)members->data[j];
	    if (sx->isTemplateInstance())
		continue;
	    s = NULL;
	    break;
	}

	if (s)
	{
	    //printf("s->kind = '%s'\n", s->kind());
	    //s->print();
	    //printf("'%s', '%s'\n", s->ident->toChars(), tempdecl->ident->toChars());
	    if (s->ident && s->ident->equals(tempdecl->ident))
	    {
		//printf("setting aliasdecl\n");
		aliasdecl = new AliasDeclaration(loc, s->ident, s);
	    }
	}
    }

    // Do semantic() analysis on template instance members
#if LOG
    printf("\tdo semantic() on template instance members '%s'\n", toChars());
#endif
    Scope *sc2;
    sc2 = scope->push(this);
    sc2->parent = this;
    for (int i = 0; i < members->dim; i++)
    {
	Dsymbol *s = (Dsymbol *)members->data[i];
	s->semantic(sc2);
    }

    /* The problem is when to parse the initializer for a variable.
     * Perhaps VarDeclaration::semantic() should do it like it does
     * for initializers inside a function.
     */
//    if (sc->parent->isFuncDeclaration())

	semantic2(sc2);

    if (sc->func)
    {
	semantic3(sc2);
    }

    sc2->pop();

    scope->pop();
#if LOG
    printf("-TemplateInstance::semantic('%s', this=%p)\n", toChars(), this);
#endif
}

void TemplateInstance::semantic2(Scope *sc)
{   int i;

    if (semanticdone >= 2)
	return;
    semanticdone = 2;
#if LOG
    printf("+TemplateInstance::semantic2('%s')\n", toChars());
#endif
    if (members)
    {
	sc = tempdecl->scope;
	assert(sc);
	sc = sc->push(argsym);
	sc = sc->push(this);
	for (i = 0; i < members->dim; i++)
	{
	    Dsymbol *s = (Dsymbol *)members->data[i];
#if LOG
printf("\tmember '%s', kind = '%s'\n", s->toChars(), s->kind());
#endif
	    s->semantic2(sc);
	}
	sc = sc->pop();
	sc->pop();
    }
#if LOG
    printf("-TemplateInstance::semantic2('%s')\n", toChars());
#endif
}

void TemplateInstance::semantic3(Scope *sc)
{   int i;

    if (semanticdone >= 3)
	return;
    semanticdone = 3;
#if LOG
    printf("TemplateInstance::semantic3('%s')\n", toChars());
#endif
    if (members)
    {
	sc = tempdecl->scope;
	sc = sc->push(argsym);
	sc = sc->push(this);
	for (i = 0; i < members->dim; i++)
	{
	    Dsymbol *s = (Dsymbol *)members->data[i];
	    s->semantic3(sc);
	}
	sc = sc->pop();
	sc->pop();
    }
}

void TemplateInstance::toObjFile()
{   int i;

#if LOG
    printf("TemplateInstance::toObjFile('%s')\n", toChars());
#endif
    if (members)
    {
	for (i = 0; i < members->dim; i++)
	{
	    Dsymbol *s = (Dsymbol *)members->data[i];
	    s->toObjFile();
	}
    }
}

void TemplateInstance::inlineScan()
{   int i;

#if LOG
    printf("TemplateInstance::inlineScan('%s')\n", toChars());
#endif
    if (members)
    {
	for (i = 0; i < members->dim; i++)
	{
	    Dsymbol *s = (Dsymbol *)members->data[i];
	    s->inlineScan();
	}
    }
}

void TemplateInstance::toCBuffer(OutBuffer *buf)
{
    int i;

    for (i = 0; i < idents.dim; i++)
    {	Identifier *id = (Identifier *)idents.data[i];

	if (i)
	    buf->writeByte('.');
	buf->writestring(id->toChars());
    }
    buf->writestring("!(");
    for (i = 0; i < tiargs.dim; i++)
    {
	if (i)
	    buf->writeByte(',');
	Object *oarg = (Object *)tiargs.data[i];
	Type *t = isType(oarg);
	Expression *e = isExpression(oarg);
	Dsymbol *s = isDsymbol(oarg);
	if (t)
	    t->toCBuffer(buf, NULL);
	else if (e)
	    e->toCBuffer(buf);
	else if (s)
	{
	    buf->writestring(s->ident->toChars());
	}
	else if (!oarg)
	{
	    buf->writestring("NULL");
	}
	else
	{
#ifdef DEBUG
	    printf("tiargs[%d] = %p\n", i, oarg);
#endif
	    assert(0);
	}
    }
    buf->writeByte(')');
}


Dsymbol *TemplateInstance::toAlias()
{
#if LOG
    printf("TemplateInstance::toAlias()\n");
#endif
    if (!inst)
    {	error("cannot resolve forward reference");
	return this;
    }

    if (inst != this)
	return inst->toAlias();

    if (aliasdecl)
	return aliasdecl->toAlias();

    return inst;
}

AliasDeclaration *TemplateInstance::isAliasDeclaration()
{
    return aliasdecl;
}

char *TemplateInstance::kind()
{
    return "template instance";
}

char *TemplateInstance::toChars()
{
    OutBuffer buf;
    char *s;

    toCBuffer(&buf);
    s = buf.toChars();
    buf.data = NULL;
    return s;
    //return s + 9;	// kludge to skip over 'instance '
}

