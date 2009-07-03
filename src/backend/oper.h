// Copyright (C) 1985-1998 by Symantec
// Copyright (C) 2000-2009 by Digital Mars
// All Rights Reserved
// http://www.digitalmars.com
// Written by Walter Bright
/*
 * This source file is made available for personal use
 * only. The license is in /dmd/src/dmd/backendlicense.txt
 * For any other uses, please contact Digital Mars.
 */


#if __SC__
#pragma once
#endif

#ifndef OPER_H
#define OPER_H	1

#if TARGET_MAC
#include "TGoper.h"
#endif

enum OPER
{
	OPunde,			/* place holder for undefined operator	*/

	OPadd,
	OPmin,
	OPmul,
	OPdiv,
	OPmod,
	OPshr,			// unsigned right shift
	OPshl,
	OPand,
	OPxor,
	OPor,
	OPashr,			// signed right shift
	OPnot,
	OPbool,			/* "booleanize"			*/
	OPcom,
	OPcond,
	OPcomma,
	OPoror,
	OPandand,
	OPbit,			/* ref to bit field		*/
	OPind,			/* *E				*/
	OPaddr,			/* &E				*/
	OPneg,			/* unary -			*/
	OPuadd,			/* unary +			*/
#if TX86
	OPvoid,			// where casting to void is not a no-op
	OPabs,			/* absolute value		*/
	OPsqrt,			/* square root			*/
	OPrndtol,		// round to short, long, long long (inline 8087 only)
	OPsin,			// sine
	OPcos,			// cosine
	OPrint,			// round to int
	OPscale,		// ldexp
	OPyl2x,			// y * log2(x)
	OPyl2xp1,		// y * log2(x + 1)
	OPstrlen,		/* strlen()			*/
	OPstrcpy,		/* strcpy()			*/
	OPstrcat,		/* strcat()			*/
	OPstrcmp,		/* strcmp()			*/
	OPmemcpy,
	OPmemcmp,
	OPmemset,
	OPsetjmp,		// setjmp()
	OPremquo,		// / and % in one operation

	OPbsf,			// bit scan forward
	OPbsr,			// bit scan reverse
	OPbt,			// bit test
	OPbtc,			// bit test and complement
	OPbtr,			// bit test and reset
	OPbts,			// bit test and set
	OPbswap,		// swap bytes
#endif

	OPstreq,		/* structure assignment		*/

	OPnegass,		// x = -x
	OPpostinc,		/* x++				*/
	OPpostdec,		/* x--				*/

	OPeq,
	OPaddass,
	OPminass,
	OPmulass,
	OPdivass,
	OPmodass,
	OPshrass,
	OPshlass,
	OPandass,
	OPxorass,
	OPorass,

/* Convert from token to assignment operator	*/
#define asgtoktoop(tok)	((int) (tok) + ((int)OPeq - (int) TKeq))

	OPashrass,

	/* relational operators	(in same order as corresponding tokens) */
#define RELOPMIN	((int)OPle)
	OPle,
	OPgt,
	OPlt,
	OPge,
	OPeqeq,
	OPne,

	OPunord,	/* !<>=		*/
	OPlg,		/* <>		*/
	OPleg,		/* <>=		*/
	OPule,		/* !>		*/
	OPul,		/* !>=		*/
	OPuge,		/* !<		*/
	OPug,		/* !<=		*/
	OPue,		/* !<>		*/
	OPngt,
	OPnge,
	OPnlt,
	OPnle,
	OPord,
	OPnlg,
	OPnleg,
	OPnule,
	OPnul,
	OPnuge,
	OPnug,
	OPnue,

#define rel_toktoop(tk)	((enum OPER)((int)tk - (int)TKle + (int)OPle))

/***************** End of relational operators ******************/
#define CNVOPMIN	(OPnue+1)

// parallel array inconvtab[] in cgelem.c)

#if TX86

/* Convert from conversion operator to conversion index		*/
#define convidx(op)	((int)(op) - CNVOPMIN)

/*	8,16,32,64	integral type of unspecified sign
	s,u		signed/unsigned
	f,d,ld		float/double/long double
	np,fp,vp,f16p	near pointer/far pointer/handle pointer/far16 pointer
	cvp		const handle pointer
 */

