
#ifndef _jcvm_mm_h_
#define _jcvm_mm_h_

#include "jcvm_api.h"

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
};

enum
{
	JC_VARIABLE_SYM = 1,
	JC_FUNCTION_SYM = 2,
	JC_SHARE_SYM  = 4
};

typedef struct CJcFileHead CJcFileHead;
typedef struct CJcSection CJcSection;
typedef struct CJcCodeSegument CJcCodeSegument;
typedef struct CJcDataSegument CJcDataSegument;
typedef struct CJcConstSegument CJcConstSegument;
typedef struct CJcSymbolItem CJcSymbolItem;
typedef struct CJcSymbolTable CJcSymbolTable;
typedef struct CJcLibTable CJcLibTable;
typedef struct CJcEntry CJcEntry;
typedef struct CJcHostFile CJcHostFile;
typedef struct CJcHostFileItem CJcHostFileItem;
typedef struct CJcFile CJcFile;
typedef struct CJcFileItem CJcFileItem;
typedef struct CJcModule CJcModule;
typedef struct CJcModuleItem CJcModuleItem;
typedef struct CJcProgram CJcProgram;

struct CJcFileHead
{
	jc_char magic[4];
	jc_uchar endian, bites;
	jc_ushort version0, version1, version2;
};

struct CJcSection
{
	jc_uint tag, len;
	jc_char val[1];
};

struct CJcCodeSegument
{
	jc_uint tag;
	jc_uint csize;
	jc_char code[1];
};

struct CJcDataSegument
{
	jc_uint tag, len;
	jc_uint dsize;
};

struct CJcConstSegument
{
	jc_uint tag;
	jc_uint csize;
	jc_char data[1];
};

struct CJcSymbolItem
{
	jc_uint name, type, opt, arg;
};

struct CJcSymbolTable
{
	jc_uint tag;
	jc_uint len;
	CJcSymbolItem pSymbolTable[1];
};

struct CJcLibTable
{
	jc_uint tag;
	jc_uint len;
	jc_uint pLibTable[1];
};

struct CJcEntry
{
	jc_uint tag;
	jc_uint len;
	jc_uint addr;
};

struct CJcHostFile
{
	jc_uint nCounter;
	jc_char* sFileName;
	jc_void* pLibrary;

	CJcHostFile *pNext, *pPrev;
};

struct CJcHostFileItem
{
	CJcHostFile* pHostFile;
	CJcHostFileItem *pNext;
};

struct CJcFileItem
{
	CJcFile* pFile;
	CJcFileItem *pNext;
};

struct CJcFile
{
	jc_uint nCounter;
	jc_uint nFileType;
	jc_char* sFileName;
	jc_char* sFileData;
	jc_uint nDataLen;
	CJcCodeSegument* pCodeSegment;
	CJcDataSegument* pDataSegment;
	CJcConstSegument* pConstSegment;
	CJcSymbolTable* pSymbolTable;
	CJcLibTable* pLibraryTable;
	CJcLibTable* pHostTable;
	CJcEntry* pEntry;

	jc_char** pSymbolLinkTable;

	CJcHostFileItem* pHostFile;
	CJcFileItem* pChildren;
	CJcFile *pNext, *pPrev;
};

struct CJcModuleItem
{
	CJcModule* pModule;
	CJcModuleItem *pNext;
};

struct CJcModule
{
	jc_uint nCounter;
	CJcFile* pFile;
	CJcProgram* pProgram;
	jc_char* pDataSegment;

	CJcModuleItem *pChildren;
	CJcModule *pNext, *pPrev;
};

struct CJcProgram
{
	jc_int argc;
	jc_char** argv;
	CJcModule* pMainModule;

	CJcProgram *pNext, *pPrev;
};

CJcProgram* LoadJcVmProgram(const jc_char* sFileName);
void UnLoadJcVmProgram(CJcProgram* pProgram);

CJcModule* LoadJcVmModule(const jc_char* sFileName, CJcProgram* pProgram);
void UnLoadJcVmModule(CJcModule* pModule);

jc_char* GetJcVmDataAddr(jc_char* SS, jc_uint nArg);
jc_char* GetJcVmProcAddr(jc_char* SS, jc_uint nArg, CJcVmSegment* pSegment);

jc_uint GetJcVmError();

#endif
