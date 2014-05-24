
#include "EhcString.h"

extern CEhcInterface g_oInterface;

EBS_API ehc_void InitializeString(CEhcString* pStr)
{
	pStr->nLen = 0;
	pStr->pStr = NULL;
}

EBS_API ehc_void ClearString(CEhcString* pStr)
{
	if(pStr->pStr)
	{
		g_oInterface.Free(pStr->pStr);
		pStr->nLen = 0;
		pStr->pStr = NULL;
	}
}

EBS_API CEhcString* CreateString(ehc_char* pStr)
{
	CEhcString* pString = New(CEhcString);
	InitializeString(pString);
	if(pStr)
	{
		pString->nLen = StringLength(pStr);
		pString->pStr = (ehc_char*)g_oInterface.Malloc(pString->nLen+1);
		StringCopy(pString->pStr, pStr);
	}
	return pString;
}

EBS_API ehc_void DestroyString(CEhcString* pStr)
{
	if(pStr)
	{
		if(pStr->pStr)
			g_oInterface.Free(pStr->pStr);
		g_oInterface.Free(pStr);
	}
}

EBS_API ehc_void CoverString(CEhcString* pDstStr, ehc_char* pSrcStr)
{
	ehc_uint nLen = 0;
	ehc_char* sStr = NULL;

	if(pSrcStr)
	{
		nLen = StringLength(pSrcStr);
		sStr = (ehc_char*)g_oInterface.Malloc(nLen+1);
		StringCopy(sStr, pSrcStr);
	}
	if(pDstStr->pStr)
		g_oInterface.Free(pDstStr->pStr);
	pDstStr->pStr = sStr;
	pDstStr->nLen = nLen;
}

EBS_API ehc_void AppendString(CEhcString* pDstStr, ehc_char* pSrcStr)
{
	ehc_uint nLen;
	ehc_char* sStr;
	if(pSrcStr)
	{
		nLen = StringLength(pSrcStr) + pDstStr->nLen;
		sStr = (ehc_char*)g_oInterface.Malloc(nLen+1);
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

EBS_API ehc_char* GetString(CEhcString* pDstStr)
{
	return pDstStr->pStr;
}
