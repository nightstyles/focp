
#include "jc_goto.h"
#include "jc_ins.h"

static jc_uint JC_CALL HashFunction(jc_void* k)
{
	return GetCrc32Hash(k, StringLength((const jc_char*)k));
}

static jc_int JC_CALL HashKeyEqual(jc_void* k1, jc_void* k2)
{
	const jc_char* key1 = (const jc_char*)k1;
	const jc_char* key2 = (const jc_char*)k2;
	return !StringCompare(key1, key2);
}

jc_void InitializeGotoStack(CJcGotoStack* pStack)
{
	pStack->pGotoTable = NULL;
}

jc_void ClearGotoStack(CJcGotoStack* pStack)
{
	while(pStack->pGotoTable)
		PopGotoTable(pStack);
}

CJcGotoStack* CreateGotoStack()
{
	CJcGotoStack* pStack = New(CJcGotoStack);
	InitializeGotoStack(pStack);
	return pStack;
}

jc_void DestroyGotoStack(CJcGotoStack * pStack)
{
	ClearGotoStack(pStack);
	g_oInterface.Free(pStack);
}

jc_void NewGotoTable(CJcGotoStack * pStack)
{
	CJcGotoTable* pTable = New(CJcGotoTable);
	pTable->pPrev = pStack->pGotoTable;
	pTable->nCount = 0;
	pTable->pGotoTable = NULL;
	pTable->pLabelTable = CreateHashTable(NULL, HashFunction, HashKeyEqual, g_oInterface.Free, g_oInterface.Free);
	pStack->pGotoTable = pTable;
}

jc_void PopGotoTable(CJcGotoStack * pStack)
{
	jc_uint i;
	CJcGoto* pGoto;
	CJcGotoTable *pPrev, * pTable;

	pTable = pStack->pGotoTable;
	if(pTable)
	{
		pPrev = pTable->pPrev;
		DestroyHashTable(pTable->pLabelTable);
		if(pTable->pGotoTable)
		{
			for(i=0; i<pTable->nCount; ++i)
			{
				pGoto = pTable->pGotoTable+i;
				ClearString(&pGoto->oLabelName);
			}
			g_oInterface.Free(pTable->pGotoTable);
		}
		g_oInterface.Free(pTable);
		pStack->pGotoTable = pPrev;
	}
}

jc_uint BackFillGotoStack(CJcGotoStack * pStack, CJcSegment* pCodeSegment, CJcGotoFillError *pError)
{
	CJcGoto* pGoto;
	jc_uint nSize, i, *pGotoAddr;
	CJcGotoTable* pTable = pStack->pGotoTable;
	if(!pTable)
		return 1;
	nSize = pTable->nCount;
	for(i=0; i<nSize; ++i)
	{
		pGoto = pTable->pGotoTable + i;
		pGotoAddr = (jc_uint*)SearchInHashTable(pTable->pLabelTable, pGoto->oLabelName.pStr);
		if(pGotoAddr)
			Emit1(pCodeSegment, jc_jmp, JC_UN, *pGotoAddr, pGoto->nInsAddr);
		else
		{
			pError->sFileName = pGoto->sFileName;
			pError->nLine = pGoto->nLine;
			pError->nCol = pGoto->nCol;
			pError->sLabel = pGoto->oLabelName.pStr;
			return 1;
		}
	}
	return 0;
}

jc_uint NewGotoLabel(CJcGotoStack * pStack, jc_char* sLabelName, jc_uint nOffset)
{
	jc_char* skey;
	jc_uint *pGotoAddr, *pOffset;
	CJcGotoTable* pTable = pStack->pGotoTable;
	if(!pTable)
		return 1;
	pGotoAddr = (jc_uint*)SearchInHashTable(pTable->pLabelTable, sLabelName);
	if(pGotoAddr)
		return 1;
	skey = (jc_char*)g_oInterface.Malloc(StringLength(sLabelName)+1);
	StringCopy(skey, sLabelName);
	pOffset = New(jc_uint);
	pOffset[0] = nOffset;
	InsertIntoHashTable(pTable->pLabelTable, skey, pOffset);
	return 0;
}

jc_void CreateGoto(CJcGotoStack * pStack, jc_uint nGoInsAddr, jc_char* sFileName, jc_char* sLabelName, jc_int nLine, jc_int nCol)
{
	CJcGoto * pGoto;
	CJcGotoTable* pTable = pStack->pGotoTable;
	if(pTable)
	{
		pTable->pGotoTable = (CJcGoto*)g_oInterface.Realloc(pTable->pGotoTable, (pTable->nCount+1)*sizeof(CJcGoto));
		pGoto = pTable->pGotoTable + pTable->nCount;
		InitializeString(&pGoto->oLabelName);
		CoverString(&pGoto->oLabelName, sLabelName);
		pGoto->nInsAddr = nGoInsAddr;
		pGoto->sFileName = sFileName;
		pGoto->nLine = nLine;
		pGoto->nCol = nCol;
		++pTable->nCount;
	}
}
