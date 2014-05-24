
#include "jc_struct.h"
#include "jc_function.h"
#include "jc_enum.h"
#include "jc_array.h"

static void TypeNameLink(CJcString* pName, CJcString* pName2)
{
	if(pName->nLen && pName2->nLen)
		AppendString(pName, " ");
	AppendString(pName, GetString(pName2));
}

jc_void InitializeType0(CJcTypeInfo* pType)
{
	InitializeType(pType, JC_NONE, JC_VOID, NULL, NULL, NULL);
}

jc_void InitializeType(CJcTypeInfo* pType,
					   jc_uint nQual,
					   jc_uint nType,
					   jc_char* sName,
					   CJcTypeInfo* pNextType,
					   jc_void* pInfo)
{
	pType->nQual = nQual;
	pType->nType = nType;
	pType->nSize = 0;
	pType->pNext = pNextType;
	pType->oInfo.pStructInfo = (CJcStructInfo*)pInfo;
	switch(nType)
	{
	case JC_VOID: case JC_STRUCT: case JC_UNION: case JC_ARRAY: case JC_ALIAS:
		pType->nSize = 0;
		break;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		pType->nSize = 1;
		break;
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
		pType->nSize = 2;
		break;
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_FLOAT: case JC_ENUM:
		pType->nSize = 4;
		break;
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG: case JC_DOUBLE:
		pType->nSize = 8;
		break;
	case JC_POINTER: case JC_FUNCTION:
		pType->nSize = sizeof(jc_void*);
		break;
	}
	InitializeString(&pType->oName);
	CoverString(&pType->oName, sName);
}

jc_void RemovePointer(CJcTypeInfo* pType)
{
	CJcTypeInfo* pOrigType = pType;
	while(pOrigType->nType == JC_POINTER)
		pOrigType = pOrigType->pNext;
	if(pOrigType != pType)
	{
		CJcTypeInfo oType2;
		InitializeType0(&oType2);
		CoverType(&oType2, pOrigType);
		ClearType(pType);
		*pType = oType2;
	}
}

jc_void ClearType(CJcTypeInfo* pType)
{
	if(pType->pNext && pType->nType != JC_ALIAS)
		DestroyType(pType->pNext);
	if(pType->oInfo.pStructInfo)switch(pType->nType)
	{
	case JC_STRUCT: case JC_UNION:
		DestroyStructInfo(pType->oInfo.pStructInfo);
		break;
	case JC_FUNCTION:
		DestroyFunctionInfo(pType->oInfo.pFunctionInfo);
		break;
	case JC_ENUM:
		DestroyEnumInfo(pType->oInfo.pEnumInfo);
		break;
	case JC_ARRAY:
		DestroyArrayInfo(pType->oInfo.pArrayInfo);
		break;
	}
	ClearString(&pType->oName);
	InitializeType0(pType);
}

jc_void DestroyType(CJcTypeInfo* pType)
{
	ClearType(pType);
	g_oInterface.Free(pType);
}

CJcTypeInfo* CreateType(jc_uint nQual,
						jc_uint nType,
						jc_char* sName,
						CJcTypeInfo* pNextType,
						jc_void* pInfo)
{
	CJcTypeInfo* pType = New(CJcTypeInfo);
	InitializeType(pType, nQual, nType, sName, pNextType, pInfo);
	return pType;
}

static jc_void CopyType(CJcTypeInfo* pDstType, CJcTypeInfo* pSrcType)
{
	CoverString(&pDstType->oName, GetString(&pSrcType->oName));
	pDstType->nQual = pSrcType->nQual;
	pDstType->nType = pSrcType->nType;
	pDstType->nSize = pSrcType->nSize;
	pDstType->oInfo.pStructInfo = NULL;
	pDstType->pNext = NULL;
	switch(pDstType->nType)
	{
	case JC_POINTER:
		pDstType->pNext = CloneType(pSrcType->pNext);
		break;
	case JC_ENUM:
		pDstType->oInfo.pEnumInfo = CloneEnumInfo(pSrcType->oInfo.pEnumInfo);
		break;
	case JC_STRUCT: case JC_UNION:
		pDstType->oInfo.pStructInfo = CloneStructInfo(pSrcType->oInfo.pStructInfo);
		break;
	case JC_ARRAY:
		pDstType->oInfo.pArrayInfo = CloneArrayInfo(pSrcType->oInfo.pArrayInfo);
		pDstType->pNext = CloneType(pSrcType->pNext);
		break;
	case JC_FUNCTION:
		pDstType->oInfo.pFunctionInfo = CloneFunctionInfo(pSrcType->oInfo.pFunctionInfo);
		break;
	case JC_ALIAS:
		pDstType->pNext = pSrcType->pNext;
		break;
	}
}

CJcTypeInfo* CloneType(CJcTypeInfo* pType)
{
	CJcTypeInfo* pDstType = New(CJcTypeInfo);
	InitializeType0(pDstType);
	CopyType(pDstType, pType);
	return pDstType;
}

