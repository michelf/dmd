
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

#include "mars.h"
#include "dsymbol.h"
#include "aggregate.h"
#include "identifier.h"
#include "module.h"
#include "mtype.h"
#include "expression.h"
#include "statement.h"
#include "declaration.h"
#include "id.h"
#include "scope.h"

/****************************** Dsymbol ******************************/

Dsymbol::Dsymbol()
{
    //printf("Dsymbol::Dsymbol(%p)\n", this);
    this->ident = NULL;
    this->c_ident = NULL;
    this->parent = NULL;
    this->csym = NULL;
    this->isym = NULL;
    this->loc = 0;
}

Dsymbol::Dsymbol(Identifier *ident)
{
    //printf("Dsymbol::Dsymbol(%p, ident)\n", this);
    this->ident = ident;
    this->c_ident = NULL;
    this->parent = NULL;
    this->csym = NULL;
    this->isym = NULL;
    this->loc = 0;
}

int Dsymbol::equals(Object *o)
{   Dsymbol *s;

    if (this == o)
	return TRUE;
    s = (Dsymbol *)(o);
    if (s && ident->equals(s->ident))
	return TRUE;
    return FALSE;
}

/**************************************
 * Copy the syntax.
 * Used for template instantiations.
 * If s is NULL, allocate the new object, otherwise fill it in.
 */

Dsymbol *Dsymbol::syntaxCopy(Dsymbol *s)
{
    print();
    printf("%s %s\n", kind(), toChars());
    assert(0);
    return NULL;
}

char *Dsymbol::toChars()
{
    return ident ? ident->toChars() : (char *)"__anonymous";
}

char *Dsymbol::toPrettyChars()
{   Dsymbol *p;
    char *s;
    char *q;
    size_t len;

    if (!parent)
	return toChars();

    len = 0;
    for (p = this; p; p = p->parent)
	len += strlen(p->toChars()) + 1;

    s = (char *)mem.malloc(len);
    q = s + len - 1;
    *q = 0;
    for (p = this; 1; p = p->parent)
    {
	char *t = p->toChars();
	len = strlen(t);
	q -= len;
	memcpy(q, t, len);
	if (q == s)
	    break;
	q--;
	*q = '.';
    }
    return s;
}

char *Dsymbol::locToChars()
{
    OutBuffer buf;
    char *p;

    Module *m = getModule();

    if (m && m->srcfile)
	loc.filename = m->srcfile->toChars();
    return loc.toChars();
}

char *Dsymbol::kind()
{
    return "symbol";
}

/*********************************
 * If this symbol is really an alias for another,
 * return that other.
 */

Dsymbol *Dsymbol::toAlias()
{
    return this;
}

Dsymbol *Dsymbol::toParent()
{
    return parent ? parent->pastMixin() : NULL;
}

Dsymbol *Dsymbol::pastMixin()
{
    Dsymbol *s = this;

    while (s && s->isTemplateMixin())
	s = s->parent;
    return s;
}

int Dsymbol::isAnonymous()
{
    return ident ? 0 : 1;
}

void Dsymbol::semantic(Scope *sc)
{
    error("%p has no semantic routine", this);
}

void Dsymbol::semantic2(Scope *sc)
{
    // Most Dsymbols have no further semantic analysis needed
}

void Dsymbol::semantic3(Scope *sc)
{
    // Most Dsymbols have no further semantic analysis needed
}

void Dsymbol::inlineScan()
{
    // Most Dsymbols have no further semantic analysis needed
}

Dsymbol *Dsymbol::search(Identifier *ident, int flags)
{
    //printf("Dsymbol::search(this=%p,%s, ident='%s')\n", this, toChars(), ident->toChars());
    return NULL;
//    error("%s.%s is undefined",toChars(), ident->toChars());
//    return this;
}

int Dsymbol::overloadInsert(Dsymbol *s)
{
    //printf("Dsymbol::overloadInsert('%s')\n", s->toChars());
    return FALSE;
}

void Dsymbol::toCBuffer(OutBuffer *buf)
{
    buf->printf("Dsymbol '%s' to C file", toChars());
    buf->writenl();
}

