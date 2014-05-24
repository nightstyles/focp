
#include "jc_switch.h"

static jc_uint JC_CALL HashFunction(jc_void* k)
{
	return GetCrc32Hash(k, sizeof(jc_ulong));
}

static jc_int JC_CALL HashKeyEqual(jc_void* k1, jc_void* k2)
{
	jc_ulong key1 = *(jc_ulong*)k1;
	jc_ulong key2 = *(jc_ulong*)k2;
	return (key1==key2);
}

jc_void InitializeSwitchStack(CJcSwitchStack* pStack)
{
	pStack->pSwitchTable = NULL;
}

jc_void ClearSwitchStack(CJcSwitchStack* pStack)
{
	while(pStack->pSwitchTable)
		PopSwitchTable(pStack);
}

jc_void NewSwitchTable(CJcSwitchStack* pStack, jc_uint nType)
{
	CJcSwitchTable* pTable = New(CJcSwitchTable);
	pTable->pPrev = pStack->pSwitchTable;
	pTable->nType = nType;
	pTable->nDefaultCase = 0xFFFFFFFF;
	pTable->pSwitchTable = CreateHashTable(NULL, HashFunction, HashKeyEqual, g_oInterface.Free, g_oInterface.Free);
	pStack->pSwitchTable = pTable;
}

jc_void PopSwitchTable(CJcSwitchStack* pStack)
{
	CJcSwitchTable* pTable = pStack->pSwitchTable;
	if(pTable)
	{
		CJcSwitchTable* pPrev = pTable->pPrev;
		DestroyHashTable(pTable->pSwitchTable);
		g_oInterface.Free(pTable);
		pStack->pSwitchTable = pPrev;
	}
}

jc_uint WriteDefault(CJcSwitchStack* pStack, jc_uint nCaseAddr, jc_uint bCheckError)
{
	CJcSwitchTable* pTable = pStack->pSwitchTable;
	if(!pTable)
		return 1;
	if(pTable->nDefaultCase != 0xFFFFFFFF)
	{
		if(bCheckError)
			return 1;
	}
	else
		pTable->nDefaultCase = nCaseAddr;
	return 0;
}

jc_uint WriteCase(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable, jc_uint nCaseAddr, CJcExpress* pCaseExpr, jc_uint nSwitchExpressType)
{
	CJcVal v;
	jc_ulong* pCase;
	jc_uint* pCaseAddr;
	CJcSwitchTable* pTable = pStack->pSwitchTable;
	if(!pTable)
		return 1;
	if(GetConstValue(pCaseExpr, &v, nSwitchExpressType, 0))
		return 1;
	if((jc_uint*)SearchInHashTable(pTable->pSwitchTable, &v.ul))
		return 1;
	pCase = New(jc_ulong);
	pCaseAddr = New(jc_uint);
	*pCase = v.ul;
	*pCaseAddr = nCaseAddr;
	InsertIntoHashTable(pTable->pSwitchTable, pCase, pCaseAddr);
	return 0;
}

static jc_uint BackFillByteToSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable)
{
	jc_uchar* pCase;
	jc_char* pVal;
	jc_uint* pCaseAddr;
	CJcCaseItem* pTable;
	CJcHashEntry* pItem;
	jc_uint i, nCount, nSize, nAlign, nRet;

	nSize = sizeof(CJcCaseItem);
	nAlign = sizeof(jc_uint);
	nCount = CountOfHashTable(pStack->pSwitchTable->pSwitchTable);
	nSize += nCount*sizeof(jc_uint);
	nSize += nCount*sizeof(jc_uchar);
	pVal = New2(jc_char, nSize);
	pTable = (CJcCaseItem*)pVal;
	pTable->nCount = nCount;
	pTable->nDefault = pStack->pSwitchTable->nDefaultCase;
	pTable->nStart = sizeof(CJcCaseItem) + nCount*sizeof(jc_uint);
	pCase = (jc_uchar*)(pVal + pTable->nStart);
	pCaseAddr = (jc_uint*)(pVal + sizeof(CJcCaseItem));
	pItem = BeginOfHashTable(pStack->pSwitchTable->pSwitchTable);
	for(i=0; i<nCount; ++i)
	{
		pCase[i] = *(jc_uchar*)pItem->pKey;
		pCaseAddr[i] = *(jc_uint*)pItem->pVal;
		pItem = NextOfHashTable(pStack->pSwitchTable->pSwitchTable, pItem);
	}
	nRet = PutConst(pDataSegment, pConstTable, pVal, nSize, nAlign);
	g_oInterface.Free(pVal);
	return nRet;
}