jc_void CoverType(CJcTypeInfo* pDstType, CJcTypeInfo* pSrcType)
{
	if(pDstType != pSrcType)
	{
		CJcTypeInfo oNewType;
		InitializeType0(&oNewType);
		CopyType(&oNewType, pSrcType);
		ClearType(pDstType);
		*pDstType = oNewType;
	}
}

CJcTypeInfo* GetOrigType(CJcTypeInfo* pType)
{
	while(JC_ALIAS == (GetTypeCode(pType)))
		pType = GetNextType(pType);
	return pType;
}

jc_uint AllowBitField(CJcTypeInfo* pType)
{
	pType = GetOrigType(pType);
	return IsIntegralType(pType->nType);
}

jc_uint GetQualCode(CJcTypeInfo* pType)
{
	return pType->nQual;
}

jc_uint GetTypeCode(CJcTypeInfo* pType)
{
	return pType->nType;
}

jc_uint GetTypeSize(CJcTypeInfo* pType)
{
	jc_uint nSize;
	switch(pType->nType)
	{
	case JC_ALIAS:
		nSize = GetTypeSize(pType->pNext);
		break;
	case JC_STRUCT: case JC_UNION:
		nSize = GetStructSize(pType->oInfo.pStructInfo);
		break;
	case JC_ARRAY:
		nSize = GetArrayDim(pType->oInfo.pArrayInfo) * GetTypeSize(pType->pNext);
		break;
	default:
		nSize = pType->nSize;
		break;
	}
	return nSize;
}

jc_uint GetTypeAlign(CJcTypeInfo* pType)
{
	jc_uint nAlign;

	switch(pType->nType)
	{
	case JC_VOID:
		nAlign = 1;
		break;
	case JC_STRUCT: case JC_UNION:
		nAlign = GetStructAlign(pType->oInfo.pStructInfo);
		break;
	case JC_ARRAY: case JC_ALIAS:
		nAlign = GetTypeAlign(pType->pNext);
		break;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_FLOAT: case JC_ENUM:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG: case JC_DOUBLE:
	case JC_POINTER: case JC_FUNCTION:
		nAlign = GetTypeSize(pType);
		if(nAlign > MaxAlign())
			nAlign = MaxAlign();
		break;
	}

	return nAlign;
}

jc_void SetQualCode(CJcTypeInfo* pType, jc_uint nQual)
{
	pType->nQual = nQual;
}

CJcStructInfo* GetStructInfo(CJcTypeInfo* pType)
{
	return pType->oInfo.pStructInfo;
}

CJcFunctionInfo* GetFunctionInfo(CJcTypeInfo* pType)
{
	return pType->oInfo.pFunctionInfo;
}

CJcEnumInfo* GetEnumInfo(CJcTypeInfo* pType)
{
	return pType->oInfo.pEnumInfo;
}

CJcArrayInfo* GetArrayInfo(CJcTypeInfo* pType)
{
	return pType->oInfo.pArrayInfo;
}

CJcTypeInfo* GetNextType(CJcTypeInfo* pType)
{
	return pType->pNext;
}

CJcString GetTypeName(CJcTypeInfo* pType, jc_bool bIncQual)
{
	CJcString oName = {0, NULL};
	CJcString oNextName = {0, NULL};
	CJcTypeInfo* pTmp;
	switch(pType->nType)
	{
	case JC_VOID:
		CoverString(&oName, "void");
		break;
	case JC_SIGNED_CHAR:
		CoverString(&oName, "char");
		break;
	case JC_UNSIGNED_CHAR:
		CoverString(&oName, "unsigned char");
		break;
	case JC_SIGNED_SHORT:
		CoverString(&oName, "short");
		break;
	case JC_UNSIGNED_SHORT:
		CoverString(&oName, "unsigned short");
		break;
	case JC_SIGNED_INT:
		CoverString(&oName, "int");
		break;
	case JC_UNSIGNED_INT:
		CoverString(&oName, "unsigned int");
		break;
	case JC_SIGNED_LONG:
		CoverString(&oName, "long");
		break;
	case JC_UNSIGNED_LONG:
		CoverString(&oName, "unsigned long");
		break;
	case JC_FLOAT:
		CoverString(&oName, "float");
		break;
	case JC_DOUBLE:
		CoverString(&oName, "double");
		break;
	case JC_POINTER:
		oName = GetTypeName(pType->pNext, True);
		CoverString(&oNextName, "*");
		TypeNameLink(&oName, &oNextName);
		CoverString(&oNextName, NULL);
		break;
	case JC_ENUM:
		CoverString(&oName, "enum");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_STRUCT:
		CoverString(&oName, "struct");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_UNION:
		CoverString(&oName, "union");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_ARRAY:
		pTmp = pType;
		while(pTmp->nType == JC_ARRAY)
		{
			oName = GetArrayTypeName(pTmp->oInfo.pArrayInfo);
			TypeNameLink(&oNextName, &oName);
			CoverString(&oName, NULL);
			pTmp = pTmp->pNext;
		}
		oName = GetTypeName(pTmp, True);
		TypeNameLink(&oName, &oNextName);
		CoverString(&oNextName, NULL);
		break;
	case JC_FUNCTION:
		oName = GetFunctionTypeName(pType->oInfo.pFunctionInfo, True);
		break;
	case JC_ALIAS:
		if(!pType->oName.nLen)
			oName = GetTypeName(pType->pNext, False);
		else
			CoverString(&oName, GetString(&pType->oName));
		break;
	}
	if(pType->nType != JC_FUNCTION)
	{
		if(bIncQual)
		{
			oNextName = GetQualName(pType);
			TypeNameLink(&oName, &oNextName);
			CoverString(&oNextName, NULL);
		}
	}
	return oName;
}