unsigned Dsymbol::size(Loc loc)
{
    error("Dsymbol '%s' has no size\n", toChars());
    return 0;
}

int Dsymbol::isforwardRef()
{
    return FALSE;
}

AggregateDeclaration *Dsymbol::isThis()
{
    return NULL;
}

ClassDeclaration *Dsymbol::isClassMember()	// are we a member of a class?
{
    Dsymbol *parent = toParent();
    if (parent && parent->isClassDeclaration())
	return (ClassDeclaration *)parent;
    return NULL;
}

void Dsymbol::defineRef(Dsymbol *s)
{
    assert(0);
}

int Dsymbol::isExport()
{
    return FALSE;
}

int Dsymbol::isImportedSymbol()
{
    return FALSE;
}

int Dsymbol::isDeprecated()
{
    return FALSE;
}

LabelDsymbol *Dsymbol::isLabel()		// is this a LabelDsymbol()?
{
    return NULL;
}

AggregateDeclaration *Dsymbol::isMember()	// is this a member of an AggregateDeclaration?
{
    Dsymbol *parent = toParent();
    return parent ? parent->isAggregateDeclaration() : NULL;
}

Type *Dsymbol::getType()
{
    return NULL;
}

int Dsymbol::needThis()
{
    return FALSE;
}

void Dsymbol::addMember(Scope *sc, ScopeDsymbol *sd)
{
    //printf("Dsymbol::addMember(this = %p, '%s' scopesym = '%s')\n", this, toChars(), sd->toChars());
    //printf("Dsymbol::addMember(this = %p, '%s' sd = %p, sd->symtab = %p)\n", this, toChars(), sd, sd->symtab);
    parent = sd;
    if (!isAnonymous())		// no name, so can't add it to symbol table
    {
	if (!sd->symtab->insert(this))	// if name is already defined
	{
	    Dsymbol *s2;

	    s2 = sd->symtab->lookup(ident);
	    if (!s2->overloadInsert(this))
	    {
		sd->multiplyDefined(this, s2);
	    }
	}
	if (sd->isAggregateDeclaration() || sd->isEnumDeclaration())
	{
	    if (ident == Id::__sizeof || ident == Id::alignof)
		error(".%s property cannot be redefined", ident->toChars());
	}
    }
}

void Dsymbol::error(const char *format, ...)
{
    //printf("Dsymbol::error()\n");
    if (!global.gag)
    {
	char *p = locToChars();

	if (*p)
	    printf("%s: ", p);
	mem.free(p);

	if (isAnonymous())
	    printf("%s ", kind());
	else
	    printf("%s %s ", kind(), toPrettyChars());

	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");
	fflush(stdout);
    }
    global.errors++;

    //fatal();
}

void Dsymbol::error(Loc loc, const char *format, ...)
{
    if (!global.gag)
    {
	char *p = loc.toChars();
	if (!*p)
	    p = locToChars();

	if (*p)
	    printf("%s: ", p);
	mem.free(p);

	printf("%s %s ", kind(), toPrettyChars());

	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");
	fflush(stdout);
    }

    global.errors++;

    //fatal();
}

void Dsymbol::checkDeprecated(Loc loc, Scope *sc)
{
    if (!global.params.useDeprecated && isDeprecated())
    {
	// Don't complain if we're inside a deprecated symbol's scope
	for (Dsymbol *sp = sc->parent; sp; sp = sp->parent)
	{   if (sp->isDeprecated())
		return;
	}

	for (; sc; sc = sc->enclosing)
	{
	    if (sc->scopesym && sc->scopesym->isDeprecated())
		return;
	}

	error(loc, "is deprecated");
    }
}

/**********************************
 * Determine which Module a Dsymbol is in.
 */

Module *Dsymbol::getModule()
{
    Module *m;
    Dsymbol *s;

    //printf("Dsymbol::getModule()\n");
    s = this;
    while (s)
    {
	//printf("\ts = '%s'\n", s->toChars());
	m = s->isModule();
	if (m)
	    return m;
	s = s->parent;
    }
    return NULL;
}

