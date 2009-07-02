
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include "declaration.h"
#include "init.h"
#include "attrib.h"
#include "expression.h"

/********************************* FuncDeclaration ****************************/

FuncDeclaration::FuncDeclaration(Loc loc, Loc endloc, Identifier *id, enum STC storage_class, Type *type)
    : Declaration(id)
{
    this->storage_class = storage_class;
    this->type = type;
    this->loc = loc;
    this->endloc = endloc;
    fthrows = NULL;
    frequire = NULL;
    outId = NULL;
    vresult = NULL;
    returnLabel = NULL;
    fensure = NULL;
    fbody = NULL;
    symtab = NULL;
    vthis = NULL;
    parameters = NULL;
    labtab = NULL;
    overnext = NULL;
    vtblIndex = -1;
    hasReturnExp = 0;
    naked = 0;
    inlineStatus = ILSuninitialized;
    inlineNest = 0;
    semanticRun = 0;
}

Dsymbol *FuncDeclaration::syntaxCopy(Dsymbol *s)
{
    FuncDeclaration *f;

    if (s)
	f = (FuncDeclaration *)s;
    else
	f = new FuncDeclaration(loc, endloc, ident, (enum STC) storage_class, type->syntaxCopy());
    f->outId = outId;
    f->frequire = frequire ? frequire->syntaxCopy() : NULL;
    f->fensure  = fensure  ? fensure->syntaxCopy()  : NULL;
    f->fbody    = fbody    ? fbody->syntaxCopy()    : NULL;
    assert(!fthrows); // deprecated
    return f;
}


// Do the semantic analysis on the external interface to the function.

void FuncDeclaration::semantic(Scope *sc)
{   TypeFunction *f;
    StructDeclaration *sd;
    ClassDeclaration *cd;
    InterfaceDeclaration *id;

    //printf("FuncDeclaration::semantic(sc = %p, '%s')\n",sc,ident->toChars());
    type = type->semantic(loc, sc);
    f = dynamic_cast<TypeFunction *>(type);
    if (!f)
    {
	error("%s must be a function", toChars());
    }

    linkage = sc->linkage;
    parent = sc->scopesym;
    protection = sc->protection;
    storage_class |= sc->stc;

    if (isConst() || isAuto())
	error("functions cannot be const or auto");

    if (isAbstract() && fbody)
	error("abstract functions cannot have bodies");

#if 0
    if (isStaticConstructor() || isStaticDestructor())
    {
	if (!isStatic() || type->next->ty != Tvoid)
	    error("static constructors / destructors must be static void");
	if (f->arguments && f->arguments->dim)
	    error("static constructors / destructors must have empty parameter list");
	// BUG: check for invalid storage classes
    }
#endif

    sd = dynamic_cast<StructDeclaration *>(parent);
    if (sd)
    {
	InvariantDeclaration *inv;

	// Verify no constructors, destructors, etc.
	if (isConstructor() ||
	    dynamic_cast<DtorDeclaration *>(this) ||
	    //dynamic_cast<InvariantDeclaration *>(this) ||
	    dynamic_cast<UnitTestDeclaration *>(this)
	   )
	{
	    error("special member functions not allowed for %ss", sd->kind());
	}

	inv = dynamic_cast<InvariantDeclaration *>(this);
	if (inv)
	{
	    sd->inv = inv;
	}
    }

    id = dynamic_cast<InterfaceDeclaration *>(parent);
    if (id)
    {
	storage_class |= STCabstract;

	if (isConstructor() ||
	    dynamic_cast<DtorDeclaration *>(this) ||
	    dynamic_cast<InvariantDeclaration *>(this) ||
	    isUnitTest())
	    error("special function not allowed in interface %s", id->toChars());
	if (fbody)
	    error("function body is not abstract in interface %s", id->toChars());
    }

    cd = dynamic_cast<ClassDeclaration *>(parent);
    if (cd)
    {	int vi;
	CtorDeclaration *ctor;
	DtorDeclaration *dtor;
	InvariantDeclaration *inv;

	if (isConstructor())
	{
	    ctor = (CtorDeclaration *)this;
	    if (!cd->ctor)
		cd->ctor = ctor;
	    return;
	}

	dtor = dynamic_cast<DtorDeclaration *>(this);
	if (dtor)
	{
	    cd->dtor = dtor;
	}

	inv = dynamic_cast<InvariantDeclaration *>(this);
	if (inv)
	{
	    cd->inv = inv;
	}

	// if static function, do not put in vtbl[]
	if (!isVirtual())
	    return;

	// Find index of existing function in vtbl[] to override
	if (cd->baseClass)
	{
	    for (vi = 0; vi < cd->baseClass->vtbl.dim; vi++)
	    {
		FuncDeclaration *fdv = dynamic_cast<FuncDeclaration *>((Object *)cd->vtbl.data[vi]);

		// BUG: should give error if argument types match,
		// but return type does not?

		//printf("\tvtbl[%d]\n", vi);
		if (fdv && fdv->ident == ident && fdv->type->equals(type))
		{
		    // Override
		    //printf("\toverride %p with %p\n", fdv, this);
		    if (fdv->isFinal())
			error("cannot override final function %s", fdv->toChars());
		    cd->vtbl.data[vi] = (void *)this;
		    vtblIndex = vi;
		    goto L1;
		}
	    }
	}

	// Append to end of vtbl[]
	//printf("\tappend with %p\n", this);
	vi = cd->vtbl.dim;
	cd->vtbl.push(this);
	vtblIndex = vi;

	if (isOverride())
	{
	    error("function %s does not override any", toChars());
	}

    L1: ;
    }
}

