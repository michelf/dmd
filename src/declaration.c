
// Copyright (c) 1999-2005 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <assert.h>

#include "init.h"
#include "declaration.h"
#include "attrib.h"
#include "mtype.h"
#include "template.h"
#include "scope.h"
#include "aggregate.h"
#include "module.h"
#include "id.h"
#include "expression.h"

/********************************* Declaration ****************************/

Declaration::Declaration(Identifier *id)
    : Dsymbol(id)
{
    type = NULL;
    storage_class = STCundefined;
    protection = PROTundefined;
    linkage = LINKdefault;
}

void Declaration::semantic(Scope *sc)
{
}

char *Declaration::kind()
{
    return "declaration";
}

unsigned Declaration::size(Loc loc)
{
    assert(type);
    return type->size();
}

int Declaration::isStaticConstructor()
{
    return FALSE;
}

int Declaration::isStaticDestructor()
{
    return FALSE;
}

int Declaration::isDelete()
{
    return FALSE;
}

int Declaration::isDataseg()
{
    return FALSE;
}

int Declaration::isCodeseg()
{
    return FALSE;
}

enum PROT Declaration::prot()
{
    return protection;
}

/********************************* TypedefDeclaration ****************************/

TypedefDeclaration::TypedefDeclaration(Loc loc, Identifier *id, Type *basetype, Initializer *init)
    : Declaration(id)
{
    this->type = new TypeTypedef(this);
    this->basetype = basetype->toBasetype();
    this->init = init;
    this->sem = 0;
    this->loc = loc;
}

Dsymbol *TypedefDeclaration::syntaxCopy(Dsymbol *s)
{
    Type *basetype = this->basetype->syntaxCopy();

    Initializer *init = NULL;
    if (this->init)
	init = this->init->syntaxCopy();

    assert(!s);
    TypedefDeclaration *st;
    st = new TypedefDeclaration(loc, ident, basetype, init);
    return st;
}

void TypedefDeclaration::semantic(Scope *sc)
{
    //printf("TypedefDeclaration::semantic(%s) sem = %d\n", toChars(), sem);
    if (sem == 0)
    {	sem = 1;
	basetype = basetype->semantic(loc, sc);
	sem = 2;
	if (sc->parent->isFuncDeclaration() && init)
	    semantic2(sc);
    }
    else if (sem == 1)
    {
	error("circular definition");
    }
}

void TypedefDeclaration::semantic2(Scope *sc)
{
    //printf("TypedefDeclaration::semantic2()\n");
    if (sem == 2)
    {	sem = 3;
	if (init)
	{
	    init = init->semantic(sc, basetype);

	    ExpInitializer *ie = init->isExpInitializer();
	    if (ie)
	    {
		if (ie->exp->type == basetype)
		    ie->exp->type = type;
	    }
	}
    }
}

char *TypedefDeclaration::kind()
{
    return "typedef";
}

Type *TypedefDeclaration::getType()
{
    return type;
}

void TypedefDeclaration::toCBuffer(OutBuffer *buf)
{
    buf->writestring("typedef ");
    basetype->toCBuffer(buf, ident);
    buf->writeByte(';');
    buf->writenl();
}

/********************************* AliasDeclaration ****************************/

AliasDeclaration::AliasDeclaration(Loc loc, Identifier *id, Type *type)
    : Declaration(id)
{
    //printf("AliasDeclaration(id = '%s')\n", id->toChars());
    this->loc = loc;
    this->type = type;
    this->aliassym = NULL;
    this->overnext = NULL;
    this->inSemantic = 0;
    assert(type);
}

AliasDeclaration::AliasDeclaration(Loc loc, Identifier *id, Dsymbol *s)
    : Declaration(id)
{
    assert(s != this);
    this->loc = loc;
    this->type = NULL;
    this->aliassym = s;
    this->overnext = NULL;
    this->inSemantic = 0;
    assert(s);
}