	OPb_8,		// convert bit to byte
	OPd_s32,
	OPs32_d,
	OPd_s16,
	OPs16_d,
	OPd_u16,
	OPu16_d,
	OPd_u32,
	OPu32_d,
	OPd_s64,
	OPs64_d,
	OPd_u64,
	OPu64_d,
	OPd_f,
	OPf_d,
	OPvp_fp,
	OPcvp_fp,	// const handle * => far *
	OPs16_32,	// short to long
	OPu16_32,	// unsigned short to long
	OP32_16,	// long to short
	OPu8_16,	// unsigned char to short
	OPs8_16,	// signed char to short
	OP16_8,		// short to 8 bits
	OPu32_64,	// unsigned long to long long
	OPs32_64,	// long to long long
	OP64_32,	// long long to long
#define OPsfltdbl	OPunde
#define OPdblsflt	OPunde
	OPoffset,	// get offset of far pointer
	OPnp_fp,	// convert near pointer to far
	OPnp_f16p,	// from 0:32 to 16:16
	OPf16p_np,	// from 16:16 to 0:32
	OPld_d,
	OPd_ld,
	OPld_u64,
#else
TARGET_CONVERSION_OPS
#endif

#define CNVOPMAX	(OPc_r-1)
#define convidx(op)	((int)(op) - CNVOPMIN)
/* Convert from conversion operator to conversion index		*/

#if 1
// The old conversion operators - retain until we get the code fixed
#define OPlngdbl	OPs32_d
#define OPdblint	OPd_s16
#define OPintdbl	OPs16_d
#define OPdbluns	OPd_u16
#define OPunsdbl	OPu16_d
#define OPdblulng	OPd_u32
#define OPulngdbl	OPu32_d
#define OPdblllng	OPd_s64
#define OPllngdbl	OPs64_d
#define OPdblullng	OPd_u64
#define OPdblflt	OPd_f
#define OPvptrfptr	OPvp_fp
#define OPcvptrfptr	OPcvp_fp	// const handle * => far *
#define OPshtlng	OPs16_32	// short to long
#define OPushtlng	OPu16_32	// unsigned short to long
#define OPlngsht	OP32_16		// long to short
#define OPu8int		OPu8_16		// unsigned char to short
#define OPs8int		OPs8_16		// signed char to short
#define OPint8		OP16_8		// short to 8 bits
#define OPulngllng	OPu32_64	// unsigned long to long long
#define OPlngllng	OPs32_64	// long to long long
#define OPllnglng	OP64_32		// long long to long
#define OPsfltdbl	OPunde
#define OPdblsflt	OPunde
#define OPoffset	OPoffset	// get offset of far pointer
#define OPptrlptr	OPnp_fp		// convert near pointer to far
#define OPtofar16	OPnp_f16p	// from 0:32 to 16:16
#define OPfromfar16	OPf16p_np	// from 16:16 to 0:32

#endif

/***************** End of conversion operators ******************/

	OPc_r,		// complex to real
	OPc_i,		// complex to imaginary
	OPmsw,		// top 32 bits of 64 bit word (32 bit code gen)
			// top 16 bits of 32 bit word (16 bit code gen)

	OPparam,		/* function parameter separator	*/
	OPcall,			/* binary function call		*/
	OPucall,		/* unary function call		*/
	OPcallns,		// binary function call, no side effects
	OPucallns,		// unary function call, no side effects

	OPsizeof,		/* for forward-ref'd structs	*/
	OPstrctor,		/* call ctor on struct param	*/
	OPstrthis,		// 'this' pointer for OPstrctor
	OPstrpar,		/* structure func param		*/
	OPconst,		/* constant			*/
	OPrelconst,		/* constant that contains an address */
	OPvar,			/* variable			*/
	OPreg,			// register (used in inline asm operand expressions)
	OPcolon,		/* : as in ?:			*/
	OPcolon2,		// alternate version with different EH semantics
	OPstring,		/* address of string		*/
	OPasm,			/* in-line assembly code	*/
	OPinfo,			// attach info (used to attach ctor/dtor
	OPhalt,			// insert HLT instruction
				// info for exception handling)
	OPctor,
	OPdtor,
	OPmark,
	OPpair,			// build register pair, E1 is lsb, E2 = msb
	OPrpair,		// build reversed register pair, E1 is msb, E2 = lsb
	OPframeptr,		// load pointer to base of frame
	OPgot,			// load pointer to global offset table