// Do the semantic analysis on the internals of the function.

void FuncDeclaration::semantic3(Scope *sc)
{   TypeFunction *f;
    AggregateDeclaration *ad;

    //printf("FuncDeclaration::semantic3('%s.%s', sc = %p)\n", parent->toChars(), toChars(), sc);
    if (semanticRun)
	return;
    semanticRun = 1;

    f = dynamic_cast<TypeFunction *>(type);
    if (!f)
	return;

    // Check the 'throws' clause
    if (fthrows)
    {	int i;

	for (i = 0; i < fthrows->dim; i++)
	{
	    Type *t = (Type *)fthrows->data[i];

	    t = t->semantic(loc, sc);
	    if (!t->isClassHandle())
		error("can only throw classes, not %s", t->toChars());
	}
    }

    if (fbody || frequire)
    {
	// Establish function scope
	ScopeDsymbol *ss;
	Scope *sc2;

	ss = new ScopeDsymbol();
	ss->parent = sc->scopesym;
	sc2 = sc->push(ss);
	sc2->func = this;
	sc2->callSuper = 0;

	// Declare 'this'
	ad = isThis();
	if (ad)
	{   VarDeclaration *v;

	    assert(ad->handle);
	    v = new ThisDeclaration(ad->handle);
	    v->storage_class |= STCparameter | STCin;
	    v->semantic(sc2);
	    if (!sc2->insert(v))
		assert(0);
	    vthis = v;
	}

	// Declare all the function parameters as variables
	if (f->arguments)
	{   int i;

	    parameters = new Array();
	    parameters->reserve(f->arguments->dim);
	    for (i = 0; i < f->arguments->dim; i++)
	    {   Argument *arg;
		VarDeclaration *v;

		arg = (Argument *)f->arguments->data[i];
		if (!arg->ident)
		    error("no identifier for parameter %d of %s", i + 1, toChars());
		else
		{
		    v = new VarDeclaration(0, arg->type, arg->ident, NULL);
		    v->storage_class |= STCparameter;
		    switch (arg->inout)
		    {	case In:    v->storage_class |= STCin;		break;
			case Out:   v->storage_class |= STCout;		break;
			case InOut: v->storage_class |= STCin | STCout;	break;
		    }
		    v->semantic(sc2);
		    if (!sc2->insert(v))
			error("parameter %s.%s is already defined", toChars(), v->toChars());
		    else
			parameters->push(v);
		}
	    }
	}

	sc2->incontract++;

	if (frequire)
	{
	    // BUG: need to error if accessing out parameters
	    // BUG: need to treat parameters as const
	    // BUG: need to disallow returns and throws
	    // BUG: verify that all in and inout parameters are read
	    frequire = frequire->semantic(sc2);
	    labtab = NULL;		// so body can't refer to labels
	}

	if (fensure || addPostInvariant())
	{
	    if (type->next->ty == Tvoid)
	    {
		if (outId)
		    error("void functions have no result");
	    }
	    else
	    {
		if (!outId)
		    outId = Id::result;		// provide a default
	    }

	    if (outId)
	    {	// Declare result variable
		VarDeclaration *v;
		Loc loc = this->loc;

		if (fensure)
		    loc = fensure->loc;

		v = new VarDeclaration(loc, type->next, outId, NULL);
		v->noauto = 1;
		v->semantic(sc2);
		if (!sc2->insert(v))
		    error("out result %s is already defined", v->toChars());
		vresult = v;

		// vresult gets initialized with the function return value
		// in ReturnStatement::semantic()
	    }

	    // BUG: need to treat parameters as const
	    // BUG: need to disallow returns and throws
	    if (fensure)
	    {	fensure = fensure->semantic(sc2);
		labtab = NULL;		// so body can't refer to labels
	    }

	    if (!global.params.useOut)
	    {	fensure = NULL;		// discard
		vresult = NULL;
	    }

	    // Postcondition invariant
	    if (addPostInvariant())
	    {
#if 1
		ThisExp *v = new ThisExp(0);
		v->type = vthis->type;
#else
		VarExp *v = new VarExp(0, vthis);
#endif
		AssertExp *e = new AssertExp(0, v);
		ExpStatement *s = new ExpStatement(0, e);
		if (fensure)
		    fensure = new CompoundStatement(0, s, fensure);
		else
		    fensure = s;
	    }

	    if (fensure)
	    {	returnLabel = new LabelDsymbol(Id::returnLabel);
		LabelStatement *ls = new LabelStatement(0, Id::returnLabel, fensure);
		ls->isReturnLabel = 1;
		returnLabel->statement = ls;
	    }
	}

	sc2->incontract--;

	if (fbody)
	{
	    fbody = fbody->semantic(sc2);

	    if (isConstructor())
	    {
		ClassDeclaration *cd = isClassMember();
		//printf("callSuper = x%x\n", sc2->callSuper);

		if (!(sc2->callSuper & CSXany_ctor) &&
		    cd->baseClass && cd->baseClass->ctor)
		{
		    sc2->callSuper = 0;

		    // Insert implicit super() at start of fbody
		    Expression *e1 = new SuperExp(0);
		    Expression *e = new CallExp(0, e1, NULL);
		    e = e->semantic(sc2);
		    Statement *s = new ExpStatement(0, e);
		    fbody = new CompoundStatement(0, s, fbody);
		}
	    }
	    else if (!hasReturnExp && type->next->ty != Tvoid)
		error("function expected to return a value of type %s", type->next->toChars());
	}

	{
	    // Merge contracts together with body into one compound statement
	    Array *a = new Array();

	    if (frequire && global.params.useIn)
		a->push(frequire);

	    // Precondition invariant
	    if (addPreInvariant())
	    {
#if 1
		ThisExp *v = new ThisExp(0);
		v->type = vthis->type;
#else
		VarExp *v = new VarExp(0, vthis);
#endif
		AssertExp *e = new AssertExp(0, v);
		ExpStatement *s = new ExpStatement(0, e);
		a->push(s);
	    }

	    if (fbody)
		a->push(fbody);

	    if (fensure)
	    {
		a->push(returnLabel->statement);

		if (type->next->ty != Tvoid)
		{
		    // Create: return vresult;
		    assert(vresult);
		    Expression *e = new VarExp(0, vresult);
		    ReturnStatement *s = new ReturnStatement(0, e);
		    a->push(s);
		}
	    }

	    fbody = new CompoundStatement(0, a);
	}

	sc2->pop();
    }
}