Dsymbol *AliasDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(!s);
    AliasDeclaration *sa;
    if (type)
	sa = new AliasDeclaration(loc, ident, type->syntaxCopy());
    else
	sa = new AliasDeclaration(loc, ident, aliassym->syntaxCopy(NULL));
    return sa;
}

void AliasDeclaration::semantic(Scope *sc)
{
    //printf("AliasDeclaration::semantic() %s\n", toChars());
    if (aliassym)
    {
	if (aliassym->isTemplateInstance())
	    aliassym->semantic(sc);
	return;
    }
    this->inSemantic = 1;

    storage_class |= sc->stc & STCdeprecated;

    // Given:
    //	alias foo.bar.abc def;
    // it is not knowable from the syntax whether this is an alias
    // for a type or an alias for a symbol. It is up to the semantic()
    // pass to distinguish.
    // If it is a type, then type is set and getType() will return that
    // type. If it is a symbol, then aliassym is set and type is NULL -
    // toAlias() will return aliasssym.

    Dsymbol *s;

    if (type->ty == Tident)
    {
	TypeIdentifier *ti = (TypeIdentifier *)type;

	s = ti->toDsymbol(sc);
	if (s)
	    goto L2;			// it's a symbolic alias
    }
    else if (type->ty == Tinstance)
    {
	// Handle forms like:
	//	alias instance TFoo(int).bar.abc def;

	TypeInstance *ti = (TypeInstance *)type;

	s = ti->tempinst;
	if (s)
	{
	    s->semantic(sc);
	    s = s->toAlias();
	    if (sc->parent->isFuncDeclaration())
		s->semantic2(sc);

	    for (int i = 0; i < ti->idents.dim; i++)
	    {	Identifier *id;

		id = (Identifier *)ti->idents.data[i];
		s = s->search(id, 0);
		if (!s)			// failed to find a symbol
		    goto L1;		// it must be a type
		s = s->toAlias();
	    }
	    goto L2;
	}
    }
  L1:
    if (overnext)
	ScopeDsymbol::multiplyDefined(this, overnext);
    type = type->semantic(loc, sc);
    this->inSemantic = 0;
    return;

  L2:
    type = NULL;
    VarDeclaration *v = s->isVarDeclaration();
    if (v && v->linkage == LINKdefault)
    {
	error("forward reference of %s", v->toChars());
	s = NULL;
    }
    else
    {
	FuncDeclaration *f = s->isFuncDeclaration();
	if (f)
	{
	    if (overnext)
	    {
		FuncAliasDeclaration *fa = new FuncAliasDeclaration(f);
		if (!fa->overloadInsert(overnext))
		    ScopeDsymbol::multiplyDefined(f, overnext);
		overnext = NULL;
		s = fa;
	    }
	}
	if (overnext)
	    ScopeDsymbol::multiplyDefined(s, overnext);
	if (s == this)
	{
	    assert(global.errors);
	    s = NULL;
	}
    }
    aliassym = s;
    this->inSemantic = 0;
}

int AliasDeclaration::overloadInsert(Dsymbol *s)
{
    /* Don't know yet what the aliased symbol is, so assume it can
     * be overloaded and check later for correctness.
     */

    //printf("AliasDeclaration::overloadInsert('%s')\n", s->toChars());
    if (overnext == NULL)
    {	overnext = s;
	return TRUE;
    }
    else
    {
	return overnext->overloadInsert(s);
    }
}

char *AliasDeclaration::kind()
{
    return "alias";
}

Type *AliasDeclaration::getType()
{
    return type;
}

Dsymbol *AliasDeclaration::toAlias()
{
    //printf("AliasDeclaration::toAlias('%s', this = %p, aliassym = %p, kind = '%s')\n", toChars(), this, aliassym, aliassym->kind());
    assert(this != aliassym);
    //static int count; if (++count == 10) *(char*)0=0;
    if (inSemantic)
	error("recursive alias declaration");
    Dsymbol *s = aliassym ? aliassym->toAlias() : this;
    return s;
}

