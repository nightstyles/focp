
#include "EhcBase.h"

#ifndef _EHC_STAB_H_
#define _EHC_STAB_H_

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

typedef struct CEhcStringTable
{
	ehc_char** pTable;
	ehc_int nSize;
	ehc_bool bMultiSet;
}CEhcStringTable;

EBS_API ehc_void InitStringTable(CEhcStringTable* pTable, ehc_bool bMultiSet);
EBS_API ehc_void ClearStringTable(CEhcStringTable* pTable);
EBS_API ehc_char* SaveString(CEhcStringTable* pTable, ehc_char* s);
EBS_API ehc_int FindString(CEhcStringTable* pTable, ehc_char *s);
EBS_API ehc_int RemoveString(CEhcStringTable* pTable, ehc_char *s);

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