void FuncDeclaration::toHBuffer(OutBuffer *buf)
{
    type->toCBuffer(buf, ident);
    buf->writeByte(';');
    buf->writenl();
}

void FuncDeclaration::toCBuffer(OutBuffer *buf)
{
    type->toCBuffer(buf, ident);
    if (fbody)
    {	buf->writenl();
	fbody->toCBuffer(buf);
    }
    else
    {	buf->writeByte(';');
	buf->writenl();
    }
}

Dsymbol *FuncDeclaration::search(Identifier *ident)
{   Dsymbol *s;

    s = symtab ? symtab->lookup(ident) : NULL;
    if (!s)
	Dsymbol::search(ident);
    return s;
}

/**********************************
 * Overload this FuncDeclaration with the new one f.
 * Return !=0 if successful; i.e. no conflict.
 */

int FuncDeclaration::overloadInsert(Dsymbol *s)
{
    FuncDeclaration **pf;
    FuncDeclaration *f;
    TypeFunction *tf1;

    //printf("FuncDeclaration::overloadInsert(%s)\n", s->toChars());
    f = dynamic_cast<FuncDeclaration *>(s);
    if (!f)
	return FALSE;
    tf1 = (TypeFunction *)f->type;
    FuncDeclaration *pthis = this;
    for (pf = &pthis; *pf; pf = &(*pf)->overnext)
    {
	// If f matches *pf, then it's a conflict
	MATCH match;
	TypeFunction *tf2;
	Array *args1;
	Array *args2;
	unsigned u;

	tf2 = (TypeFunction *)(*pf)->type;
	if (!tf1 || !tf2)		// happens with overloaded constructors
	    continue;
	if (tf1->varargs != tf2->varargs)
	    continue;

	args1 = tf1->arguments;
	args2 = tf2->arguments;

	if (args1->dim != args2->dim)
	    continue;

	for (u = 0; u < args1->dim; u++)
	{   Argument *a1 = (Argument *)args1->data[u];
	    Argument *a2 = (Argument *)args2->data[u];

	    //printf("a1->type = %s\n", a1->type->toChars());
	    //printf("a2->type = %s\n", a2->type->toChars());
	    if (!a1->type->equals(a2->type))
		goto Lcontinue;
	}
	//printf("false: conflict\n");
	return FALSE;

     Lcontinue:
	;
    }

    *pf = f;
    //printf("true: no conflict\n");
    return TRUE;
}

