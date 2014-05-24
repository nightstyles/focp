
#ifndef _jc_link_h_
#define _jc_link_h_

#include "jc_obj.h"
#include "jc_stab.h"
#include "jc_error.h"

typedef struct CJcFile
{
	jc_uint nUsed;
	jc_uint nFileType;
	jc_char sFileName[256];
	jc_char* sFileData;
	jc_uint nDataLen;
	CJcSection* pCodeSegment;
	CJcSection* pDataSegment;
	CJcSection* pConstSegment;
	CJcSection* pSymbolTable;
	CJcSection* pLibraryTable;
	CJcSection* pHostTable;
	CJcSection* pEntry;
	CJcSection* pSymbolSegment;
	jc_uint nCodeOffset;
	jc_uint nDataOffset;
	jc_uint nConstOffset;
	jc_uint nSymbolOffset;
	jc_uint nLibraryOffset;
	jc_uint nHostOffset;
	struct CJcFile* pNext;
}CJcFile;

typedef struct CJcHostFile
{
	jc_uint nUsed;
	jc_char sFileName[256];
	jc_void* hModule;
	struct CJcHostFile* pNext;
}CJcHostFile;

typedef struct CJcLinker
{
	CJcFile* pHead;
	CJcFile* pTail;
	CJcSegment* pTarget;
	CJcStringTable* pSearchPath;
	CJcHostFile* pHostHead;
	CJcHostFile* pHostTail;
	jc_char sCurPath[256];
	CJcErrorSystem* pErrorSystem;
}CJcLinker;

enum
{
	JC_OBJ_FILE, JC_LIB_FILE, JC_DLL_FILE, JC_EXE_FILE
};

jc_void InitializeLinker(CJcLinker* pLinker, CJcErrorSystem* pErrorSystem, jc_char* sCurPath, jc_uint nFileType, jc_int nArgc, jc_char* sArgv[]);
jc_void DestroyLinker(CJcLinker* pLinker);
jc_void LinkJcFile(CJcLinker* pLinker, jc_uint nFileType);

#endif
