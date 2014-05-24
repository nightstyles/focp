
#ifndef _jc_codeseg_h_
#define _jc_codeseg_h_

#include "jc_seg.h"

#define JC_DEFAULT_INSADDR 0xFFFFFFFF

jc_void Emit0(CJcSegment* pSegment, 
			  jc_ushort nOp,
			  jc_uint nPos);
jc_void Emit1(CJcSegment* pSegment, 
			  jc_ushort nOp, 
			  jc_uchar nOpt, jc_uint nArg, 
			  jc_uint nPos);
jc_void Emit2(CJcSegment* pSegment, 
			  jc_ushort nOp, 
			  jc_uchar nOpt1, jc_uint nArg1, 
			  jc_uchar nOpt2, jc_uint nArg2, 
			  jc_uint nPos);

#endif
