
#include "jc_codeseg.h"
#include "jc_array.h"
#include "jc_struct.h"
#include "jc_enum.h"
#include "jc_symbol.h"

void SetError(CJcParser* pParser, int bWarning, const char* msg, ...);

static jc_void PrintfVal(jc_char* pBuf, CJcVal* pVal, jc_uint nType);
static CJcVal GetBitVal(jc_uint nAlignCode, jc_uint nBits);
static jc_void BuildCompareExpress(CJcExpress* pExp, jc_uint nCompare);
static jc_uint GetCompareInstruction(jc_uint nType, jc_uint nOp);
static CJcTypeInfo* GetAutoCastType(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_uint nOp);
static const char* GetAssignOperatorString(jc_uint nAssignOperator);
static jc_uint CheckAutoAssignCast(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_bool bConstNull);
static jc_uint CheckAutoArgumentCast(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_uint nArgIdx, jc_bool bConstNull);
static jc_ushort GetAssignInstruction(jc_uint nType, jc_uint nAssignOperator);
static jc_void LoadIntegerExpress(CJcExpress* pExp);
static jc_void BuildConstAssignmentExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nAssignOperator);

jc_void InitializeExpress(CJcSymbolStack* pStack, CJcExpress* pExp)
{
	pExp->pType = NULL;
	pExp->nVarId = -1;
	pExp->nOpt = JC_UN;
	pExp->nArg = 0;
	pExp->nBits = 0;
	pExp->nStart = 0;
	pExp->nQuote = 0;
	pExp->nLeftValue = 0;
	pExp->pStack = pStack;
}

jc_void DestroyExpress(CJcExpress* pExp)
{
	if(pExp->pType)
	{
		DestroyType(pExp->pType);
		pExp->pType = NULL;
	}
	FreeTempVar(pExp);
}

jc_void AllocTempVar(CJcExpress *pExp)
{
	CJcSymbolStack* pStack = pExp->pStack;
	CJcLocalStack* pLocalStack = (pStack->pInFunction?(&pStack->oLocalStack):(&pStack->oGlobalStack));
	pExp->nOpt = JC_LS;
	pExp->nArg = AllocVarInStack(pLocalStack, GetTypeSize(pExp->pType), GetTypeAlign(pExp->pType), (jc_uint*)&pExp->nVarId);
}

jc_void FreeTempVar(CJcExpress *pExp)
{
	if(pExp->nVarId >= 0)
	{
		CJcSymbolStack* pStack = pExp->pStack;
		CJcLocalStack* pLocalStack = (pStack->pInFunction?(&pStack->oLocalStack):(&pStack->oGlobalStack));
		FreeVarFromStack(pLocalStack, pExp->nVarId);
		pExp->nVarId = -1;
	}
}

jc_void UpdateExpressType(CJcExpress *pExp, CJcTypeInfo* pType)
{
	if(pType != pExp->pType)
	{
		pType = CloneType(pType);
		CoverType(pExp->pType, pType);
		DestroyType(pType);
	}
}

jc_bool CheckCondExpress(CJcExpress *pExp)
{
	jc_bool bRet;
	switch(GetTypeCode(GetOrigType(pExp->pType)))
	{
	default:
		SetError(pExp->pStack->pParser, 0, "condition express must be number type or pointer type");
		bRet = False;
		break;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
	case JC_FLOAT:
	case JC_DOUBLE:
	case JC_POINTER:
		bRet = True;
		break;
	}
	return bRet;
}

jc_void CheckSwitchExpress(CJcExpress *pExp)
{
	switch(GetTypeCode(GetOrigType(pExp->pType)))
	{
	default:
		SetError(pExp->pStack->pParser, 1, "switch express must be integral type");
		break;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		break;
	}
}

jc_void CheckCaseExpress(CJcExpress *pExp)
{
	switch(GetTypeCode(GetOrigType(pExp->pType)))
	{
	default:
		SetError(pExp->pStack->pParser, 0, "case express must be integralable type");
		break;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		break;
	}
	if(pExp->nOpt != JC_IS)
		SetError(pExp->pStack->pParser, 0, "case express must be immediate constant");
}

jc_uint GetConstValue(CJcExpress *pExp, CJcVal* pVal, jc_uint nDstType, jc_uint bAllowFloat)
{
	CJcSymbolStack* pStack = pExp->pStack;
	return !GetConstant(&pStack->oConstSegment, &pStack->oConstTable, pVal, pExp, nDstType, bAllowFloat);
}

jc_uint GetEnumValue(CJcExpress *pExp, jc_int *pEnumValue)
{
	CJcVal v;
	jc_uint nRet = GetConstValue(pExp, &v, JC_SIGNED_INT, 0);
	pEnumValue[0] = v.i;
	return nRet;
}

CJcExpress BuildCharConstantExpress(CJcSymbolStack* pStack, jc_char* pVal)
{
	CJcExpress oExp;
	jc_uchar c, d;
	jc_char* val = pVal;
	jc_char* s = val + 1;
	if(s[0] != '\\')
		c = s[0];
	else
	{
		++s;
		switch(s[0])
		{
		case 'a': c = '\a'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case '\'': c = '\''; break;
		case '\"': c = '\"'; break;
		case '\\': c = '\\'; break;
		case '?': c = '?'; break;
		default:
			if(s[0] == 'x' || s[0] == 'X')
			{
				++s;
				c = s[0];
				if(c >= '0' && c <= '9')
					c -= '0';
				else if(c >= 'A' && c <= 'F')
					c = c - 'A' + 10;
				else
					c = c - 'a' + 10;
				s++;
				if(s[0] != '\'')
				{
					c <<= 4;
					d = s[0];
					if(d >= '0' && d <= '9')
						d -= '0';
					else if(d >= 'A' && d <= 'F')
						d = d - 'A' + 10;
					else
						d = d - 'a' + 10;
					c |= d;
				}
			}
			else
			{
				c = s[0] - '0';
				++s;
				if(s[0] != '\'')
				{
					c <<= 3;
					d = s[0] - '0';
					c |= d;
					++s;
					if(s[0] != '\'')
					{
						d = s[0] - '0';
						if(c>(d|(c<<3)))
							SetError(pStack->pParser, 1, "octal char:%u overflow", (((jc_uint)c)<<3)|d);
						c <<= 3;
						c |= d;
					}
				}
			}
		}
	}
	InitializeExpress(pStack, &oExp);
	oExp.pType = CreateType(JC_CONST, JC_SIGNED_CHAR, NULL, NULL, NULL);
	oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &c, 1, 1);
	oExp.nOpt = JC_IS;
	return oExp;
}

CJcExpress BuildStringConstantExpress(CJcSymbolStack* pStack, jc_char* pVal)
{
	CJcExpress oExp;
	jc_uchar c, d;
	jc_char* val = pVal;
	jc_char* sSrc = val+1, * sDst = val;
	while(sSrc[0])
	{
		if(sSrc[0] == '\"')
			break;
		if(sSrc[0] != '\\')
			*sDst = sSrc[0];
		else
		{
			++sSrc;
			switch(sSrc[0])
			{
			case 'a': *sDst = '\a'; break;
			case 'b': *sDst = '\b'; break;
			case 'f': *sDst = '\f'; break;
			case 'n': *sDst = '\n'; break;
			case 'r': *sDst = '\r'; break;
			case 't': *sDst = '\t'; break;
			case 'v': *sDst = '\v'; break;
			case '\'': *sDst = '\''; break;
			case '\"': *sDst = '\"'; break;
			case '\\': *sDst = '\\'; break;
			case '?': *sDst = '?'; break;
			default:
				if(sSrc[0] == 'x' || sSrc[0] == 'X')
				{
					++sSrc;
					c = sSrc[0];
					if(c >= '0' && c <= '9')
						c -= '0';
					else if(c >= 'A' && c <= 'F')
						c = c - 'A' + 10;
					else
						c = c - 'a' + 10;
					sSrc++;
					if(sSrc[0] != '\"')
					{
						c <<= 4;
						d = sSrc[0];
						if(d >= '0' && d <= '9')
							d -= '0';
						else if(d >= 'A' && d <= 'F')
							d = d - 'A' + 10;
						else
							d = d - 'a' + 10;
						c |= d;
					}
				}
				else
				{
					c = sSrc[0] - '0';
					++sSrc;
					if(sSrc[0] != '\"')
					{
						c <<= 3;
						d = sSrc[0] - '0';
						c |= d;
						++sSrc;
						if(sSrc[0] != '\"')
						{
							d = sSrc[0] - '0';
							if(c>(d|(c<<3)))
								SetError(pStack->pParser, 1, "octal char:%u overflow", (((jc_uint)c)<<3)|d);
							c <<= 3;
							c |= d;
						}
					}
				}
				*sDst = c;
			}
		}
		++sSrc; ++sDst;
	}
	*sDst = 0;

	InitializeExpress(pStack, &oExp);
	oExp.pType = CreateType(JC_CONST, JC_ARRAY, NULL,
								CreateType(JC_CONST, JC_SIGNED_CHAR, NULL, NULL, NULL),
								CreateArrayInfo(sDst - val + 1));
	oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, val, sDst - val + 1, 1);
	oExp.nOpt = JC_IS;
	return oExp;
}

CJcExpress BuildIntegerConstantExpress(CJcSymbolStack* pStack, jc_char* pVal)
{
	enum
	{
		JCC_SUFFIX_NON = 0,
		JCC_SUFFIX_U = 1,
		JCC_SUFFIX_L = 2,
		JCC_SUFFIX_I = 4
	};
	enum
	{
		JCC_INT_CONST_0 = 0,
		JCC_INT_CONST_8 = 8,
		JCC_INT_CONST_16 = 16,
		JCC_INT_CONST_32 = 32,
		JCC_INT_CONST_64 = 64
	};
	enum
	{
		JCC_INT_CONST_SIGNED = 1,
		JCC_INT_CONST_UNSIGNED = 2
	};

	CJcExpress oExp;
	jc_int nSys = 10;
	jc_ulong ul = 0;
	jc_char* val = pVal;
	jc_uchar* s = (jc_uchar*)val;
	jc_uint nSuffixType = JCC_SUFFIX_NON;
	jc_uint nConstBits = JCC_INT_CONST_0;
	jc_uint nSigned = JCC_INT_CONST_0;
	jc_ulong c, d;
	jc_uchar a1; jc_ushort a2; jc_uint a3;
	if(s[0] == '0')
	{
		++s;
		nSys = 8;
		if(s[0] == 'x' || s[0] == 'X')
		{
			nSys = 16;
			++s;
		}
	}
	while(s[0])
	{
		if(s[0] == 'u' || s[0] =='U')
		{
			nSuffixType = JCC_SUFFIX_U;
			nSigned = JCC_INT_CONST_UNSIGNED;
			++s;
			break;
		}
		if(s[0] == 'l' || s[0] =='L')
		{
			nSuffixType = JCC_SUFFIX_L;
			nSigned = JCC_INT_CONST_SIGNED;
			++s;
			break;
		}
		if(s[0] == 'i' || s[0] =='I')
		{
			nSuffixType = JCC_SUFFIX_I;
			nSigned = JCC_INT_CONST_SIGNED;
			++s;
			break;
		}
		d = ul * nSys;
		if(d < ul)
			SetError(pStack->pParser, 1, "integer constant overflow");
		ul = d;
		c = s[0];
		if(c >= '0' && c <= '9')
			c -= '0';
		else if(c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if(c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		d += c;
		if(d < ul)
			SetError(pStack->pParser, 1, "integer constant overflow");
		ul = d;
		++s;
	}
	if(nSuffixType == JCC_SUFFIX_U)
	{
		nConstBits = JCC_INT_CONST_32;
		if(s[0] == 'l' || s[0] == 'L')
			nConstBits = JCC_INT_CONST_64;
		else
		{
			if(!StringCompare(s, "8"))
				nConstBits = JCC_INT_CONST_8;
			else if(!StringCompare(s, "16"))
				nConstBits = JCC_INT_CONST_16;
			else if(!StringCompare(s, "32"))
				nConstBits = JCC_INT_CONST_32;
			else if(!StringCompare(s, "64"))
				nConstBits = JCC_INT_CONST_64;
		}
	}
	else if(nSuffixType == JCC_SUFFIX_L)
	{
		nConstBits = JCC_INT_CONST_64;
		if(s[0] == 'u' || s[0] == 'U')
			nSigned = JCC_INT_CONST_UNSIGNED;
	}
	else if(nSuffixType == JCC_SUFFIX_I)
	{
		if(!StringCompare(s, "8"))
			nConstBits = JCC_INT_CONST_8;
		else if(!StringCompare(s, "16"))
			nConstBits = JCC_INT_CONST_16;
		else if(!StringCompare(s, "32"))
			nConstBits = JCC_INT_CONST_32;
		else if(!StringCompare(s, "64"))
			nConstBits = JCC_INT_CONST_64;
	}
	else
	{
		nSigned = JCC_INT_CONST_SIGNED;
		if(ul >> 32)
		{
			nConstBits = JCC_INT_CONST_64;
			if(0 > (jc_long)ul)
				nSigned = JCC_INT_CONST_UNSIGNED;
		}
		else
		{
			nConstBits = JCC_INT_CONST_32;
			if(0 > (jc_int)ul)
				nSigned = JCC_INT_CONST_UNSIGNED;
		}
	}
	InitializeExpress(pStack, &oExp);
	oExp.nOpt = JC_IS;
	switch(nConstBits)
	{
	case JCC_INT_CONST_8:
		if(nSigned == JCC_INT_CONST_SIGNED && ul > 0x7F || ul > 0xFF)
			SetError(pStack->pParser, 1, "integer constant overflow");
		a1 = (jc_uchar)ul;
		oExp.pType = CreateType(JC_CONST, ((nSigned == JCC_INT_CONST_SIGNED)?JC_SIGNED_CHAR:JC_UNSIGNED_CHAR), NULL, NULL, NULL);
		oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &a1, 1, 1);
		break;
	case JCC_INT_CONST_16:
		if(nSigned == JCC_INT_CONST_SIGNED && ul > 0x7FFF || ul > 0xFFFF)
			SetError(pStack->pParser, 1, "integer constant overflow");
		a2 = (jc_ushort)ul;
		oExp.pType = CreateType(JC_CONST, ((nSigned == JCC_INT_CONST_SIGNED)?JC_SIGNED_SHORT:JC_UNSIGNED_SHORT), NULL, NULL, NULL);
		oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &a2, 2, 2);
		break;
	case JCC_INT_CONST_32:
		if(nSigned == JCC_INT_CONST_SIGNED && ul > 0x7FFFFFFF || ul > 0xFFFFFFFF)
			SetError(pStack->pParser, 1, "integer constant overflow");
		a3 = (jc_uint)ul;
		oExp.pType = CreateType(JC_CONST, ((nSigned == JCC_INT_CONST_SIGNED)?JC_SIGNED_INT:JC_UNSIGNED_INT), NULL, NULL, NULL);
		oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &a3, 4, 4);
		break;
	case JCC_INT_CONST_64:
		if(nSigned == JCC_INT_CONST_SIGNED && ul > 0x7FFFFFFFFFFFFFFF)
			SetError(pStack->pParser, 1, "integer constant overflow");
		oExp.pType = CreateType(JC_CONST, ((nSigned == JCC_INT_CONST_SIGNED)?JC_SIGNED_LONG:JC_UNSIGNED_LONG), NULL, NULL, NULL);
		oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &ul, 8, 8);
		break;
	}
	return oExp;
}

CJcExpress BuildIntegerConstantExpressA(CJcSymbolStack* pStack, jc_uint nVal, jc_bool bSigned)
{
	CJcExpress oExp;
	InitializeExpress(pStack, &oExp);
	oExp.nOpt = JC_IS;
	oExp.pType = CreateType(JC_CONST, (bSigned?JC_SIGNED_INT:JC_UNSIGNED_INT), NULL, NULL, NULL);
	oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nVal, 4, 4);
	return oExp;
}

CJcExpress BuildLongConstantExpressA(CJcSymbolStack* pStack, jc_ulong nVal, jc_bool bSigned)
{
	CJcExpress oExp;
	InitializeExpress(pStack, &oExp);
	oExp.nOpt = JC_IS;
	oExp.pType = CreateType(JC_CONST, (bSigned?JC_SIGNED_LONG:JC_UNSIGNED_LONG), NULL, NULL, NULL);
	oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nVal, 8, 8);
	return oExp;
}