/********************************************
 * Decide which function matches the arguments best.
 */

FuncDeclaration *FuncDeclaration::overloadResolve(Loc loc, Array *arguments)
{
    FuncDeclaration *f;
    FuncDeclaration *lastf = NULL;
    FuncDeclaration *nextf;
    TypeFunction *tf;
    MATCH match;
    MATCH lastmatch = MATCHnomatch;
    int matchcount = 0;

#if 0
if (arguments)
{   int i;

    for (i = 0; i < arguments->dim; i++)
    {   Expression *arg;

	arg = (Expression *)arguments->data[i];
	assert(arg->type);
	printf("%s: ", arg->toChars());
	arg->type->print();
    }
}
#endif

    for (f = this; f; f = f->overnext)
    {
	tf = (TypeFunction *)f->type;
	match = (MATCH) tf->callMatch(arguments);
	//printf("match = %d\n", match);
	if (match != MATCHnomatch)
	{
	    if (match > lastmatch)
	    {
		lastmatch = match;
		lastf = f;
		matchcount = 1;
	    }
	    else if (match == lastmatch)
	    {   nextf = f;
		matchcount++;
	    }
	}
    }

    if (matchcount == 1)		// exactly one match
    {
	return lastf;
    }
    else if (lastmatch == MATCHnomatch)
    {
	OutBuffer tbuf;
	tf = (TypeFunction *)type;
	if (tf->arguments)
	{   int i;
	    OutBuffer argbuf;

	    for (i = 0; i < tf->arguments->dim; i++)
	    {   Argument *arg;

		arg = (Argument *)tf->arguments->data[i];
		argbuf.reset();
		arg->type->toCBuffer2(&argbuf, arg->ident);
		if (i)
		    tbuf.writeByte(',');
		tbuf.write(&argbuf);
	    }
	    if (tf->varargs)
	    {
		if (i)
		    tbuf.writeByte(',');
		tbuf.writestring("...");
	    }
	}

	OutBuffer buf;

	if (arguments)
	{   int i;
	    OutBuffer argbuf;

	    for (i = 0; i < arguments->dim; i++)
	    {	Expression *arg;

		arg = (Expression *)arguments->data[i];
		argbuf.reset();
		assert(arg->type);
		arg->type->toCBuffer2(&argbuf, NULL);
		if (i)
		    buf.writeByte(',');
		buf.write(&argbuf);
	    }
	}

	error(loc, "(%s) does not match argument types (%s)",
	    tbuf.toChars(), buf.toChars());
	return this;
    }
    else
    {
	error(loc, "overloads %s and %s both match argument list for %s",
		lastf->type->toChars(),
		nextf->type->toChars(),
		lastf->toChars());
	return lastf;
    }
}

