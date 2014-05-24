
#ifndef _jc_switch_h_
#define _jc_switch_h_

#include "jc_dataseg.h"
#include "jc_hash.h"

typedef struct CJcSwitchTable
{
	struct CJcSwitchTable* pPrev;
	jc_uint nType;
	jc_uint nDefaultCase;
	CJcHashTable* pSwitchTable;/*jc_ulong, jc_uint*/
}CJcSwitchTable;

typedef struct CJcCaseItem
{
	jc_uint nCount;
	jc_uint nStart;
	jc_uint nDefault;
}CJcCaseItem;

typedef struct CJcSwitchStack
{
	CJcSwitchTable* pSwitchTable;
}CJcSwitchStack;

jc_void InitializeSwitchStack(CJcSwitchStack* pStack);
jc_void ClearSwitchStack(CJcSwitchStack* pStack);

jc_void NewSwitchTable(CJcSwitchStack* pStack, jc_uint nType);
jc_void PopSwitchTable(CJcSwitchStack* pStack);

jc_uint WriteDefault(CJcSwitchStack* pStack, jc_uint nCaseAddr, jc_uint bCheckError);
jc_uint WriteCase(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable, jc_uint nCaseAddr, CJcExpress* pCaseExpr, jc_uint nSwitchExpressType);

jc_uint BackFillSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable);

#endif
