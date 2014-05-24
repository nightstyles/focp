
#include "jc_stab.h"

static jc_void Resize(CJcStringTable* pTable, jc_int nSize)
{
	jc_char ** sTab;
	jc_int i, nCount;
	if(nSize)
		sTab = New2(jc_char*, nSize);
	else
		sTab = NULL;
	nCount = nSize;
	if(pTable->nSize < nCount)
		nCount = pTable->nSize;
	for(i=0; i<nCount; ++i)
		sTab[i] = pTable->pTable[i];
	for(; i<nSize; ++i)
		sTab[i] = (jc_char*)0;
	for(; i<pTable->nSize; ++i)
		g_oInterface.Free(pTable->pTable[i]);
	g_oInterface.Free(pTable->pTable);
	pTable->pTable = sTab;
	pTable->nSize = nSize;
}

jc_void InitStringTable(CJcStringTable* pTable, jc_bool bMultiSet)
{
	pTable->bMultiSet = bMultiSet;
	pTable->nSize = 0;
	pTable->pTable = NULL;
}

jc_void ClearStringTable(CJcStringTable* pTable)
{
	Resize(pTable, 0);
}

jc_char* SaveString(CJcStringTable* pTable, jc_char* s)
{
	jc_int nIdx = -1;
	if(!pTable->bMultiSet)
		nIdx = FindString(pTable, s);
	if(nIdx < 0)
	{
		jc_char*s2 = (jc_char*)g_oInterface.Malloc(StringLength(s)+1);
		nIdx = pTable->nSize;
		Resize(pTable, nIdx+1);
		pTable->pTable[nIdx] = s2;
		StringCopy(s2, s);
	}
	return pTable->pTable[nIdx];
}

jc_int FindString(CJcStringTable* pTable, jc_char *s)
{
	jc_int i;
	for(i=0; i<pTable->nSize; ++i)
	{
		if(!StringCompare(s, pTable->pTable[i]))
			return i;
	}
	return -1;
}

jc_int RemoveString(CJcStringTable* pTable, jc_char *s)
{
	jc_int i, nRet = FindString(pTable, s);
	if(nRet >= 0)
	{
		jc_char* s1 = pTable->pTable[nRet];
		for(i=nRet; i<pTable->nSize-1; ++i)
			pTable->pTable[i] = pTable->pTable[i+1];
		pTable->pTable[i] = s1;
		Resize(pTable, pTable->nSize - 1);
	}
	return nRet;
}

