
#ifndef _jc_function_h_
#define _jc_function_h_

#include "jc_typeinfo.h"
#include "jc_variable.h"

struct CJcFunctionInfo
{
	CJcTypeInfo* pType;
	jc_uint nArgCount;
	CJcVariableInfo* pHead, *pTail;
	jc_uint nArgSize;
	jc_uint nOpt;
	jc_uint nArg;
};

CJcFunctionInfo* CreateFunctionInfo(CJcTypeInfo* pType);
CJcFunctionInfo* CloneFunctionInfo(CJcFunctionInfo* pSrc);
jc_void CoverFunctionInfo(CJcFunctionInfo* pDst, CJcFunctionInfo* pSrc);
jc_void DestroyFunctionInfo(CJcFunctionInfo* pFunction);

CJcTypeInfo* GetFunctionType(CJcFunctionInfo* pFunction);
jc_uint GetFunctionArgSize(CJcFunctionInfo* pFunction);
jc_uint GetFunctionOpt(CJcFunctionInfo* pFunction);
jc_uint GetFunctionArg(CJcFunctionInfo* pFunction);

jc_uint GetFunctionParaCount(CJcFunctionInfo* pFunction);
CJcVariableInfo* GetFunctionPara(CJcFunctionInfo* pFunction, jc_uint nIdx);
CJcVariableInfo* FindFunctionPara(CJcFunctionInfo* pFunction, jc_char* sName);
jc_uint AddFunctionPara(CJcFunctionInfo* pFunction, jc_char* sName, CJcTypeInfo* pType);

jc_uint FunctionImplemented(CJcFunctionInfo* pFunction);
jc_void FinishFunctionDefine(CJcFunctionInfo* pFunction, jc_uint nOpt, jc_uint nArg);
jc_void ResetFunctionDefine(CJcFunctionInfo* pFunction, jc_bool bAll);

CJcString GetFunctionTypeName(CJcFunctionInfo* pFunction, jc_bool bIncQual);
CJcString GetFunctionOrigTypeName(CJcFunctionInfo* pFunction, jc_bool bIncQual);

#endif
