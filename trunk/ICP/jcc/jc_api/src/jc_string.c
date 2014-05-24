
#include "jc_string.h"

jc_void InitializeString(CJcString* pStr)
{
	pStr->nLen = 0;
	pStr->pStr = NULL;
}

jc_void ClearString(CJcString* pStr)
{
	if(pStr->pStr)
	{
		g_oInterface.Free(pStr->pStr);
		pStr->nLen = 0;
		pStr->pStr = NULL;
	}
}

CJcString* CreateString(jc_char* pStr)
{
	CJcString* pString = New(CJcString);
	InitializeString(pString);
	if(pStr)
	{
		pString->nLen = StringLength(pStr);
		pString->pStr = (jc_char*)g_oInterface.Malloc(pString->nLen+1);
		StringCopy(pString->pStr, pStr);
	}
	return pString;
}

jc_void DestroyString(CJcString* pStr)
{
	if(pStr)
	{
		if(pStr->pStr)
			g_oInterface.Free(pStr->pStr);
		g_oInterface.Free(pStr);
	}
}

jc_void CoverString(CJcString* pDstStr, jc_char* pSrcStr)
{
	jc_uint nLen = 0;
	jc_char* sStr = NULL;

	if(pSrcStr)
	{
		nLen = StringLength(pSrcStr);
		sStr = (jc_char*)g_oInterface.Malloc(nLen+1);
		StringCopy(sStr, pSrcStr);
	}
	if(pDstStr->pStr)
		g_oInterface.Free(pDstStr->pStr);
	pDstStr->pStr = sStr;
	pDstStr->nLen = nLen;
}

jc_void AppendString(CJcString* pDstStr, jc_char* pSrcStr)
{
	jc_uint nLen;
	jc_char* sStr;
	if(pSrcStr)
	{
		nLen = StringLength(pSrcStr) + pDstStr->nLen;
		sStr = (jc_char*)g_oInterface.Malloc(nLen+1);
		if(pDstStr->nLen)
		{
			StringCopy(sStr, pDstStr->pStr);
			g_oInterface.Free(pDstStr->pStr);
		}
		StringCopy(sStr+pDstStr->nLen, pSrcStr);
		pDstStr->pStr = sStr;
		pDstStr->nLen = nLen;
	}
}

jc_char* GetString(CJcString* pDstStr)
{
	return pDstStr->pStr;
}
