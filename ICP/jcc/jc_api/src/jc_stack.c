
#include "jc_stack.h"

static jc_void ExtendStack(CJcLocalStack* pStack, jc_uint nSize)
{
	if(nSize)
	{
		jc_uint nExt = pStack->nSize - pStack->nPos;
		if(nExt < nSize)
			pStack->nSize += (nSize - nExt);
		pStack->nPos += nSize;
	}
}

static jc_void AlignStack(CJcLocalStack* pStack, jc_uint nAlign)
{
	jc_uint nMod;
	if(nAlign && (nMod = (pStack->nPos % nAlign)))
		ExtendStack(pStack, nAlign - nMod);
}

static jc_void CompactStack(CJcLocalStack* pStack)
{
	if(pStack->nSize > pStack->nPos)
	{
		jc_uint nPos = pStack->nPos;
		while(pStack->pVariableTable && pStack->pVariableTable->nStartPosition >= nPos)
			PopVariableTable(pStack);
		if(pStack->pVariableTable)
		{
			CJcVariableInStack *pPrev=NULL, *pVar = pStack->pVariableTable->pHead;
			while(pVar)
			{
				CJcVariableInStack * pNext = pVar->pNext;
				if(pVar->nPosition >= nPos)
				{
					if(pStack->pVariableTable->pTail == pVar)
						pStack->pVariableTable->pTail = pPrev;
					if(pPrev)
						pPrev->pNext = pNext;
					else
						pStack->pVariableTable->pHead = pNext;
					g_oInterface.Free(pVar);
				}
				else
					pPrev = pVar;
				pVar = pNext;
			}
		}
		pStack->nSize = pStack->nPos = nPos;
	}
}

jc_void InitializeLocalStack(CJcLocalStack* pStack)
{
	pStack->nPos = 0;
	pStack->nSize = 0;
	pStack->pVariableTable = NULL;
}

jc_void ClearLocalStack(CJcLocalStack* pStack)
{
	SetPosOfStack(pStack, 0);
	CompactStack(pStack);
}

jc_uint GetSizeOfStack(CJcLocalStack* pStack)
{
	return pStack->nSize;
}

jc_uint GetPosOfStack(CJcLocalStack* pStack)
{
	return pStack->nPos;
}

jc_void SetPosOfStack(CJcLocalStack* pStack, jc_uint nPos)
{
	if(nPos <= pStack->nSize)
		pStack->nPos = nPos;
}

jc_void NewVariableTable(CJcLocalStack* pStack)
{
	CJcVariableTable* pTable = New(CJcVariableTable);
	pTable->pPrev = pStack->pVariableTable;
	pTable->nStartPosition = pStack->nPos;
	pTable->pHead = pTable->pTail = NULL;
	pStack->pVariableTable = pTable;
}

jc_void PopVariableTable(CJcLocalStack* pStack)
{
	if(pStack->pVariableTable)
	{
		CJcVariableTable* pTable = pStack->pVariableTable->pPrev;
		if(pStack->nPos > pStack->pVariableTable->nStartPosition)
			SetPosOfStack(pStack, pStack->pVariableTable->nStartPosition);
		while(pStack->pVariableTable->pHead)
		{
			pStack->pVariableTable->pTail = pStack->pVariableTable->pHead->pNext;
			g_oInterface.Free(pStack->pVariableTable->pHead);
			pStack->pVariableTable->pHead = pStack->pVariableTable->pTail;
		}
		g_oInterface.Free(pStack->pVariableTable);
		pStack->pVariableTable = pTable;
	}
}

jc_uint AllocVarInStack(CJcLocalStack* pStack, jc_uint nDataLen, jc_uint nAlign, jc_uint* pVarId)
{
	CJcVariableInStack* pVar;
	if(pVarId)
		pVarId[0] = 0;
	pVar = pStack->pVariableTable->pHead;
	while(pVar)
	{
		if(!pVar->nUsed && nDataLen == pVar->nDataLen && nAlign == pVar->nAlign)
		{
			pVar->nUsed = 1;
			return pVar->nPosition;
		}
		pVar = pVar->pNext;
		if(pVarId)
			++pVarId[0];
	}
	AlignStack(pStack, nAlign);
	pVar = New(CJcVariableInStack);
	pVar->nUsed = 1;
	pVar->nDataLen = nDataLen;
	pVar->nAlign = nAlign;
	pVar->nPosition = GetPosOfStack(pStack);
	pVar->pNext = NULL;
	if(pStack->pVariableTable->pTail)
		pStack->pVariableTable->pTail->pNext = pVar;
	else
		pStack->pVariableTable->pHead = pVar;
	pStack->pVariableTable->pTail = pVar;
	ExtendStack(pStack, nDataLen);
	return pVar->nPosition;
}

jc_void FreeVarFromStack(CJcLocalStack* pStack, jc_uint nVarId)
{
	jc_uint i=0;
	CJcVariableInStack* pVar = pStack->pVariableTable->pHead;
	for(; pVar && i<nVarId; ++i)
		pVar = pVar->pNext;
	if(pVar)
		pVar->nUsed = 0;
}

