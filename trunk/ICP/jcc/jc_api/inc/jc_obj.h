
#ifndef _jc_obj_h_
#define _jc_obj_h_

#include "jc_type.h"

typedef struct CJcSegment CJcSegment;
typedef struct CJcSymbolStack CJcSymbolStack;

typedef struct CJcFileHead
{
	jc_char magic[4];
	jc_uchar endian, bites;
	jc_ushort version0, version1, version2;
}CJcFileHead;

typedef struct CJcSection
{
	jc_uint tag, len;
	jc_char val[1];
}CJcSection;

enum
{
	JC_ALIGN_SECTION = 0,
	JC_CODESEG_SECTION = 1,
	JC_DATASEG_SECTION = 2,
	JC_CONSTSEG_SECTION = 3,
	JC_SYMTAB_SECTION = 4,
	JC_LIBTAB_SECTION = 5,
	JC_HOSTTAB_SECTION = 6,
	JC_ENTRY_SECTION = 7,
	JC_SYMBOLSEG_SECTION = 8,
};

enum
{
	JC_VARIABLE_SYM = 1,
	JC_FUNCTION_SYM = 2,
	JC_SHARE_SYM  = 4
};

typedef struct CJcSymbolItem
{
	jc_uint name, type, opt, arg;
}CJcSymbolItem;

CJcSegment* CreateObjectFile(CJcSymbolStack* pStack, char* sEntrySymbol);

#endif
