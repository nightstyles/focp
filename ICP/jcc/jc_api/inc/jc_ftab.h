
#ifndef _jc_ftab_h_
#define _jc_ftab_h_

#include "jc_api.h"
#include "jc_stab.h"

typedef struct CJcFileItem
{
    jc_void* fp;                /* file descriptor */
    jc_char* sFileName;             /* file name */
	jc_char* curpath;			/* current path */
    jc_uint lineno;				/* file lineno */
    struct CJcFileItem * pNext;
}CJcFileItem;

typedef struct CJcFileTable
{
	CJcFileItem *pFileTable;
	CJcStringTable oFileNameTable, oSearchPath;
	jc_char sInitCurPath[JC_MAX_PATH];
	jc_int nFileCount;
	jc_char* sBuf;
	jc_uint nBufLen;
}CJcFileTable;

jc_void InitializeFileTable(CJcFileTable* pFileTable);
jc_void ClearFileTable(CJcFileTable* pFileTable);
jc_int PushFile(CJcFileTable* pFileTable, jc_char* sFileName);
jc_int InsertSearchPath(CJcFileTable* pFileTable, jc_char*sPath);
const jc_char* GetLineFromFileTable(CJcFileTable* pFileTable);
jc_int GetLineNo(CJcFileTable* pFileTable);
jc_char* GetFileName(CJcFileTable* pFileTable);
jc_int GetFileCount(CJcFileTable* pFileTable);
jc_void PopFile(CJcFileTable* pFileTable);
jc_int PushIncludeFile(CJcFileTable* pFileTable, jc_char* sIncFile, jc_int nMode);

#endif
