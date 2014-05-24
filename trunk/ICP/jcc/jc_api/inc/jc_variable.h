
#ifndef _jc_variable_h_
#define _jc_variable_h_

#include "jc_typeinfo.h"

typedef struct CJcVariableInfo
{
	CJcString oName;
	CJcTypeInfo* pType;
	jc_uint nVarId;
	jc_uint nOpt;
	jc_uint nArg;
	struct CJcVariableInfo* pNext;
}CJcVariableInfo;

jc_void InitializeVariableInfo(CJcVariableInfo* pVariable, jc_char* sName, CJcTypeInfo* pType);
jc_void ClearVariableInfo(CJcVariableInfo* pVariable);

CJcVariableInfo* CreateVariableInfo(jc_char* sName, CJcTypeInfo* pType);
CJcVariableInfo* CloneVariableInfo(CJcVariableInfo* pSrc);
jc_void CoverVariableInfo(CJcVariableInfo* pDst, CJcVariableInfo* pSrc);
jc_void DestroyVariableInfo(CJcVariableInfo* pVariable);

#endif
