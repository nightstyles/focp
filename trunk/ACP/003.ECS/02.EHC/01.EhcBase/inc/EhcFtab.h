
#include "EhcStab.h"

#ifndef _EHC_FTAB_H_
#define _EHC_FTAB_H_

typedef struct CEhcFileItem
{
    ehc_void* fp;                /* file descriptor */
    ehc_char* sFileName;             /* file name */
	ehc_char* curpath;			/* current path */
    ehc_uint lineno;				/* file lineno */
    struct CEhcFileItem * pNext;
}CEhcFileItem;

typedef struct CEhcFileTable
{
	CEhcFileItem *pFileTable;
	CEhcStringTable oFileNameTable, oSearchPath;
	ehc_char sInitCurPath[EHC_MAX_PATH];
	ehc_int nFileCount;
	ehc_char* sBuf;
	ehc_uint nBufLen;
}CEhcFileTable;

ehc_void InitializeFileTable(CEhcFileTable* pFileTable);
ehc_void ClearFileTable(CEhcFileTable* pFileTable);
ehc_int PushFile(CEhcFileTable* pFileTable, ehc_char* sFileName);
ehc_int InsertSearchPath(CEhcFileTable* pFileTable, ehc_char*sPath);
ehc_char* GetLineFromFileTable(CEhcFileTable* pFileTable);
ehc_int GetLineNo(CEhcFileTable* pFileTable);
ehc_char* GetFileName(CEhcFileTable* pFileTable);
ehc_int GetFileCount(CEhcFileTable* pFileTable);
ehc_void PopFile(CEhcFileTable* pFileTable);
ehc_int PushIncludeFile(CEhcFileTable* pFileTable, ehc_char* sIncFile, ehc_int nMode);

#endif
