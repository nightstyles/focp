
#ifndef _jc_enum_h_
#define _jc_enum_h_

#include "jc_typeinfo.h"

struct CJcEnumInfo
{
	jc_uint nImplemented;
	jc_int nNextValue;
};

CJcEnumInfo* CreateEnumInfo();
CJcEnumInfo* CloneEnumInfo(CJcEnumInfo* pSrc);
jc_void CoverEnumInfo(CJcEnumInfo* pDst, CJcEnumInfo* pSrc);
jc_void DestroyEnumInfo(CJcEnumInfo* pEnum);

jc_int GetNextEnumValue(CJcEnumInfo* pEnum);
jc_void UseEnumValue(CJcEnumInfo* pEnum, jc_int nVal);

jc_uint EnumImplemented(CJcEnumInfo* pEnum);
jc_void FinishEnumDefine(CJcEnumInfo* pEnum);
jc_void ResetEnumDefine(CJcEnumInfo* pEnum);

#endif