CJcExpress BuildFloatConstantExpress(CJcSymbolStack* pStack, jc_char* pVal)
{
	jc_char *end, *val;
	jc_float f;
	jc_double d;
	jc_uint nFloatType;
	jc_bool bError;
	CJcExpress oExp;

	val = pVal;
	nFloatType = JC_DOUBLE;

	d = g_oInterface.StringToDouble(val, &end, &bError);

	if(bError)
	{
		if(!d)
			SetError(pStack->pParser, 1, "float constant downflow");
		else
			SetError(pStack->pParser, 1, "float constant overflow");
	}
	if(end && end[0] == 'F' || end[0] == 'f')
	{
		nFloatType = JC_FLOAT;
		f = (jc_float)d;
		if(!bError)
		{
			if(*(jc_uint*)&f == 0x7F800000)
				SetError(pStack->pParser, 1, "float constant overflow");
			if(!f && d)
				SetError(pStack->pParser, 1, "float constant downflow");
		}
	}
	InitializeExpress(pStack, &oExp);
	oExp.nOpt = JC_IS;
	oExp.pType = CreateType(JC_CONST, nFloatType, NULL, NULL, NULL);
	oExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable,
		(nFloatType == JC_FLOAT)?((jc_void*)&f):((jc_void*)&d), GetTypeSize(oExp.pType), GetTypeAlign(oExp.pType));
	return oExp;
}

CJcExpress BuildSymbolExpress(CJcSymbolStack* pStack, CJcSymbol* pSymbol)
{
	CJcExpress oExp;
	InitializeExpress(pStack, &oExp);
	if(pSymbol->nSymbol == JC_VARIABLE_SYMBOL)
	{
		CJcVariableInfo* pVariable = pSymbol->info.pVariable;
		if(pVariable->nOpt == JC_SS)
		{
			CJcProtoType* pProto = GetProtoType(pStack, pVariable->nArg);
			pProto->nUsed = 1;
			pSymbol = &pProto->oSymbol;
			if(pSymbol->info.pVariable->nOpt && pSymbol->info.pVariable->nOpt != JC_HS)
				pVariable = pSymbol->info.pVariable;
		}
		oExp.pType = CloneType(pVariable->pType);
		oExp.nOpt = pVariable->nOpt;
		oExp.nArg = pVariable->nArg;
		oExp.nLeftValue = (IsConstType(oExp.pType)?0:1);
	}
	else if(pSymbol->nSymbol == JC_FUNCTION_SYMBOL)
	{
		CJcFunctionInfo* pFunction = pSymbol->info.pFunction;
		if(pFunction->nOpt == JC_SS)
		{
			CJcProtoType* pProto = GetProtoType(pStack, pFunction->nArg);
			pProto->nUsed = 1;
			pSymbol = &pProto->oSymbol;
			if(pSymbol->info.pFunction->nOpt && pSymbol->info.pFunction->nOpt != JC_HS)
				pFunction = pSymbol->info.pFunction;
		}
		oExp.pType = CreateType(JC_CONST, JC_FUNCTION, NULL, NULL, CloneFunctionInfo(pFunction));
		oExp.nOpt = pFunction->nOpt;
		oExp.nArg = pFunction->nArg;
		oExp.nLeftValue = 0;
	}
	else
	{
		CJcTypeInfo * pType = pSymbol->info.pType;
		oExp.pType = CloneType(pType);
		oExp.nOpt = JC_IS;
		oExp.nArg = GetEnumInfo(pType)->nNextValue;
		oExp.nLeftValue = 0;
	}
	return oExp;
}

CJcExpress BuildSizeExpress(CJcSymbolStack* pStack, CJcTypeInfo* pType, jc_bool bSizeOf)
{
	return BuildIntegerConstantExpressA(pStack, bSizeOf?GetTypeSize(pType):GetTypeAlign(pType), 0);
}

CJcExpress LoadExpress(CJcExpress* pExp)
{
	jc_ushort nOp;
	CJcExpress oNewExp;
	jc_uint nSize, nAlign, nCount, nTypeCode, nAlignCode;
	CJcSymbolStack* pStack;

	pStack = pExp->pStack;
	InitializeExpress(pStack, &oNewExp);
	oNewExp.pType = CloneType(pExp->pType);
	oNewExp.nBits = pExp->nBits;
	oNewExp.nStart = pExp->nStart;
	AllocTempVar(&oNewExp);

	nSize = GetTypeSize(pExp->pType);
	nTypeCode = GetTypeCode(GetOrigType(pExp->pType));
	if(nTypeCode == JC_STRUCT || nTypeCode == JC_UNION)
	{
		nAlign = GetTypeAlign(pExp->pType);
		nCount = nSize / nAlign;
		if(nCount > 1)
			Emit1(&pStack->oCodeSegment, jc_rep, JC_UN, nCount, JC_DEFAULT_INSADDR);
		nAlignCode = nAlign;
	}
	else nAlignCode = nSize;
	switch(nAlignCode)
	{
	case 1:
		nOp = jc_loadc;
		break;
	case 2:
		nOp = jc_loads;
		break;
	case 4:
		nOp = jc_loadi;
		break;
	case 8:
		nOp = jc_loadl;
		break;
	}
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)oNewExp.nOpt, oNewExp.nArg,
		(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
	return oNewExp;
}

jc_void SaveExpress(CJcExpress* pLeftExp, CJcExpress *pRightExp)
{
	jc_ushort nOp;
	jc_uint nSize, nAlign, nCount, nTypeCode, nAlignCode;
	CJcSymbolStack* pStack;

	pStack = pLeftExp->pStack;
	nSize = GetTypeSize(pLeftExp->pType);
	nTypeCode = GetTypeCode(GetOrigType(pLeftExp->pType));
	if(nTypeCode == JC_STRUCT || nTypeCode == JC_UNION)
	{
		nAlign = GetTypeAlign(pLeftExp->pType);
		nCount = nSize / nAlign;
		if(nCount > 1)
			Emit1(&pStack->oCodeSegment, jc_rep, JC_UN, nCount, JC_DEFAULT_INSADDR);
		nAlignCode = nAlign;
	}
	else nAlignCode = nSize;
	switch(nAlignCode)
	{
	case 1:
		nOp = (pLeftExp->nQuote?jc_savec:jc_movc);
		break;
	case 2:
		nOp = (pLeftExp->nQuote?jc_saves:jc_movs);
		break;
	case 4:
		nOp = (pLeftExp->nQuote?jc_savei:jc_movi);
		break;
	case 8:
		nOp = (pLeftExp->nQuote?jc_savel:jc_movl);
		break;
	}
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pLeftExp->nOpt, pLeftExp->nArg,
		(jc_uchar)pRightExp->nOpt, pRightExp->nArg, JC_DEFAULT_INSADDR);
}

CJcExpress CopyExpress(CJcExpress* pExp)
{
	jc_ushort nOp;
	CJcExpress oNewExp;
	jc_uint nSize, nAlign, nCount, nTypeCode, nAlignCode;
	CJcSymbolStack* pStack;

	pStack = pExp->pStack;
	InitializeExpress(pStack, &oNewExp);
	oNewExp.pType = CloneType(pExp->pType);
	oNewExp.nBits = pExp->nBits;
	oNewExp.nStart = pExp->nStart;
	AllocTempVar(&oNewExp);

	nSize = GetTypeSize(pExp->pType);
	nTypeCode = GetTypeCode(GetOrigType(pExp->pType));
	if(nTypeCode == JC_STRUCT || nTypeCode == JC_UNION)
	{
		nAlign = GetTypeAlign(pExp->pType);
		nCount = nSize / nAlign;
		if(nCount > 1)
			Emit1(&pStack->oCodeSegment, jc_rep, JC_UN, nCount, JC_DEFAULT_INSADDR);
		nAlignCode = nAlign;
	}
	else nAlignCode = nSize;
	switch(nAlignCode)
	{
	case 1:
		nOp = jc_movc;
		break;
	case 2:
		nOp = jc_movs;
		break;
	case 4:
		nOp = jc_movi;
		break;
	case 8:
		nOp = jc_movl;
		break;
	}
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)oNewExp.nOpt, oNewExp.nArg,
		(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
	return oNewExp;
}

jc_void ZeroBitExpress(CJcExpress *pExp)
{
	jc_ushort nOp;
	CJcVal oVal;
	CJcSymbolStack* pStack;
	jc_uint nAlign, nAlignCode, nArg, nType;

	nAlign = GetTypeAlign(pExp->pType);
	nAlignCode = GetTypeSize(pExp->pType);
	oVal = GetBitVal(nAlignCode, pExp->nBits);
	switch(nAlignCode)
	{
	case 1:
		nOp = jc_anduc;
		if(pExp->nStart>0)
			oVal.uc <<= pExp->nStart;
		oVal.uc = ~oVal.uc;
		nType = JC_UNSIGNED_CHAR;
		break;
	case 2:
		nOp = jc_andus;
		if(pExp->nStart>0)
			oVal.us <<= pExp->nStart;
		oVal.us = ~oVal.us;
		nType = JC_UNSIGNED_SHORT;
		break;
	case 4:
		nOp = jc_andui;
		if(pExp->nStart>0)
			oVal.ui <<= pExp->nStart;
		oVal.ui = ~oVal.ui;
		nType = JC_UNSIGNED_INT;
		break;
	case 8:
		nOp = jc_andul;
		if(pExp->nStart>0)
			oVal.ul <<= pExp->nStart;
		oVal.ul = ~oVal.ul;
		nType = JC_UNSIGNED_LONG;
		break;
	}

	pStack = pExp->pStack;
	nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, nAlignCode, nAlign);
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pExp->nOpt, pExp->nArg,
		(jc_uchar)JC_IS, nArg, JC_DEFAULT_INSADDR);
	pExp->nBits = 0;
	pExp->nStart = -1;
}

jc_void LoadBitExpress(CJcExpress *pExp)
{
	CJcSymbolStack* pStack;
	jc_ushort nLhOp, nRhOp;
	jc_uint nSize, nCode, nLCount, nRCount;
	CJcTypeInfo* pType;

	pType = GetOrigType(pExp->pType);
	nCode = GetTypeCode(pType);
	nSize = GetTypeSize(pType);

	switch(nSize)
	{
	case 1:
		nLhOp = jc_lshuc;
		nRhOp = ((nCode&JC_SIGNED)?jc_rshc:jc_rshuc);
		nRCount = 8 - pExp->nBits;
		break;
	case 2:
		nLhOp = jc_lshus;
		nRhOp = ((nCode&JC_SIGNED)?jc_rshs:jc_rshus);
		nRCount = 16 - pExp->nBits;
		break;
	case 4:
		nLhOp = jc_lshui;
		nRhOp = ((nCode&JC_SIGNED)?jc_rshi:jc_rshui);
		nRCount = 16 - pExp->nBits;
		break;
	case 8:
		nLhOp = jc_lshul;
		nRhOp = ((nCode&JC_SIGNED)?jc_rshl:jc_rshul);
		nRCount = 16 - pExp->nBits;
		break;
	}
	nLCount = nRCount - pExp->nStart;
	pStack = pExp->pStack;
	if(nLCount)
	{
		Emit2(&pStack->oCodeSegment, nLhOp, (jc_uchar)pExp->nOpt, pExp->nArg,
			JC_IS, PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nLCount, 4, 4),
			JC_DEFAULT_INSADDR);
	}
	if(nRCount)
	{
		Emit2(&pStack->oCodeSegment, nRhOp, (jc_uchar)pExp->nOpt, pExp->nArg,
			JC_IS, PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nRCount, 4, 4),
			JC_DEFAULT_INSADDR);
	}
	pExp->nBits = 0;
	pExp->nStart = -1;
}

jc_void BuildBitExpress(CJcExpress* pExp, jc_uint nBits, jc_int nStart)
{
	CJcVal oVal;
	jc_ushort nOp, nShiftOp;
	jc_uint nAlign, nAlignCode, nArg;
	CJcSymbolStack* pStack;

	pStack = pExp->pStack;
	nAlign = GetTypeAlign(pExp->pType);
	nAlignCode = GetTypeSize(pExp->pType);

	oVal = GetBitVal(nAlignCode, nBits);
	switch(nAlignCode)
	{
	case 1:
		nOp = jc_anduc;
		nShiftOp = jc_lshuc;
		break;
	case 2:
		nOp = jc_andus;
		nShiftOp = jc_lshus;
		break;
	case 4:
		nOp = jc_andui;
		nShiftOp = jc_lshui;
		break;
	case 8:
		nOp = jc_andul;
		nShiftOp = jc_lshul;
		break;
	}

	nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, nAlignCode, nAlign);
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pExp->nOpt, pExp->nArg,
		(jc_uchar)JC_IS, nArg, JC_DEFAULT_INSADDR);

	if(nStart > 0)
	{
		nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nStart, 4, 4);
		Emit2(&pStack->oCodeSegment, nShiftOp, (jc_uchar)pExp->nOpt, pExp->nArg,
			(jc_uchar)JC_IS, nArg, JC_DEFAULT_INSADDR);
	}

	pExp->nBits = nBits;
	pExp->nStart = nStart;
}

jc_void ReturnExpress(CJcExpress *pExp)
{
	CJcTypeInfo* pRetType;
	CJcVariableInfo* pPara;
	CJcExpress oRetExp;
	CJcString oStr1, oStr2;

	CJcSymbolStack* pStack = pExp->pStack;
	pRetType = GetFunctionType(pStack->pInFunction);
	oStr1 = GetTypeName(pExp->pType, False);
	oStr2 = GetTypeName(pRetType, False);

	if(StringCompare(oStr1.pStr, oStr2.pStr))
	{
		if(CheckAutoAssignCast(pStack, pRetType, pExp->pType, False))
		{
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
		BuildCastExpress(pExp, pRetType);
	}
	pPara = GetFunctionPara(pStack->pInFunction, 0);
	InitializeExpress(pStack, &oRetExp);
	oRetExp.pType = CloneType(pPara->pType);
	oRetExp.nOpt = pPara->nOpt;
	oRetExp.nArg = pPara->nArg;
	BuildUnaryExpress(&oRetExp, JC_UNARY_GETVALUE);
	oRetExp.nLeftValue = 1;
	BuildAssignmentExpress(&oRetExp, pExp, JC_ASSIGN_EQUAL, 1);
	DestroyExpress(&oRetExp);
	ClearString(&oStr1);
	ClearString(&oStr2);
}

static jc_void LoadIntegerExpress(CJcExpress* pExp)
{
	CJcExpress oTmpExp;
	jc_bool bNew = False;
	if(pExp->nQuote)
	{
		oTmpExp = LoadExpress(pExp);
		DestroyExpress(pExp);
		*pExp = oTmpExp;
		bNew = True;
	}
	if(pExp->nBits)
	{
		if(bNew)
			LoadBitExpress(pExp);
		else
		{
			oTmpExp = CopyExpress(pExp);
			LoadBitExpress(&oTmpExp);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
	}
}

jc_void EmitCondJmp(CJcExpress *pExp, jc_bool bTrue, jc_uint nJmpAddr, jc_uint nInsAddr)
{
	jc_ushort nOp;
	jc_uint nType;
	CJcTypeInfo* pType;
	CJcSymbolStack* pStack;

	LoadIntegerExpress(pExp);
	pType = GetOrigType(pExp->pType);
	nType = GetTypeCode(pType);
	switch(nType)
	{
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		nOp = (bTrue)?jc_jmptc:jc_jmpfc;
		break;
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
		nOp = (bTrue)?jc_jmpts:jc_jmpfs;
		break;
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
		nOp = (bTrue)?jc_jmpti:jc_jmpfi;
		break;
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		nOp = (bTrue)?jc_jmptl:jc_jmpfl;
		break;
	case JC_FLOAT:
		nOp = (bTrue)?jc_jmptf:jc_jmpff;
		break;
	case JC_DOUBLE:
		nOp = (bTrue)?jc_jmptd:jc_jmpfd;
		break;
	case JC_POINTER:
		if(sizeof(void*) == 8)
			nOp = (bTrue)?jc_jmptl:jc_jmpfl;
		else
			nOp = (bTrue)?jc_jmpti:jc_jmpfi;
		break;
	}
	pStack = pExp->pStack;
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pExp->nOpt, pExp->nArg, JC_UN, nJmpAddr, nInsAddr);
}

jc_void EmitSwitchJmp(CJcExpress *pExp, jc_uint nJmpAddr, jc_uint nInsAddr)
{
	jc_ushort nOp;
	jc_uint nType;
	CJcTypeInfo* pType;
	CJcSymbolStack* pStack;

	LoadIntegerExpress(pExp);
	pType = GetOrigType(pExp->pType);
	nType = GetTypeCode(pType);
	switch(nType)
	{
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		nOp = jc_jtabc;
		break;
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
		nOp = jc_jtabs;
		break;
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
		nOp = jc_jtabi;
		break;
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		nOp = jc_jtabl;
		break;
	}
	pStack = pExp->pStack;
	Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pExp->nOpt, pExp->nArg, JC_IS, nJmpAddr, nInsAddr);
}

