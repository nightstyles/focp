
#include "jc_struct.h"

jc_void InitializeStructInfo(CJcStructInfo* pStruct)
{
	pStruct->nImplemented = 0;
	pStruct->nSize = 0;
	pStruct->nAlign = 1;
	pStruct->nFieldCount = 0;
	pStruct->pHead = pStruct->pTail = NULL;
}

jc_void ClearStructInfo(CJcStructInfo* pStruct)
{
	ResetStructDefine(pStruct);
}

jc_void DestroyStructInfo(CJcStructInfo* pStruct)
{
	if(pStruct)
	{
		ResetStructDefine(pStruct);
		g_oInterface.Free(pStruct);
	}
}

CJcStructInfo* CreateStructInfo()
{
	CJcStructInfo* pStruct = New(CJcStructInfo);
	InitializeStructInfo(pStruct);
	return pStruct;
}

CJcStructInfo* CloneStructInfo(CJcStructInfo* pSrc)
{
	CJcStructInfo* pStruct = CreateStructInfo();
	CoverStructInfo(pStruct, pSrc);
	return pStruct;
}

jc_void CoverStructInfo(CJcStructInfo* pDst, CJcStructInfo* pSrc)
{
	CJcStructFieldInfo* pField, *pNewField;
	ResetStructDefine(pDst);
	pDst->nImplemented = pSrc->nImplemented;
	pDst->nSize = pSrc->nSize;
	pDst->nAlign = pSrc->nAlign;
	pDst->nFieldCount = pSrc->nFieldCount;
	pField = pSrc->pHead;
	while(pField)
	{
		pNewField = New(CJcStructFieldInfo);
		pNewField->pType = CloneType(pField->pType);
		InitializeString(&pNewField->oName);
		CoverString(&pNewField->oName, GetString(&pField->oName));
		pNewField->pNext = NULL;
		pNewField->nBitCount = pField->nBitCount;
		pNewField->nBitOffset = pField->nBitOffset;
		pNewField->nOffset = pField->nOffset;
		if(pDst->pTail)
			pDst->pTail->pNext = pNewField;
		else
			pDst->pHead = pNewField;
		pDst->pTail = pNewField;
		pField = pField->pNext;
	}
}

jc_uint GetStructSize(CJcStructInfo* pStruct)
{
	return pStruct->nSize;
}

jc_uint GetStructAlign(CJcStructInfo* pStruct)
{
	return pStruct->nAlign;
}

jc_uint GetStructFieldCount(CJcStructInfo* pStruct)
{
	return pStruct->nFieldCount;
}

CJcStructFieldInfo* GetStructFieldByIdx(CJcStructInfo* pStruct, jc_uint nIdx)
{
	jc_uint i;
	CJcStructFieldInfo* pField;
	if(nIdx >= pStruct->nFieldCount)
		return NULL;
	pField = pStruct->pHead;
	for(i=0; i<nIdx && pField; ++i)
		pField = pField->pNext;
	return pField;
}

CJcStructFieldInfo* GetStructField(CJcStructInfo* pStruct, jc_char* sName)
{
	jc_uint i;
	CJcStructFieldInfo* pField;
	pField = pStruct->pHead;
	for(i=0; pField; ++i)
	{
		if(!StringCompare(GetString(&pField->oName), sName))
			break;
		pField = pField->pNext;
	}
	return pField;
}

jc_uint AddStructField(CJcStructInfo* pStruct,
					   jc_uint nStructType, 
					   jc_char* sName, 
					   CJcTypeInfo* pType, 
					   jc_uint nBitCount)
{
	jc_uint nCount;
	CJcStructFieldInfo* pPrevField;
	CJcStructFieldInfo* pField;
	if(nBitCount && (nStructType == JC_UNION || !AllowBitField(pType)))
		return 1;
	if(GetTypeCode(GetOrigType(pType)) == JC_VOID)
		return 1;
	if(GetStructField(pStruct, sName))
		return 2;
	nCount = pStruct->nFieldCount;
	pField = New(CJcStructFieldInfo);
	pField->pType = pType;
	InitializeString(&pField->oName);
	CoverString(&pField->oName, sName);
	pField->pNext = NULL;
	pField->nBitCount = nBitCount;
	pField->nBitOffset = 0;
	pField->nOffset = 0;
	pPrevField = pStruct->pTail;
	if(pStruct->pTail)
		pStruct->pTail->pNext = pField;
	else
		pStruct->pHead = pField;
	pStruct->pTail = pField;
	++pStruct->nFieldCount;

	if(nStructType == JC_UNION)
	{
		jc_uint nAlign = GetTypeAlign(pType);
		jc_uint nSize = GetTypeSize(pType);
		if(pStruct->nSize < nSize)
			pStruct->nSize = nSize;
		if(pStruct->nAlign < nAlign)
			pStruct->nAlign = nAlign;
		return 0;
	}

	if(!nBitCount ||
		!nCount ||
		!pPrevField->nBitCount ||
		GetTypeSize(pType) != GetTypeSize(pPrevField->pType) ||
		pPrevField->nBitOffset + pPrevField->nBitCount + nBitCount > 8*GetTypeSize(pType))
	{
		jc_uint nAlign = GetTypeAlign(pType);
		jc_uint nSize = GetTypeSize(pType);
		jc_uint nMod = pStruct->nSize % nAlign;
		if(nMod)
			pStruct->nSize += nAlign - nMod;
		pField->nOffset = pStruct->nSize;
		pStruct->nSize += nSize;
		if(pStruct->nAlign < nAlign)
			pStruct->nAlign = nAlign;
	}
	else if(pPrevField)
	{
		pField->nOffset = pPrevField->nOffset;
		pField->nBitOffset = pPrevField->nBitOffset + pPrevField->nBitCount;
	}
	return 0;
}

jc_uint StructImplemented(CJcStructInfo* pStruct)
{
	return pStruct->nImplemented;
}

jc_void FinishStructDefine(CJcStructInfo* pStruct)
{
	jc_uint nMod;
	if(!pStruct->nImplemented)
	{
		pStruct->nImplemented = 1;
		if(pStruct->nSize)
		{
			nMod = pStruct->nSize % pStruct->nAlign;
			if(nMod)
				pStruct->nSize += pStruct->nAlign - nMod;
		}
	}
}

jc_void ResetStructDefine(CJcStructInfo* pStruct)
{
	pStruct->nImplemented = 0;
	pStruct->nSize = 0;
	pStruct->nAlign = 1;
	pStruct->nFieldCount = 0;
	while(pStruct->pHead)
	{
		pStruct->pTail = pStruct->pHead->pNext;
		ClearString(&pStruct->pHead->oName);
		DestroyType(pStruct->pHead->pType);
		g_oInterface.Free(pStruct->pHead);
		pStruct->pHead = pStruct->pTail;
	}
}
