
#include "jc_dataseg.h"

jc_void InitializeConstTable(CJcConstTable* pTable)
{
	pTable->pHead = NULL;
	pTable->pTail = NULL;
}

jc_void ClearConstTable(CJcConstTable* pTable)
{
	while(pTable->pHead)
	{
		pTable->pTail = pTable->pHead->pNext;
		g_oInterface.Free(pTable->pHead);
		pTable->pHead = pTable->pTail;
	}
}

jc_uint PutConst(CJcSegment* pSegment, CJcConstTable* pTable, jc_void* pData, jc_uint nDataLen, jc_uint nAlign)
{
	CJcConst* pConst = FindConst(pSegment, pTable, pData, nDataLen, nAlign);
	if(!pConst)
	{
		pConst = New(CJcConst);
		pConst->nOffset = PutInSegment(pSegment, pData, nDataLen, nAlign);
		pConst->nLen = nDataLen;
		pConst->nAlign = nAlign;
		pConst->pNext = NULL;
		if(pTable->pTail)
			pTable->pTail->pNext = pConst;
		else
			pTable->pHead = pConst;
		pTable->pTail = pConst;
	}
	return pConst->nOffset;
}

#define OverFlowTest(pDstVal, TDstType, nSrcVal)\
do{\
	if(*(TDstType*)pDstVal != (TDstType)nSrcVal)\
		return 2;\
}while(0)