jc_void BuildFieldExpress(CJcExpress* pStruct, jc_char* sField, jc_bool bIsPointer)
{
	CJcExpress oField;
	CJcStructInfo* pStructInfo;
	CJcStructFieldInfo* pFieldInfo;
	CJcSymbolStack* pStack;
	CJcTypeInfo* pType;
	jc_uint nType;

	pStack = pStruct->pStack;
	InitializeExpress(pStack, &oField);
	pType = GetOrigType(pStruct->pType);
	nType = GetTypeCode(pType);

	if(bIsPointer)
	{
		CJcExpress oTmpExp;
		if(nType != JC_POINTER)
		{
			SetError(pStack->pParser, 0, "left of '->%s' must be a pointer of struct or union", sField);
			return;
		}
		if(pStruct->nQuote)
		{
			oTmpExp = LoadExpress(pStruct);
			DestroyExpress(pStruct);
			*pStruct = oTmpExp;
		}
		BuildUnaryExpress(pStruct, JC_UNARY_GETVALUE);
		pType = GetOrigType(pStruct->pType);
		nType = GetTypeCode(pType);
	}
	if(nType != JC_STRUCT && nType != JC_UNION)
	{
		if(bIsPointer)
			SetError(pStack->pParser, 0, "left of '->%s' must be a pointer of struct or union", sField);
		else
			SetError(pStack->pParser, 0, "left of '->%s' must be a struct or union", sField);
		return;
	}
	pStructInfo = GetStructInfo(pType);
	pFieldInfo = GetStructField(pStructInfo, sField);
	if(!pFieldInfo)
	{
		SetError(pStack->pParser, 0, "'%s' is not member of '%s'", sField, pType->oName.pStr);
		return;
	}
	oField.pType = CloneType(pFieldInfo->pType);
	oField.nStart = pFieldInfo->nBitOffset;
	oField.nBits = pFieldInfo->nBitCount;
	oField.nLeftValue = pStruct->nLeftValue;
	oField.nQuote = pStruct->nQuote;
	if(pStruct->nQuote)
	{
		CJcExpress oAddrExp, oOffsetExp;
		oOffsetExp = BuildIntegerConstantExpressA(pStack, pFieldInfo->nOffset, 0);
		InitializeExpress(pStack, &oAddrExp);
		oAddrExp.pType = CreateType(JC_NONE, JC_POINTER, NULL,
			CreateType(JC_NONE, JC_SIGNED_CHAR, NULL, NULL, NULL), NULL);
		oAddrExp.nLeftValue = 1;
		oAddrExp.nOpt = pStruct->nOpt;
		oAddrExp.nArg = pStruct->nArg;
		BuildAssignmentExpress(&oAddrExp, &oOffsetExp, JC_ASSIGN_ADD_EQ, 1);
		oField.nOpt = oAddrExp.nOpt;
		oField.nArg = oAddrExp.nArg;
		oField.nVarId = oAddrExp.nVarId;
		DestroyExpress(&oOffsetExp);
		DestroyType(oAddrExp.pType);
	}
	else
	{
		oField.nOpt = pStruct->nOpt;
		oField.nArg = pStruct->nArg + pFieldInfo->nOffset;
	}
	if(IsConstType(pType))
		SetQualCode(oField.pType, JC_CONST);
	FreeTempVar(pStruct);
	*pStruct = oField;
}

jc_void BuildArrayExpress(CJcExpress* pArray, CJcExpress* pDim)
{
	CJcExpress oTmpExp;
	jc_uint nDimType, nArrayType;
	CJcTypeInfo* pDimType, *pArrayType;
	CJcSymbolStack* pStack = pDim->pStack;

	LoadIntegerExpress(pDim);
	pDimType = GetOrigType(pDim->pType);
	nDimType = GetTypeCode(pDimType);
	switch(nDimType)
	{
	default:
		SetError(pStack->pParser, 0, "subscript is not of integral type");
		return;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		break;
	}
	UpdateExpressType(pDim, pDimType);
	if(pArray->nQuote)
	{
		oTmpExp = LoadExpress(pArray);
		DestroyExpress(pArray);
		*pArray = oTmpExp;
	}
	pArrayType = GetOrigType(pArray->pType);
	nArrayType = GetTypeCode(pArrayType);
	UpdateExpressType(pArray, pArrayType);
	if(nArrayType == JC_POINTER)
	{
		jc_uint nNewType;
		CJcTypeInfo * pNewType = pArrayType->pNext;
		pNewType = GetOrigType(pNewType);
		nNewType = GetTypeCode(pNewType);
		if(nNewType == JC_FUNCTION || nNewType == JC_VOID)
		{
			SetError(pStack->pParser, 0, "the array's destination type cann't be function or void");
			return;
		}
		if(pArray->nVarId < 0)
		{
			oTmpExp = CopyExpress(pArray);
			FreeTempVar(pArray);
			*pArray = oTmpExp;
		}
	}
	else if(nArrayType == JC_ARRAY)
	{
		CJcTypeInfo* pTmpType;
		if(pDim->nOpt == JC_IS)
		{
			CJcVal oConstVal;
			CJcArrayInfo* pArrayInfo = GetArrayInfo(pArrayType);
			GetConstValue(pDim, &oConstVal, JC_UNSIGNED_LONG, 0);
			if(pArrayInfo->nDim && oConstVal.ul >= (jc_ulong)pArrayInfo->nDim)
			{
				jc_char pBuf[100];
				PrintfVal(pBuf, &oConstVal, JC_UNSIGNED_LONG);
				SetError(pStack->pParser, 0, "subscript(%s) is beyond the array's boundary(%u)", pBuf, pArrayInfo->nDim);
				return;
			}
		}
		pTmpType = CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pArrayType->pNext), NULL);
		if(pArray->nOpt != JC_AS)
			BuildUnaryExpress(pArray, JC_UNARY_GETADDR);
		DestroyType(pArray->pType);
		pArray->pType = pTmpType;
	}
	else
	{
		SetError(pStack->pParser, 0, "subscript requires array or pointer type");
		return;
	}

	pArray->nLeftValue = 1;
	BuildAssignmentExpress(pArray, pDim, JC_ASSIGN_ADD_EQ, 1);
	pArray->nQuote = 1;
	UpdateExpressType(pArray, pArray->pType->pNext);
	if(IsConstType(pArray->pType))
		pArray->nLeftValue = 0;
}

jc_void BuildArgumentExpress(CJcExpress* pFunction, CJcExpress* pArg, jc_uint nArgIdx, jc_uint* pArgSize)
{
	jc_uint nArgPos;
	jc_uint nFunctionType;
	CJcTypeInfo* pFunctionType;
	CJcFunctionInfo* pFunctionInfo;
	CJcVariableInfo* pPara;
	jc_uint nArgType;
	jc_uint nArgNo;
	CJcTypeInfo* pArgType;
	jc_uint nParaType;
	CJcTypeInfo* pParaType;
	jc_uint nAlign, nMod;
	CJcExpress oArg;
	CJcSymbolStack* pStack;

	pStack = pFunction->pStack;
	if(pFunction->nQuote)
	{
		oArg = LoadExpress(pFunction);
		DestroyExpress(pFunction);
		*pFunction = oArg;
	}
	pFunctionType = GetOrigType(pFunction->pType);
	nFunctionType = GetTypeCode(pFunctionType);
	if(nFunctionType == JC_POINTER)
	{
		pFunctionType = GetOrigType(pFunctionType->pNext);
		nFunctionType = GetTypeCode(pFunctionType);
	}
	if(nFunctionType != JC_FUNCTION)
	{
		SetError(pStack->pParser, 0, "invalid function express");
		return;
	}
	pFunctionInfo = GetFunctionInfo(pFunctionType);
	pParaType = GetOrigType(GetFunctionType(pFunctionInfo));
	nParaType = GetTypeCode(pParaType);
	nArgNo = nArgIdx;
	if(nParaType == JC_VOID)
		--nArgIdx;
	else if(nArgIdx == 1)
		*pArgSize = sizeof(jc_void*);
	pPara = GetFunctionPara(pFunctionInfo, nArgIdx);
	if(!pPara)
	{
		SetError(pStack->pParser, 0, "too many argument(%u)", GetFunctionParaCount(pFunctionInfo));
		return;
	}
	pParaType = GetOrigType(pPara->pType);
	nParaType = GetTypeCode(pParaType);
	LoadIntegerExpress(pArg);
	pArgType = GetOrigType(pArg->pType);
	nArgType = GetTypeCode(pArgType);
	if(StringCompare(pPara->oName.pStr, "..."))
	{
		CJcString oStr1, oStr2;
		if(nParaType == JC_POINTER || nParaType == JC_ARRAY)
		{
			if(nArgType == JC_POINTER || nArgType == JC_ARRAY)
			{
				if(nParaType == JC_ARRAY && nArgType == JC_ARRAY)
				{
					CJcArrayInfo* pParaArray = GetArrayInfo(pParaType);
					CJcArrayInfo* pArgArray = GetArrayInfo(pArgType);
					if(pParaArray->nDim && pArgArray->nDim && pParaArray->nDim > pArgArray->nDim)
					{
						oStr1 = GetTypeName(pArgType, True);
						oStr2 = GetTypeName(pParaType, True);
						SetError(pStack->pParser, 0, "parameter %u is '%s', but the argument passed is '%s'",
							nArgNo, oStr2.pStr, oStr1.pStr);
						ClearString(&oStr1);
						ClearString(&oStr2);
						return;
					}
				}
/*
				oStr1 = GetTypeName(pArgType->pNext, True);
				oStr2 = GetTypeName(pParaType->pNext, True);
				if(StringCompare(oStr2.pStr, "void") && StringCompare(oStr1.pStr, oStr2.pStr))
				{
					ClearString(&oStr1);
					ClearString(&oStr2);
					oStr1 = GetTypeName(pArgType, True);
					oStr2 = GetTypeName(pParaType, True);
					SetError(pStack->pParser, 0, "parameter %u is '%s', but the argument passed is '%s'",
						nArgNo, oStr2.pStr, oStr1.pStr);
					ClearString(&oStr1);
					ClearString(&oStr2);
					return;
				}
				ClearString(&oStr1);
				ClearString(&oStr2);
*/
			}
			else
			{
				if(nParaType == JC_POINTER && JC_FUNCTION == GetTypeCode(GetOrigType(pParaType->pNext)) && 
					nArgType == JC_FUNCTION)
				{
					oStr1 = GetTypeName(pArgType, True);
					oStr2 = GetTypeName(pParaType->pNext, True);
					if(StringCompare(oStr1.pStr, oStr2.pStr))
					{
						SetError(pStack->pParser, 0, "parameter %u is '%s', but the argument passed is '%s'",
							nArgNo, oStr2.pStr, oStr1.pStr);
						ClearString(&oStr1);
						ClearString(&oStr2);
						return;
					}
				}
				else
				{
					oStr1 = GetTypeName(pArgType, True);
					oStr2 = GetTypeName(pParaType, True);
					SetError(pStack->pParser, 0, "parameter %u is '%s', but the argument passed is '%s'",
						nArgNo, oStr2.pStr, oStr1.pStr);
					ClearString(&oStr1);
					ClearString(&oStr2);
					return;
				}
			}
		}
		else
		{
			jc_bool bConstNull;
			if(pArg->nOpt == JC_IS && GetTypeCode(pArg->pType)==JC_POINTER)
			{
				CJcVal oVal;
				if(GetConstValue(pArg, &oVal, JC_POINTER, False))
					return;
				if(sizeof(jc_void*) == sizeof(jc_uint))
				{
					if(oVal.ui == 0)
						bConstNull = True;
				}
				else
				{
					if(oVal.ul == (jc_ulong)0)
						bConstNull = True;
				}
			}
			if(CheckAutoArgumentCast(pStack, pParaType, pArgType, nArgIdx, bConstNull))
				return;
		}
	}
	if(nArgType == JC_ARRAY)
	{
		pArgType = CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pArgType->pNext), NULL);
		if(pArg->nOpt != JC_AS)
			BuildUnaryExpress(pArg, JC_UNARY_GETADDR);
		DestroyType(pArg->pType);
		pArg->pType = pArgType;
	}

	InitializeExpress(pStack, &oArg);
	if(StringCompare(pPara->oName.pStr, "..."))
	{
		nArgPos = pPara->nArg;
		if(nParaType == JC_ARRAY)
		{
			oArg.pType = CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pArgType->pNext), NULL);
			(*pArgSize) += GetTypeSize(oArg.pType);
		}
		else
		{
			oArg.pType = CloneType(pPara->pType);
			(*pArgSize) += GetTypeSize(pPara->pType);
		}
	}
	else
	{
		nAlign = GetTypeAlign(pArgType);
		nMod = (*pArgSize) % nAlign;
		if(nMod)
			(*pArgSize) += nAlign - nMod;		
		nArgPos = (*pArgSize);
		(*pArgSize) += GetTypeSize(pArgType);
		oArg.pType = CloneType(pArg->pType);
	}
	oArg.nOpt = JC_TS;
	oArg.nArg = nArgPos;
	oArg.nLeftValue = 1;
	BuildAssignmentExpress(&oArg, pArg, JC_ASSIGN_EQUAL, 1);
	DestroyExpress(&oArg);
}

jc_void BuildCallExpress(CJcExpress* pFunction, jc_uint nArgSize, jc_uint nArgInsAddr)
{
	jc_uint nFunctionType;
	CJcTypeInfo* pFunctionType;
	CJcFunctionInfo* pFunctionInfo;
	CJcExpress oRetExp, oTmpExp, oArgExp;
	CJcSymbolStack* pStack;

	pStack = pFunction->pStack;
	if(pFunction->nQuote)
	{
		oTmpExp = LoadExpress(pFunction);
		DestroyExpress(pFunction);
		*pFunction = oTmpExp;
	}
	pFunctionType = GetOrigType(pFunction->pType);
	nFunctionType = GetTypeCode(pFunctionType);
	if(nFunctionType == JC_POINTER)
	{
		pFunctionType = GetOrigType(pFunctionType->pNext);
		nFunctionType = GetTypeCode(pFunctionType);
	}
	if(nFunctionType != JC_FUNCTION)
	{
		SetError(pStack->pParser, 0, "invalid function express");
		return;
	}
	pFunctionInfo = GetFunctionInfo(pFunctionType);
	pFunctionType = GetOrigType(GetFunctionType(pFunctionInfo));
	nFunctionType = GetTypeCode(pFunctionType);
	InitializeExpress(pStack, &oRetExp);
	oRetExp.pType = CloneType(pFunctionType);
	if(nFunctionType != JC_VOID)
	{
		if(!nArgSize)
			nArgSize = sizeof(jc_void*);
		AllocTempVar(&oRetExp);
		oTmpExp = oRetExp;
		oTmpExp.pType = CloneType(oTmpExp.pType);
		oTmpExp.nVarId = -1;
		BuildUnaryExpress(&oTmpExp, JC_UNARY_GETADDR);
		InitializeExpress(pStack, &oArgExp);
		oArgExp.pType = CloneType(oTmpExp.pType);
		oArgExp.nOpt = JC_TS;
		oArgExp.nArg = 0;
		oArgExp.nLeftValue = 1;
		BuildAssignmentExpress(&oArgExp, &oTmpExp, JC_ASSIGN_EQUAL, 1);
		DestroyExpress(&oTmpExp);
		DestroyExpress(&oArgExp);
	}
	Emit1(&pStack->oCodeSegment, jc_newarg, JC_UN, nArgSize, nArgInsAddr);
	Emit1(&pStack->oCodeSegment, jc_call, (jc_uchar)pFunction->nOpt, pFunction->nArg, JC_DEFAULT_INSADDR);
	Emit0(&pStack->oCodeSegment, jc_delarg, JC_DEFAULT_INSADDR);

	DestroyExpress(pFunction);
	*pFunction = oRetExp;
}