	// Jupiter operators
	OParray,		// access Jupiter array, left is handle, right is index
	OParraylength,		// evaluates array handle into array length
	OPfield,		// access Jupiter object field, left is handle, right is offset
	OPnewarray,		// allocate Jupiter array, left is dimension, right is type
	OPmultinewarray,	// allocate multidimensional Jupiter array
				// left is dimensions, right is (numdims,type signature)
	OPinstanceof,		// left is class id, right is handle
	OPfinalinstanceof,      // left is class id, right is handle
	OPcheckcast,		// left is class id, right is handle
	OPhstring,		// handle to static string
	OPnullcheck,		// check if pointer is null

#if TX86
	OPinp,			/* input from I/O port		*/
	OPoutp,			/* output to I/O port		*/
#endif
	/* C++ operators */
	OPnew,			// operator new
	OPanew,			// operator new[]
	OPdelete,		// operator delete
	OPadelete,		// operator delete[]
	OPbrack,		/* [] subscript			*/
	OParrow,		/* for -> overloading		*/
	OParrowstar,		/* for ->* overloading		*/
	OPpreinc,		/* ++x overloading		*/
	OPpredec,		/* --x overloading		*/

#ifdef TARGET_INLINEFUNC_OPS
	TARGET_INLINEFUNC_OPS
#endif

#if (TARGET_POWERPC)
	OPeieio,
#endif
	
	OPMAX			/* 1 past last operator		*/
};
typedef enum OPER OPER;		/* needed for optabgen		*/

/************************************
 * Determine things about relational operators.
 */

extern unsigned char rel_not[];
extern unsigned char rel_swap[];
extern unsigned char rel_integral[];
extern unsigned char rel_exception[];
extern unsigned char rel_unord[];

#define rel_not(op)		rel_not[(int)(op) - RELOPMIN]
#define rel_swap(op)		rel_swap[(int)(op) - RELOPMIN]
#define rel_integral(op)	rel_integral[(int)(op) - RELOPMIN]
#define rel_exception(op)	rel_exception[(int)(op) - RELOPMIN]
#define rel_unord(op)		rel_unord[(int)(op) - RELOPMIN]


/**********************************
 * Various types of operators:
 *	OTbinary	binary
 *	OTunary		unary
 *	OTleaf		leaf
 *	OTcommut	commutative (e1 op e2) == (e2 op e1)
 *			(assoc == !=)
 *	OTassoc		associative (e1 op (e2 op e3)) == ((e1 op e2) op e3)
 *			(also commutative)
 *	OTassign	assignment = op= i++ i-- i=-i str=
 *	OTpost		post inc or post dec operator
 *	OTeop0e		if (e op 0) => e
 *	OTeop00		if (e op 0) => 0
 *	OTeop1e		if (e op 1) => e
 *	OTsideff	there are side effects to the operator (assign call
 *			post ?: && ||)
 *	OTconv		type conversion operator that could appear on lhs of
 *			assignment operator
 *	OTlogical	logical operator (result is 0 or 1)
 *	OTwid		high order bits of operation are irrelevant
 *	OTopeq		an op= operator
 *	OTop		an operator that has a corresponding op=
 *	OTcall		function call
 *	OTrtol		operators that evaluate right subtree first then left
 *	OTrel		== != < <= > >= operators
 *	OTrel2		< <= > >= operators
 *	OTdef		definition operator (assign call post asm)
 *	OTae		potential common subexpression operator
 *	OTexp		expression elem
 *	OTboolnop	operation is a nop if boolean result is desired
 */