void AliasDeclaration::toCBuffer(OutBuffer *buf)
{
    buf->writestring("alias ");
    if (aliassym)
    {
	aliassym->toCBuffer(buf);
	buf->writeByte(' ');
	buf->writestring(ident->toChars());
    }
    else
	type->toCBuffer(buf, ident);
    buf->writeByte(';');
    buf->writenl();
}

/********************************* VarDeclaration ****************************/

VarDeclaration::VarDeclaration(Loc loc, Type *type, Identifier *id, Initializer *init)
    : Declaration(id)
{
#ifdef DEBUG
    if (!type)
    {	printf("VarDeclaration('%s')\n", id->toChars());
	*(char*)0=0;
    }
#endif
    assert(type);
    this->type = type;
    this->init = init;
    this->loc = loc;
    offset = 0;
    noauto = 0;
    nestedref = 0;
    inuse = 0;
}

Dsymbol *VarDeclaration::syntaxCopy(Dsymbol *s)
{
    //printf("VarDeclaration::syntaxCopy(%s)\n", toChars());

    VarDeclaration *sv;
    if (s)
    {	sv = (VarDeclaration *)s;
    }
    else
    {
	Initializer *init = NULL;
	if (this->init)
	{   init = this->init->syntaxCopy();
	    //init->isExpInitializer()->exp->print();
	    //init->isExpInitializer()->exp->dump(0);
	}

	sv = new VarDeclaration(loc, type->syntaxCopy(), ident, init);
	sv->storage_class = storage_class;
	//sv->storage_class |= storage_class & STCauto;
    }
    return sv;
}