jc_void BuildIncreaseExpress(CJcExpress *pExp, jc_bool bReturnNewVal, jc_bool bIncrearse)
{
	CJcTypeInfo* pType;
	jc_uint nSubCode, nTypeCode;
	jc_ushort nOp;
	CJcSymbolStack* pStack;
	CJcExpress oRetExp, oTmpExp;
	CJcString oStr;
	const jc_char* sOperator = bIncrearse?"++":"--";

	pStack = pExp->pStack;
	if(!pExp->nLeftValue)
	{
		SetError(pStack->pParser, 0, "%s operator need the left value", sOperator);
		return;
	}

	if(pExp->nQuote)
	{
		oTmpExp = LoadExpress(pExp);
		if(!bReturnNewVal)
		{
			oRetExp = CopyExpress(&oTmpExp);
			if(oRetExp.nBits)
				LoadBitExpress(&oRetExp);
			oRetExp.nLeftValue = 0;
		}
		oTmpExp.nLeftValue = 1;
		BuildIncreaseExpress(&oTmpExp, False, bIncrearse);
		SaveExpress(pExp, &oTmpExp);
	}
	else if(pExp->nBits)
	{
		oTmpExp = CopyExpress(pExp);
		LoadBitExpress(&oTmpExp);
		if(!bReturnNewVal)
		{
			oRetExp = CopyExpress(&oTmpExp);
			oRetExp.nLeftValue = 0;
		}
		oTmpExp.nLeftValue = 1;
		BuildIncreaseExpress(&oTmpExp, False, bIncrearse);
		BuildBitExpress(&oTmpExp, pExp->nBits, pExp->nStart);
		ZeroBitExpress(pExp);
		BuildAssignmentExpress(pExp, &oTmpExp, JC_ASSIGN_OR_EQ, 1);
	}
	else
	{
		pType = GetOrigType(pExp->pType);
		nTypeCode = GetTypeCode(pType);
		switch(nTypeCode)
		{
		default:
			oStr = GetTypeName(pExp->pType, True);
			SetError(pStack->pParser, 0, "%s don't support %s operator", oStr.pStr, sOperator);
			ClearString(&oStr);
			return;
		case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
			nOp = (bIncrearse?jc_incc:jc_decc);
			break;
		case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
			nOp = (bIncrearse?jc_incs:jc_decs);
			break;
		case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
			nOp = (bIncrearse?jc_inci:jc_deci);
			break;
		case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
			nOp = (bIncrearse?jc_incl:jc_decl);
			break;
		case JC_FLOAT:
			nOp = (bIncrearse?jc_incf:jc_decf);
			break;
		case JC_DOUBLE:
			nOp = (bIncrearse?jc_incd:jc_decd);
			break;
		case JC_POINTER:
			nSubCode = GetTypeCode(GetOrigType(pType->pNext));
			if(nSubCode == JC_FUNCTION || nSubCode == JC_VOID)
			{
				oStr = GetTypeName(pExp->pType, True);
				SetError(pStack->pParser, 0, "%s don't support %s operator", oStr.pStr, sOperator);
				ClearString(&oStr);
				return;
			}
			if(sizeof(void*) == sizeof(jc_int))
				nOp = bIncrearse?jc_addi:jc_subi;
			else
				nOp = bIncrearse?jc_addl:jc_subl;
			break;
		}
		if(!bReturnNewVal)
		{
			oRetExp = CopyExpress(pExp);
			oRetExp.nLeftValue = 0;
		}
		if(nTypeCode != JC_POINTER)
			Emit1(&pStack->oCodeSegment, nOp, (jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
		else
		{
			CJcExpress oRightExp = BuildSizeExpress(pStack, pType->pNext, 1);
			BuildAssignmentExpress(pExp, &oRightExp, JC_ASSIGN_ADD_EQ, 1);
			DestroyExpress(&oRightExp);
		}
	}
	if(!bReturnNewVal)
	{
		DestroyExpress(pExp);
		*pExp = oRetExp;
	}
}

jc_void BuildConditionalExpress(CJcExpress *pCondExp, CJcExpress *pIfExp, CJcExpress *pElseExp)
{
	CJcString oStr1, oStr2;
	CJcTypeInfo* pCondType, *pRetType, *pLeftType, *pRightType;
	jc_uint nCondTypeCode, nNoCond=0, nUseIf;
	jc_ushort nJmpOp;
	CJcExpress oRetExp;
	CJcSymbolStack* pStack;

	pStack = pCondExp->pStack;
	pCondType = GetOrigType(pCondExp->pType);
	nCondTypeCode = GetTypeCode(pCondType);

	if(pCondExp->nOpt == JC_IS)
	{
		CJcVal oVal;
		nNoCond = 1;
		if(GetConstValue(pCondExp, &oVal, nCondTypeCode, 1))
		{
			oStr1 = GetTypeName(pCondExp->pType, True);
			SetError(pStack->pParser, 0, "'%s' express is invalid condition express", oStr1.pStr);
			ClearString(&oStr1);
			return;
		}
		switch(nCondTypeCode)
		{
		case JC_FLOAT:
			nUseIf = (oVal.f?1:0);
			break;
		case JC_DOUBLE:
			nUseIf = (oVal.d?1:0);
			break;
		default:
			nUseIf = (oVal.ul?1:0);
			break;
		}
	}
	else switch(nCondTypeCode)
	{
	default:
		oStr1 = GetTypeName(pCondExp->pType, True);
		SetError(pStack->pParser, 0, "'%s' express is invalid condition express", oStr1.pStr);
		ClearString(&oStr1);
		return;
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		nJmpOp = jc_jmpfc;
		break;
	case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
		nJmpOp = jc_jmpfs;
		break;
	case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
		nJmpOp = jc_jmpfi;
		break;
	case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
		nJmpOp = jc_jmpfl;
		break;
	case JC_FLOAT:
		nJmpOp = jc_jmpff;
		break;
	case JC_DOUBLE:
		nJmpOp = jc_jmpfd;
		break;
	case JC_POINTER:
		if(sizeof(void*) == sizeof(jc_int))
			nJmpOp = jc_jmpfi;
		else
			nJmpOp = jc_jmpfl;
		break;
	case JC_ARRAY: case JC_FUNCTION:
		nNoCond = 1;
		nUseIf = 1;
		break;
	}

	pLeftType = GetOrigType(pIfExp->pType);
	pRightType = GetOrigType(pElseExp->pType);
	
	if((GetTypeCode(pLeftType) == JC_POINTER || GetTypeCode(pLeftType) == JC_ARRAY) &&
		(GetTypeCode(pRightType) == JC_POINTER || GetTypeCode(pRightType) == JC_ARRAY))
	{
		oStr1 = GetTypeName(pLeftType->pNext, False);
		oStr2 = GetTypeName(pRightType->pNext, False);
		if(StringCompare(oStr1.pStr, oStr2.pStr))
		{
			SetError(pStack->pParser, 0, "the if-express's type('%s') is not same as the else-express's type('%s')", oStr1.pStr, oStr2.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
	}
	else
	{
		oStr1 = GetTypeName(pLeftType, False);
		oStr2 = GetTypeName(pRightType, False);
		if(StringCompare(oStr1.pStr, oStr2.pStr))
		{
			SetError(pStack->pParser, 0, "the if-express's type('%s') is not same as the else-express's type('%s')", oStr1.pStr, oStr2.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
	}
	ClearString(&oStr1);
	ClearString(&oStr2);

	pRetType = pLeftType;
	if(pRightType->nQual & JC_CONST)
		pRetType = pRightType;
	InitializeExpress(pStack, &oRetExp);
	if(pRetType->nType == JC_ARRAY)
		oRetExp.pType = CreateType(JC_NONE, JC_POINTER, NULL, CloneType(GetNextType(pRetType)), NULL);
	else
		oRetExp.pType = CloneType(pRetType);
	AllocTempVar(&oRetExp);
	oRetExp.nLeftValue = 1;
	if(nNoCond)
		BuildAssignmentExpress(&oRetExp, nUseIf?pIfExp:pElseExp, JC_ASSIGN_EQUAL, 1);
	else
	{
		jc_uint nJmpAddr, nInsPos = GetPosOfSegment(&pStack->oCodeSegment);
		Emit2(&pStack->oCodeSegment, nJmpOp, (jc_uchar)pCondExp->nOpt, pCondExp->nArg, JC_UN, 0, JC_DEFAULT_INSADDR);
		BuildAssignmentExpress(&oRetExp, pIfExp, JC_ASSIGN_EQUAL, 1);
		nJmpAddr = GetPosOfSegment(&pStack->oCodeSegment);
		Emit2(&pStack->oCodeSegment, nJmpOp, (jc_uchar)pCondExp->nOpt, pCondExp->nArg, JC_UN, nJmpAddr, nInsPos);
		BuildAssignmentExpress(&oRetExp, pElseExp, JC_ASSIGN_EQUAL, 1);
	}
	oRetExp.nLeftValue = 0;
	DestroyExpress(pCondExp);
	*pCondExp = oRetExp;
}

jc_void BuildUnaryExpress(CJcExpress *pExp, jc_uint nUnaryOperator)
{
	jc_uint nCode;
	CJcTypeInfo * pType;
	jc_ushort nOp;
	CJcExpress oTmpExp;
	CJcSymbolStack* pStack;

	pStack = pExp->pStack;
	if(nUnaryOperator == JC_UNARY_GETADDR)
	{
		if(pExp->nBits)
		{
			SetError(pStack->pParser, 0, "the bit field doesn't suport '&' operator");
			return;
		}
		pType = CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pExp->pType), NULL);
		if(pExp->nQuote || GetTypeCode(GetOrigType(pExp->pType)) == JC_ARRAY && pExp->nOpt == JC_AS)
		{
			DestroyType(pExp->pType);
			pExp->pType = pType;
			pExp->nQuote = 0;
		}
		else
		{
			InitializeExpress(pStack, &oTmpExp);
			oTmpExp.pType = pType;
			AllocTempVar(&oTmpExp);
			Emit2(&pStack->oCodeSegment, jc_lea, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg, (jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
		pExp->nLeftValue = 0;
	}
	else if(nUnaryOperator == JC_UNARY_GETVALUE)
	{
		if(pExp->nQuote)
		{
			oTmpExp = LoadExpress(pExp);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
		pType = GetOrigType(pExp->pType);
		nCode = GetTypeCode(pType);
		if(nCode == JC_ARRAY)
		{
			pType = CloneType(pType->pNext);
			BuildUnaryExpress(pExp, JC_UNARY_GETADDR);
			DestroyType(pExp->pType);
			pExp->pType = pType;
			pExp->nQuote = 1;
			pExp->nLeftValue = 1;
			if(IsConstType(pType))
				pExp->nLeftValue = 0;
		}
		else if(nCode == JC_POINTER)
		{
			nCode = GetTypeCode(GetOrigType(pType->pNext));
			if(nCode == JC_FUNCTION)
				return;
			if(nCode == JC_VOID)
			{
				SetError(pStack->pParser, 0, "the void* data doesn't support '*' operator");
				return;
			}
			pType = CloneType(pType->pNext);
			DestroyType(pExp->pType);
			pExp->pType = pType;
			pExp->nQuote = 1;
			if(IsConstType(pType))
				pExp->nLeftValue = 0;
			else
				pExp->nLeftValue = 1;
		}
		else
		{
			SetError(pStack->pParser, 0, "the non-pointer data doesn't support '*' operator");
			return;
		}
	}
	else if(nUnaryOperator == JC_UNARY_POSITIVE || nUnaryOperator == JC_UNARY_NEGATIVE)
	{
		pType = GetOrigType(pExp->pType);
		nCode = GetTypeCode(pType);
		switch(nCode)
		{
		case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
			nOp = jc_negc;
			break;
		case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
			nOp = jc_negs;
			break;
		case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
			nOp = jc_negi;
			break;
		case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
			nOp = jc_negl;
			break;
		case JC_FLOAT:
			nOp = jc_negf;
			break;
		case JC_DOUBLE:
			nOp = jc_negd;
			break;
		default:
			SetError(pStack->pParser, 0, "the non-number data doesn't support unary '+' or '-' operator");
			return;
		}
		LoadIntegerExpress(pExp);
		if(nUnaryOperator == JC_UNARY_NEGATIVE)
		{
			if(pExp->nOpt == JC_IS)
			{
				CJcVal oVal;
				GetConstValue(pExp, &oVal, nCode, 1);
				switch(nOp)
				{
				case jc_negc:
					oVal.c = -oVal.c;
					break;
				case jc_negs:
					oVal.s = -oVal.s;
					break;
				case jc_negi:
					oVal.i = -oVal.i;
					break;
				case jc_negl:
					oVal.l = -oVal.l;
					break;
				case jc_negf:
					oVal.f = -oVal.f;
					break;
				case jc_negd:
					oVal.d = -oVal.d;
					break;
				}
				pExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, GetTypeSize(pType), GetTypeAlign(pType));
			}
			else
			{
				oTmpExp = CopyExpress(pExp);
				Emit1(&pStack->oCodeSegment, nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg, JC_DEFAULT_INSADDR);
				DestroyExpress(pExp);
				*pExp = oTmpExp;
			}
		}
		pExp->nLeftValue = 0;
	}
	else if(nUnaryOperator == JC_UNARY_BIT_NOT)
	{
		pType = GetOrigType(pExp->pType);
		nCode = GetTypeCode(pType);
		switch(nCode)
		{
		case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
			nOp = jc_notuc;
			break;
		case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
			nOp = jc_notus;
			break;
		case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
			nOp = jc_notui;
			break;
		case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
			nOp = jc_notul;
			break;
		default:
			SetError(pStack->pParser, 0, "the non-integral data doesn't support bit '~' operator");
			return;
		}
		LoadIntegerExpress(pExp);
		if(pExp->nOpt == JC_IS)
		{
			CJcVal oVal;
			GetConstValue(pExp, &oVal, nCode, 1);
			switch(nOp)
			{
			case jc_notuc:
				oVal.uc = ~oVal.uc;
				break;
			case jc_notus:
				oVal.us = ~oVal.us;
				break;
			case jc_notui:
				oVal.ui = ~oVal.ui;
				break;
			case jc_notul:
				oVal.ul = ~oVal.ul;
				break;
			}
			pExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, GetTypeSize(pType), GetTypeAlign(pType));
		}
		else
		{
			oTmpExp = CopyExpress(pExp);
			Emit1(&pStack->oCodeSegment, nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg, JC_DEFAULT_INSADDR);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
		pExp->nLeftValue = 0;
	}
	else if(nUnaryOperator == JC_UNARY_LOGIC_NOT)
	{
		jc_uint nSize;
		jc_uint nAlign;
		pType = GetOrigType(pExp->pType);
		nCode = GetTypeCode(pType);
		nSize = GetTypeSize(pType);
		nAlign = GetTypeAlign(pType);
		switch(nCode)
		{
		case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
			nOp = jc_eqc;
			break;
		case JC_SIGNED_SHORT: case JC_UNSIGNED_SHORT:
			nOp = jc_eqs;
			break;
		case JC_SIGNED_INT: case JC_UNSIGNED_INT: case JC_ENUM:
			nOp = jc_eqi;
			break;
		case JC_SIGNED_LONG: case JC_UNSIGNED_LONG:
			nOp = jc_eql;
			break;
		case JC_FLOAT:
			nOp = jc_eqf;
			break;
		case JC_DOUBLE:
			nOp = jc_eqd;
			break;
		case JC_POINTER:
			if(sizeof(jc_void*)==sizeof(jc_int))
				nOp = jc_eqi;
			else
				nOp = jc_eql;
			break;
		default:
			SetError(pStack->pParser, 0, "the non-number data doesn't support unary '!' operator");
			return;
		}
		LoadIntegerExpress(pExp);
		if(pExp->nOpt == JC_IS)
		{
			CJcVal oVal;
			CJcExpress oTmpExp;
			GetConstValue(pExp, &oVal, nCode, 1);
			switch(nOp)
			{
			case jc_eqc:
				oVal.ui = oVal.uc?0:1;
				break;
			case jc_eqs:
				oVal.ui = oVal.us?0:1;
				break;
			case jc_eqi:
				oVal.ui = oVal.ui?0:1;
				break;
			case jc_eql:
				oVal.ui = oVal.ul?0:1;
				break;
			case jc_eqf:
				oVal.ui = oVal.f?0:1;
				break;
			case jc_eqd:
				oVal.ui = oVal.d?0:1;
				break;
			}
			oTmpExp = BuildIntegerConstantExpressA(pStack, oVal.ui, False);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
		else
		{
			InitializeExpress(pStack, &oTmpExp);
			oTmpExp.pType = CreateType(JC_CONST, JC_UNSIGNED_INT, NULL, NULL, NULL);
			AllocTempVar(&oTmpExp);
			Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg,
				(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
			DestroyExpress(pExp);
			*pExp = oTmpExp;
		}
		pExp->nLeftValue = 0;
	}
}

static CJcCastNode g_pCastTable0[] =
{
	{2, JC_FLOAT, jc_non, jc_f2d, jc_non, jc_f2i},
	{3, JC_DOUBLE, jc_d2f, jc_non, jc_non, jc_d2l}
};

static CJcCastNode g_pCastTable1[] =
{
	{0, JC_SIGNED_CHAR, jc_non, jc_c2s, jc_non, jc_non},
	{1, JC_SIGNED_SHORT, jc_s2c, jc_s2i, jc_non, jc_non},
	{2, JC_SIGNED_INT, jc_i2s, jc_i2l, jc_i2f, jc_non},
	{3, JC_SIGNED_LONG, jc_l2i, jc_non, jc_l2d, jc_non}
};

static CJcCastNode g_pCastTable2[] =
{
	{0, JC_UNSIGNED_CHAR, jc_non, jc_uc2us, jc_non, jc_non},
	{1, JC_UNSIGNED_SHORT, jc_us2uc, jc_us2ui, jc_non, jc_non},
	{2, JC_UNSIGNED_INT, jc_ui2us, jc_ui2ul, jc_non, jc_non},
	{3, JC_UNSIGNED_LONG, jc_ul2ui, jc_non, jc_non, jc_non}
};

static CJcCastTable g_pCastTable[] =
{
	{2, g_pCastTable0}, {4, g_pCastTable1}, {4, g_pCastTable2}
};

static jc_uint GetCastClass(jc_uint nTypeCode, jc_uint * pClass)
{
	jc_uint nHash;
	switch(nTypeCode)
	{
	case JC_FLOAT:
		nHash = 0;
		*pClass = 2;
		break;
	case JC_DOUBLE:
		nHash = 0;
		*pClass = 3;
		break;
	case JC_SIGNED_CHAR:
		nHash = 1;
		*pClass = 0;
		break;
	case JC_SIGNED_SHORT:
		nHash = 1;
		*pClass = 1;
		break;
	case JC_SIGNED_INT: case JC_ENUM:
		nHash = 1;
		*pClass = 2;
		break;
	case JC_SIGNED_LONG:
		nHash = 1;
		*pClass = 3;
		break;
	case JC_UNSIGNED_CHAR:
		nHash = 2;
		*pClass = 0;
		break;
	case JC_UNSIGNED_SHORT:
		nHash = 2;
		*pClass = 1;
		break;
	case JC_UNSIGNED_INT:
		nHash = 2;
		*pClass = 2;
		break;
	case JC_UNSIGNED_LONG:
		nHash = 2;
		*pClass = 3;
		break;
	case JC_POINTER:
		nHash = 2;
		if(IsBit32())
			*pClass = 2;
		else
			*pClass = 3;
		break;
	}
	return nHash;
}

static CJcCastNode* GetCastNode(jc_uint nHash, jc_uint nClass)
{
	jc_uint i;
	CJcCastNode* pNode = NULL;
	CJcCastTable* pTable = g_pCastTable + nHash;
	for(i=0; i<pTable->nCount; ++i)
	{
		if(nClass == pTable->pNode[i].nClass)
		{
			pNode = pTable->pNode + i;
			break;
		}
	}
	return pNode;
}

static jc_void BuildConstCastExpress(CJcExpress *pDstExp, CJcExpress *pExp, jc_ushort nOp)
{
	CJcVal oVal, oSrcVal;
	CJcSymbolStack* pStack = pExp->pStack;
	pDstExp->nOpt = JC_IS;
	switch(nOp)
	{
	case jc_f2d:
		GetConstValue(pExp, &oSrcVal, JC_FLOAT, 1);
		oVal.d = oSrcVal.f;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_double), JC_MAX(sizeof(jc_double), MaxAlign()));
		break;
	case jc_f2i:
		GetConstValue(pExp, &oSrcVal, JC_FLOAT, 1);
		oVal.i = (jc_int)oSrcVal.f;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_int), JC_MAX(sizeof(jc_int), MaxAlign()));
		break;
	case jc_d2f:
		GetConstValue(pExp, &oSrcVal, JC_DOUBLE, 1);
		oVal.f = (jc_float)oSrcVal.d;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_float), JC_MAX(sizeof(jc_float), MaxAlign()));
		break;
	case jc_d2l:
		GetConstValue(pExp, &oSrcVal, JC_DOUBLE, 1);
		oVal.l = (jc_long)oSrcVal.d;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_long), JC_MAX(sizeof(jc_long), MaxAlign()));
		break;
	case jc_c2s:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_CHAR, 0);
		oVal.s = (jc_short)oSrcVal.c;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_short), JC_MAX(sizeof(jc_short), MaxAlign()));
		break;
	case jc_s2c:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_SHORT, 0);
		oVal.c = (jc_char)oSrcVal.s;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_char), JC_MAX(sizeof(jc_char), MaxAlign()));
		break;
	case jc_s2i:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_SHORT, 0);
		oVal.i = (jc_int)oSrcVal.s;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_int), JC_MAX(sizeof(jc_int), MaxAlign()));
		break;
	case jc_i2s:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_INT, 0);
		oVal.s = (jc_short)oSrcVal.i;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_short), JC_MAX(sizeof(jc_short), MaxAlign()));
		break;
	case jc_i2l:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_INT, 0);
		oVal.l = (jc_long)oSrcVal.i;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_long), JC_MIN(sizeof(jc_long), MaxAlign()));
		break;
	case jc_i2f:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_INT, 0);
		oVal.f = (jc_float)oSrcVal.i;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_float), JC_MIN(sizeof(jc_float), MaxAlign()));
		break;
	case jc_l2i:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_LONG, 0);
		oVal.i = (jc_int)oSrcVal.l;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_int), JC_MIN(sizeof(jc_int), MaxAlign()));
		break;
	case jc_l2d:
		GetConstValue(pExp, &oSrcVal, JC_SIGNED_LONG, 0);
		oVal.d = (jc_double)oSrcVal.l;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_double), JC_MIN(sizeof(jc_double), MaxAlign()));
		break;
	case jc_uc2us:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_CHAR, 0);
		oVal.us = (jc_ushort)oSrcVal.uc;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_ushort), JC_MIN(sizeof(jc_ushort), MaxAlign()));
		break;
	case jc_us2uc:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_SHORT, 0);
		oVal.uc = (jc_uchar)oSrcVal.us;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_uchar), JC_MIN(sizeof(jc_uchar), MaxAlign()));
		break;
	case jc_us2ui:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_SHORT, 0);
		oVal.ui = (jc_uint)oSrcVal.us;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_uint), JC_MIN(sizeof(jc_uint), MaxAlign()));
		break;
	case jc_ui2us:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_INT, 0);
		oVal.us = (jc_ushort)oSrcVal.ui;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_ushort), JC_MIN(sizeof(jc_ushort), MaxAlign()));
		break;
	case jc_ui2ul:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_INT, 0);
		oVal.ul = (jc_ulong)oSrcVal.ui;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_ulong), JC_MIN(sizeof(jc_ulong), MaxAlign()));
		break;
	case jc_ul2ui:
		GetConstValue(pExp, &oSrcVal, JC_UNSIGNED_LONG, 0);
		oVal.ui = (jc_uint)oSrcVal.ul;
		pDstExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, sizeof(jc_uint), JC_MIN(sizeof(jc_uint), MaxAlign()));
		break;
	}
}

