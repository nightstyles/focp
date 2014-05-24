
#include "jc_variable.h"

jc_void InitializeVariableInfo(CJcVariableInfo* pVariable, jc_char* sName, CJcTypeInfo* pType)
{
	InitializeString(&pVariable->oName);
	CoverString(&pVariable->oName, sName);
	pVariable->pType = pType;
	pVariable->pNext = NULL;
}

jc_void ClearVariableInfo(CJcVariableInfo* pVariable)
{
	ClearString(&pVariable->oName);
	DestroyType(pVariable->pType);
}

CJcVariableInfo* CreateVariableInfo(jc_char* sName, CJcTypeInfo* pType)
{
	CJcVariableInfo* pVar = New(CJcVariableInfo);
	InitializeVariableInfo(pVar, sName, pType);
	return pVar;
}

CJcVariableInfo* CloneVariableInfo(CJcVariableInfo* pSrc)
{
	CJcVariableInfo* pDst = CreateVariableInfo(GetString(&pSrc->oName), CloneType(pSrc->pType));
	pDst->nArg = pSrc->nArg;
	pDst->nOpt = pSrc->nOpt;
	pDst->nVarId = pSrc->nVarId;
	return pDst;
}

jc_void CoverVariableInfo(CJcVariableInfo* pDst, CJcVariableInfo* pSrc)
{
	ClearVariableInfo(pDst);
	CoverString(&pDst->oName, GetString(&pSrc->oName));
	pDst->pType = CloneType(pSrc->pType);
	pDst->nArg = pSrc->nArg;
	pDst->nOpt = pSrc->nOpt;
	pDst->nVarId = pSrc->nVarId;
}

jc_void DestroyVariableInfo(CJcVariableInfo* pVariable)
{
	ClearVariableInfo(pVariable);
	g_oInterface.Free(pVariable);
}
