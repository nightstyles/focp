
#ifndef _jc_array_h_
#define _jc_array_h_

#include "jc_typeinfo.h"

struct CJcArrayInfo
{
	jc_uint nDim;
};

CJcArrayInfo* CreateArrayInfo(jc_uint nDim);
CJcArrayInfo* CloneArrayInfo(CJcArrayInfo* pSrc);
jc_void CoverArrayInfo(CJcArrayInfo* pDst, CJcArrayInfo* pSrc);
jc_void DestroyArrayInfo(CJcArrayInfo* pArray);

jc_uint GetArrayDim(CJcArrayInfo* pArray);

jc_uint ArrayImplemented(CJcArrayInfo* pArray);
jc_void FinishArrayDefine(CJcArrayInfo* pArray, jc_uint nDim);
jc_void ResetArrayDefine(CJcArrayInfo* pArray);

CJcString GetArrayTypeName(CJcArrayInfo* pArray);

#endif
