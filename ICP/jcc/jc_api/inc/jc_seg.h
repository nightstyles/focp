
#ifndef _jc_seg_h_
#define _jc_seg_h_

#include "jc_type.h"

typedef struct CJcSegment
{
	jc_uint nPos;
	jc_uint nSize;
	jc_char* pHead;
	jc_char cFillChar;
}CJcSegment;

jc_void InitializeSegment(CJcSegment* pSegment, jc_char cFillChar);

jc_void ClearSegment(CJcSegment* pSegment);
jc_void CompactSegment(CJcSegment* pSegment);

jc_uint PutInSegment(CJcSegment* pSegment, jc_void* pData, jc_uint nDataLen, jc_uint nAlign);

jc_uint GetSizeOfSegment(CJcSegment* pSegment);
jc_char* GetDataOfSegment(CJcSegment* pSegment);

jc_uint GetPosOfSegment(CJcSegment* pSegment);
jc_void SetPosOfSegment(CJcSegment* pSegment, jc_uint nPos);

#define RollBackSegment(pSegment, nOffset) \
do{\
	SetPosOfSegment(pSegment, GetPosOfSegment(pSegment)-nOffset);\
	CompactSegment(pSegment);\
}while(0)

#endif
