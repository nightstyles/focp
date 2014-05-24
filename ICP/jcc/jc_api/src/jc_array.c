
#include "jc_array.h"

jc_void DestroyArrayInfo(CJcArrayInfo* pArray)
{
	g_oInterface.Free(pArray);
}

CJcArrayInfo* CreateArrayInfo(jc_uint nDim)
{
	CJcArrayInfo* pArray = New(CJcArrayInfo);
	pArray->nDim = nDim;
	return pArray;
}

CJcArrayInfo* CloneArrayInfo(CJcArrayInfo* pSrc)
{
	CJcArrayInfo* pArray = New(CJcArrayInfo);
	pArray->nDim = pSrc->nDim;
	return pArray;
}

jc_void CoverArrayInfo(CJcArrayInfo* pDst, CJcArrayInfo* pSrc)
{
	if(pDst != pSrc)
		pDst->nDim = pSrc->nDim;
}

jc_uint GetArrayDim(CJcArrayInfo* pArray)
{
	return pArray->nDim;
}

jc_uint ArrayImplemented(CJcArrayInfo* pArray)
{
	return pArray->nDim;
}

jc_void FinishArrayDefine(CJcArrayInfo* pArray, jc_uint nDim)
{
	if(!pArray->nDim)
		pArray->nDim = nDim;
}

jc_void ResetArrayDefine(CJcArrayInfo* pArray)
{
	pArray->nDim = 0;
}

CJcString GetArrayTypeName(CJcArrayInfo* pArray)
{
	jc_char pBuf[128];
	CJcString oStr = {0, NULL};
	if(pArray->nDim)
		g_oInterface.FormatPrint(pBuf, "[%u]", pArray->nDim);
	else
		g_oInterface.FormatPrint(pBuf, "[]");
	CoverString(&oStr, pBuf);
	return oStr;
}
