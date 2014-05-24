
#ifndef _jc_struct_h_
#define _jc_struct_h_

#include "jc_typeinfo.h"

typedef struct CJcStructFieldInfo
{
	CJcTypeInfo* pType;
	CJcString oName;
	jc_uint nOffset;
	jc_uint nBitCount;
	jc_uint nBitOffset;
	struct CJcStructFieldInfo* pNext;
}CJcStructFieldInfo;

struct CJcStructInfo
{
	jc_uint nImplemented;
	jc_uint nSize;
	jc_uint nAlign;
	jc_uint nFieldCount;
	CJcStructFieldInfo* pHead, * pTail;
};

jc_void InitializeStructInfo(CJcStructInfo* pStruct);
jc_void ClearStructInfo(CJcStructInfo* pStruct);

CJcStructInfo* CreateStructInfo();
CJcStructInfo* CloneStructInfo(CJcStructInfo* pSrc);
jc_void CoverStructInfo(CJcStructInfo* pDst, CJcStructInfo* pSrc);
jc_void DestroyStructInfo(CJcStructInfo* pStruct);

jc_uint GetStructSize(CJcStructInfo* pStruct);
jc_uint GetStructAlign(CJcStructInfo* pStruct);
jc_uint GetStructFieldCount(CJcStructInfo* pStruct);
CJcStructFieldInfo* GetStructFieldByIdx(CJcStructInfo* pStruct, jc_uint nIdx);
CJcStructFieldInfo* GetStructField(CJcStructInfo* pStruct, jc_char* sName);
jc_uint AddStructField(CJcStructInfo* pStruct,
					   jc_uint nStructType, 
					   jc_char* sName, 
					   CJcTypeInfo* pType, 
					   jc_uint nBitCount);

jc_uint StructImplemented(CJcStructInfo* pStruct);
jc_void FinishStructDefine(CJcStructInfo* pStruct);

jc_void ResetStructDefine(CJcStructInfo* pStruct);

#endif