#if TX86
extern const unsigned char optab1[OPMAX],optab2[OPMAX],optab3[OPMAX];
extern const unsigned char opcost[OPMAX];
#else
extern unsigned char optab1[OPMAX],optab2[OPMAX];
#endif
/* optab1[]	*/	/* Use byte arrays to avoid index scaling	*/
#define _OTbinary	1
#define _OTunary	2
#define _OTcommut	4
#define _OTassoc	8
#define _OTsideff	0x10
#define _OTeop0e	0x20
#define _OTeop00	0x40
#define _OTeop1e	0x80

/* optab2[]	*/
#define _OTlogical	1
#define _OTwid		2
#define _OTcall		4
#define _OTrtol		8
#define _OTassign	0x10
#define _OTdef		0x20
#define _OTae		0x40
#define _OTexp		0x80

#if TX86
// optab3[]
#define _OTboolnop	1
#endif
#define OTbinary(op)	(optab1[op]&_OTbinary)
#define OTunary(op)	(optab1[op]&_OTunary)
#define OTleaf(op)	(!(optab1[op]&(_OTunary|_OTbinary)))
#define OTcommut(op)	(optab1[op]&_OTcommut)
#define OTassoc(op)	(optab1[op]&_OTassoc)
#define OTassign(op)	(optab2[op]&_OTassign)
#define OTpost(op)	((op) == OPpostinc || (op) == OPpostdec)
#define OTeop0e(op)	(optab1[op]&_OTeop0e)
#define OTeop00(op)	(optab1[op]&_OTeop00)
#define OTeop1e(op)	(optab1[op]&_OTeop1e)
#define OTsideff(op)	(optab1[op]&_OTsideff)
#define OTconv(op)	((op) >= CNVOPMIN && (op) <= CNVOPMAX)
#define OTlogical(op)	(optab2[op]&_OTlogical)
#define OTwid(op)	(optab2[op]&_OTwid)
#define OTopeq(op)	((op) >= OPaddass && (op) <= OPashrass)
#define OTop(op)	((op) >= OPadd && (op) <= OPor)
#define OTcall(op)	(optab2[op]&_OTcall)
#define OTrtol(op)	(optab2[op]&_OTrtol)
#define OTrel(op)	((op) >= OPle && (op) <= OPnue)
#define OTrel2(op)	((op) >= OPle && (op) <= OPge)
#define OTdef(op)	(optab2[op]&_OTdef)
#define OTae(op)	(optab2[op]&_OTae)
#define OTexp(op)	(optab2[op]&_OTexp)
#if TX86
#define OTboolnop(op)	(optab3[op]&_OTboolnop)
#define OTcalldef(op)	(OTcall(op) || (op) == OPstrcpy || (op) == OPstrcat || (op) == OPmemcpy)
#else
#define OTcalldef(op)	(OTcall(op))
#endif

/* Convert op= to op	*/
#define opeqtoop(opx)	((opx) - OPaddass + OPadd)

/* Convert op to op=	*/
#define optoopeq(opx)	((opx) - OPadd + OPaddass)

/***************************
 * Determine properties of an elem.
 * EBIN		binary node?
 * EUNA		unary node?
 * EOP		operator node (unary or binary)?
 * ERTOL	right to left evaluation (left to right is default)
 * Eunambig	unambiguous definition elem?
 */

#define	EBIN(e)	(OTbinary((e)->Eoper))
#define EUNA(e) (OTunary((e)->Eoper))

/* ERTOL(e) is moved to el.c	*/

#if KEEPBITFIELDS
#define Elvalue(e)	(((e)->E1->Eoper == OPbit) ? (e)->E1->E1 : (e)->E1)
#define Eunambig(e)	(OTassign((e)->Eoper) && \
			    ((e)->E1->Eoper == OPvar || \
				((e)->E1->Eoper == OPbit && \
				 (e)->E1->E1->Eoper == OPvar)))
#else
#define Elvalue(e)	((e)->E1)
#define Eunambig(e)	(OTassign((e)->Eoper) && \
			    (e)->E1->Eoper == OPvar)
#endif

#define	EOP(e)	(!OTleaf((e)->Eoper))

#endif /* OPER_H */

