
#include "jc_seg.h"

static jc_void ExtendSegment(CJcSegment* pSegment, jc_uint nSize, jc_char* pData)
{
	jc_uint nExt;
	if(nSize)
	{
		nExt = pSegment->nSize - pSegment->nPos;
		if(nExt < nSize)
		{
			jc_char* pNewBuf;
			nExt = nSize - nExt;
			pNewBuf = (jc_char*)g_oInterface.Realloc(pSegment->pHead, pSegment->nSize+nExt);
			pSegment->pHead = pNewBuf;
			pSegment->nSize += nExt;
		}
		if(pData)
			MemoryCopy(pSegment->pHead+pSegment->nPos, pData, nSize);
		else
			MemorySet(pSegment->pHead+pSegment->nPos, pSegment->cFillChar, nSize);
		pSegment->nPos += nSize;
	}
}

static jc_void AlignSegment(CJcSegment* pSegment, jc_uint nAlign)
{
	jc_uint nMod;
	if(nAlign && (nMod = (pSegment->nPos % nAlign)))
		ExtendSegment(pSegment, nAlign - nMod, NULL);
}

jc_void InitializeSegment(CJcSegment* pSegment, jc_char cFillChar)
{
	pSegment->cFillChar = cFillChar;
	pSegment->nPos = 0;
	pSegment->nSize = 0;
	pSegment->pHead = NULL;
}

jc_void ClearSegment(CJcSegment* pSegment)
{
	SetPosOfSegment(pSegment, 0);
	CompactSegment(pSegment);
}

jc_void CompactSegment(CJcSegment* pSegment)
{
	if(pSegment->nSize > pSegment->nPos)
	{
		pSegment->nSize = pSegment->nPos;
		if(!pSegment->nSize)
		{
			g_oInterface.Free(pSegment->pHead);
			pSegment->pHead = NULL;
		}
		else
		{
			jc_char* pNewBuf = (jc_char*)g_oInterface.Malloc(pSegment->nSize);
			MemoryCopy(pNewBuf, pSegment->pHead, pSegment->nSize);
			g_oInterface.Free(pSegment->pHead);
			pSegment->pHead = pNewBuf;
		}
	}
}

jc_uint PutInSegment(CJcSegment* pSegment, jc_void* pData, jc_uint nDataLen, jc_uint nAlign)
{
	jc_uint nPos;
	AlignSegment(pSegment, nAlign);
	nPos = pSegment->nPos;
	ExtendSegment(pSegment, nDataLen, pData);
	return nPos;
}

jc_uint GetPosOfSegment(CJcSegment* pSegment)
{
	return pSegment->nPos;
}

jc_void SetPosOfSegment(CJcSegment* pSegment, jc_uint nPos)
{
	if(nPos <= pSegment->nSize)
		pSegment->nPos = nPos;
}

jc_uint GetSizeOfSegment(CJcSegment* pSegment)
{
	return pSegment->nSize;
}

jc_char* GetDataOfSegment(CJcSegment* pSegment)
{
	return pSegment->pHead;
}