CJcString GetOrigTypeName(CJcTypeInfo* pType, jc_bool bIncQual)
{
	CJcString oName = {0, NULL};
	CJcString oNextName = {0, NULL};
	CJcTypeInfo* pTmp;
	switch(pType->nType)
	{
	case JC_VOID:
		CoverString(&oName, "void");
		break;
	case JC_SIGNED_CHAR:
		CoverString(&oName, "char");
		break;
	case JC_UNSIGNED_CHAR:
		CoverString(&oName, "unsigned char");
		break;
	case JC_SIGNED_SHORT:
		CoverString(&oName, "short");
		break;
	case JC_UNSIGNED_SHORT:
		CoverString(&oName, "unsigned short");
		break;
	case JC_SIGNED_INT:
		CoverString(&oName, "int");
		break;
	case JC_UNSIGNED_INT:
		CoverString(&oName, "unsigned int");
		break;
	case JC_SIGNED_LONG:
		CoverString(&oName, "long");
		break;
	case JC_UNSIGNED_LONG:
		CoverString(&oName, "unsigned long");
		break;
	case JC_FLOAT:
		CoverString(&oName, "float");
		break;
	case JC_DOUBLE:
		CoverString(&oName, "double");
		break;
	case JC_POINTER:
		oName = GetOrigTypeName(pType->pNext, True);
		CoverString(&oNextName, "*");
		TypeNameLink(&oName, &oNextName);
		CoverString(&oNextName, NULL);
		break;
	case JC_ENUM:
		CoverString(&oName, "enum");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_STRUCT:
		CoverString(&oName, "struct");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_UNION:
		CoverString(&oName, "union");
		TypeNameLink(&oName, &pType->oName);
		break;
	case JC_ARRAY:
		pTmp = pType;
		while(pTmp->nType == JC_ARRAY)
		{
			oName = GetArrayTypeName(pTmp->oInfo.pArrayInfo);
			TypeNameLink(&oNextName, &oName);
			CoverString(&oName, NULL);
			pTmp = pTmp->pNext;
		}
		oName = GetOrigTypeName(pTmp, True);
		TypeNameLink(&oName, &oNextName);
		CoverString(&oNextName, NULL);
		break;
	case JC_FUNCTION:
		oName = GetFunctionOrigTypeName(pType->oInfo.pFunctionInfo, True);
		break;
	case JC_ALIAS:
		oName = GetOrigTypeName(pType->pNext, False);
		break;
	}
	if(pType->nType != JC_FUNCTION)
	{
		if(bIncQual)
		{
			oNextName = GetQualName(pType);
			TypeNameLink(&oName, &oNextName);
			CoverString(&oNextName, NULL);
		}
	}
	return oName;
}

CJcString GetQualName(CJcTypeInfo* pType)
{
	CJcString oName = {0, NULL};
	CJcString oName2 = {0, NULL};
	if(pType->nQual & JC_CONST)
	{
		CoverString(&oName2, "const");
		TypeNameLink(&oName, &oName2);
		CoverString(&oName2, NULL);
	}
	if(pType->nQual & JC_VOLATILE)
	{
		CoverString(&oName2, "volatile");
		TypeNameLink(&oName, &oName2);
		CoverString(&oName2, NULL);
	}
	return oName;
}

jc_bool IsIntegralType(jc_uint nType)
{
	switch(nType)
	{
	default:
		return False;
	case JC_ENUM:
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		break;
	}
	return True;
}

jc_bool IsNumberalType(jc_uint nType)
{
	if(nType == JC_FLOAT || nType == JC_DOUBLE || IsIntegralType(nType))
		return True;
	return False;
}

jc_bool IsConstType(CJcTypeInfo* pType)
{
	if(pType->nQual & JC_CONST)
		return True;
	while(pType->nType == JC_ALIAS)
	{
		pType = pType->pNext;
		if(pType->nQual & JC_CONST)
			return True;
	}
	return False;
}
