
#ifndef _jc_string_h_
#define _jc_string_h_

#include "jc_type.h"

typedef struct CJcString
{
	jc_uint nLen;
	jc_char* pStr;
}CJcString;

jc_void InitializeString(CJcString* pStr);
jc_void ClearString(CJcString* pStr);
CJcString* CreateString(jc_char* pStr);
jc_void DestroyString(CJcString* pStr);
jc_void CoverString(CJcString* pDstStr, jc_char* pSrcStr);
jc_void AppendString(CJcString* pDstStr, jc_char* pSrcStr);
jc_char* GetString(CJcString* pDstStr);

#endif
