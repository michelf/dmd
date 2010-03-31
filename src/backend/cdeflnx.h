// Copyright (C) ?-1998 by Symantec
// Copyright (C) 2000-2009 by Digital Mars
// All Rights Reserved
// http://www.digitalmars.com
// Written by Walter Bright
/*
 * This source file is made available for personal use
 * only. The license is in /dmd/src/dmd/backendlicense.txt
 * For any other uses, please contact Digital Mars.
 */

#include <time.h>

#define M_UNIX 1
#define IMPLIED_PRAGMA_ONCE 1
#define MEMMODELS 1
#if __GNUC__
#define __SC__ 0
#define _MSC_VER 0
#endif

#define ERRSTREAM stderr

#define isleadbyte(c) 0

#define __cdecl __attribute__ ((__cdecl__))
#define _cdecl __attribute__ ((__cdecl__))
#define __stdcall __attribute__ ((__stdcall__))

#define __pascal
#define __near
#define  _near
#define __far
#define  _far
#define __ss
#define __cs

#if SCPP
// Need to define the token enum before any refs
#define TOKENS_ONLY     1
#include "token.h"
#undef TOKEN_H
#undef TOKENS_ONLY
#define TOKENS_ONLY     0
#endif

//
//      Error messages generated by msgsx.c
//

//#include "msgs2.h"

char * strupr(char *);

//
//      Attributes
//

//      Types of attributes
#define ATTR_LINKMOD    0x0001  // link modifier
#define ATTR_TYPEMOD    0x0002  // basic type modifier
#define ATTR_FUNCINFO   0x0004  // function information
#define ATTR_DATAINFO   0x0008  // data information
#define ATTR_TRANSU     0x0010  // transparent union
#define ATTR_IGNORED    0x0020  // attribute can be ignored
#define ATTR_WARNING    0x0040  // attribute was ignored
#define ATTR_SEGMENT    0x0080  // segment secified


//      attribute location in code
#define ALOC_DECSTART   0x001   // start of declaration
#define ALOC_SYMDEF     0x002   // symbol defined
#define ALOC_PARAM      0x004   // follows function parameter
#define ALOC_FUNC       0x008   // follows function declaration

#define ATTR_LINK_MODIFIERS (mTYconst|mTYvolatile|mTYcdecl|mTYstdcall)
#define ATTR_CAN_IGNORE(a) \
        (((a) & (ATTR_LINKMOD|ATTR_TYPEMOD|ATTR_FUNCINFO|ATTR_DATAINFO|ATTR_TRANSU)) == 0)
#define LNX_CHECK_ATTRIBUTES(a,x) assert(((a) & ~(x|ATTR_IGNORED|ATTR_WARNING)) == 0)

#if SCPP
#include "optdata.h"
#endif

#define TRIGRAPHS (ANSI || OPT_IS_SET(OPTtrigraphs))
