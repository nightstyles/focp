
#ifndef _jc_continue_h_
#define _jc_continue_h_

#include "jc_codeseg.h"

typedef struct CJcContinueTable
{
	struct CJcContinueTable* pPrev;
	jc_uint nCount;
	jc_uint* pContinueTable;
}CJcContinueTable;

typedef struct CJcContinueStack
{
	CJcContinueTable* pContinueTable;
}CJcContinueStack;

jc_void InitializeContinueStack(CJcContinueStack* pStack);
jc_void ClearContinueStack(CJcContinueStack* pStack);

jc_void NewContinueTable(CJcContinueStack* pStack);
jc_void PopContinueTable(CJcContinueStack* pStack);

jc_uint ContinueEmpty(CJcContinueStack* pStack);

jc_void BackFillContinueStack(CJcContinueStack* pStack, CJcSegment* pCodeSegment, jc_uint nLoopAddr);
jc_uint CreateContinue(CJcContinueStack* pStack, jc_uint nContinueInsAddr);

#endif
