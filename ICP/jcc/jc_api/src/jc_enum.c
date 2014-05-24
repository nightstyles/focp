
#include "jc_enum.h"

jc_void DestroyEnumInfo(CJcEnumInfo* pEnum)
{
	g_oInterface.Free(pEnum);
}

CJcEnumInfo* CreateEnumInfo()
{
	CJcEnumInfo* pEnum = New(CJcEnumInfo);
	pEnum->nImplemented = 0;
	pEnum->nNextValue = 0;
	return pEnum;
}

CJcEnumInfo* CloneEnumInfo(CJcEnumInfo* pSrc)
{
	CJcEnumInfo* pEnum = New(CJcEnumInfo);
	pEnum->nImplemented = pSrc->nImplemented;
	pEnum->nNextValue = pSrc->nNextValue;
	return pEnum;
}

jc_void CoverEnumInfo(CJcEnumInfo* pDst, CJcEnumInfo* pSrc)
{
	if(pDst != pSrc)
	{
		pDst->nImplemented = pSrc->nImplemented;
		pDst->nNextValue = pSrc->nNextValue;
	}
}

jc_int GetNextEnumValue(CJcEnumInfo* pEnum)
{
	return pEnum->nNextValue;
}

jc_void UseEnumValue(CJcEnumInfo* pEnum, jc_int nVal)
{
	pEnum->nNextValue = nVal+1;
}

jc_uint EnumImplemented(CJcEnumInfo* pEnum)
{
	return pEnum->nImplemented;
}

jc_void FinishEnumDefine(CJcEnumInfo* pEnum)
{
	pEnum->nImplemented = 1;
}

jc_void ResetEnumDefine(CJcEnumInfo* pEnum)
{
	pEnum->nImplemented = 0;
	pEnum->nNextValue = 0;
}
