
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#ifndef DMD_PARSE_H
#define DMD_PARSE_H

#ifdef __DMC__
#pragma once
#endif /* __DMC__ */

#include "lexer.h"
#include "enum.h"

struct Type;
struct Expression;
struct Declaration;
struct Statement;
struct Import;
struct Initializer;
struct FuncDeclaration;
struct CtorDeclaration;
struct DtorDeclaration;
struct StaticCtorDeclaration;
struct StaticDtorDeclaration;
struct InvariantDeclaration;
struct UnitTestDeclaration;
struct NewDeclaration;
struct DeleteDeclaration;
struct DebugCondition;
struct VersionCondition;
struct Module;
struct ModuleDeclaration;
struct TemplateDeclaration;
struct TemplateInstance;
struct StaticAssert;

struct Parser : Lexer
{
    ModuleDeclaration *md;
    enum LINK linkage;
    Loc endloc;			// set to location of last right curly

    Parser(Module *module, unsigned char *base, unsigned length);

    Array *parseModule();
    Array *parseDeclDefs(int once);
    Array *parseBlock();
    TemplateDeclaration *parseTemplateDeclaration();
    TemplateInstance *parseTemplateInstance();
    StaticAssert *parseStaticAssert();
    DebugCondition *parseDebugCondition();
    VersionCondition *parseVersionCondition();
    CtorDeclaration *parseCtor();
    DtorDeclaration *parseDtor();
    StaticCtorDeclaration *parseStaticCtor();
    StaticDtorDeclaration *parseStaticDtor();
    InvariantDeclaration *parseInvariant();
    UnitTestDeclaration *parseUnitTest();
    NewDeclaration *parseNew();
    DeleteDeclaration *parseDelete();
    Array *parseParameters(int *pvarargs);
    EnumDeclaration *parseEnum();
    AggregateDeclaration *parseAggregate();
    Import *parseImport(Array *decldefs);
    Type *parseBasicType();
    Type *parseBasicType2(Type *t);
    Type *parseDeclarator(Type *t, Identifier **pident);
    Array *parseDeclaration();
    void parseContracts(FuncDeclaration *f);
    Statement *parseStatement(int flags);
    Initializer *parseInitializer();
    void check(enum TOK value);
    int isDeclaration(Token *t, int needId, enum TOK endtok, Token **pt);
    int isBasicType(Token **pt);
    int isDeclarator(Token **pt, int *haveId, enum TOK endtok);
    int isParameters(Token **pt);
    int isExpression(Token **pt);
    int isTemplateInstance(Token *t, Token **pt);

    Expression *parseExpression();
    Expression *parsePrimaryExp();
    Expression *parseUnaryExp();
    Expression *parsePostExp(Expression *e);
    Expression *parseMulExp();
    Expression *parseAddExp();
    Expression *parseShiftExp();
    Expression *parseRelExp();
    Expression *parseEqualExp();
    Expression *parseAndExp();
    Expression *parseXorExp();
    Expression *parseOrExp();
    Expression *parseAndAndExp();
    Expression *parseOrOrExp();
    Expression *parseCondExp();
    Expression *parseAssignExp();

    Array *parseArguments();
};

#endif /* DMD_PARSE_H */
