
#ifndef _jc_typeinfo_h_
#define _jc_typeinfo_h_

#include "jc_string.h"

enum JC_TYPEMOD
{
	JC_VOID,
	JC_FLOAT,
	JC_DOUBLE,
	JC_CHAR,
	JC_SHORT,
	JC_INT,
	JC_LONG,

	JC_POINTER,

	JC_ENUM,
	JC_STRUCT,
	JC_UNION,
	JC_ARRAY,
	JC_FUNCTION,

	JC_ALIAS,

	JC_SIGNED = 0x00001000,
	JC_UNSIGNED = 0x00002000,

	JC_SIGNED_CHAR = JC_CHAR | JC_SIGNED,
	JC_UNSIGNED_CHAR = JC_CHAR | JC_UNSIGNED,
	JC_SIGNED_SHORT = JC_SHORT | JC_SIGNED,
	JC_UNSIGNED_SHORT = JC_SHORT | JC_UNSIGNED,
	JC_SIGNED_INT = JC_INT | JC_SIGNED,
	JC_UNSIGNED_INT = JC_INT | JC_UNSIGNED,
	JC_SIGNED_LONG = JC_LONG | JC_SIGNED,
	JC_UNSIGNED_LONG = JC_LONG | JC_UNSIGNED,
};

enum JC_STORAGEACCESS
{
	JC_NONE = 0,
	JC_TYPEDEF = 1,
	JC_EXTERN = 2,
	JC_HOST = 4,
	JC_SHARE = 8,
	JC_STATIC = 16,
	JC_AUTO = 32,
	JC_REGISTER = 64,
	JC_CONST = 128,
	JC_VOLATILE = 256
};

typedef struct CJcStructInfo CJcStructInfo;
typedef struct CJcFunctionInfo CJcFunctionInfo;
typedef struct CJcEnumInfo CJcEnumInfo;
typedef struct CJcArrayInfo CJcArrayInfo;
typedef struct CJcTypeInfo CJcTypeInfo;

struct CJcTypeInfo
{
	CJcString oName;
	jc_uint nQual;
	jc_uint nType;
	jc_uint nSize;
	union CJcTypeDetailInfo
	{
		CJcStructInfo* pStructInfo;
		CJcFunctionInfo* pFunctionInfo;
		CJcEnumInfo* pEnumInfo;
		CJcArrayInfo* pArrayInfo;
	}oInfo;
	CJcTypeInfo* pNext;
};

jc_void InitializeType0(CJcTypeInfo* pType);
jc_void InitializeType(CJcTypeInfo* pType,
					   jc_uint nQual,
					   jc_uint nType,
					   jc_char* sName,
					   CJcTypeInfo* pNextType,
					   jc_void* pInfo);
jc_void ClearType(CJcTypeInfo* pType);

CJcTypeInfo* CreateType(jc_uint nQual,
						jc_uint nType,
						jc_char* sName,
						CJcTypeInfo* pNextType,
						jc_void* pInfo);
CJcTypeInfo* CloneType(CJcTypeInfo* pType);
jc_void CoverType(CJcTypeInfo* pDstType, CJcTypeInfo* pSrcType);
jc_void DestroyType(CJcTypeInfo* pType);
CJcTypeInfo* GetOrigType(CJcTypeInfo* pType);
jc_uint AllowBitField(CJcTypeInfo* pType);
jc_uint GetQualCode(CJcTypeInfo* pType);
jc_void SetQualCode(CJcTypeInfo* pType, jc_uint nQual);
jc_uint GetTypeCode(CJcTypeInfo* pType);
jc_uint GetTypeSize(CJcTypeInfo* pType);
jc_uint GetTypeAlign(CJcTypeInfo* pType);
CJcStructInfo* GetStructInfo(CJcTypeInfo* pType);
CJcFunctionInfo* GetFunctionInfo(CJcTypeInfo* pType);
CJcEnumInfo* GetEnumInfo(CJcTypeInfo* pType);
CJcArrayInfo* GetArrayInfo(CJcTypeInfo* pType);
CJcTypeInfo* GetNextType(CJcTypeInfo* pType);
CJcString GetTypeName(CJcTypeInfo* pType, jc_bool bIncQual);
CJcString GetQualName(CJcTypeInfo* pType);
CJcString GetOrigTypeName(CJcTypeInfo* pType, jc_bool bIncQual);
jc_bool IsIntegralType(jc_uint nType);
jc_bool IsNumberalType(jc_uint nType);
jc_bool IsConstType(CJcTypeInfo* pType);
jc_void RemovePointer(CJcTypeInfo* pType);

#endif
