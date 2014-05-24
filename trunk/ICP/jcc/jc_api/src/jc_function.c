
#include "jc_function.h"
#include "jc_ins.h"

jc_void DestroyFunctionInfo(CJcFunctionInfo* pFunction)
{
	ResetFunctionDefine(pFunction, True);
	g_oInterface.Free(pFunction);
}

CJcFunctionInfo* CreateFunctionInfo(CJcTypeInfo* pType)
{
	CJcFunctionInfo* pFunction = New(CJcFunctionInfo);
	pFunction->pType = pType;
	pFunction->nOpt = 0;
	pFunction->nArg = 0;
	pFunction->nArgSize = 0;
	pFunction->nArgCount = 0;
	pFunction->pHead = NULL;
	pFunction->pTail = NULL;
	if(GetTypeCode(GetOrigType(pType)) != JC_VOID)
	{
		AddFunctionPara(pFunction, NULL, 
			CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pType), NULL));
		if(IsBit32())
		{
			GetFunctionPara(pFunction, 0)->nArg = 4;
			pFunction->nArgSize = 8;
		}
	}
	return pFunction;
}

static jc_void CopyFunctionInfo(CJcFunctionInfo* pDst, CJcFunctionInfo* pSrc)
{
	jc_uint i;
	CJcVariableInfo* pArg, *pSrcArg;
	CJcFunctionInfo* pFunction = pDst;
	pFunction->pType = CloneType(pSrc->pType);
	pFunction->nOpt = pSrc->nOpt;
	pFunction->nArg = pSrc->nArg;
	pFunction->nArgSize = pSrc->nArgSize;
	pFunction->nArgCount = pSrc->nArgCount;
	pFunction->pHead = NULL;
	pFunction->pTail = NULL;
	pSrcArg = pSrc->pHead;
	for(i=0; i<pSrc->nArgCount; ++i)
	{
		pArg = CloneVariableInfo(pSrcArg);
		if(pFunction->pTail)
			pFunction->pTail->pNext = pArg;
		else
			pFunction->pHead = pArg;
		pFunction->pTail = pArg;
		pSrcArg = pSrcArg->pNext;
	}
}

CJcFunctionInfo* CloneFunctionInfo(CJcFunctionInfo* pSrc)
{
	CJcFunctionInfo* pFunction = New(CJcFunctionInfo);
	CopyFunctionInfo(pFunction, pSrc);
	return pFunction;
}

jc_void CoverFunctionInfo(CJcFunctionInfo* pDst, CJcFunctionInfo* pSrc)
{
	CJcFunctionInfo* pFunction = pDst;
	ResetFunctionDefine(pFunction, True);
	if(pFunction->pType)
		DestroyType(pFunction->pType);
	CopyFunctionInfo(pFunction, pSrc);
}

CJcTypeInfo* GetFunctionType(CJcFunctionInfo* pFunction)
{
	return pFunction->pType;
}

jc_uint GetFunctionArgSize(CJcFunctionInfo* pFunction)
{
	return pFunction->nArgSize;
}

jc_uint GetFunctionOpt(CJcFunctionInfo* pFunction)
{
	return pFunction->nOpt;
}

jc_uint GetFunctionArg(CJcFunctionInfo* pFunction)
{
	return pFunction->nArg;
}

jc_uint GetFunctionParaCount(CJcFunctionInfo* pFunction)
{
	return pFunction->nArgCount;
}

CJcVariableInfo* GetFunctionPara(CJcFunctionInfo* pFunction, jc_uint nIdx)
{
	jc_uint i;
	CJcVariableInfo* pHead = pFunction->pHead;
	for(i = 0; i<nIdx && pHead; ++i)
	{
		if(!StringCompare(pHead->oName.pStr, "..."))
			break;
		pHead = pHead->pNext;
	}
	return pHead;
}

CJcVariableInfo* FindFunctionPara(CJcFunctionInfo* pFunction, jc_char* sName)
{
	CJcVariableInfo* pHead;
	pHead = pFunction->pHead;
	while(pHead)
	{
		if(!StringCompare(pHead->oName.pStr, sName))
			break;
		pHead = pHead->pNext;
	}
	return pHead;
}

