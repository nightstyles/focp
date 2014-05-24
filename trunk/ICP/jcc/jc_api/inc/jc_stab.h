
#ifndef _jc_stab_h_
#define _jc_stab_h_

#include "jc_type.h"

typedef struct CJcStringTable
{
	jc_char** pTable;
	jc_int nSize;
	jc_bool bMultiSet;
}CJcStringTable;

jc_void InitStringTable(CJcStringTable* pTable, jc_bool bMultiSet);
jc_void ClearStringTable(CJcStringTable* pTable);
jc_char* SaveString(CJcStringTable* pTable, jc_char* s);
jc_int FindString(CJcStringTable* pTable, jc_char *s);
jc_int RemoveString(CJcStringTable* pTable, jc_char *s);

#endif