/*************************************
 */

enum PROT Dsymbol::prot()
{
    return PROTpublic;
}

/*************************************
 * Do syntax copy of an array of Dsymbol's.
 */


Array *Dsymbol::arraySyntaxCopy(Array *a)
{

    Array *b = NULL;
    if (a)
    {
	b = a->copy();
	for (int i = 0; i < b->dim; i++)
	{
	    Dsymbol *s = (Dsymbol *)b->data[i];

	    s = s->syntaxCopy(NULL);
	    b->data[i] = (void *)s;
	}
    }
    return b;
}

/********************************* ScopeDsymbol ****************************/

ScopeDsymbol::ScopeDsymbol()
    : Dsymbol()
{
    members = NULL;
    symtab = NULL;
    imports = NULL;
    prots = NULL;
}

ScopeDsymbol::ScopeDsymbol(Identifier *id)
    : Dsymbol(id)
{
    members = NULL;
    symtab = NULL;
    imports = NULL;
    prots = NULL;
}

Dsymbol *ScopeDsymbol::syntaxCopy(Dsymbol *s)
{
    //printf("ScopeDsymbol::syntaxCopy('%s')\n", toChars());

    ScopeDsymbol *sd;
    if (s)
	sd = (ScopeDsymbol *)s;
    else
	sd = new ScopeDsymbol(ident);
    sd->members = arraySyntaxCopy(members);
    return sd;
}

Dsymbol *ScopeDsymbol::search(Identifier *ident, int flags)
{   Dsymbol *s;
    int i;

    //printf("%s->ScopeDsymbol::search(ident='%s', flags=x%x)\n", toChars(), ident->toChars(), flags);
    // Look in symbols declared in this module
    s = symtab ? symtab->lookup(ident) : NULL;
    if (s)
    {
	//printf("\ts = '%s.%s'\n",toChars(),s->toChars());
    }
    else if (imports)
    {
	// Look in imported modules
	for (i = 0; i < imports->dim; i++)
	{   ScopeDsymbol *ss = (ScopeDsymbol *)imports->data[i];
	    Dsymbol *s2;

	    // If private import, don't search it
	    if (flags & 1 && prots[i] == PROTprivate)
		continue;

	    //printf("\tscanning import '%s', prots = %d, isModule = %p, isImport = %p\n", ss->toChars(), prots[i], ss->isModule(), ss->isImport());
	    s2 = ss->search(ident, ss->isModule() ? 1 : 0);
	    if (!s)
		s = s2;
	    else if (s2 && s != s2)
	    {
		ss->multiplyDefined(s, s2);
		break;
	    }
	}
	if (s)
	{
	    Declaration *d = s->isDeclaration();
	    if (d && d->protection == PROTprivate)
		error("%s is private", d->toPrettyChars());
	}
    }
    return s;
}

void ScopeDsymbol::importScope(ScopeDsymbol *s, enum PROT protection)
{
    //printf("%s->ScopeDsymbol::importScope(%s, %d)\n", toChars(), s->toChars(), protection);

    // No circular or redundant import's
    if (s != this)
    {
	if (!imports)
	    imports = new Array();
	else
	{
	    for (int i = 0; i < imports->dim; i++)
	    {   ScopeDsymbol *ss;

		ss = (ScopeDsymbol *) imports->data[i];
		if (ss == s)
		{
		    if (protection > prots[i])
			prots[i] = protection;	// upgrade access
		    return;
		}
	    }
	}
	imports->push(s);
	prots = (unsigned char *)mem.realloc(prots, imports->dim * sizeof(prots[0]));
	prots[imports->dim - 1] = protection;
    }
}

int ScopeDsymbol::isforwardRef()
{
    return (members == NULL);
}

void ScopeDsymbol::defineRef(Dsymbol *s)
{
    ScopeDsymbol *ss;

    ss = s->isScopeDsymbol();
    members = ss->members;
    ss->members = NULL;
}

