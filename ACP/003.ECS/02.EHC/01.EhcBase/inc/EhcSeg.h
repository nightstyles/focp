
#include "EhcBase.h"

#ifndef _EHC_SEG_H_
#define _EHC_SEG_H_

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

typedef struct CEhcSegment
{
	ehc_uint nPos;
	ehc_uint nSize;
	ehc_char* pHead;
	ehc_char cFillChar;
}CEhcSegment;

EBS_API ehc_void InitializeSegment(CEhcSegment* pSegment, ehc_char cFillChar);

EBS_API ehc_void ClearSegment(CEhcSegment* pSegment);
EBS_API ehc_void CompactSegment(CEhcSegment* pSegment);

EBS_API ehc_uint PutInSegment(CEhcSegment* pSegment, ehc_void* pData, ehc_uint nDataLen, ehc_uint nAlign);

EBS_API ehc_uint GetSizeOfSegment(CEhcSegment* pSegment);
EBS_API ehc_char* GetDataOfSegment(CEhcSegment* pSegment);

EBS_API ehc_uint GetPosOfSegment(CEhcSegment* pSegment);
EBS_API ehc_void SetPosOfSegment(CEhcSegment* pSegment, ehc_uint nPos);

#define RollBackSegment(pSegment, nOffset) \
do{\
	SetPosOfSegment(pSegment, GetPosOfSegment(pSegment)-nOffset);\
	CompactSegment(pSegment);\
}while(0)

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