jc_void BuildCastExpress(CJcExpress *pExp, CJcTypeInfo* pTargetType)
{
	CJcTypeInfo* pSrcType;
	jc_uint nSrcTypeCode;
	CJcSymbolStack* pStack;
	CJcExpress oTmpExp;

	CJcTypeInfo* pDstType = GetOrigType(pTargetType);
	jc_uint nDstTypeCode = GetTypeCode(pDstType);

	pStack = pExp->pStack;
	LoadIntegerExpress(pExp);
	pSrcType = GetOrigType(pExp->pType);
	nSrcTypeCode = GetTypeCode(pSrcType);
	if(nDstTypeCode == JC_VOID)
	{
		DestroyExpress(pExp);
		pExp->pType = CloneType(pTargetType);
		return;
	}
	if(nSrcTypeCode == JC_VOID || nDstTypeCode == JC_FUNCTION || nDstTypeCode == JC_ARRAY)
	{
		CJcString oSrcTypeName = GetTypeName(pExp->pType, True);
		CJcString oDstTypeName = GetTypeName(pTargetType, True);
		SetError(pStack->pParser, 0, "can not convert '%s' to '%s'", oSrcTypeName.pStr, oDstTypeName.pStr);
		ClearString(&oSrcTypeName);
		ClearString(&oDstTypeName);
		return;
	}
	if(nSrcTypeCode == JC_STRUCT || nSrcTypeCode == JC_UNION ||
		nDstTypeCode == JC_STRUCT || nDstTypeCode == JC_UNION)
	{
		jc_uint nCopyType;
		jc_ushort nCopyOp;
		jc_uint nCopyCount;

		jc_uint nSrcAlign = GetTypeAlign(pSrcType);
		jc_uint nDstAlign = GetTypeAlign(pDstType);
		jc_uint nCopyAlign = nDstAlign;
		jc_uint nSrcSize = GetTypeSize(pSrcType);
		jc_uint nDstSize = GetTypeSize(pDstType);
		jc_uint nCopySize = nDstSize;
		if(nCopyAlign > nSrcAlign)
			nCopyAlign = nSrcAlign;
		if(nCopySize > nSrcSize)
			nCopySize = nSrcSize;
		if(nSrcSize != nDstSize)
		{
			CJcString oSrcTypeName = GetTypeName(pExp->pType, True);
			CJcString oDstTypeName = GetTypeName(pTargetType, True);
			SetError(pStack->pParser, 1, "the src type's size isn't matched with dst type's size", oSrcTypeName.pStr, oDstTypeName.pStr);
			ClearString(&oSrcTypeName);
			ClearString(&oDstTypeName);
		}
		switch(nCopyAlign)
		{
		case 1:
			nCopyType = JC_SIGNED_CHAR;
			nCopyOp = jc_movc;
			break;
		case 2:
			nCopyType = JC_SIGNED_SHORT;
			nCopyOp = jc_movs;
			break;
		case 4:
			nCopyType = JC_SIGNED_INT;
			nCopyOp = jc_movi;
			break;
		case 8:
			nCopyType = JC_SIGNED_LONG;
			nCopyOp = jc_movl;
			break;
		}
		InitializeExpress(pStack, &oTmpExp);
		oTmpExp.pType = CloneType(pTargetType);
		AllocTempVar(&oTmpExp);
		if(nSrcSize < nDstSize)
		{
			CJcVal oVal;
			CJcExpress oZeroExp;

			oVal.ul = 0;
			nCopyCount = nDstSize / nDstAlign;
			if(nCopyCount > 1)
				Emit1(&pStack->oCodeSegment, jc_rep, JC_UN, nCopyCount, JC_DEFAULT_INSADDR);
			InitializeExpress(pStack, &oZeroExp);
			oZeroExp.pType = CreateType(JC_CONST, nCopyType, NULL, NULL, NULL);
			oZeroExp.nOpt = JC_IS;
			oZeroExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oVal, nCopyAlign, nCopyAlign);
			Emit2(&pStack->oCodeSegment, nCopyOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg,
				(jc_uchar)oZeroExp.nOpt, oZeroExp.nArg, JC_DEFAULT_INSADDR);
			DestroyExpress(&oZeroExp);
		}
		nCopyCount = nCopySize / nCopyAlign;
		if(nCopyCount > 1)
			Emit1(&pStack->oCodeSegment, jc_rep, JC_UN, nCopyCount, JC_DEFAULT_INSADDR);
		Emit2(&pStack->oCodeSegment, nCopyOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg,
			(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
		DestroyExpress(pExp);
		*pExp = oTmpExp;
	}
	else
	{
		CJcCastNode *pSrcNode, *pDstNode, *pTmpNode;
		jc_uint nSrcClass, nSrcHash, nDstClass, nDstHash, nTmpClass, nTmpHash;
		jc_ushort nOp;

		if(nSrcTypeCode == JC_ARRAY)
		{
			pSrcType = CloneType(pSrcType->pNext);
			BuildUnaryExpress(pExp, JC_UNARY_GETADDR);
			DestroyType(pExp->pType->pNext);
			pExp->pType->pNext = pSrcType;
			pSrcType = GetOrigType(pExp->pType);
			nSrcTypeCode = GetTypeCode(pSrcType);
		}
		else if(nSrcTypeCode == JC_FUNCTION)
		{
			BuildUnaryExpress(pExp, JC_UNARY_GETADDR);
			pSrcType = GetOrigType(pExp->pType);
			nSrcTypeCode = GetTypeCode(pSrcType);
		}
		nSrcHash = GetCastClass(nSrcTypeCode, &nSrcClass);
		pSrcNode = GetCastNode(nSrcHash, nSrcClass);
		nDstHash = GetCastClass(nDstTypeCode, &nDstClass);
		pDstNode = GetCastNode(nDstHash, nDstClass);
		while(nSrcClass != nDstClass)
		{
			if(nSrcClass < nDstClass)
			{
				nTmpClass = nSrcClass + 1;
				pTmpNode = pSrcNode + 1;
				nOp = pSrcNode->nNextArith;
			}
			else if(pSrcNode->nType == JC_FLOAT)
			{
				++nSrcHash;
				nTmpClass = nSrcClass;
				pTmpNode = GetCastNode(nSrcHash, nTmpClass);
				nOp = pSrcNode->nDownArith;
			}
			else
			{
				nTmpClass = nSrcClass - 1;
				pTmpNode = pSrcNode - 1;
				nOp = pSrcNode->nPrevArith;
			}
			InitializeExpress(pStack, &oTmpExp);
			oTmpExp.pType = CreateType(JC_NONE, pTmpNode->nType, NULL, NULL, NULL);
			if(pExp->nOpt == JC_IS)
				BuildConstCastExpress(&oTmpExp, pExp, nOp);
			else
			{
				AllocTempVar(&oTmpExp);
				Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg,
					(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
			}
			DestroyExpress(pExp);
			*pExp = oTmpExp;
			pSrcNode = pTmpNode;
			nSrcClass = nTmpClass;
		}
		while(nSrcHash != nDstHash)
		{
			if(nSrcHash < nDstHash)
			{
				nTmpHash = nSrcHash + 1;
				pTmpNode = GetCastNode(nTmpHash, nDstClass);
				nOp = pSrcNode->nDownArith;
			}
			else
			{
				nTmpHash = nSrcHash - 1;
				pTmpNode = GetCastNode(nTmpHash, nDstClass);
				nOp = pSrcNode->nUpArith;
			}
			InitializeExpress(pStack, &oTmpExp);
			oTmpExp.pType = CreateType(JC_NONE, pTmpNode->nType, NULL, NULL, NULL);
			if(nOp)
			{
				if(pExp->nOpt == JC_IS)
					BuildConstCastExpress(&oTmpExp, pExp, nOp);
				else
				{
					AllocTempVar(&oTmpExp);
					Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg,
						(jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
				}
				DestroyExpress(pExp);
				*pExp = oTmpExp;
			}
			else
			{
				DestroyType(pExp->pType);
				pExp->pType = oTmpExp.pType;
			}
			pSrcNode = pTmpNode;
			nSrcHash = nTmpHash;
		}
		DestroyType(pExp->pType);
		pExp->pType = CloneType(pTargetType);
	}
	pExp->nLeftValue = 0;
}

#define JC_CMP(t, op, oVal) ((oVal op (t)0)?1:0)

static jc_void BuildCompareExpress(CJcExpress* pExp, jc_uint nCompare)
{
	CJcString oName;
	CJcSymbolStack* pStack = pExp->pStack;
	CJcTypeInfo* pNewType = NULL;
	CJcTypeInfo* pType = GetOrigType(pExp->pType);
	jc_uint nType = GetTypeCode(pType);
	switch(nType)
	{
	default:
		oName = GetTypeName(pExp->pType, True);
		SetError(pExp->pStack->pParser, 0, "'%s' don't support relation operation", oName.pStr);
		ClearString(&oName);
		break;
	case JC_FLOAT: case JC_DOUBLE:
	case JC_SIGNED_CHAR: case JC_SIGNED_SHORT: case JC_SIGNED_INT: case JC_SIGNED_LONG: case JC_ENUM:
		pNewType = CloneType(pType);
		break;
	case JC_UNSIGNED_CHAR: case JC_UNSIGNED_SHORT: case JC_UNSIGNED_INT: case JC_UNSIGNED_LONG:
		pNewType = CreateType(JC_CONST, ((nType&(~JC_UNSIGNED))|JC_SIGNED), NULL, NULL, NULL);
	case JC_POINTER:
		pNewType = CreateType(JC_CONST, IsBit32()?JC_SIGNED_INT:JC_SIGNED_LONG, NULL, NULL, NULL);
		break;
	}
	if(pNewType)
	{
		jc_uint nOp;
		CJcExpress oTmpExp;
		DestroyType(pExp->pType);
		pExp->pType = pNewType;
		pType = GetOrigType(pExp->pType);
		nType = GetTypeCode(pType);
		nOp = GetCompareInstruction(nType, nCompare);
		InitializeExpress(pExp->pStack, &oTmpExp);
		oTmpExp.pType = CreateType(JC_CONST, JC_UNSIGNED_INT, NULL, NULL, NULL);
		if(pExp->nOpt == JC_IS)
		{
			CJcVal oVal;
			jc_uint nCmpRet;
			GetConstValue(pExp, &oVal, nType, 1);
			oTmpExp.nOpt = JC_IS;
			//#define JC_CMP(t, op, oVal) (nRet = (oVal op (t)0)?1:0)
			switch(nOp)
			{
			case jc_ltc:	nCmpRet = JC_CMP(jc_char, 	< , oVal.c);		break;
			case jc_lts:	nCmpRet = JC_CMP(jc_short, 	< , oVal.s);		break;
			case jc_lti:	nCmpRet = JC_CMP(jc_int, 	< , oVal.i);		break;
			case jc_ltl:	nCmpRet = JC_CMP(jc_long, 	< , oVal.l);		break;
			case jc_ltf:	nCmpRet = JC_CMP(jc_float, 	< , oVal.f);		break;
			case jc_ltd:	nCmpRet = JC_CMP(jc_double, < , oVal.d);		break;
			case jc_ltuc:	nCmpRet = JC_CMP(jc_uchar, 	< , oVal.uc);		break;
			case jc_ltus:	nCmpRet = JC_CMP(jc_ushort,	< , oVal.us);		break;
			case jc_ltui:	nCmpRet = JC_CMP(jc_uint,	< , oVal.ui);		break;
			case jc_ltul:	nCmpRet = JC_CMP(jc_ulong, 	< , oVal.ul);		break;
			case jc_lec:	nCmpRet = JC_CMP(jc_char, 	<= , oVal.c);		break;
			case jc_les:	nCmpRet = JC_CMP(jc_short, 	<= , oVal.s);		break;
			case jc_lei:	nCmpRet = JC_CMP(jc_int, 	<= , oVal.i);		break;
			case jc_lel:	nCmpRet = JC_CMP(jc_long, 	<= , oVal.l);		break;
			case jc_lef:	nCmpRet = JC_CMP(jc_float, 	<= , oVal.f);		break;
			case jc_led:	nCmpRet = JC_CMP(jc_double, <= , oVal.d);		break;
			case jc_leuc:	nCmpRet = JC_CMP(jc_uchar, 	<= , oVal.uc);		break;
			case jc_leus:	nCmpRet = JC_CMP(jc_ushort,	<= , oVal.us);		break;
			case jc_leui:	nCmpRet = JC_CMP(jc_uint,	<= , oVal.ui);		break;
			case jc_leul:	nCmpRet = JC_CMP(jc_ulong, 	<= , oVal.ul);		break;
			case jc_eqf:	nCmpRet = JC_CMP(jc_float, 	== , oVal.f);		break;
			case jc_eqd:	nCmpRet = JC_CMP(jc_double, == , oVal.d);		break;
			case jc_eqc:	nCmpRet = JC_CMP(jc_uchar, 	== , oVal.uc);		break;
			case jc_eqs:	nCmpRet = JC_CMP(jc_ushort,	== , oVal.us);		break;
			case jc_eqi:	nCmpRet = JC_CMP(jc_uint,	== , oVal.ui);		break;
			case jc_eql:	nCmpRet = JC_CMP(jc_ulong, 	== , oVal.ul);		break;
			case jc_nef:	nCmpRet = JC_CMP(jc_float, 	!= , oVal.f);		break;
			case jc_ned:	nCmpRet = JC_CMP(jc_double, != , oVal.d);		break;
			case jc_nec:	nCmpRet = JC_CMP(jc_uchar, 	!= , oVal.uc);		break;
			case jc_nes:	nCmpRet = JC_CMP(jc_ushort,	!= , oVal.us);		break;
			case jc_nei:	nCmpRet = JC_CMP(jc_uint,	!= , oVal.ui);		break;
			case jc_nel:	nCmpRet = JC_CMP(jc_ulong, 	!= , oVal.ul);		break;
			case jc_gec:	nCmpRet = JC_CMP(jc_char, 	>= , oVal.c);		break;
			case jc_ges:	nCmpRet = JC_CMP(jc_short, 	>= , oVal.s);		break;
			case jc_gei:	nCmpRet = JC_CMP(jc_int, 	>= , oVal.i);		break;
			case jc_gel:	nCmpRet = JC_CMP(jc_long, 	>= , oVal.l);		break;
			case jc_gef:	nCmpRet = JC_CMP(jc_float, 	>= , oVal.f);		break;
			case jc_ged:	nCmpRet = JC_CMP(jc_double, >= , oVal.d);		break;
			case jc_geuc:	nCmpRet = JC_CMP(jc_uchar, 	>= , oVal.uc);		break;
			case jc_geus:	nCmpRet = JC_CMP(jc_ushort,	>= , oVal.us);		break;
			case jc_geui:	nCmpRet = JC_CMP(jc_uint,	>= , oVal.ui);		break;
			case jc_geul:	nCmpRet = JC_CMP(jc_ulong, 	>= , oVal.ul);		break;
			case jc_gtc:	nCmpRet = JC_CMP(jc_char, 	> , oVal.c);		break;
			case jc_gts:	nCmpRet = JC_CMP(jc_short, 	> , oVal.s);		break;
			case jc_gti:	nCmpRet = JC_CMP(jc_int, 	> , oVal.i);		break;
			case jc_gtl:	nCmpRet = JC_CMP(jc_long, 	> , oVal.l);		break;
			case jc_gtf:	nCmpRet = JC_CMP(jc_float, 	> , oVal.f);		break;
			case jc_gtd:	nCmpRet = JC_CMP(jc_double, > , oVal.d);		break;
			case jc_gtuc:	nCmpRet = JC_CMP(jc_uchar, 	> , oVal.uc);		break;
			case jc_gtus:	nCmpRet = JC_CMP(jc_ushort,	> , oVal.us);		break;
			case jc_gtui:	nCmpRet = JC_CMP(jc_uint,	> , oVal.ui);		break;
			case jc_gtul:	nCmpRet = JC_CMP(jc_ulong, 	> , oVal.ul);		break;
			}
			oTmpExp.nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nCmpRet, GetTypeSize(oTmpExp.pType), GetTypeAlign(oTmpExp.pType));
		}
		else
		{
			AllocTempVar(&oTmpExp);
			Emit2(&pStack->oCodeSegment, (jc_ushort)nOp, (jc_uchar)oTmpExp.nOpt, oTmpExp.nArg, (jc_uchar)pExp->nOpt, pExp->nArg, JC_DEFAULT_INSADDR);
		}

		DestroyExpress(pExp);
		*pExp = oTmpExp;
	}
}

jc_void BuildBinaryExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nBinaryOperator)
{
	jc_uint nSize, nLeftTypeCode, nTarTypeCode, nRightTypeCode;
	CJcExpress oTmpExp;
	CJcTypeInfo* pTarType = NULL;
	CJcSymbolStack* pStack = pLeftExp->pStack;

	nLeftTypeCode = GetTypeCode(GetOrigType(pLeftExp->pType));
	if(nLeftTypeCode == JC_ENUM)
	{
		jc_uint nQual = (pLeftExp->nOpt == JC_IS)?JC_CONST:JC_NONE;
		DestroyType(pLeftExp->pType);
		pLeftExp->pType = CreateType(nQual, JC_SIGNED_INT, NULL, NULL, NULL);
	}
	LoadIntegerExpress(pLeftExp);
	LoadIntegerExpress(pRightExp);
	switch(nBinaryOperator)
	{
	case JC_BINARY_ADD: case JC_BINARY_SUB: case JC_BINARY_MUL: case JC_BINARY_DIV: case JC_BINARY_MOD:
		pTarType = GetAutoCastType(pStack, pLeftExp->pType, pRightExp->pType, nBinaryOperator);
		if(!pTarType)
			return;
		nTarTypeCode = GetTypeCode(GetOrigType(pTarType));
		if(nLeftTypeCode == JC_POINTER || nLeftTypeCode == JC_ARRAY)
		{
			CJcExpress oSizeExp;
			nSize = GetTypeSize(pTarType);
			if(nTarTypeCode == JC_POINTER)
			{
				CJcTypeInfo* pNewTarType;
				if(sizeof(jc_void*)==sizeof(jc_int))
					pNewTarType = CreateType(JC_NONE, JC_UNSIGNED_INT, NULL, NULL, NULL);
				else
					pNewTarType = CreateType(JC_NONE, JC_UNSIGNED_LONG, NULL, NULL, NULL);
				BuildCastExpress(pLeftExp, pNewTarType);
				BuildCastExpress(pRightExp, pNewTarType);
				DestroyType(pNewTarType);
				oSizeExp = BuildIntegerConstantExpressA(pLeftExp->pStack, nSize, False);
				BuildBinaryExpress(pRightExp, &oSizeExp, JC_ASSIGN_EQUAL | JC_BINARY_MUL);
				DestroyExpress(&oSizeExp);
				pLeftExp->nLeftValue = 1;
				BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator, 1);
				BuildCastExpress(pLeftExp, pTarType);
				DestroyType(pTarType);
				return;
			}
			BuildCastExpress(pLeftExp, pTarType);
			BuildCastExpress(pRightExp, pTarType);
			pLeftExp->nLeftValue = 1;
			DestroyType(pTarType);
			BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator, 1);
			oSizeExp = BuildIntegerConstantExpressA(pLeftExp->pStack, nSize, False);
			BuildBinaryExpress(pLeftExp, &oSizeExp, JC_ASSIGN_EQUAL | JC_BINARY_DIV);
			return;
		}
		BuildCastExpress(pLeftExp, pTarType);
		BuildCastExpress(pRightExp, pTarType);
		nRightTypeCode = nTarTypeCode;
		DestroyType(pTarType);
		if(pLeftExp->nOpt == JC_IS && pRightExp->nOpt == JC_IS)
			BuildConstAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator);
		else
		{
			if(pLeftExp->nVarId < 0)
			{
				oTmpExp = CopyExpress(pLeftExp);
				DestroyExpress(pLeftExp);
				*pLeftExp = oTmpExp;
			}
			pLeftExp->nLeftValue = 1;
			BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator, 1);
		}
		break;
	case JC_BINARY_LSHIFT: case JC_BINARY_RSHIFT:
	case JC_BINARY_BIT_AND: case JC_BINARY_BIT_OR: case JC_BINARY_BIT_XOR:
		nLeftTypeCode = GetTypeCode(GetOrigType(pLeftExp->pType));
		if(nLeftTypeCode == JC_ENUM)
		{
			jc_uint nQual = (pLeftExp->nOpt == JC_IS)?JC_CONST:JC_NONE;
			DestroyType(pLeftExp->pType);
			pLeftExp->pType = CreateType(nQual, JC_SIGNED_INT, NULL, NULL, NULL);
		}
		if(pLeftExp->nOpt == JC_IS && pRightExp->nOpt == JC_IS)
			BuildConstAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator);
		else
		{
			if(pLeftExp->nVarId < 0)
			{
				oTmpExp = CopyExpress(pLeftExp);
				DestroyExpress(pLeftExp);
				*pLeftExp = oTmpExp;
			}
			pLeftExp->nLeftValue = 1;
			BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL | nBinaryOperator, 1);
		}
		break;
	case JC_BINARY_LT: case JC_BINARY_LE: case JC_BINARY_MT: case JC_BINARY_ME: case JC_BINARY_EQ: case JC_BINARY_NE:
		if(pRightExp->nOpt == JC_IS)
		{
			CJcVal oConstV;
			MemorySet(&oConstV, 0, sizeof(oConstV));
			if(!GetConstValue(pRightExp, &oConstV, pRightExp->pType->nType, 1))
			{
				if(oConstV.ul == 0)
				{
					BuildCompareExpress(pLeftExp, nBinaryOperator);
					break;
				}
			}
		}
		BuildBinaryExpress(pLeftExp, pRightExp, JC_BINARY_SUB);
		BuildCompareExpress(pLeftExp, nBinaryOperator);
		break;
	case JC_BINARY_LOGIC_AND: case JC_BINARY_LOGIC_OR:
		BuildCompareExpress(pLeftExp, JC_BINARY_NE);
		BuildCompareExpress(pRightExp, JC_BINARY_NE);
		if(pLeftExp->nOpt == JC_IS && pRightExp->nOpt == JC_IS)
		{
			if(nBinaryOperator == JC_BINARY_LOGIC_AND)
				BuildConstAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL|JC_BINARY_BIT_AND);
			else
				BuildConstAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL|JC_BINARY_BIT_OR);
		}
		else
		{
			pLeftExp->nLeftValue = 1;
			if(nBinaryOperator == JC_BINARY_LOGIC_AND)
				BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL|JC_BINARY_BIT_AND, 1);
			else
				BuildAssignmentExpress(pLeftExp, pRightExp, JC_ASSIGN_EQUAL|JC_BINARY_BIT_OR, 1);
		}
		break;
	}
	pLeftExp->nLeftValue = 0;
}

