
#include "jc_continue.h"
#include "jc_ins.h"

jc_void InitializeContinueStack(CJcContinueStack* pStack)
{
	pStack->pContinueTable = NULL;
}

jc_void ClearContinueStack(CJcContinueStack* pStack)
{
	while(pStack->pContinueTable)
		PopContinueTable(pStack);
}

jc_void NewContinueTable(CJcContinueStack* pStack)
{
	CJcContinueTable* pTable = New(CJcContinueTable);
	pTable->nCount = 0;
	pTable->pContinueTable = NULL;
	pTable->pPrev = pStack->pContinueTable;
	pStack->pContinueTable = pTable;
}

jc_void PopContinueTable(CJcContinueStack* pStack)
{
	CJcContinueTable* pTable = pStack->pContinueTable;
	if(pTable)
	{
		CJcContinueTable* pPrev = pTable->pPrev;
		if(pTable->pContinueTable)
			g_oInterface.Free(pTable->pContinueTable);
		g_oInterface.Free(pTable);
		pStack->pContinueTable = pPrev;
	}
}

jc_uint ContinueEmpty(CJcContinueStack* pStack)
{
	return (pStack->pContinueTable == NULL);
}

jc_void BackFillContinueStack(CJcContinueStack* pStack, CJcSegment* pCodeSegment, jc_uint nLoopAddr)
{
	CJcContinueTable* pTable;
	jc_uint nSize, i;

	pTable = pStack->pContinueTable;
	if(pTable)
	{
		nSize = pTable->nCount;
		for(i=0; i<nSize; ++i)
			Emit1(pCodeSegment, jc_jmp, JC_UN, nLoopAddr, pTable->pContinueTable[i]);
	}
}

jc_uint CreateContinue(CJcContinueStack* pStack, jc_uint nContinueInsAddr)
{
	CJcContinueTable* pTable = pStack->pContinueTable;
	if(pTable)
	{
		pTable->pContinueTable = (jc_uint*)g_oInterface.Realloc
			(pTable->pContinueTable, (pTable->nCount+1)*sizeof(jc_uint));
		pTable->pContinueTable[pTable->nCount] = nContinueInsAddr;
		++pTable->nCount;
		return 0;
	}
	return 1;
}
