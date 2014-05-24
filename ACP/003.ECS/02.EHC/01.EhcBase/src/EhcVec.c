
#include "EhcVec.h"

extern CEhcInterface g_oInterface;

EBS_API ehc_void InitializeVector(CEhcVector* pVector)
{
	pVector->nSize = 0;
	pVector->pTable = NULL;
}

EBS_API ehc_void ClearVector(CEhcVector* pVector, FOnDestroyVectorItem fOnDestroy)
{
	ehc_uint i;
	for(i=0; i<pVector->nSize; ++i)
	{
		if(pVector->pTable[i] && fOnDestroy)
			fOnDestroy(pVector->pTable[i]);
	}
	if(pVector->pTable)
		g_oInterface.Free(pVector->pTable);
	pVector->nSize = 0;
	pVector->pTable = NULL;
}

EBS_API ehc_void Push(CEhcVector* pVector, ehc_void* pItem)
{
	ehc_void** pNewTable = g_oInterface.Realloc(pVector->pTable, (pVector->nSize+1)*sizeof(ehc_void*));
	pVector->pTable = pNewTable;
	pVector->pTable[pVector->nSize] = pItem;
	pVector->nSize++;
}

EBS_API ehc_void* Pop(CEhcVector* pVector)
{
	ehc_void *pItem = NULL;
	if(pVector->nSize)
	{
		pItem = pVector->pTable[pVector->nSize];
		pVector->nSize--;
	}
	return pItem;
}

EBS_API ehc_uint Remove(CEhcVector* pVector, ehc_uint Idx, FOnDestroyVectorItem fOnDestroy)
{
	ehc_uint i;
	if(Idx < pVector->nSize)
	{
		pVector->nSize--;
		if(fOnDestroy)
			fOnDestroy(pVector->pTable[Idx]);
		for(i=Idx; i<pVector->nSize; ++i)
			pVector->pTable[i] = pVector->pTable[i+1];
		return 0;
	}
	return 1;
}
