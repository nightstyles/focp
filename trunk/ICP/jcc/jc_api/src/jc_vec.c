
#include "jc_vec.h"

jc_void InitializeVector(CJcVector* pVector)
{
	pVector->nSize = 0;
	pVector->pTable = NULL;
}

jc_void ClearVector(CJcVector* pVector, FOnDestroyVectorItem fOnDestroy)
{
	jc_uint i;
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

jc_void Push(CJcVector* pVector, jc_void* pItem)
{
	jc_void** pNewTable = g_oInterface.Realloc(pVector->pTable, (pVector->nSize+1)*sizeof(jc_void*));
	pVector->pTable = pNewTable;
	pVector->pTable[pVector->nSize] = pItem;
	pVector->nSize++;
}

jc_void* Pop(CJcVector* pVector)
{
	jc_void *pItem = NULL;
	if(pVector->nSize)
	{
		pItem = pVector->pTable[pVector->nSize];
		pVector->nSize--;
	}
	return pItem;
}

jc_uint Remove(CJcVector* pVector, jc_uint Idx, FOnDestroyVectorItem fOnDestroy)
{
	jc_uint i;
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
