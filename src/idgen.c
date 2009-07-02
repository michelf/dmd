
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

// Program to generate string files in d data structures.
// Saves much tedious typing, and eliminates typo problems.
// Generates:
//	id.h
//	id.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

struct Msgtable
{
	char *ident;	// name to use in DMD source
	char *name;	// name in D executable
};

Msgtable msgtable[] =
{
    { "IUnknown" },
    { "Object" },
    { "object" },
    { "max" },
    { "min" },
    { "This", "this" },
    { "ctor", "_ctor" },
    { "dtor", "_dtor" },
    { "classInvariant", "_invariant" },
    { "unitTest", "_unitTest" },
    { "staticCtor", "_staticCtor" },
    { "staticDtor", "_staticDtor" },
    { "init" },
    { "size" },
    { "__sizeof", "sizeof" },
    { "alignof" },
    { "length" },
    { "offset" },
    { "ModuleInfo" },
    { "ClassInfo" },
    { "classinfo" },
    { "TypeInfo" },
    { "typeinfo" },
    { "Exception" },
    { "withSym", "__withSym" },
    { "result", "__result" },
    { "returnLabel", "__returnLabel" },
    { "delegate" },
    { "line" },
    { "empty", "" },

    { "nan" },
    { "infinity" },
    { "dig" },
    { "epsilon" },
    { "mant_dig" },
    { "max_10_exp" },
    { "max_exp" },
    { "min_10_exp" },
    { "min_exp" },
    { "re" },
    { "im" },

    { "C" },
    { "D" },
    { "Windows" },
    { "Pascal" },

    { "keys" },
    { "values" },
    { "rehash" },

    { "sort" },
    { "reverse" },
    { "dup" },

    // For inline assembler
    { "___out", "out" },
    { "___in", "in" },
    { "__int", "int" },
    { "__dollar", "$" },
    { "__LOCAL_SIZE" },

    // For operator overloads
#if 1
    { "neg",     "opNeg" },
    { "com",     "opCom" },
    { "add",     "opAdd" },
    { "sub",     "opSub" },
    { "sub_r",   "opSub_r" },
    { "mul",     "opMul" },
    { "div",     "opDiv" },
    { "div_r",   "opDiv_r" },
    { "mod",     "opMod" },
    { "mod_r",   "opMod_r" },
    { "eq",      "opEquals" },
    { "cmp",     "opCmp" },
    { "iand",    "opAnd" },
    { "ior",     "opOr" },
    { "ixor",    "opXor" },
    { "shl",     "opShl" },
    { "shl_r",   "opShl_r" },
    { "shr",     "opShr" },
    { "shr_r",   "opShr_r" },
    { "ushr",    "opUShr" },
    { "ushr_r",  "opUShr_r" },
    { "cat",     "opCat" },
    { "cat_r",   "opCat_r" },
    { "addass",  "opAddAssign" },
    { "subass",  "opSubAssign" },
    { "mulass",  "opMulAssign" },
    { "divass",  "opDivAssign" },
    { "modass",  "opModAssign" },
    { "andass",  "opAndAssign" },
    { "orass",   "opOrAssign" },
    { "xorass",  "opXorAssign" },
    { "shlass",  "opShlAssign" },
    { "shrass",  "opShrAssign" },
    { "ushrass", "opUShrAssign" },
    { "catass",  "opCatAssign" },
    { "postinc", "opPostInc" },
    { "postdec", "opPostDec" },
#else
    { "neg" },
    { "com" },
    { "add" },
    { "sub" },
    { "sub_r" },
    { "mul" },
    { "div" },
    { "div_r" },
    { "mod" },
    { "mod_r" },
    { "eq" },
    { "cmp" },
    { "iand", "and" },
    { "ior", "or" },
    { "ixor", "xor" },
    { "shl" },
    { "shl_r" },
    { "shr" },
    { "shr_r" },
    { "ushr" },
    { "ushr_r" },
    { "cat" },
    { "cat_r" },
    { "addass" },
    { "subass" },
    { "mulass" },
    { "divass" },
    { "modass" },
    { "andass" },
    { "orass" },
    { "xorass" },
    { "shlass" },
    { "shrass" },
    { "ushrass" },
    { "catass" },
    { "postinc" },
    { "postdec" },
#endif
    { "index", "opIndex" },
    { "slice", "opSlice" },
    { "call", "opCall" },

    { "classNew", "new" },
    { "classDelete", "delete" },

    // For foreach
    { "apply", "opApply" },

    // For pragma's
    { "msg" },
};


int main()
{
    FILE *fp;
    unsigned i;

    {
	fp = fopen("id.h","w");
	if (!fp)
	{   printf("can't open id.h\n");
	    exit(EXIT_FAILURE);
	}

	fprintf(fp, "// File generated by idgen.c\n");
#if __DMC__
	fprintf(fp, "#pragma once\n");
#endif
	fprintf(fp, "#ifndef DMD_ID_H\n");
	fprintf(fp, "#define DMD_ID_H 1\n");
	fprintf(fp, "struct Identifier;\n");
	fprintf(fp, "struct Id\n");
	fprintf(fp, "{\n");

	for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
	{   char *id = msgtable[i].ident;

	    fprintf(fp,"    static Identifier *%s;\n", id);
	}

	fprintf(fp, "    static void initialize();\n");
	fprintf(fp, "};\n");
	fprintf(fp, "#endif\n");

	fclose(fp);
    }

    {
	fp = fopen("id.c","w");
	if (!fp)
	{   printf("can't open id.c\n");
	    exit(EXIT_FAILURE);
	}

	fprintf(fp, "// File generated by idgen.c\n");
	fprintf(fp, "#include \"id.h\"\n");
	fprintf(fp, "#include \"identifier.h\"\n");
	fprintf(fp, "#include \"lexer.h\"\n");

	for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
	{   char *id = msgtable[i].ident;
	    char *p = msgtable[i].name;

	    if (!p)
		p = id;
	    fprintf(fp,"Identifier *Id::%s;\n", id);
	}

	fprintf(fp, "void Id::initialize()\n");
	fprintf(fp, "{\n");

	for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
	{   char *id = msgtable[i].ident;
	    char *p = msgtable[i].name;

	    if (!p)
		p = id;
	    fprintf(fp,"    %s = Lexer::idPool(\"%s\");\n", id, p);
	}

	fprintf(fp, "}\n");

	fclose(fp);
    }

    return EXIT_SUCCESS;
}