/********************************
 * Labels are in a separate scope, one per function.
 */

LabelDsymbol *FuncDeclaration::searchLabel(Identifier *ident)
{   Dsymbol *s;

    if (!labtab)
	labtab = new DsymbolTable();	// guess we need one

    s = labtab->lookup(ident);
    if (!s)
    {
	s = new LabelDsymbol(ident);
	labtab->insert(s);
    }
    return (LabelDsymbol *)s;
}

AggregateDeclaration *FuncDeclaration::isThis()
{   AggregateDeclaration *ad;

    ad = NULL;
    if ((storage_class & STCstatic) == 0)
	ad = dynamic_cast<AggregateDeclaration *>(parent);
    return ad;
}

void FuncDeclaration::appendExp(Expression *e)
{   Statement *s;

    s = new ExpStatement(0, e);
    appendState(s);
}

void FuncDeclaration::appendState(Statement *s)
{   CompoundStatement *cs;

    if (!fbody)
    {	Array *a;

	a = new Array();
	fbody = new CompoundStatement(0, a);
    }
    cs = dynamic_cast<CompoundStatement *>(fbody);
    cs->statements->push(s);
}


int FuncDeclaration::isMain()
{
    return strcmp(ident->toChars(), "main") == 0 && linkage != LINKc;
}

int FuncDeclaration::isWinMain()
{
    return strcmp(ident->toChars(), "WinMain") == 0 && linkage != LINKc;
}

int FuncDeclaration::isDllMain()
{
    return strcmp(ident->toChars(), "DllMain") == 0 && linkage != LINKc;
}

int FuncDeclaration::isExport()
{
    return protection == PROTexport;
}

int FuncDeclaration::isImport()
{
    //printf("isImport()\n");
    //printf("protection = %d\n", protection);
    return (protection == PROTexport) && !fbody;
}

// Determine if function goes into virtual function pointer table

int FuncDeclaration::isVirtual()
{
    //printf("FuncDeclaration::isVirtual(%s)\n", toChars());
    //printf("%d %d %d %d\n", isStatic(), protection == PROTprivate, isConstructor(), linkage != LINKd);
    return !(isStatic() || protection == PROTprivate) &&
	!dynamic_cast<StructDeclaration *>(parent);
}

int FuncDeclaration::isAbstract()
{
    return storage_class & STCabstract;
}

int FuncDeclaration::needThis()
{
    return !isStatic() && dynamic_cast<AggregateDeclaration *>(parent);
}

int FuncDeclaration::addPreInvariant()
{
    AggregateDeclaration *ad = isThis();
    return (ad &&
	    //dynamic_cast<ClassDeclaration *>(ad) &&
	    global.params.useInvariants &&
	    !naked);
}

int FuncDeclaration::addPostInvariant()
{
    AggregateDeclaration *ad = isThis();
    return (ad &&
	    ad->inv &&
	    //dynamic_cast<ClassDeclaration *>(ad) &&
	    global.params.useInvariants &&
	    !naked);
}

/**********************************
 * Generate a FuncDeclaration for a runtime library function.
 */

FuncDeclaration *FuncDeclaration::genCfunc(Type *treturn, char *name)
{
    FuncDeclaration *fd;
    TypeFunction *tf;
    Dsymbol *s;
    Identifier *id;
    static DsymbolTable *st = NULL;

    id = Lexer::idPool(name);

    // See if already in table
    if (!st)
	st = new DsymbolTable();
    s = st->lookup(id);
    if (s)
    {
	fd = dynamic_cast<FuncDeclaration *>(s);
	assert(fd);
	assert(fd->type->next->equals(treturn));
    }
    else
    {
	tf = new TypeFunction(NULL, treturn, 0, LINKc);
	fd = new FuncDeclaration(0, 0, id, STCstatic, tf);
	fd->protection = PROTpublic;
	fd->linkage = LINKc;

	st->insert(fd);
    }
    return fd;
}

char *FuncDeclaration::kind()
{
    return "function";
}

/********************************* CtorDeclaration ****************************/

CtorDeclaration::CtorDeclaration(Loc loc, Loc endloc, Array *arguments, int varargs)
    : FuncDeclaration(loc, endloc, Id::ctor, STCundefined, NULL)
{
    this->arguments = arguments;
    this->varargs = varargs;
}

