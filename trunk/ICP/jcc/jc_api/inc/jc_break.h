
#ifndef _jc_break_h_
#define _jc_break_h_

#include "jc_codeseg.h"

typedef struct CJcBreakTable
{
	struct CJcBreakTable* pPrev;
	jc_uint nCount;
	jc_uint* pBreakTable;
}CJcBreakTable;

typedef struct CJcBreakStack
{
	CJcBreakTable* pBreakTable;
}CJcBreakStack;

jc_void InitializeBreakStack(CJcBreakStack* pStack);
jc_void ClearBreakStack(CJcBreakStack* pStack);

jc_void NewBreakTable(CJcBreakStack* pStack);
jc_void PopBreakTable(CJcBreakStack* pStack);

jc_uint BreakEmpty(CJcBreakStack* pStack);

jc_void BackFillBreakStack(CJcBreakStack* pStack, CJcSegment* pCodeSegment, jc_uint nLoopAddr);
jc_uint CreateBreak(CJcBreakStack* pStack, jc_uint nBreakInsAddr);

#endif
