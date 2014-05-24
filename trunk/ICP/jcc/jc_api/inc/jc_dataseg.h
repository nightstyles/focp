
#ifndef _jc_dataseg_h_
#define _jc_dataseg_h_

#include "jc_seg.h"
#include "jc_expr.h"
#include "jc_ins.h"

typedef struct CJcConst
{
	jc_uint nOffset;
	jc_uint nLen;
	jc_uint nAlign;
	struct CJcConst* pNext;
}CJcConst;

typedef struct CJcConstTable
{
	CJcConst* pHead;
	CJcConst* pTail;
}CJcConstTable;

jc_void InitializeConstTable(CJcConstTable* pTable);
jc_void ClearConstTable(CJcConstTable* pTable);

jc_uint PutConst(CJcSegment* pSegment, CJcConstTable* pTable, jc_void* pData, jc_uint nDataLen, jc_uint nAlign);
CJcConst* FindConst(CJcSegment* pSegment, CJcConstTable* pTable, jc_void* pData, jc_uint nDataLen, jc_uint nAlign);
jc_void CompactConstTable(CJcSegment* pSegment, CJcConstTable* pTable);
jc_uint GetConstant(CJcSegment* pSegment, CJcConstTable* pTable, CJcVal* pVal, CJcExpress* pConstExpr, jc_uint nDstType, jc_uint bAllowFloat);

#endif
