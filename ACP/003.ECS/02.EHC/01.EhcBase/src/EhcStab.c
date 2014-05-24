
#include "EhcStab.h"

extern CEhcInterface g_oInterface;

static ehc_void Resize(CEhcStringTable* pTable, ehc_int nSize)
{
	ehc_char ** sTab;
	ehc_int i, nCount;
	if(nSize)
		sTab = New2(ehc_char*, nSize);
	else
		sTab = NULL;
	nCount = nSize;
	if(pTable->nSize < nCount)
		nCount = pTable->nSize;
	for(i=0; i<nCount; ++i)
		sTab[i] = pTable->pTable[i];
	for(; i<nSize; ++i)
		sTab[i] = (ehc_char*)0;
	for(; i<pTable->nSize; ++i)
		g_oInterface.Free(pTable->pTable[i]);
	g_oInterface.Free(pTable->pTable);
	pTable->pTable = sTab;
	pTable->nSize = nSize;
}

EBS_API ehc_void InitStringTable(CEhcStringTable* pTable, ehc_bool bMultiSet)
{
	pTable->bMultiSet = bMultiSet;
	pTable->nSize = 0;
	pTable->pTable = NULL;
}

EBS_API ehc_void ClearStringTable(CEhcStringTable* pTable)
{
	Resize(pTable, 0);
}

EBS_API ehc_char* SaveString(CEhcStringTable* pTable, ehc_char* s)
{
	ehc_int nIdx = -1;
	if(!pTable->bMultiSet)
		nIdx = FindString(pTable, s);
	if(nIdx < 0)
	{
		ehc_char*s2 = (ehc_char*)g_oInterface.Malloc(StringLength(s)+1);
		nIdx = pTable->nSize;
		Resize(pTable, nIdx+1);
		pTable->pTable[nIdx] = s2;
		StringCopy(s2, s);
	}
	return pTable->pTable[nIdx];
}

EBS_API ehc_int FindString(CEhcStringTable* pTable, ehc_char *s)
{
	ehc_int i;
	for(i=0; i<pTable->nSize; ++i)
	{
		if(!StringCompare(s, pTable->pTable[i]))
			return i;
	}
	return -1;
}

EBS_API ehc_int RemoveString(CEhcStringTable* pTable, ehc_char *s)
{
	ehc_int i, nRet = FindString(pTable, s);
	if(nRet >= 0)
	{
		ehc_char* s1 = pTable->pTable[nRet];
		for(i=nRet; i<pTable->nSize-1; ++i)
			pTable->pTable[i] = pTable->pTable[i+1];
		pTable->pTable[i] = s1;
		Resize(pTable, pTable->nSize - 1);
	}
	return nRet;
}