jc_uint AddFunctionPara(CJcFunctionInfo* pFunction, jc_char* sName, CJcTypeInfo* pType)
{
	jc_char sHideName[40];
	jc_uint nAlign, nMod, nParaType;
	CJcVariableInfo* pArg;
	jc_bool bIsArrayPara;
	if(!sName || !sName[0])
	{
		g_oInterface.FormatPrint(sHideName, "$%u", pFunction->nArgCount);
		sName = sHideName;
	}
	if(FindFunctionPara(pFunction, sName))
		return 2;
	if(!StringCompare(sName, "..."))
	{
		pArg = New(CJcVariableInfo);
		pArg->nOpt = JC_AS;
		pArg->nVarId = pFunction->nArgCount;
		pArg->pType = pType;
		pArg->nArg = pFunction->nArgSize;
	}
	else
	{
		nParaType = GetTypeCode(GetOrigType(pType));
		if(nParaType == JC_VOID)
			return 1;
		bIsArrayPara = (nParaType == JC_ARRAY);
		pArg = New(CJcVariableInfo);
		pArg->nOpt = JC_AS;
		pArg->nVarId = pFunction->nArgCount;
		pArg->pType = pType;
		if(bIsArrayPara)
			nAlign = sizeof(jc_void*);
		else
			nAlign = GetTypeAlign(pType);
		nMod = pFunction->nArgSize % nAlign;
		if(nMod)
			pFunction->nArgSize += nAlign - nMod;
		pArg->nArg = pFunction->nArgSize;
		if(bIsArrayPara)
			pFunction->nArgSize += sizeof(jc_void*);
		else
			pFunction->nArgSize += GetTypeSize(pType);
	}
	InitializeString(&pArg->oName);
	CoverString(&pArg->oName, sName);
	pArg->pNext = NULL;
	if(pFunction->pTail)
		pFunction->pTail->pNext = pArg;
	else
		pFunction->pHead = pArg;
	pFunction->pTail = pArg;
	++pFunction->nArgCount;
	return 0;
}

jc_uint FunctionImplemented(CJcFunctionInfo* pFunction)
{
	return pFunction->nOpt;
}

jc_void FinishFunctionDefine(CJcFunctionInfo* pFunction, jc_uint nOpt, jc_uint nArg)
{
	pFunction->nOpt = nOpt;
	pFunction->nArg = nArg;
}

jc_void ResetFunctionDefine(CJcFunctionInfo* pFunction, jc_bool bAll)
{
	if(bAll)
	{
		if(pFunction->pType)
		{
			DestroyType(pFunction->pType);
			pFunction->pType = NULL;
		}
		while(pFunction->pHead)
		{
			pFunction->pTail = pFunction->pHead->pNext;
			CoverString(&pFunction->pHead->oName, NULL);
			DestroyType(pFunction->pHead->pType);
			g_oInterface.Free(pFunction->pHead);
			pFunction->pHead = pFunction->pTail;
		}
		pFunction->nArgSize = 0;
		pFunction->nArgCount = 0;
	}
	pFunction->nOpt = 0;
	pFunction->nArg = 0;
}

CJcString GetFunctionTypeName(CJcFunctionInfo* pFunction, jc_bool bIncQual)
{
	CJcString oName = {0, NULL};
	CJcString oTmp = {0, NULL};
	jc_uint nParaCount = pFunction->nArgCount;
	CJcVariableInfo* pHead = pFunction->pHead;
	oName = GetTypeName(pFunction->pType, True);
	AppendString(&oName, "(");
	while(pHead)
	{
		if(pHead != pFunction->pHead)
			AppendString(&oName, ",");
		oTmp = GetTypeName(pHead->pType, True);
		AppendString(&oName, GetString(&oTmp));
		CoverString(&oTmp, NULL);
		pHead = pHead->pNext;
	}
	AppendString(&oName, ")");
	return oName;
}

CJcString GetFunctionOrigTypeName(CJcFunctionInfo* pFunction, jc_bool bIncQual)
{
	CJcString oName = {0, NULL};
	CJcString oTmp = {0, NULL};
	jc_uint nParaCount = pFunction->nArgCount;
	CJcVariableInfo* pHead = pFunction->pHead;
	oName = GetOrigTypeName(pFunction->pType, True);
	AppendString(&oName, "(");
	while(pHead)
	{
		if(pHead != pFunction->pHead)
			AppendString(&oName, ",");
		oTmp = GetTypeName(pHead->pType, True);
		AppendString(&oName, GetString(&oTmp));
		CoverString(&oTmp, NULL);
	}
	AppendString(&oName, ")");
	return oName;
}