void VarDeclaration::semantic(Scope *sc)
{
    //printf("VarDeclaration::semantic('%s', parent = '%s')\n", toChars(), sc->parent->toChars());

    type = type->semantic(loc, sc);
    type->checkDeprecated(loc, sc);
    linkage = sc->linkage;
    this->parent = sc->parent;
    //printf("this = %p, parent = %p, '%s'\n", this, parent, parent->toChars());
    protection = sc->protection;
    storage_class |= sc->stc;
    //printf("sc->stc = %x\n", sc->stc);
    //printf("storage_class = %x\n", storage_class);

    Dsymbol *parent = toParent();
    FuncDeclaration *fd = parent->isFuncDeclaration();

    Type *tb = type->toBasetype();
    if (tb->ty == Tvoid)
    {	error("voids have no value");
	type = Type::terror;
	tb = type;
    }
    if (tb->ty == Tfunction)
    {	error("cannot be declared to be a function");
	type = Type::terror;
	tb = type;
    }

    if (isConst())
    {
    }
    else if (isStatic())
    {
    }
    else if (isSynchronized())
    {
	error("variable %s cannot be synchronized", toChars());
    }
    else if (isOverride())
    {
	error("override cannot be applied to variable");
    }
    else if (isAbstract())
    {
	error("abstract cannot be applied to variable");
    }
    else
    {
	AnonymousAggregateDeclaration *aad = sc->anonAgg;
	if (aad)
	{
	    aad->addField(sc, this);
	}
	else
	{
	    AggregateDeclaration *ad = parent->isAggregateDeclaration();
	    if (ad)
		ad->addField(sc, this);
	}

	InterfaceDeclaration *id = parent->isInterfaceDeclaration();
	if (id)
	{
	    error("field not allowed in interface");
	}

	TemplateInstance *ti = parent->isTemplateInstance();
	if (ti)
	{
	    // Take care of nested templates
	    while (1)
	    {
		TemplateInstance *ti2 = ti->tempdecl->parent->isTemplateInstance();
		if (!ti2)
		    break;
		ti = ti2;
	    }

	    // If it's a member template
	    AggregateDeclaration *ad = ti->tempdecl->isMember();
	    if (ad)
	    {
		error("cannot use template to add field to aggregate '%s'", ad->toChars());
	    }
	}
    }

    if (type->isauto() && !noauto)
    {
	if (storage_class & (STCfield | STCout | STCstatic) || !fd)
	{
	    error("globals, statics, fields, inout and out parameters cannot be auto");
	}

	if (!(storage_class & STCauto))
	{
	    if (!(storage_class & STCparameter) && ident != Id::withSym)
		error("reference to auto class must be auto");
	}
    }

    if (!init && !sc->inunion && !isStatic() && !isConst() && fd &&
	!(storage_class & (STCfield | STCin | STCforeach)))
    {
	// Provide a default initializer
	//printf("Providing default initializer for '%s'\n", toChars());
	if (type->ty == Tstruct &&
	    ((TypeStruct *)type)->sym->zeroInit == 1)
	{
	    Expression *e = new IntegerExp(loc, 0, Type::tint32);
	    Expression *e1;
	    e1 = new VarExp(loc, this);
	    e = new AssignExp(loc, e1, e);
	    e->type = e1->type;
	    init = new ExpInitializer(loc, e);
	    return;
	}
	else
	{
	    init = getExpInitializer();
	}
    }

    // If inside function, there is no semantic3() call
    if (sc->func && init)
    {
	// If local variable, use AssignExp to handle all the various
	// possibilities.
	if (fd && !isStatic() && !isConst())
	{
	    ExpInitializer *ie;
	    Expression *e1;
	    Type *t;
	    int dim;

	    //printf("fd = '%s', var = '%s'\n", fd->toChars(), toChars());
	    ie = init->isExpInitializer();
	    if (!ie)
	    {
		error("is not a static and cannot have static initializer");
		return;
	    }

	    e1 = new VarExp(loc, this);

	    t = type->toBasetype();
	    if (t->ty == Tsarray)
	    {
		dim = ((TypeSArray *)t)->dim->toInteger();
		// If multidimensional static array, treat as one large array
		while (1)
		{
		    t = t->next->toBasetype();
		    if (t->ty != Tsarray)
			break;
		    dim *= ((TypeSArray *)t)->dim->toInteger();
		    e1->type = new TypeSArray(t->next, new IntegerExp(0, dim, Type::tindex));
		}
		e1 = new SliceExp(loc, e1, NULL, NULL);
	    }
	    ie->exp = new AssignExp(loc, e1, ie->exp);
	    ie->exp = ie->exp->semantic(sc);
	    ie->exp->optimize(WANTvalue);
	}
	else
	{
	    init = init->semantic(sc, type);
	    if (fd && isConst() && !isStatic())
	    {	// Make it static
		storage_class |= STCstatic;
	    }
	}
    }
}

ExpInitializer *VarDeclaration::getExpInitializer()
{
    ExpInitializer *ei;

    if (init)
	ei = init->isExpInitializer();
    else
    {
	Expression *e = type->defaultInit();
	if (e)
	    ei = new ExpInitializer(loc, e);
	else
	    ei = NULL;
    }
    return ei;
}

void VarDeclaration::semantic2(Scope *sc)
{
    //printf("VarDeclaration::semantic2('%s')\n", toChars());
    if (init && !sc->parent->isFuncDeclaration())
    {	inuse = 1;
	init = init->semantic(sc, type);
	inuse = 0;
    }
}

char *VarDeclaration::kind()
{
    return "variable";
}

void VarDeclaration::toCBuffer(OutBuffer *buf)
{
    type->toCBuffer(buf, ident);
    if (init)
    {	buf->writestring(" = ");
	init->toCBuffer(buf);
    }
    buf->writeByte(';');
    buf->writenl();
}

int VarDeclaration::needThis()
{
    return storage_class & STCfield;
}

int VarDeclaration::isImportedSymbol()
{
    if (protection == PROTexport && !init && (isStatic() || isConst() || parent->isModule()))
	return TRUE;
    return FALSE;
}

/*******************************
 * Does symbol go into data segment?
 */