static jc_uint BackFillWordToSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable)
{
	jc_ushort* pCase;
	jc_char* pVal;
	jc_uint* pCaseAddr;
	CJcCaseItem* pTable;
	CJcHashEntry* pItem;
	jc_uint i, nCount, nSize, nAlign, nRet;

	nSize = sizeof(CJcCaseItem);
	nAlign = sizeof(jc_uint);
	nCount = CountOfHashTable(pStack->pSwitchTable->pSwitchTable);
	nSize += nCount*sizeof(jc_uint);
	nSize += nCount*sizeof(jc_ushort);
	pVal = New2(jc_char, nSize);
	pTable = (CJcCaseItem*)pVal;
	pTable->nCount = nCount;
	pTable->nDefault = pStack->pSwitchTable->nDefaultCase;
	pTable->nStart = sizeof(CJcCaseItem) + nCount*sizeof(jc_uint);
	pCase = (jc_ushort*)(pVal + pTable->nStart);
	pCaseAddr = (jc_uint*)(pVal + sizeof(CJcCaseItem));
	pItem = BeginOfHashTable(pStack->pSwitchTable->pSwitchTable);
	for(i=0; i<nCount; ++i)
	{
		pCase[i] = *(jc_ushort*)pItem->pKey;
		pCaseAddr[i] = *(jc_uint*)pItem->pVal;
		pItem = NextOfHashTable(pStack->pSwitchTable->pSwitchTable, pItem);
	}
	nRet = PutConst(pDataSegment, pConstTable, pVal, nSize, nAlign);
	g_oInterface.Free(pVal);
	return nRet;
}

static jc_uint BackFillDWordToSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable)
{
	jc_uint* pCase;
	jc_char* pVal;
	jc_uint* pCaseAddr;
	CJcCaseItem* pTable;
	CJcHashEntry* pItem;
	jc_uint i, nCount, nSize, nAlign, nRet;

	nSize = sizeof(CJcCaseItem);
	nAlign = sizeof(jc_uint);
	nCount = CountOfHashTable(pStack->pSwitchTable->pSwitchTable);
	nSize += nCount*sizeof(jc_uint);
	nSize += nCount*sizeof(jc_uint);
	pVal = New2(jc_char, nSize);
	pTable = (CJcCaseItem*)pVal;
	pTable->nCount = nCount;
	pTable->nDefault = pStack->pSwitchTable->nDefaultCase;
	pTable->nStart = sizeof(CJcCaseItem) + nCount*sizeof(jc_uint);
	pCase = (jc_uint*)(pVal + pTable->nStart);
	pCaseAddr = (jc_uint*)(pVal + sizeof(CJcCaseItem));
	pItem = BeginOfHashTable(pStack->pSwitchTable->pSwitchTable);
	for(i=0; i<nCount; ++i)
	{
		pCase[i] = *(jc_uint*)pItem->pKey;
		pCaseAddr[i] = *(jc_uint*)pItem->pVal;
		pItem = NextOfHashTable(pStack->pSwitchTable->pSwitchTable, pItem);
	}
	nRet = PutConst(pDataSegment, pConstTable, pVal, nSize, nAlign);
	g_oInterface.Free(pVal);
	return nRet;
}

static jc_uint BackFillQWordToSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable)
{
	jc_ulong* pCase;
	jc_char* pVal;
	jc_uint* pCaseAddr;
	CJcCaseItem* pTable;
	CJcHashEntry* pItem;
	jc_uint i, nCount, nSize, nAlign, nMod, nTAlign, nTMod, nRet;

	nSize = sizeof(CJcCaseItem);
	nAlign = sizeof(jc_uint);
	nCount = CountOfHashTable(pStack->pSwitchTable->pSwitchTable);
	if(sizeof(jc_ulong) > nAlign)
		nAlign = sizeof(jc_ulong);
	nSize += nCount*sizeof(jc_uint);
	nSize += nCount*sizeof(jc_ulong);
	nMod = nSize%nAlign;
	if(nMod)
		nSize += nAlign - nMod;
	pVal = New2(jc_char, nSize);
	pTable = (CJcCaseItem*)pVal;
	pTable->nCount = nCount;
	pTable->nDefault = pStack->pSwitchTable->nDefaultCase;
	pTable->nStart = sizeof(CJcCaseItem) + nCount*sizeof(jc_uint);
	nTAlign = sizeof(jc_ulong);
	nTMod = pTable->nStart % nTAlign;
	if(nTMod)
		pTable->nStart += nTAlign - nTMod;
	pCase = (jc_ulong*)(pVal + pTable->nStart);
	pCaseAddr = (jc_uint*)(pVal + sizeof(CJcCaseItem));
	pItem = BeginOfHashTable(pStack->pSwitchTable->pSwitchTable);
	for(i=0; i<nCount; ++i)
	{
		pCase[i] = *(jc_ulong*)pItem->pKey;
		pCaseAddr[i] = *(jc_uint*)pItem->pVal;
		pItem = NextOfHashTable(pStack->pSwitchTable->pSwitchTable, pItem);
	}
	nRet = PutConst(pDataSegment, pConstTable, pVal, nSize, nAlign);
	g_oInterface.Free(pVal);
	return nRet;
}

jc_uint BackFillSwitchStack(CJcSwitchStack* pStack, CJcSegment* pDataSegment, CJcConstTable* pConstTable)
{
	jc_uint nRet;
	CJcSwitchTable* pTable = pStack->pSwitchTable;
	if(!pTable)
		return 0;
	switch(pTable->nType)
	{
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		nRet = BackFillByteToSwitchStack(pStack, pDataSegment, pConstTable);
		break;
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
		nRet = BackFillWordToSwitchStack(pStack, pDataSegment, pConstTable);
		break;
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
		nRet = BackFillDWordToSwitchStack(pStack, pDataSegment, pConstTable);
		break;
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		nRet = BackFillQWordToSwitchStack(pStack, pDataSegment, pConstTable);
		break;
	}
	return nRet;
}