static jc_void BuildConstAssignmentExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nAssignOperator)
{
	CJcSymbolStack* pStack = pLeftExp->pStack;
	CJcExpress oTmpExp;
	jc_ushort nOp;
	jc_uint nLeftTypeCode;
	jc_uint nRightTypeCode;
	CJcVal oDstVal, oSrcVal;

	LoadIntegerExpress(pRightExp);
	nLeftTypeCode = GetTypeCode(GetOrigType(pLeftExp->pType));
	nRightTypeCode = GetTypeCode(GetOrigType(pRightExp->pType));
	if(nAssignOperator == JC_ASSIGN_ADD_EQ || nAssignOperator == JC_ASSIGN_SUB_EQ)
	{
		if(nLeftTypeCode == JC_POINTER)
		{
			if(!IsIntegralType(nRightTypeCode))
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}
		}
		else if(nLeftTypeCode == JC_ENUM || !IsNumberalType(nLeftTypeCode) || !IsNumberalType(nRightTypeCode))
		{
			CJcString oStr1, oStr2;
			oStr1 = GetTypeName(pLeftExp->pType, True);
			oStr2 = GetTypeName(pRightExp->pType, True);
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
		else if(nLeftTypeCode != JC_POINTER)
		{
			if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
				return;
		}
		if(nLeftTypeCode == JC_POINTER)
		{
			CJcTypeInfo* pNewType;
			jc_uint nSize = GetTypeSize(pLeftExp->pType);
			if(sizeof(jc_void*)==sizeof(jc_int))
				pNewType = CreateType(JC_NONE, JC_UNSIGNED_INT, NULL, NULL, NULL);
			else
				pNewType = CreateType(JC_NONE, JC_UNSIGNED_LONG, NULL, NULL, NULL);
			BuildCastExpress(pRightExp, pNewType);
			DestroyType(pNewType);
			if(nSize > 1)
			{
				oTmpExp = BuildIntegerConstantExpressA(pStack, nSize, 0);
				BuildConstAssignmentExpress(pRightExp, &oTmpExp, JC_ASSIGN_MUL_EQ);
				DestroyExpress(&oTmpExp);
			}
		}
	}
	else if(nAssignOperator == JC_ASSIGN_MUL_EQ || nAssignOperator == JC_ASSIGN_DIV_EQ || nAssignOperator == JC_ASSIGN_MOD_EQ)
	{
		if(nAssignOperator == JC_ASSIGN_MOD_EQ)
		{
			if(nLeftTypeCode == JC_FLOAT || nLeftTypeCode == JC_DOUBLE || nRightTypeCode == JC_FLOAT || nRightTypeCode == JC_DOUBLE)
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}
		}
		if(nLeftTypeCode == JC_ENUM ||
			!IsNumberalType(nLeftTypeCode) || !IsNumberalType(nRightTypeCode))
		{
			CJcString oStr1, oStr2;
			oStr1 = GetTypeName(pLeftExp->pType, True);
			oStr2 = GetTypeName(pRightExp->pType, True);
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}

		if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
			return;
	}
	else if(nAssignOperator == JC_ASSIGN_LSH_EQ || nAssignOperator == JC_ASSIGN_RSH_EQ)
	{
		CJcTypeInfo* pNewType;
		if(nLeftTypeCode == JC_ENUM || !IsIntegralType(nLeftTypeCode) || !IsIntegralType(nRightTypeCode))
		{
			CJcString oStr1, oStr2;
			oStr1 = GetTypeName(pLeftExp->pType, True);
			oStr2 = GetTypeName(pRightExp->pType, True);
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
		pNewType = CreateType(JC_NONE, JC_UNSIGNED_CHAR, NULL, NULL, NULL);
		BuildCastExpress(pRightExp, pNewType);
		DestroyType(pNewType);
	}
	else if(nAssignOperator == JC_ASSIGN_AND_EQ || nAssignOperator == JC_ASSIGN_OR_EQ || nAssignOperator == JC_ASSIGN_XOR_EQ)
	{
		if(!IsIntegralType(nLeftTypeCode) || !IsIntegralType(nRightTypeCode))
		{
			CJcString oStr1, oStr2;
			oStr1 = GetTypeName(pLeftExp->pType, True);
			oStr2 = GetTypeName(pRightExp->pType, True);
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return;
		}
		if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
			return;
	}
	if(nAssignOperator != JC_ASSIGN_LSH_EQ && nAssignOperator != JC_ASSIGN_RSH_EQ)
		BuildCastExpress(pRightExp, pLeftExp->pType);
	nOp = GetAssignInstruction(nLeftTypeCode, nAssignOperator);
	nRightTypeCode = GetTypeCode(GetOrigType(pRightExp->pType));
	GetConstValue(pLeftExp, &oDstVal, nLeftTypeCode, 1);
	GetConstValue(pRightExp, &oSrcVal, nRightTypeCode, 1);
	switch(nOp)
	{
	case jc_addc:	oDstVal.uc += oSrcVal.uc;	break;
	case jc_adds:	oDstVal.us += oSrcVal.us;	break;
	case jc_addi:	oDstVal.ui += oSrcVal.ui;	break;
	case jc_addl:	oDstVal.ul += oSrcVal.ul;	break;
	case jc_addf:	oDstVal.f += oSrcVal.f;		break;
	case jc_addd:	oDstVal.d += oSrcVal.d;		break;
	case jc_subc:	oDstVal.uc -= oSrcVal.uc;	break;
	case jc_subs:	oDstVal.us -= oSrcVal.us;	break;
	case jc_subi:	oDstVal.ui -= oSrcVal.ui;	break;
	case jc_subl:	oDstVal.ul -= oSrcVal.ul;	break;
	case jc_subf:	oDstVal.f -= oSrcVal.f;		break;
	case jc_subd:	oDstVal.d -= oSrcVal.d;		break;
	case jc_mulc:	oDstVal.c *= oSrcVal.c;		break;
	case jc_muls:	oDstVal.s *= oSrcVal.s;		break;
	case jc_muli:	oDstVal.i *= oSrcVal.i;		break;
	case jc_mull:	oDstVal.l *= oSrcVal.l;		break;
	case jc_muluc:	oDstVal.uc *= oSrcVal.uc;	break;
	case jc_mulus:	oDstVal.us *= oSrcVal.us;	break;
	case jc_mului:	oDstVal.ui *= oSrcVal.ui;	break;
	case jc_mulul:	oDstVal.ul *= oSrcVal.ul;	break;
	case jc_mulf:	oDstVal.f *= oSrcVal.f;		break;
	case jc_muld:	oDstVal.d *= oSrcVal.d;		break;
	case jc_divc:	oDstVal.c /= oSrcVal.c;		break;
	case jc_divs:	oDstVal.s /= oSrcVal.s;		break;
	case jc_divi:	oDstVal.i /= oSrcVal.i;		break;
	case jc_divl:	oDstVal.l /= oSrcVal.l;		break;
	case jc_divuc:	oDstVal.uc /= oSrcVal.uc;	break;
	case jc_divus:	oDstVal.us /= oSrcVal.us;	break;
	case jc_divui:	oDstVal.ui /= oSrcVal.ui;	break;
	case jc_divul:	oDstVal.ul /= oSrcVal.ul;	break;
	case jc_divf:	oDstVal.f /= oSrcVal.f;		break;
	case jc_divd:	oDstVal.d /= oSrcVal.d;		break;
	case jc_modc:	oDstVal.c %= oSrcVal.c;		break;
	case jc_mods:	oDstVal.s %= oSrcVal.s;		break;
	case jc_modi:	oDstVal.i %= oSrcVal.i;		break;
	case jc_modl:	oDstVal.l %= oSrcVal.l;		break;
	case jc_moduc:	oDstVal.uc %= oSrcVal.uc;	break;
	case jc_modus:	oDstVal.us %= oSrcVal.us;	break;
	case jc_modui:	oDstVal.ui %= oSrcVal.ui;	break;
	case jc_modul:	oDstVal.ul %= oSrcVal.ul;	break;
	case jc_lshuc:	oDstVal.uc <<= (0x000000FF&oSrcVal.ui);	break;
	case jc_lshus:	oDstVal.us <<= (0x000000FF&oSrcVal.ui);	break;
	case jc_lshui:	oDstVal.ui <<= (0x000000FF&oSrcVal.ui);	break;
	case jc_lshul:	oDstVal.ul <<= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshc:	oDstVal.c >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshs:	oDstVal.s >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshi:	oDstVal.i >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshl:	oDstVal.l >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshuc:	oDstVal.uc >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshus:	oDstVal.us >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshui:	oDstVal.ui >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_rshul:	oDstVal.ul >>= (0x000000FF&oSrcVal.ui);	break;
	case jc_anduc:	oDstVal.uc &= oSrcVal.uc;	break;
	case jc_andus:	oDstVal.us &= oSrcVal.us;	break;
	case jc_andui:	oDstVal.ui &= oSrcVal.ui;	break;
	case jc_andul:	oDstVal.ul &= oSrcVal.ul;	break;
	case jc_oruc:	oDstVal.uc |= oSrcVal.uc;	break;
	case jc_orus:	oDstVal.us |= oSrcVal.us;	break;
	case jc_orui:	oDstVal.ui |= oSrcVal.ui;	break;
	case jc_orul:	oDstVal.ul |= oSrcVal.ul;	break;
	case jc_xoruc:	oDstVal.uc ^= oSrcVal.uc;	break;
	case jc_xorus:	oDstVal.us ^= oSrcVal.us;	break;
	case jc_xorui:	oDstVal.ui ^= oSrcVal.ui;	break;
	case jc_xorul:	oDstVal.ul ^= oSrcVal.ul;	break;
	}
	pLeftExp->nArg = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &oDstVal, GetTypeSize(pLeftExp->pType), GetTypeAlign(pLeftExp->pType));
}