jc_uint GetConstant(CJcSegment* pSegment, CJcConstTable* pTable, CJcVal* pVal, CJcExpress* pConstExpr, jc_uint nDstType, jc_uint bAllowFloat)
{
	jc_uint nType;
	CJcConst *pConst;
	jc_char *pData;
	CJcTypeInfo* pType;

	if(pConstExpr->nOpt != JC_IS)
		return 0;

	pConst = pTable->pHead;
	pData = GetDataOfSegment(pSegment);
	while(pConst)
	{
		if(pConst->nOffset == pConstExpr->nArg)
			break;
		pConst = pConst->pNext;
	}
	if(!pConst)
		return 0;

	pVal->ul = 0;
	pData += pConstExpr->nArg;
	pType = GetOrigType(pConstExpr->pType);
	nType = GetTypeCode(pType);
	switch(nDstType)
	{
	default: 
		return 0;

	case JC_POINTER:
		switch(nType)
		{
		default:
			return 0;
		case JC_POINTER:
			if(sizeof(void*) == sizeof(jc_uint))
				pVal->ui = (jc_uint)((jc_uint*)pData)[0];
			else
				pVal->ul = (jc_uint)((jc_ulong*)pData)[0];
			break;
		}
		break;

	case JC_SIGNED_CHAR:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR:
			pVal->c = (jc_char)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->c = (jc_char)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->c = (jc_char)((jc_short*)pData)[0];
			OverFlowTest(pData, jc_short, pVal->c);
			break;
		case JC_UNSIGNED_SHORT:
			pVal->c = (jc_char)((jc_ushort*)pData)[0];
			OverFlowTest(pData, jc_ushort, pVal->c);
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->c = (jc_char)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->c);
			break;
		case JC_UNSIGNED_INT:
			pVal->c = (jc_char)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->c);
			break;
		case JC_SIGNED_LONG:
			pVal->c = (jc_char)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->c);
			break;
		case JC_UNSIGNED_LONG:
			pVal->c = (jc_char)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->c);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->c = (jc_char)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->c);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->c = (jc_char)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->c);
			break;
		}
		break;
	case JC_UNSIGNED_CHAR:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->uc = (jc_uchar)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->uc = (jc_uchar)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->uc = (jc_uchar)((jc_short*)pData)[0];
			OverFlowTest(pData, jc_short, pVal->uc);
			break;
		case JC_UNSIGNED_SHORT:
			pVal->uc = (jc_uchar)((jc_ushort*)pData)[0];
			OverFlowTest(pData, jc_ushort, pVal->uc);
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->uc = (jc_uchar)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->uc);
			break;
		case JC_UNSIGNED_INT:
			pVal->uc = (jc_uchar)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->uc);
			break;
		case JC_SIGNED_LONG:
			pVal->uc = (jc_uchar)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->uc);
			break;
		case JC_UNSIGNED_LONG:
			pVal->uc = (jc_uchar)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->uc);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->uc = (jc_uchar)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->uc);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->uc = (jc_uchar)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->uc);
			break;
		}
		break;
	case JC_SIGNED_SHORT:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->s = (jc_short)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->s = (jc_short)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->s = (jc_short)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->s = (jc_short)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->s = (jc_short)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->s);
			break;
		case JC_UNSIGNED_INT:
			pVal->s = (jc_short)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->s);
			break;
		case JC_SIGNED_LONG:
			pVal->s = (jc_short)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->s);
			break;
		case JC_UNSIGNED_LONG:
			pVal->s = (jc_short)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->s);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->s = (jc_short)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->s);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->s = (jc_short)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->s);
			break;
		}
		break;
	case JC_UNSIGNED_SHORT:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->us = (jc_ushort)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->us = (jc_ushort)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->us = (jc_ushort)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->us = (jc_ushort)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->us = (jc_ushort)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->us);
			break;
		case JC_UNSIGNED_INT:
			pVal->us = (jc_ushort)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->us);
			break;
		case JC_SIGNED_LONG:
			pVal->us = (jc_ushort)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->us);
			break;
		case JC_UNSIGNED_LONG:
			pVal->us = (jc_ushort)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->us);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->us = (jc_ushort)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->us);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->us = (jc_ushort)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->us);
			break;
		}
		break;
	case JC_SIGNED_INT: case JC_ENUM:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->i = (jc_int)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->i = (jc_int)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->i = (jc_int)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->i = (jc_int)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->i = (jc_int)((jc_int*)pData)[0];
			break;
		case JC_UNSIGNED_INT:
			pVal->i = (jc_int)((jc_uint*)pData)[0];
			break;
		case JC_SIGNED_LONG:
			pVal->i = (jc_int)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->i);
			break;
		case JC_UNSIGNED_LONG:
			pVal->i = (jc_int)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->i);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->i = (jc_int)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->i);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->i = (jc_int)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->i);
			break;
		}
		break;
	case JC_UNSIGNED_INT:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->ui = (jc_uint)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->ui = (jc_uint)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->ui = (jc_uint)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->ui = (jc_uint)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->ui = (jc_uint)((jc_int*)pData)[0];
			break;
		case JC_UNSIGNED_INT:
			pVal->ui = (jc_uint)((jc_uint*)pData)[0];
			break;
		case JC_SIGNED_LONG:
			pVal->ui = (jc_uint)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->ui);
			break;
		case JC_UNSIGNED_LONG:
			pVal->ui = (jc_uint)((jc_ulong*)pData)[0];
			OverFlowTest(pData, jc_ulong, pVal->ui);
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->ui = (jc_uint)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->ui);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->ui = (jc_uint)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->ui);
			break;
		}
		break;
	case JC_SIGNED_LONG:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->l = (jc_long)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->l = (jc_long)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->l = (jc_long)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->l = (jc_long)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->l = (jc_long)((jc_int*)pData)[0];
			break;
		case JC_UNSIGNED_INT:
			pVal->l = (jc_long)((jc_uint*)pData)[0];
			break;
		case JC_SIGNED_LONG:
			pVal->l = (jc_long)((jc_long*)pData)[0];
			break;
		case JC_UNSIGNED_LONG:
			pVal->l = (jc_long)((jc_ulong*)pData)[0];
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->l = (jc_long)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->i);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->l = (jc_long)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->i);
			break;
		}
		break;
	case JC_UNSIGNED_LONG:
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->ul = (jc_ulong)((jc_char*)pData)[0];
			break;
		case JC_UNSIGNED_CHAR:
			pVal->ul = (jc_ulong)((jc_uchar*)pData)[0];
			break;
		case JC_SIGNED_SHORT:
			pVal->ul = (jc_ulong)((jc_short*)pData)[0];
			break;
		case JC_UNSIGNED_SHORT:
			pVal->ul = (jc_ulong)((jc_ushort*)pData)[0];
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->ul = (jc_ulong)((jc_int*)pData)[0];
			break;
		case JC_UNSIGNED_INT:
			pVal->ul = (jc_ulong)((jc_uint*)pData)[0];
			break;
		case JC_SIGNED_LONG:
			pVal->ul = (jc_ulong)((jc_long*)pData)[0];
			break;
		case JC_UNSIGNED_LONG:
			pVal->ul = (jc_ulong)((jc_ulong*)pData)[0];
			break;
		case JC_FLOAT:
			if(!bAllowFloat)
				return 0;
			pVal->ul = (jc_ulong)((jc_float*)pData)[0];
			OverFlowTest(pData, jc_float, pVal->l);
			break;
		case JC_DOUBLE:
			if(!bAllowFloat)
				return 0;
			pVal->ul = (jc_ulong)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->l);
			break;
		}
		break;
	case JC_FLOAT:
		if(!bAllowFloat)
			return 0;
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->f = (jc_float)((jc_char*)pData)[0];
			OverFlowTest(pData, jc_char, pVal->f);
			break;
		case JC_UNSIGNED_CHAR:
			pVal->f = (jc_float)((jc_uchar*)pData)[0];
			OverFlowTest(pData, jc_uchar, pVal->f);
			break;
		case JC_SIGNED_SHORT:
			pVal->f = (jc_float)((jc_short*)pData)[0];
			OverFlowTest(pData, jc_short, pVal->f);
			break;
		case JC_UNSIGNED_SHORT:
			pVal->f = (jc_float)((jc_ushort*)pData)[0];
			OverFlowTest(pData, jc_ushort, pVal->f);
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->f = (jc_float)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->f);
			break;
		case JC_UNSIGNED_INT:
			pVal->f = (jc_float)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->f);
			break;
		case JC_SIGNED_LONG:
		case JC_UNSIGNED_LONG:
			pVal->f = (jc_float)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->f);
			break;
		case JC_FLOAT:
			pVal->f = (jc_float)((jc_float*)pData)[0];
			break;
		case JC_DOUBLE:
			pVal->f = (jc_float)((jc_double*)pData)[0];
			OverFlowTest(pData, jc_double, pVal->f);
			break;
		}
		break;
	case JC_DOUBLE:
		if(!bAllowFloat)
			return 0;
		switch(nType)
		{
		default:
			return 0;
		case JC_SIGNED_CHAR: 
			pVal->d = (jc_double)((jc_char*)pData)[0];
			OverFlowTest(pData, jc_char, pVal->d);
			break;
		case JC_UNSIGNED_CHAR:
			pVal->d = (jc_double)((jc_uchar*)pData)[0];
			OverFlowTest(pData, jc_uchar, pVal->d);
			break;
		case JC_SIGNED_SHORT:
			pVal->d = (jc_double)((jc_short*)pData)[0];
			OverFlowTest(pData, jc_short, pVal->d);
			break;
		case JC_UNSIGNED_SHORT:
			pVal->d = (jc_double)((jc_ushort*)pData)[0];
			OverFlowTest(pData, jc_ushort, pVal->d);
			break;
		case JC_SIGNED_INT: case JC_ENUM:
			pVal->d = (jc_double)((jc_int*)pData)[0];
			OverFlowTest(pData, jc_int, pVal->d);
			break;
		case JC_UNSIGNED_INT:
			pVal->d = (jc_double)((jc_uint*)pData)[0];
			OverFlowTest(pData, jc_uint, pVal->d);
			break;
		case JC_SIGNED_LONG:
		case JC_UNSIGNED_LONG:
			pVal->d = (jc_double)((jc_long*)pData)[0];
			OverFlowTest(pData, jc_long, pVal->d);
			break;
		case JC_FLOAT:
			pVal->d = (jc_double)((jc_float*)pData)[0];
			break;
		case JC_DOUBLE:
			pVal->d = (jc_double)((jc_double*)pData)[0];
			break;
		}
		break;
	}
	return 1;
}

