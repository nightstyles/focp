
#include "EhcBase.h"

#ifndef _EHC_STRING_H_
#define _EHC_STRING_H_

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

typedef struct CEhcString
{
	ehc_uint nLen;
	ehc_char* pStr;
}CEhcString;

EBS_API ehc_void InitializeString(CEhcString* pStr);
EBS_API ehc_void ClearString(CEhcString* pStr);
EBS_API CEhcString* CreateString(ehc_char* pStr);
EBS_API ehc_void DestroyString(CEhcString* pStr);
EBS_API ehc_void CoverString(CEhcString* pDstStr, ehc_char* pSrcStr);
EBS_API ehc_void AppendString(CEhcString* pDstStr, ehc_char* pSrcStr);
EBS_API ehc_char* GetString(CEhcString* pDstStr);

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