Dsymbol *CtorDeclaration::syntaxCopy(Dsymbol *s)
{
    CtorDeclaration *f;

    f = new CtorDeclaration(loc, endloc, NULL, varargs);

    f->outId = outId;
    f->frequire = frequire ? frequire->syntaxCopy() : NULL;
    f->fensure  = fensure  ? fensure->syntaxCopy()  : NULL;
    f->fbody    = fbody    ? fbody->syntaxCopy()    : NULL;
    assert(!fthrows); // deprecated

    if (arguments)
    {
	f->arguments = new Array();
	f->arguments->setDim(arguments->dim);
	for (int i = 0; i < arguments->dim; i++)
	{   Argument *arg;
	    Argument *a;

	    arg = (Argument *)arguments->data[i];
	    a = new Argument(arg->type->syntaxCopy(), arg->ident, arg->inout);
	    f->arguments->data[i] = (void *)a;
	}
    }

    return f;
}


void CtorDeclaration::semantic(Scope *sc)
{
    ClassDeclaration *cd;
    Type *tret;

    //printf("CtorDeclaration::semantic()\n");

    assert(!(sc->stc & STCstatic));

    cd = dynamic_cast<ClassDeclaration *>(sc->scopesym);
    if (!cd)
    {
	error("constructors only are for class definitions");
	tret = Type::tvoid;
    }
    else
	tret = cd->type; //->referenceTo();
    type = new TypeFunction(arguments, tret, varargs, LINKd);

    sc->flags |= SCOPEctor;
    type = type->semantic(loc, sc);
    sc->flags &= ~SCOPEctor;

    // Append:
    //	return this;
    // to the function body
    if (fbody)
    {	Expression *e;
	Statement *s;

	e = new ThisExp(0);
	s = new ReturnStatement(0, e);
	fbody = new CompoundStatement(0, fbody, s);
    }

    FuncDeclaration::semantic(sc);
}

char *CtorDeclaration::kind()
{
    return "constructor";
}

char *CtorDeclaration::toChars()
{
    return "this";
}

int CtorDeclaration::isConstructor()
{
    return TRUE;
}

int CtorDeclaration::isVirtual()
{
    return FALSE;
}

int CtorDeclaration::addPreInvariant()
{
    return FALSE;
}

int CtorDeclaration::addPostInvariant()
{
    return (vthis && global.params.useInvariants);
}


/********************************* DtorDeclaration ****************************/

DtorDeclaration::DtorDeclaration(Loc loc, Loc endloc)
    : FuncDeclaration(loc, endloc, Id::dtor, STCundefined, NULL)
{
}

Dsymbol *DtorDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);	// BUG: implement
    return NULL;
}


void DtorDeclaration::semantic(Scope *sc)
{
    ClassDeclaration *cd;
    Type *tret;

    cd = dynamic_cast<ClassDeclaration *>(sc->scopesym);
    if (!cd)
    {
	error("destructors only are for class definitions");
    }
    type = new TypeFunction(NULL, Type::tvoid, FALSE, LINKd);

    FuncDeclaration::semantic(sc);
}

int DtorDeclaration::addPreInvariant()
{
    return (vthis && global.params.useInvariants);
}

int DtorDeclaration::addPostInvariant()
{
    return FALSE;
}

/********************************* StaticCtorDeclaration ****************************/

StaticCtorDeclaration::StaticCtorDeclaration(Loc loc, Loc endloc)
    : FuncDeclaration(loc, endloc, Id::staticCtor, STCundefined, NULL)
{
}

Dsymbol *StaticCtorDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);	// BUG: implement
    return NULL;
}


void StaticCtorDeclaration::semantic(Scope *sc)
{
    ClassDeclaration *cd;
    Type *tret;

    cd = dynamic_cast<ClassDeclaration *>(sc->scopesym);
    if (!cd)
    {
    }
    type = new TypeFunction(NULL, Type::tvoid, FALSE, LINKd);

    FuncDeclaration::semantic(sc);

    // We're going to need ModuleInfo
    Module *m = getModule();
    if (m)
	m->needmoduleinfo = 1;
}

AggregateDeclaration *StaticCtorDeclaration::isThis()
{
    return NULL;
}

