
// Copyright (c) 1999-2005 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <assert.h>

#include "id.h"
#include "init.h"
#include "declaration.h"
#include "identifier.h"
#include "expression.h"
#include "cond.h"
#include "module.h"
#include "template.h"

int findCondition(Array *ids, Identifier *ident)
{
    if (ids)
    {
	for (int i = 0; i < ids->dim; i++)
	{
	    char *id = (char *)ids->data[i];

	    if (strcmp(id, ident->toChars()) == 0)
		return TRUE;
	}
    }

    return FALSE;
}

/* ============================================================ */

Condition::Condition(Loc loc)
{
    this->loc = loc;
    inc = 0;
}

/* ============================================================ */

DVCondition::DVCondition(Module *mod, unsigned level, Identifier *ident)
	: Condition(0)
{
    this->mod = mod;
    this->level = level;
    this->ident = ident;
}

Condition *DVCondition::syntaxCopy()
{
    return this;	// don't need to copy
}

/* ============================================================ */

void DebugCondition::setGlobalLevel(unsigned level)
{
    global.params.debuglevel = level;
}

void DebugCondition::addGlobalIdent(char *ident)
{
    if (!global.params.debugids)
	global.params.debugids = new Array();
    global.params.debugids->push(ident);
}


DebugCondition::DebugCondition(Module *mod, unsigned level, Identifier *ident)
    : DVCondition(mod, level, ident)
{
}

int DebugCondition::include(Scope *sc, ScopeDsymbol *s)
{
    //printf("DebugCondition::include() level = %d, debuglevel = %d\n", level, global.params.debuglevel);
    if (inc == 0)
    {
	inc = 2;
	if (ident)
	{
	    if (findCondition(mod->debugids, ident))
		inc = 1;
	    else if (findCondition(global.params.debugids, ident))
		inc = 1;
	}
	else if (level <= global.params.debuglevel || level <= mod->debuglevel)
	    inc = 1;
    }
    return (inc == 1);
}

void DebugCondition::toCBuffer(OutBuffer *buf)
{
    if (ident)
	buf->printf("debug (%s)", ident->toChars());
    else
	buf->printf("debug (%u)", level);
}

/* ============================================================ */

void VersionCondition::setGlobalLevel(unsigned level)
{
    global.params.versionlevel = level;
}

void VersionCondition::checkPredefined(Loc loc, char *ident)
{
    static char* reserved[] =
    {
	"DigitalMars", "X86", "X86_64",
	"Windows", "Win32", "Win64",
	"linux",
	"LittleEndian", "BigEndian",
	"all",
	"none",
    };

    for (unsigned i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++)
    {
	if (strcmp(ident, reserved[i]) == 0)
	    goto Lerror;
    }

    if (ident[0] == 'D' && ident[1] == '_')
	goto Lerror;

    return;

  Lerror:
    error(loc, "version identifier '%s' is reserved and cannot be set", ident);
}

void VersionCondition::addGlobalIdent(char *ident)
{
    checkPredefined(0, ident);
    addPredefinedGlobalIdent(ident);
}

void VersionCondition::addPredefinedGlobalIdent(char *ident)
{
    if (!global.params.versionids)
	global.params.versionids = new Array();
    global.params.versionids->push(ident);
}


VersionCondition::VersionCondition(Module *mod, unsigned level, Identifier *ident)
    : DVCondition(mod, level, ident)
{
}

int VersionCondition::include(Scope *sc, ScopeDsymbol *s)
{
    //printf("VersionCondition::include() level = %d, versionlevel = %d\n", level, global.params.versionlevel);
    if (inc == 0)
    {
	inc = 2;
	if (ident)
	{
	    if (findCondition(mod->versionids, ident))
		inc = 1;
	    else if (findCondition(global.params.versionids, ident))
		inc = 1;
	}
	else if (level <= global.params.versionlevel || level <= mod->versionlevel)
	    inc = 1;
    }
    return (inc == 1);
}

void VersionCondition::toCBuffer(OutBuffer *buf)
{
    if (ident)
	buf->printf("version (%s)", ident->toChars());
    else
	buf->printf("version (%u)", level);
}


