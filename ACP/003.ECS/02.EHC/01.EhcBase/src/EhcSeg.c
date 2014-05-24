
#include "EhcSeg.h"

extern CEhcInterface g_oInterface;

static ehc_void ExtendSegment(CEhcSegment* pSegment, ehc_uint nSize, ehc_char* pData)
{
	ehc_uint nExt;
	if(nSize)
	{
		nExt = pSegment->nSize - pSegment->nPos;
		if(nExt < nSize)
		{
			ehc_char* pNewBuf;
			nExt = nSize - nExt;
			pNewBuf = (ehc_char*)g_oInterface.Realloc(pSegment->pHead, pSegment->nSize+nExt);
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

static ehc_void AlignSegment(CEhcSegment* pSegment, ehc_uint nAlign)
{
	ehc_uint nMod;
	if(nAlign && (nMod = (pSegment->nPos % nAlign)))
		ExtendSegment(pSegment, nAlign - nMod, NULL);
}

EBS_API ehc_void InitializeSegment(CEhcSegment* pSegment, ehc_char cFillChar)
{
	pSegment->cFillChar = cFillChar;
	pSegment->nPos = 0;
	pSegment->nSize = 0;
	pSegment->pHead = NULL;
}

EBS_API ehc_void ClearSegment(CEhcSegment* pSegment)
{
	SetPosOfSegment(pSegment, 0);
	CompactSegment(pSegment);
}

EBS_API ehc_void CompactSegment(CEhcSegment* pSegment)
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
			ehc_char* pNewBuf = (ehc_char*)g_oInterface.Malloc(pSegment->nSize);
			MemoryCopy(pNewBuf, pSegment->pHead, pSegment->nSize);
			g_oInterface.Free(pSegment->pHead);
			pSegment->pHead = pNewBuf;
		}
	}
}

EBS_API ehc_uint PutInSegment(CEhcSegment* pSegment, ehc_void* pData, ehc_uint nDataLen, ehc_uint nAlign)
{
	ehc_uint nPos;
	AlignSegment(pSegment, nAlign);
	nPos = pSegment->nPos;
	ExtendSegment(pSegment, nDataLen, pData);
	return nPos;
}

EBS_API ehc_uint GetPosOfSegment(CEhcSegment* pSegment)
{
	return pSegment->nPos;
}

EBS_API ehc_void SetPosOfSegment(CEhcSegment* pSegment, ehc_uint nPos)
{
	if(nPos <= pSegment->nSize)
		pSegment->nPos = nPos;
}

EBS_API ehc_uint GetSizeOfSegment(CEhcSegment* pSegment)
{
	return pSegment->nSize;
}

EBS_API ehc_char* GetDataOfSegment(CEhcSegment* pSegment)
{
	return pSegment->pHead;
}