jc_void BuildAssignmentExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nAssignOperator, jc_uint nCheck)
{
	CJcSymbolStack* pStack = pLeftExp->pStack;
	CJcExpress oTmpExp;

	if(!pLeftExp->nLeftValue)
	{
		SetError(pStack->pParser, 0, "the non-left-value express don't support the '%s' operator", GetAssignOperatorString(nAssignOperator));
		return;
	}

	LoadIntegerExpress(pRightExp);
	if(nAssignOperator == JC_ASSIGN_EQUAL)
	{
		if(nCheck)
		{
			jc_bool bConstNull = False;
			jc_uint nType = GetTypeCode(pRightExp->pType);
			if(pRightExp->nOpt == JC_IS && (nType==JC_POINTER||nType==JC_SIGNED_INT))
			{
				CJcVal oVal;
				if(GetConstValue(pRightExp, &oVal, JC_POINTER, False))
					return;
				if(sizeof(jc_void*) == sizeof(jc_uint))
				{
					if(oVal.ui == 0)
						bConstNull = True;
				}
				else
				{
					if(oVal.ul == (jc_ulong)0)
						bConstNull = True;
				}
			}
			if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, bConstNull))
				return;
		}
		BuildCastExpress(pRightExp, pLeftExp->pType);
		if(!pLeftExp->nBits)
			SaveExpress(pLeftExp, pRightExp);
		else
		{
			if(pLeftExp->nQuote)
				oTmpExp = LoadExpress(pLeftExp);
			else
				oTmpExp = CopyExpress(pLeftExp);
			ZeroBitExpress(&oTmpExp);
			BuildBitExpress(pRightExp, pLeftExp->nBits, pLeftExp->nStart);
			oTmpExp.nLeftValue = 1;
			BuildAssignmentExpress(&oTmpExp, pRightExp, JC_ASSIGN_OR_EQ, 1);
			SaveExpress(pLeftExp, &oTmpExp);
			DestroyExpress(&oTmpExp);
		}
	}
	else
	{
		jc_ushort nOp;
		CJcExpress oTmpExp1, oTmpExp2;
		CJcExpress* pLeftExp1;

		jc_uint nLeftTypeCode = GetTypeCode(GetOrigType(pLeftExp->pType));
		jc_uint nRightTypeCode = GetTypeCode(GetOrigType(pRightExp->pType));

		if(nAssignOperator == JC_ASSIGN_ADD_EQ || nAssignOperator == JC_ASSIGN_SUB_EQ)
		{
			if(nLeftTypeCode == JC_POINTER)
			{
				if(!IsIntegralType(nRightTypeCode))
				{
					CJcString oStr1, oStr2;
					oStr1 = GetTypeName(pLeftExp->pType, True);
					oStr2 = GetTypeName(pRightExp->pType, True);
					SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
					ClearString(&oStr1);
					ClearString(&oStr2);
					return;
				}
			}
			else if(nLeftTypeCode == JC_ENUM || !IsNumberalType(nLeftTypeCode) || !IsNumberalType(nRightTypeCode))
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}
			else if(nLeftTypeCode != JC_POINTER)
			{
				if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
					return;
			}
			if(nLeftTypeCode == JC_POINTER)
			{
				CJcTypeInfo* pNewType;
				jc_uint nSize = GetTypeSize(pLeftExp->pType);
				if(sizeof(jc_void*)==sizeof(jc_int))
					pNewType = CreateType(JC_NONE, JC_UNSIGNED_INT, NULL, NULL, NULL);
				else
					pNewType = CreateType(JC_NONE, JC_UNSIGNED_LONG, NULL, NULL, NULL);
				BuildCastExpress(pRightExp, pNewType);
				DestroyType(pNewType);
				if(nSize > 1)
				{
					oTmpExp = BuildIntegerConstantExpressA(pStack, nSize, 0);
					if(pRightExp->nVarId < 0)
					{
						CJcExpress oTmpExp2 = CopyExpress(pRightExp);
						DestroyExpress(pRightExp);
						*pRightExp = oTmpExp2;
					}
					pRightExp->nLeftValue = 1;
					BuildAssignmentExpress(pRightExp, &oTmpExp, JC_ASSIGN_MUL_EQ, 1);
					pRightExp->nLeftValue = 0;
					DestroyExpress(&oTmpExp);
				}
			}
		}
		else if(nAssignOperator == JC_ASSIGN_MUL_EQ || nAssignOperator == JC_ASSIGN_DIV_EQ || nAssignOperator == JC_ASSIGN_MOD_EQ)
		{
			if(nAssignOperator == JC_ASSIGN_MOD_EQ)
			{
				if(nLeftTypeCode == JC_FLOAT || nLeftTypeCode == JC_DOUBLE || nRightTypeCode == JC_FLOAT || nRightTypeCode == JC_DOUBLE)
				{
					CJcString oStr1, oStr2;
					oStr1 = GetTypeName(pLeftExp->pType, True);
					oStr2 = GetTypeName(pRightExp->pType, True);
					SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
					ClearString(&oStr1);
					ClearString(&oStr2);
					return;
				}
			}
			if(nLeftTypeCode == JC_ENUM ||
				!IsNumberalType(nLeftTypeCode) || !IsNumberalType(nRightTypeCode))
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}

			if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
				return;
		}
		else if(nAssignOperator == JC_ASSIGN_LSH_EQ || nAssignOperator == JC_ASSIGN_RSH_EQ)
		{
			CJcTypeInfo* pNewType;
			if(nLeftTypeCode == JC_ENUM || !IsIntegralType(nLeftTypeCode) || !IsIntegralType(nRightTypeCode))
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}
			pNewType = CreateType(JC_NONE, JC_UNSIGNED_CHAR, NULL, NULL, NULL);
			BuildCastExpress(pRightExp, pNewType);
			DestroyType(pNewType);
		}
		else if(nAssignOperator == JC_ASSIGN_AND_EQ || nAssignOperator == JC_ASSIGN_OR_EQ || nAssignOperator == JC_ASSIGN_XOR_EQ)
		{
			if(!IsIntegralType(nLeftTypeCode) || !IsIntegralType(nRightTypeCode))
			{
				CJcString oStr1, oStr2;
				oStr1 = GetTypeName(pLeftExp->pType, True);
				oStr2 = GetTypeName(pRightExp->pType, True);
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return;
			}
			if(CheckAutoAssignCast(pStack, pLeftExp->pType, pRightExp->pType, False))
				return;
		}

		if(nAssignOperator != JC_ASSIGN_LSH_EQ && nAssignOperator != JC_ASSIGN_RSH_EQ)
			BuildCastExpress(pRightExp, pLeftExp->pType);
		nOp = GetAssignInstruction(nLeftTypeCode, nAssignOperator);
		pLeftExp1 = pLeftExp;
		if(pLeftExp1->nQuote)
		{
			oTmpExp1 = LoadExpress(pLeftExp);
			pLeftExp1 = &oTmpExp1;
		}
		if(pLeftExp1->nBits)
		{
			if(pLeftExp1 == pLeftExp)
			{
				oTmpExp1 = CopyExpress(pLeftExp1);
				pLeftExp1 = &oTmpExp1;
			}
			LoadBitExpress(pLeftExp1);
		}
		if(pLeftExp1 != pLeftExp && pLeftExp->nBits)
		{
			oTmpExp2 = CopyExpress(pLeftExp1);
			oTmpExp2.nBits = pLeftExp->nBits;
			oTmpExp2.nStart = pLeftExp->nStart;
			ZeroBitExpress(&oTmpExp2);
		}
		Emit2(&pStack->oCodeSegment, nOp, (jc_uchar)pLeftExp1->nOpt, pLeftExp1->nArg,
			(jc_uchar)pRightExp->nOpt, pRightExp->nArg, JC_DEFAULT_INSADDR);
		if(pLeftExp1 != pLeftExp)
		{
			if(pLeftExp->nBits)
			{
				BuildBitExpress(pLeftExp1, pLeftExp->nBits, pLeftExp->nArg);
				pLeftExp1->nLeftValue = 1;
				BuildAssignmentExpress(pLeftExp1, &oTmpExp2, JC_ASSIGN_OR_EQ, 1);
				DestroyExpress(&oTmpExp2);
			}
			SaveExpress(pLeftExp, pLeftExp1);
			DestroyExpress(pLeftExp1);
		}
	}
}

static jc_void PrintfVal(jc_char* pBuf, CJcVal* pVal, jc_uint nType)
{
	switch(nType)
	{
	case JC_SIGNED_CHAR: case JC_UNSIGNED_CHAR:
		if(g_oInterface.IsPrint(pVal->i))
			g_oInterface.FormatPrint(pBuf, "'%c'", pVal->c);
		else
			g_oInterface.FormatPrint(pBuf, "'\\x%2X'", pVal->ui);
		break;
	case JC_SIGNED_SHORT: case JC_SIGNED_INT:
		g_oInterface.FormatPrint(pBuf, "%d", pVal->i);
		break;
	case JC_UNSIGNED_SHORT: case JC_UNSIGNED_INT:
		g_oInterface.FormatPrint(pBuf, "%u", pVal->ui);
		break;
	case JC_SIGNED_LONG:
		g_oInterface.FormatPrint(pBuf, "ld", pVal->l);
		break;
	case JC_UNSIGNED_LONG:
		g_oInterface.FormatPrint(pBuf, "lu", pVal->ul);
		break;
	case JC_FLOAT:
		g_oInterface.FormatPrint(pBuf, "%g", (jc_double)pVal->f);
		break;
	case JC_DOUBLE:
		g_oInterface.FormatPrint(pBuf, "%g", pVal->d);
		break;
	}
}

static CJcVal GetBitVal(jc_uint nAlignCode, jc_uint nBits)
{
	CJcVal oVal;
	switch(nAlignCode)
	{
	case 1:
		oVal.c = (jc_char)MIN_JC_CHAR;
		if(nBits > 2)
			oVal.c >>= (nBits-1);
		oVal.uc >>= (8-nBits);
		break;
	case 2:
		oVal.s = (jc_short)MIN_JC_SHORT;
		if(nBits > 2)
			oVal.s >>= (nBits-1);
		oVal.us >>= (16-nBits);
		break;
	case 4:
		oVal.i = (jc_int)MIN_JC_INT;
		if(nBits > 2)
			oVal.i >>= (nBits-1);
		oVal.ui >>= (32-nBits);
		break;
	case 8:
		oVal.l = (jc_long)MIN_JC_LONG;
		if(nBits > 2)
			oVal.l >>= (nBits-1);
		oVal.ul >>= (64-nBits);
		break;
	}
	return oVal;
}