void ScopeDsymbol::multiplyDefined(Dsymbol *s1, Dsymbol *s2)
{
    //printf("s1 = '%s'\n", s1->toChars());
    //printf("s2 = '%s', parent = %p\n", s2->toChars(), s2->parent);
#if 1
    s1->error("conflicts with %s at %s",
	s2->toPrettyChars(),
	s2->locToChars());
#else
    s1->error("symbol %s conflicts with %s at %s",
	s1->toPrettyChars(),
	s2->toPrettyChars(),
	s2->locToChars());
#endif
}

Dsymbol *ScopeDsymbol::nameCollision(Dsymbol *s)
{
    Dsymbol *sprev;

    // Look to see if we are defining a forward referenced symbol

    sprev = symtab->lookup(s->ident);
    assert(sprev);
    if (s->equals(sprev))		// if the same symbol
    {
	if (s->isforwardRef())		// if second declaration is a forward reference
	    return sprev;
	if (sprev->isforwardRef())
	{
	    sprev->defineRef(s);	// copy data from s into sprev
	    return sprev;
	}
    }
    multiplyDefined(s, sprev);
    return sprev;
}

char *ScopeDsymbol::kind()
{
    return "ScopeDsymbol";
}


/****************************** WithScopeSymbol ******************************/

WithScopeSymbol::WithScopeSymbol(WithStatement *withstate)
    : ScopeDsymbol()
{
    this->withstate = withstate;
}

Dsymbol *WithScopeSymbol::search(Identifier *ident, int flags)
{
    // Acts as proxy to the with class declaration
    return withstate->exp->type->toDsymbol(NULL)->search(ident, 0);
}

/****************************** ArrayScopeSymbol ******************************/

ArrayScopeSymbol::ArrayScopeSymbol(Expression *e)
    : ScopeDsymbol()
{
    assert(e->op == TOKindex || e->op == TOKslice);
    exp = e;
}

Dsymbol *ArrayScopeSymbol::search(Identifier *ident, int flags)
{
    if (ident == Id::length || ident == Id::dollar)
    {	VarDeclaration **pvar;

	if (exp->op == TOKindex)
	{
	    IndexExp *ie = (IndexExp *)exp;

	    pvar = &ie->lengthVar;
	}
	else if (exp->op == TOKslice)
	{
	    SliceExp *se = (SliceExp *)exp;

	    pvar = &se->lengthVar;
	}
	else
	    return NULL;
	if (!*pvar)
	{
	    VarDeclaration *v = new VarDeclaration(0, Type::tsize_t, Id::dollar, NULL);

	    *pvar = v;
	}
	return (*pvar);
    }
    return NULL;
}


/****************************** DsymbolTable ******************************/

DsymbolTable::DsymbolTable()
{
    tab = new StringTable;
}

DsymbolTable::~DsymbolTable()
{
    delete tab;
}

Dsymbol *DsymbolTable::lookup(Identifier *ident)
{   StringValue *sv;

#ifdef DEBUG
    assert(ident);
    assert(tab);
#endif
    sv = tab->lookup((char*)ident->string, ident->len);
    return (Dsymbol *)(sv ? sv->ptrvalue : NULL);
}

Dsymbol *DsymbolTable::insert(Dsymbol *s)
{   StringValue *sv;
    Identifier *ident;

    //printf("DsymbolTable::insert(this = %p, '%s')\n", this, s->ident->toChars());
    ident = s->ident;
#ifdef DEBUG
    assert(ident);
    assert(tab);
#endif
    sv = tab->insert(ident->toChars(), ident->len);
    if (!sv)
	return NULL;		// already in table
    sv->ptrvalue = s;
    return s;
}

Dsymbol *DsymbolTable::insert(Identifier *ident, Dsymbol *s)
{   StringValue *sv;

    //printf("DsymbolTable::insert()\n");
    sv = tab->insert(ident->toChars(), ident->len);
    if (!sv)
	return NULL;		// already in table
    sv->ptrvalue = s;
    return s;
}

Dsymbol *DsymbolTable::update(Dsymbol *s)
{   StringValue *sv;
    Identifier *ident;

    ident = s->ident;
    sv = tab->update(ident->toChars(), ident->len);
    sv->ptrvalue = s;
    return s;
}




