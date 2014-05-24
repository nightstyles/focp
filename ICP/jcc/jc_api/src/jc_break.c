
#include "jc_break.h"
#include "jc_ins.h"

jc_void InitializeBreakStack(CJcBreakStack* pStack)
{
	pStack->pBreakTable = NULL;
}

jc_void ClearBreakStack(CJcBreakStack* pStack)
{
	while(pStack->pBreakTable)
		PopBreakTable(pStack);
}

jc_void NewBreakTable(CJcBreakStack* pStack)
{
	CJcBreakTable* pTable = New(CJcBreakTable);
	pTable->nCount = 0;
	pTable->pBreakTable = NULL;
	pTable->pPrev = pStack->pBreakTable;
	pStack->pBreakTable = pTable;
}

jc_void PopBreakTable(CJcBreakStack* pStack)
{
	CJcBreakTable* pTable = pStack->pBreakTable;
	if(pTable)
	{
		CJcBreakTable* pPrev = pTable->pPrev;
		if(pTable->pBreakTable)
			g_oInterface.Free(pTable->pBreakTable);
		g_oInterface.Free(pTable);
		pStack->pBreakTable = pPrev;
	}
}

jc_uint BreakEmpty(CJcBreakStack* pStack)
{
	return (pStack->pBreakTable == NULL);
}

jc_void BackFillBreakStack(CJcBreakStack* pStack, CJcSegment* pCodeSegment, jc_uint nLoopAddr)
{
	CJcBreakTable* pTable;
	jc_uint i, nSize;

	pTable = pStack->pBreakTable;
	if(pTable)
	{
		nSize = pTable->nCount;
		for(i=0; i<nSize; ++i)
			Emit1(pCodeSegment, (jc_short)jc_jmp, (jc_char)JC_UN, nLoopAddr, pTable->pBreakTable[i]);
	}
}

jc_uint CreateBreak(CJcBreakStack* pStack, jc_uint nBreakInsAddr)
{
	CJcBreakTable* pTable = pStack->pBreakTable;
	if(pTable)
	{
		pTable->pBreakTable = (jc_uint*)g_oInterface.Realloc
			(pTable->pBreakTable, (pTable->nCount+1)*sizeof(jc_uint));
		pTable->pBreakTable[pTable->nCount] = nBreakInsAddr;
		++pTable->nCount;
		return 0;
	}
	return 1;
}