CJcConst* FindConst(CJcSegment* pSegment, CJcConstTable* pTable, jc_void* pData, jc_uint nDataLen, jc_uint nAlign)
{
	jc_int nRet;
	jc_uint nLen;
	CJcConst *pConst = pTable->pHead;
	jc_char *pBuf, *pSrc = GetDataOfSegment(pSegment);
	while(pConst)
	{
		if(pConst->nAlign == nAlign)
		{
			nLen = pConst->nLen;
			if(nLen > nDataLen)
				nLen = nDataLen;
			pBuf = pSrc + pConst->nOffset;
			nRet = MemoryCompare(pBuf, pData, nLen);
			if(!nRet)
				break;
		}
		pConst = pConst->pNext;
	}
	return pConst;
}

jc_void CompactConstTable(CJcSegment* pSegment, CJcConstTable* pTable)
{
	jc_uint nPos = GetPosOfSegment(pSegment);
	CJcConst *pNext, *pHead = pTable->pHead, *pPrev = NULL;
	while(pHead)
	{
		pNext = pHead->pNext;
		if(pHead->nOffset >= nPos)
		{			
			if(pHead == pTable->pTail)
				pTable->pTail = pPrev;
			if(pPrev)
				pPrev->pNext = pNext;
			else
				pTable->pHead = pNext;
			g_oInterface.Free(pHead);
		}
		else
			pPrev = pHead;
		pHead = pNext;
	}
}