int VarDeclaration::isDataseg()
{
#if 0
    printf("VarDeclaration::isDataseg(%p, '%s')\n", this, toChars());
    printf("%x, %p, %p\n", storage_class & (STCstatic | STCconst), parent->isModule(), parent->isTemplateInstance());
    printf("parent = '%s'\n", parent->toChars());
#endif
    Dsymbol *parent = this->toParent();
    return (storage_class & (STCstatic | STCconst) ||
	   parent->isModule() ||
	   parent->isTemplateInstance());
}

/******************************************
 * If a variable has an auto destructor call, return call for it.
 * Otherwise, return NULL.
 */

Expression *VarDeclaration::callAutoDtor()
{   Expression *e = NULL;

    if (storage_class & STCauto && !noauto)
    {
	for (ClassDeclaration *cd = type->isClassHandle();
	     cd;
	     cd = cd->baseClass)
	{
	    if (cd->dtor)
	    {   FuncDeclaration *fd;
		Expression *efd;
		Expression *ec;
		Array *arguments;

		fd = FuncDeclaration::genCfunc(Type::tvoid, "_d_callfinalizer");
		efd = new VarExp(loc, fd);
		ec = new VarExp(loc, this);
		arguments = new Array();
		arguments->push(ec);
		e = new CallExp(loc, efd, arguments);
		e->type = fd->type->next;
		break;
	    }
	}
    }
    return e;
}

/********************************* ClassInfoDeclaration ****************************/

ClassInfoDeclaration::ClassInfoDeclaration(ClassDeclaration *cd)
    : VarDeclaration(0, ClassDeclaration::classinfo->type, cd->ident, NULL)
{
    this->cd = cd;
    storage_class = STCstatic;
}

Dsymbol *ClassInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void ClassInfoDeclaration::semantic(Scope *sc)
{
}

/********************************* ModuleInfoDeclaration ****************************/

ModuleInfoDeclaration::ModuleInfoDeclaration(Module *mod)
    : VarDeclaration(0, Module::moduleinfo->type, mod->ident, NULL)
{
    this->mod = mod;
    storage_class = STCstatic;
}

Dsymbol *ModuleInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void ModuleInfoDeclaration::semantic(Scope *sc)
{
}

/********************************* TypeInfoDeclaration ****************************/

TypeInfoDeclaration::TypeInfoDeclaration(Type *tinfo, int internal)
    : VarDeclaration(0, Type::typeinfo->type, tinfo->getTypeInfoIdent(internal), NULL)
{
    this->tinfo = tinfo;
    storage_class = STCstatic;
    protection = PROTpublic;
    linkage = LINKc;
}

Dsymbol *TypeInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void TypeInfoDeclaration::semantic(Scope *sc)
{
    assert(linkage == LINKc);
}

/***************************** TypeInfoStructDeclaration ***********************/

TypeInfoStructDeclaration::TypeInfoStructDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoClassDeclaration ***********************/

TypeInfoClassDeclaration::TypeInfoClassDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoTypedefDeclaration *********************/

TypeInfoTypedefDeclaration::TypeInfoTypedefDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoPointerDeclaration *********************/

TypeInfoPointerDeclaration::TypeInfoPointerDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoArrayDeclaration ***********************/

TypeInfoArrayDeclaration::TypeInfoArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoStaticArrayDeclaration *****************/

TypeInfoStaticArrayDeclaration::TypeInfoStaticArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoAssociativeArrayDeclaration ************/

TypeInfoAssociativeArrayDeclaration::TypeInfoAssociativeArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoEnumDeclaration ***********************/

TypeInfoEnumDeclaration::TypeInfoEnumDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoFunctionDeclaration ********************/

TypeInfoFunctionDeclaration::TypeInfoFunctionDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoDelegateDeclaration ********************/

TypeInfoDelegateDeclaration::TypeInfoDelegateDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/********************************* ThisDeclaration ****************************/

// For the "this" parameter to member functions

ThisDeclaration::ThisDeclaration(Type *t)
   : VarDeclaration(0, t, Id::This, NULL)
{
    noauto = 1;
}

Dsymbol *ThisDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}