static jc_uint GetCompareInstruction(jc_uint nType, jc_uint nOp)
{
	jc_uint nIns;
	switch(nOp)
	{
	case JC_BINARY_LT:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_ltc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_lts;	break;
		case JC_SIGNED_INT:			nIns = jc_lti;	break;
		case JC_SIGNED_LONG:		nIns = jc_ltl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_ltuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_ltus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_ltui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_ltul;	break;
		case JC_FLOAT:				nIns = jc_ltf;	break;
		case JC_DOUBLE:				nIns = jc_ltd;	break;
		case JC_ENUM:				nIns = jc_lti;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_ltui:jc_ltul);
			break;
		}
		break;
	case JC_BINARY_LE:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_lec;	break;
		case JC_SIGNED_SHORT:		nIns = jc_les;	break;
		case JC_SIGNED_INT:			nIns = jc_lei;	break;
		case JC_SIGNED_LONG:		nIns = jc_lel;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_leuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_leus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_leui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_leul;	break;
		case JC_FLOAT:				nIns = jc_lef;	break;
		case JC_DOUBLE:				nIns = jc_led;	break;
		case JC_ENUM:				nIns = jc_lei;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_leui:jc_leul);
			break;
		}
		break;
	case JC_BINARY_MT:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_gtc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_gts;	break;
		case JC_SIGNED_INT:			nIns = jc_gti;	break;
		case JC_SIGNED_LONG:		nIns = jc_gtl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_gtuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_gtus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_gtui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_gtul;	break;
		case JC_FLOAT:				nIns = jc_gtf;	break;
		case JC_DOUBLE:				nIns = jc_gtd;	break;
		case JC_ENUM:				nIns = jc_gti;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_gtui:jc_gtul);
			break;
		}
		break;
	case JC_BINARY_ME:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_gec;	break;
		case JC_SIGNED_SHORT:		nIns = jc_ges;	break;
		case JC_SIGNED_INT:			nIns = jc_gei;	break;
		case JC_SIGNED_LONG:		nIns = jc_gel;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_geuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_geus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_geui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_geul;	break;
		case JC_FLOAT:				nIns = jc_gef;	break;
		case JC_DOUBLE:				nIns = jc_ged;	break;
		case JC_ENUM:				nIns = jc_gei;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_geui:jc_geul);
			break;
		}
		break;
	case JC_BINARY_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_eqc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_eqs;	break;
		case JC_SIGNED_INT:			nIns = jc_eqi;	break;
		case JC_SIGNED_LONG:		nIns = jc_eql;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_eqc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_eqs;	break;
		case JC_UNSIGNED_INT:		nIns = jc_eqi;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_eql;	break;
		case JC_FLOAT:				nIns = jc_eqf;	break;
		case JC_DOUBLE:				nIns = jc_eqd;	break;
		case JC_ENUM:				nIns = jc_eqi;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_eqi:jc_eql);
			break;
		}
		break;
	case JC_BINARY_NE:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_nec;	break;
		case JC_SIGNED_SHORT:		nIns = jc_nes;	break;
		case JC_SIGNED_INT:			nIns = jc_nei;	break;
		case JC_SIGNED_LONG:		nIns = jc_nel;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_nec;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_nes;	break;
		case JC_UNSIGNED_INT:		nIns = jc_nei;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_nel;	break;
		case JC_FLOAT:				nIns = jc_nef;	break;
		case JC_DOUBLE:				nIns = jc_ned;	break;
		case JC_ENUM:				nIns = jc_nei;	break;
		case JC_POINTER:
			nIns = (IsBit32()?jc_nei:jc_nel);
			break;
		}
		break;
	}
	return nIns;
}

static CJcTypeInfo* GetAutoCastType(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_uint nOp)
{
	jc_uint nLeftType, nRightType;
	jc_uint nLeftHash, nRightHash, nLeftClass, nRightClass;
	jc_uint nHash, nClass;

	pLeftType = GetOrigType(pLeftType);
	nLeftType = GetTypeCode(pLeftType);
	pRightType = GetOrigType(pRightType);
	nRightType = GetTypeCode(pRightType);

	if(nLeftType == JC_POINTER || nLeftType == JC_ARRAY)
	{
		if(nOp == JC_BINARY_MUL || nOp == JC_BINARY_DIV || nOp == JC_BINARY_MOD)
		{
			SetError(pStack->pParser, 0, "pointer doesn't support '*', '/' or '%' mathematic arithmetic");
			return NULL;
		}
		if(nOp == JC_BINARY_ADD && !IsIntegralType(nRightType))
		{
			SetError(pStack->pParser, 0, "pointer only can '+' an integral number");
			return NULL;
		}
		if(nOp == JC_BINARY_SUB && !IsIntegralType(nRightType) && nRightType != JC_POINTER && nRightType != JC_ARRAY)
		{
			SetError(pStack->pParser, 0, "pointer only can '-' an integral number or pointer");
			return NULL;
		}
		if(nRightType == JC_POINTER || nRightType == JC_ARRAY)
		{
			CJcString oLeftName = GetTypeName(pLeftType, True);
			CJcString oRightName = GetTypeName(pRightType, True);
			if(StringCompare(oLeftName.pStr, oRightName.pStr))
			{
				SetError(pStack->pParser, 0, "'%s' can't subtract '%s'", oLeftName.pStr, oRightName.pStr);
				ClearString(&oLeftName);
				ClearString(&oRightName);
				return NULL;
			}
			if(sizeof(jc_void*)==sizeof(jc_int))
				return CreateType(JC_NONE, JC_UNSIGNED_INT, NULL, NULL, NULL);
			else
				return CreateType(JC_NONE, JC_UNSIGNED_LONG, NULL, NULL, NULL);
		}
		if(nLeftType == JC_POINTER)
			return CloneType(pLeftType);
		return CreateType(JC_NONE, JC_POINTER, NULL, CloneType(pLeftType->pNext), NULL);//CloneArrayInfo(pLeftType->oInfo.pArrayInfo));
	}
	if(nLeftType == JC_FUNCTION)
	{
		SetError(pStack->pParser, 0, "function type don't support the mathematic arithmetic");
		return NULL;
	}

	if(nRightType == JC_POINTER || nRightType == JC_ARRAY || nRightType == JC_FUNCTION)
	{
		SetError(pStack->pParser, 0, "pointer or function can't be as the right operand");
		return NULL;
	}

	if(nLeftType == JC_STRUCT || nLeftType == JC_UNION || nRightType == JC_STRUCT || nRightType == JC_UNION)
	{
		SetError(pStack->pParser, 0, "struct or union don't support mathematic arithmetic");
		return NULL;
	}

	nLeftHash = GetCastClass(nLeftType, &nLeftClass);
	nRightHash = GetCastClass(nRightType, &nRightClass);
	nHash = nLeftHash;
	nClass = nLeftClass;
	if(nLeftHash > nRightHash)
		nHash = nRightHash;
	if(nLeftClass < nRightClass)
		nClass = nRightClass;
	return CreateType(JC_NONE, GetCastNode(nHash, nClass)->nType, NULL, NULL, NULL);
}

static jc_ushort GetAssignInstruction(jc_uint nType, jc_uint nAssignOperator)
{
	jc_ushort nIns;
	switch(nAssignOperator)
	{
	case JC_ASSIGN_ADD_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_addc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_adds;	break;
		case JC_SIGNED_INT:			nIns = jc_addi;	break;
		case JC_SIGNED_LONG:		nIns = jc_addl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_addc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_adds;	break;
		case JC_UNSIGNED_INT:		nIns = jc_addi;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_addl;	break;
		case JC_FLOAT:				nIns = jc_addf;	break;
		case JC_DOUBLE:				nIns = jc_addd;	break;
		case JC_POINTER:			nIns = (IsBit32()?jc_addi:jc_addl);	break;
		}
		break;
	case JC_ASSIGN_SUB_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_subc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_subs;	break;
		case JC_SIGNED_INT:			nIns = jc_subi;	break;
		case JC_SIGNED_LONG:		nIns = jc_subl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_subc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_subs;	break;
		case JC_UNSIGNED_INT:		nIns = jc_subi;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_subl;	break;
		case JC_FLOAT:				nIns = jc_subf;	break;
		case JC_DOUBLE:				nIns = jc_subd;	break;
		case JC_POINTER:			nIns = (IsBit32()?jc_subi:jc_subl);	break;
		}
		break;
	case JC_ASSIGN_MUL_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_mulc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_muls;	break;
		case JC_SIGNED_INT:			nIns = jc_muli;	break;
		case JC_SIGNED_LONG:		nIns = jc_mull;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_muluc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_mulus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_mului;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_mulul;	break;
		case JC_FLOAT:				nIns = jc_mulf;	break;
		case JC_DOUBLE:				nIns = jc_muld;	break;
		}
		break;
	case JC_ASSIGN_DIV_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_divc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_divs;	break;
		case JC_SIGNED_INT:			nIns = jc_divi;	break;
		case JC_SIGNED_LONG:		nIns = jc_divl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_divuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_divus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_divui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_divul;	break;
		case JC_FLOAT:				nIns = jc_divf;	break;
		case JC_DOUBLE:				nIns = jc_divd;	break;
		}
		break;
	case JC_ASSIGN_MOD_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_modc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_mods;	break;
		case JC_SIGNED_INT:			nIns = jc_modi;	break;
		case JC_SIGNED_LONG:		nIns = jc_modl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_moduc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_modus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_modui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_modul;	break;
		}
		break;
	case JC_ASSIGN_LSH_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_lshuc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_lshus;	break;
		case JC_SIGNED_INT:			nIns = jc_lshui;	break;
		case JC_SIGNED_LONG:		nIns = jc_lshul;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_lshuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_lshus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_lshui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_lshul;	break;
		}
		break;
	case JC_ASSIGN_RSH_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_rshc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_rshs;	break;
		case JC_SIGNED_INT:			nIns = jc_rshi;	break;
		case JC_SIGNED_LONG:		nIns = jc_rshl;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_rshuc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_rshus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_rshui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_rshul;	break;
		}
		break;
	case JC_ASSIGN_AND_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_anduc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_andus;	break;
		case JC_SIGNED_INT:			nIns = jc_andui;	break;
		case JC_SIGNED_LONG:		nIns = jc_andul;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_anduc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_andus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_andui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_andul;	break;
		case JC_ENUM:				nIns = jc_andui;	break;
		}
		break;
	case JC_ASSIGN_OR_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_oruc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_orus;	break;
		case JC_SIGNED_INT:			nIns = jc_orui;	break;
		case JC_SIGNED_LONG:		nIns = jc_orul;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_oruc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_orus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_orui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_orul;	break;
		case JC_ENUM:				nIns = jc_orui;	break;
		}
		break;
	case JC_ASSIGN_XOR_EQ:
		switch(nType)
		{
		case JC_SIGNED_CHAR:		nIns = jc_xoruc;	break;
		case JC_SIGNED_SHORT:		nIns = jc_xorus;	break;
		case JC_SIGNED_INT:			nIns = jc_xorui;	break;
		case JC_SIGNED_LONG:		nIns = jc_xorul;	break;
		case JC_UNSIGNED_CHAR:		nIns = jc_xoruc;	break;
		case JC_UNSIGNED_SHORT:		nIns = jc_xorus;	break;
		case JC_UNSIGNED_INT:		nIns = jc_xorui;	break;
		case JC_UNSIGNED_LONG:		nIns = jc_xorul;	break;
		case JC_ENUM:				nIns = jc_xorui;	break;
		}
		break;
	}
	return nIns;
}

static const char* GetAssignOperatorString(jc_uint nAssignOperator)
{
	const char* sRet = NULL;
	switch(nAssignOperator)
	{
	case JC_ASSIGN_EQUAL:	sRet = "=";		break;
	case JC_ASSIGN_ADD_EQ:	sRet = "+=";	break;
	case JC_ASSIGN_SUB_EQ:	sRet = "-=";	break;
	case JC_ASSIGN_MUL_EQ:	sRet = "*=";	break;
	case JC_ASSIGN_DIV_EQ:	sRet = "/=";	break;
	case JC_ASSIGN_MOD_EQ:	sRet = "%=";	break;
	case JC_ASSIGN_LSH_EQ:	sRet = "<<=";	break;
	case JC_ASSIGN_RSH_EQ:	sRet = ">>=";	break;
	case JC_ASSIGN_AND_EQ:	sRet = "&=";	break;
	case JC_ASSIGN_OR_EQ:	sRet = "|=";	break;
	case JC_ASSIGN_XOR_EQ:	sRet = "^=";	break;
	}
	return sRet;
}

static jc_uint CheckAutoAssignCast(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_bool bConstNull)
{
	jc_uint nLeftType, nRightType;
	jc_uint nLeftHash, nRightHash, nLeftClass, nRightClass;
	CJcString oStr1, oStr2, oStr3, oStr4;

	oStr1 = GetTypeName(pLeftType, False);
	oStr2 = GetTypeName(pRightType, False);

	pLeftType = GetOrigType(pLeftType);
	nLeftType = GetTypeCode(pLeftType);
	pRightType = GetOrigType(pRightType);
	nRightType = GetTypeCode(pRightType);

	if(nLeftType == JC_POINTER)
	{
		if(nRightType == JC_ARRAY || nRightType == JC_POINTER)
		{
/*
			pLeftType = GetOrigType(pLeftType->pNext);
			pRightType = GetOrigType(pRightType->pNext);
			oStr3 = GetTypeName(pLeftType, True);
			oStr4 = GetTypeName(pRightType, True);
			if(StringCompare(oStr3.pStr, oStr4.pStr))
			{
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				ClearString(&oStr3);
				ClearString(&oStr4);
				return 1;
			}
			ClearString(&oStr3);
			ClearString(&oStr4);
*/
		}
		else if(nRightType == JC_FUNCTION)
		{
			pLeftType = GetOrigType(pLeftType->pNext);
			oStr3 = GetTypeName(pLeftType, True);
			if(StringCompare(oStr2.pStr, oStr3.pStr))
			{
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				ClearString(&oStr3);
				return 1;
			}
			ClearString(&oStr3);
		}
		else if(StringCompare(oStr1.pStr, oStr2.pStr))
		{
			if(!bConstNull)
			{
				SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return 1;
			}
		}
		if(nRightType != JC_POINTER && nRightType != JC_ARRAY && nRightType != JC_FUNCTION)
		{
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 0;
	}

	if(nLeftType == JC_STRUCT || nLeftType == JC_UNION || nLeftType == JC_ENUM)
	{
		if(nRightType != nLeftType)
		{
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		if(pLeftType->oName.pStr && pRightType->oName.pStr && 
			StringCompare(pLeftType->oName.pStr, pRightType->oName.pStr))
		{
			SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 0;
	}

/* 需要改进，当右端是字面常量，可以根据其数值是否真越界，若没有越界，是可以的。
	nLeftHash = GetCastClass(nLeftType, &nLeftClass);
	nRightHash = GetCastClass(nRightType, &nRightClass);
	if(nRightClass > nLeftClass)
	{
		SetError(pStack->pParser, 0, "can not assign '%s' to '%s'", oStr2.pStr, oStr1.pStr);
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 1;
	}
*/
	ClearString(&oStr1);
	ClearString(&oStr2);
	return 0;
}

static jc_uint CheckAutoArgumentCast(CJcSymbolStack* pStack, CJcTypeInfo* pLeftType, CJcTypeInfo* pRightType, jc_uint nArgIdx, jc_bool bConstNull)
{
	jc_uint nLeftType, nRightType;
	jc_uint nLeftHash, nRightHash, nLeftClass, nRightClass;
	CJcString oStr1, oStr2, oStr3, oStr4;

	oStr1 = GetTypeName(pLeftType, False);
	oStr2 = GetTypeName(pRightType, False);

	pLeftType = GetOrigType(pLeftType);
	nLeftType = GetTypeCode(pLeftType);
	pRightType = GetOrigType(pRightType);
	nRightType = GetTypeCode(pRightType);

	if(nLeftType == JC_POINTER)
	{
		if(nRightType == JC_ARRAY)
		{
			pLeftType = GetOrigType(pLeftType->pNext);
			pRightType = GetOrigType(pRightType->pNext);
			oStr3 = GetTypeName(pLeftType, True);
			oStr4 = GetTypeName(pRightType, True);
			if(StringCompare(oStr3.pStr, oStr4.pStr))
			{
				SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				ClearString(&oStr3);
				ClearString(&oStr4);
				return 1;
			}
			ClearString(&oStr3);
			ClearString(&oStr4);
		}
		else if(nRightType == JC_FUNCTION)
		{
			pLeftType = GetOrigType(pLeftType->pNext);
			oStr3 = GetTypeName(pLeftType, True);
			if(StringCompare(oStr2.pStr, oStr3.pStr))
			{
				SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				ClearString(&oStr3);
				return 1;
			}
			ClearString(&oStr3);
		}
		else if(StringCompare(oStr1.pStr, oStr2.pStr))
		{
			if(!bConstNull)
			{
				SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
				ClearString(&oStr1);
				ClearString(&oStr2);
				return 1;
			}
		}
		if(nRightType != JC_POINTER && nRightType != JC_ARRAY && nRightType != JC_FUNCTION)
		{
			SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 0;
	}

	if(nLeftType == JC_STRUCT || nLeftType == JC_UNION || nLeftType == JC_ENUM)
	{
		if(nRightType != nLeftType)
		{
			SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		if(pLeftType->oName.pStr && pRightType->oName.pStr && 
			StringCompare(pLeftType->oName.pStr, pRightType->oName.pStr))
		{
			SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
			ClearString(&oStr1);
			ClearString(&oStr2);
			return 1;
		}
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 0;
	}

	nLeftHash = GetCastClass(nLeftType, &nLeftClass);
	nRightHash = GetCastClass(nRightType, &nRightClass);
	if(nRightClass > nLeftClass)
	{
		SetError(pStack->pParser, 0, "cann't convert argument %u from '%s' to '%s'", nArgIdx, oStr2.pStr, oStr1.pStr);
		ClearString(&oStr1);
		ClearString(&oStr2);
		return 1;
	}

	ClearString(&oStr1);
	ClearString(&oStr2);
	return 0;
}