int StaticCtorDeclaration::isStaticConstructor()
{
    return TRUE;
}

int StaticCtorDeclaration::isVirtual()
{
    return FALSE;
}

int StaticCtorDeclaration::addPreInvariant()
{
    return FALSE;
}

int StaticCtorDeclaration::addPostInvariant()
{
    return FALSE;
}

/********************************* StaticDtorDeclaration ****************************/

StaticDtorDeclaration::StaticDtorDeclaration(Loc loc, Loc endloc)
    : FuncDeclaration(loc, endloc, Id::staticDtor, STCundefined, NULL)
{
}

Dsymbol *StaticDtorDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);	// BUG: implement
    return NULL;
}


void StaticDtorDeclaration::semantic(Scope *sc)
{
    ClassDeclaration *cd;
    Type *tret;

    cd = dynamic_cast<ClassDeclaration *>(sc->scopesym);
    if (!cd)
    {
    }
    type = new TypeFunction(NULL, Type::tvoid, FALSE, LINKd);

    FuncDeclaration::semantic(sc);

    // We're going to need ModuleInfo
    Module *m = getModule();
    if (m)
	m->needmoduleinfo = 1;
}

AggregateDeclaration *StaticDtorDeclaration::isThis()
{
    return NULL;
}

int StaticDtorDeclaration::isStaticDestructor()
{
    return TRUE;
}

int StaticDtorDeclaration::isVirtual()
{
    return FALSE;
}

int StaticDtorDeclaration::addPreInvariant()
{
    return FALSE;
}

int StaticDtorDeclaration::addPostInvariant()
{
    return FALSE;
}

/********************************* InvariantDeclaration ****************************/

InvariantDeclaration::InvariantDeclaration(Loc loc, Loc endloc)
    : FuncDeclaration(loc, endloc, Id::classInvariant, STCundefined, NULL)
{
}

Dsymbol *InvariantDeclaration::syntaxCopy(Dsymbol *s)
{
    InvariantDeclaration *id;

    assert(!s);
    id = new InvariantDeclaration(loc, endloc);
    FuncDeclaration::syntaxCopy(id);
    return id;
}


void InvariantDeclaration::semantic(Scope *sc)
{
    AggregateDeclaration *ad;
    Type *tret;

    ad = dynamic_cast<AggregateDeclaration *>(sc->scopesym);
    if (!ad)
    {
	error("invariants only are for struct/union/class definitions");
    }
    type = new TypeFunction(NULL, Type::tvoid, FALSE, LINKd);

    sc->incontract++;
    FuncDeclaration::semantic(sc);
    sc->incontract--;
}

int InvariantDeclaration::isVirtual()
{
    return FALSE;
}

int InvariantDeclaration::addPreInvariant()
{
    return FALSE;
}

int InvariantDeclaration::addPostInvariant()
{
    return FALSE;
}


/********************************* UnitTestDeclaration ****************************/

/*******************************
 * Generate unique unittest function Id so we can have multiple
 * instances per module.
 */

static Identifier *unitTestId()
{
    static int n;
    char buffer[8+3*4+1];

    sprintf(buffer,"unittest%d", n);
    n++;
    return Lexer::idPool(buffer);
}

UnitTestDeclaration::UnitTestDeclaration(Loc loc, Loc endloc)
    : FuncDeclaration(loc, endloc, unitTestId(), STCundefined, NULL)
{
}

Dsymbol *UnitTestDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);	// BUG: implement
    return NULL;
}


void UnitTestDeclaration::semantic(Scope *sc)
{
    if (global.params.useUnitTests)
    {
	Type *tret;

	type = new TypeFunction(NULL, Type::tvoid, FALSE, LINKd);
	FuncDeclaration::semantic(sc);

	// We're going to need ModuleInfo
	Module *m = getModule();
	if (m)
	    m->needmoduleinfo = 1;
    }
}

AggregateDeclaration *UnitTestDeclaration::isThis()
{
    return NULL;
}

int UnitTestDeclaration::isUnitTest()
{
    return TRUE;
}

int UnitTestDeclaration::isVirtual()
{
    return FALSE;
}

int UnitTestDeclaration::addPreInvariant()
{
    return FALSE;
}

int UnitTestDeclaration::addPostInvariant()
{
    return FALSE;
}