/**************************** StaticIfCondition *******************************/

StaticIfCondition::StaticIfCondition(Loc loc, Expression *exp)
    : Condition(loc)
{
    this->exp = exp;
}

Condition *StaticIfCondition::syntaxCopy()
{
    return new StaticIfCondition(loc, exp->syntaxCopy());
}

int StaticIfCondition::include(Scope *sc, ScopeDsymbol *s)
{
    if (inc == 0)
    {
	if (!sc)
	{
	    error(loc, "static if conditional cannot be at global scope");
	    inc = 2;
	    return 0;
	}

	Expression *e = exp->semantic(sc);
	e = e->optimize(WANTvalue);
	if (e->isBool(TRUE))
	    inc = 1;
	else if (e->isBool(FALSE))
	    inc = 2;
	else
	{
	    e->error("expression %s does not evaluate to a boolean", e->toChars());
	    inc = 2;
	}
    }
    return (inc == 1);
}

void StaticIfCondition::toCBuffer(OutBuffer *buf)
{
    buf->writestring("static if(");
    exp->toCBuffer(buf);
    buf->writeByte(')');
}


/**************************** IftypeCondition *******************************/

IftypeCondition::IftypeCondition(Loc loc, Type *targ, Identifier *id, Type *tspec)
    : Condition(loc)
{
    this->targ = targ;
    this->id = id;
    this->tspec = tspec;
}

Condition *IftypeCondition::syntaxCopy()
{
    return new IftypeCondition(loc,
	targ->syntaxCopy(),
	id,
	tspec ? tspec->syntaxCopy() : NULL);
}

int IftypeCondition::include(Scope *sc, ScopeDsymbol *sd)
{
    //printf("IftypeCondition::include()\n");
    if (inc == 0)
    {
	if (!sc)
	{
	    error(loc, "iftype conditional cannot be at global scope");
	    inc = 2;
	    return 0;
	}
	targ = targ->semantic(loc, sc);

	if (id && tspec)
	{
	    /* Evaluate to TRUE if targ matches tspec.
	     * If TRUE, declare id as an alias for the specialized type.
	     */

	    MATCH m;
	    TemplateTypeParameter tp(loc, id, NULL, NULL);

	    Array parameters;
	    parameters.setDim(1);
	    parameters.data[0] = (void *)&tp;

	    Array dedtypes;
	    dedtypes.setDim(1);

	    m = targ->deduceType(tspec, &parameters, &dedtypes);
	    if (m == MATCHnomatch)
		inc = 2;
	    else
	    {
		inc = 1;
		Type *tded = (Type *)dedtypes.data[0];
		if (!tded)
		    tded = targ;
		Dsymbol *s = new AliasDeclaration(loc, id, tded);
		s->semantic(sc);
		sc->insert(s);
		if (sd)
		    s->addMember(sc, sd);
	    }
	}
	else if (id)
	{
	    /* Declare id as an alias for type targ. Evaluate to TRUE
	     */
	    Dsymbol *s = new AliasDeclaration(loc, id, targ);
	    s->semantic(sc);
	    sc->insert(s);
	    if (sd)
		s->addMember(sc, sd);
	    inc = 1;
	}
	else if (tspec)
	{
	    /* Evaluate to TRUE if targ matches tspec
	     */
	    tspec = tspec->semantic(loc, sc);
	    //printf("targ  = %s\n", targ->toChars());
	    //printf("tspec = %s\n", tspec->toChars());
	    //if (targ->equals(tspec))
	    if (targ->implicitConvTo(tspec))
		inc = 1;
	    else
		inc = 2;
	}
	else
	     assert(0);
	//printf("inc = %d\n", inc);
    }
    return (inc == 1);
}

void IftypeCondition::toCBuffer(OutBuffer *buf)
{
    buf->writestring("iftype(");
    targ->toCBuffer(buf, id);
    if (tspec)
    {
	buf->writestring(" : ");
	tspec->toCBuffer(buf, NULL);
    }
    buf->writeByte(')');
}


