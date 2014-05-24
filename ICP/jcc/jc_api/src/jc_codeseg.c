
#include "jc_codeseg.h"
#include "jc_ins.h"

jc_void Emit0(CJcSegment* pSegment, jc_ushort nOp, jc_uint nPos)
{
	jc_uint nNowPos;
	CJcIns oIns = {nOp, {0, 0}};
	if(nPos == JC_DEFAULT_INSADDR)
		PutInSegment(pSegment, &oIns, 4, 4);
	else
	{
		nNowPos = GetPosOfSegment(pSegment);
		SetPosOfSegment(pSegment, nPos);
		PutInSegment(pSegment, &oIns, 4, 4);
		SetPosOfSegment(pSegment, nNowPos);
	}
}

jc_void Emit1(CJcSegment* pSegment, jc_ushort nOp, jc_uchar nOpt, jc_uint nArg, jc_uint nPos)
{
	jc_uint nNowPos;
	CJcIns oIns = {nOp, {nOpt, 0}, {nArg}};
	jc_uint nLen = 8;
	if(nPos == JC_DEFAULT_INSADDR)
		PutInSegment(pSegment, &oIns, nLen, 4);
	else
	{
		nNowPos = GetPosOfSegment(pSegment);
		SetPosOfSegment(pSegment, nPos);
		PutInSegment(pSegment, &oIns, nLen, 4);
		SetPosOfSegment(pSegment, nNowPos);
	}
}

jc_void Emit2(CJcSegment* pSegment, jc_ushort nOp, jc_uchar nOpt1, jc_uint nArg1, jc_uchar nOpt2, jc_uint nArg2, jc_uint nPos)
{
	jc_uint nNowPos;
	CJcIns oIns = {nOp, {nOpt1, nOpt2}, {nArg1, nArg2}};
	jc_uint nLen = 12;
	if(nPos == JC_DEFAULT_INSADDR)
		PutInSegment(pSegment, &oIns, nLen, 4);
	else
	{
		nNowPos = GetPosOfSegment(pSegment);
		SetPosOfSegment(pSegment, nPos);
		PutInSegment(pSegment, &oIns, nLen, 4);
		SetPosOfSegment(pSegment, nNowPos);
	}
}
