
#include "jc_symbol.h"
#include "jc_enum.h"

void SetError(CJcParser* pParser, int bWarning, const char* msg, ...);

static jc_uint JC_CALL HashFunction(jc_void* k)
{
	return GetCrc32Hash(k, StringLength((const jc_char*)k));
}

static jc_int JC_CALL HashKeyEqual(jc_void* k1, jc_void* k2)
{
	const jc_char* key1 = (const jc_char*)k1;
	const jc_char* key2 = (const jc_char*)k2;
	return !StringCompare(key1, key2);
}

static jc_void JC_CALL FreeSymbolItem(void* p)
{
	ClearSymbol((CJcSymbol*)p);
	g_oInterface.Free(p);
}

jc_void InitializeProtoTypeTable(CJcProtoTypeTable* pTable)
{
	pTable->nCount = 0;
	pTable->pProtoTypeTable = NULL;
}

jc_void ClearProtoTypeTable(CJcProtoTypeTable* pTable)
{
	jc_uint i, nCount;
	CJcProtoType* pProto;
	nCount = pTable->nCount;
	if(nCount)
	{
		pProto = pTable->pProtoTypeTable;
		for(i=0; i<nCount; ++i)
		{
			ClearString(&pProto->oName);
			ClearSymbol(&pProto->oSymbol);
			pProto++;
		}
		g_oInterface.Free(pTable->pProtoTypeTable);
		pTable->nCount = 0;
		pTable->pProtoTypeTable = NULL;
	}
}

jc_void ClearSymbol(CJcSymbol* pSymbol)
{
	switch(pSymbol->nSymbol)
	{
	case JC_ENUM_SYMBOL:
	case JC_TYPE_SYMBOL:
		if(pSymbol->info.pType)
		{
			DestroyType(pSymbol->info.pType);
			pSymbol->info.pType = NULL;
		}
		break;
	case JC_FUNCTION_SYMBOL:
		if(pSymbol->info.pFunction)
		{
			DestroyFunctionInfo(pSymbol->info.pFunction);
			pSymbol->info.pFunction = NULL;
		}
		break;
	case JC_VARIABLE_SYMBOL:
		if(pSymbol->info.pVariable)
		{
			ClearVariableInfo(pSymbol->info.pVariable);
			g_oInterface.Free(pSymbol->info.pVariable);
			pSymbol->info.pVariable = NULL;
		}
		break;
	}
}

jc_void DestroySymbol(CJcSymbol* pSymbol)
{
	ClearSymbol(pSymbol);
	g_oInterface.Free(pSymbol);
}

jc_void InitializeSymbolStack(CJcSymbolStack*pStack)
{
	MemorySet(pStack, 0, sizeof(CJcSymbolStack));
	InitializeSegment(&pStack->oCodeSegment, '\0');
	InitializeSegment(&pStack->oConstSegment, '\0');
	InitializeConstTable(&pStack->oConstTable);
	InitializeLocalStack(&pStack->oGlobalStack);
	InitializeLocalStack(&pStack->oLocalStack);
	InitializeLocalStack(&pStack->oDataSegment);
	InitializeProtoTypeTable(&pStack->oProtoTypeTable);
}

jc_void ClearSymbolStack(CJcSymbolStack*pStack)
{
	while(pStack->pSymbolTable)
		PopSymbolTable(pStack);
	ClearProtoTypeTable(&pStack->oProtoTypeTable);
	ClearSegment(&pStack->oCodeSegment);
	ClearSegment(&pStack->oConstSegment);
	ClearConstTable(&pStack->oConstTable);
	ClearLocalStack(&pStack->oLocalStack);
	ClearLocalStack(&pStack->oGlobalStack);
	ClearLocalStack(&pStack->oDataSegment);
}

CJcSymbolStack* CreateSymbolStack()
{
	CJcSymbolStack * pStack = New(CJcSymbolStack);
	InitializeSymbolStack(pStack);
	return pStack;
}

jc_void DestroySymbolStack(CJcSymbolStack* pStack)
{
	ClearSymbolStack(pStack);
	g_oInterface.Free(pStack);
}

jc_void NewSymbolTable(CJcSymbolStack* pStack)
{
	CJcSymbolTable* pTable = New(CJcSymbolTable);
	pTable->pPrev = pStack->pSymbolTable;
	pTable->pFunction = NULL;
	pTable->pSymbolTable = CreateHashTable(NULL, HashFunction, HashKeyEqual, g_oInterface.Free, FreeSymbolItem);
	pStack->pSymbolTable = pTable;
	++pStack->nLevel;
}

jc_void PopSymbolTable(CJcSymbolStack* pStack)
{
	CJcSymbolTable *pPrev, *pTable = pStack->pSymbolTable;
	if(pTable)
	{
		pPrev = pTable->pPrev;
		if(pTable->pFunction)
			pStack->pInFunction = NULL;
		DestroyHashTable(pTable->pSymbolTable);
		g_oInterface.Free(pTable);
		pStack->pSymbolTable = pPrev;
		--pStack->nLevel;
	}
}

jc_uint GetSymbolTableLevel(CJcSymbolStack* pStack)
{
	return pStack->nLevel;
}

jc_void SetFunction(CJcSymbolStack* pStack, CJcFunctionInfo* pFunction)
{
	if(!pStack->pInFunction && pStack->nLevel==2)
	{
		pStack->pInFunction = pFunction;
		pStack->pSymbolTable->pFunction = pFunction;
	}
}

CJcFunctionInfo* GetFunction(CJcSymbolStack* pStack)
{
	return pStack->pInFunction;
}

jc_uint FindSymbol(CJcSymbolStack* pStack,
				   jc_char* sSymbolName,
				   jc_uint bFindAll,
				   CJcSymbol * pSymbol)
{
	jc_uint nArg;
	jc_uint nRet = 1;
	CJcProtoType* pProto;
	CJcVariableInfo* pVariable;
	CJcFunctionInfo* pFunction;
	CJcSymbolTable* pTable = pStack->pSymbolTable;
	if(pTable)do
	{
		CJcSymbol* pOldSym = (CJcSymbol*)SearchInHashTable(pTable->pSymbolTable, sSymbolName);
		if(pOldSym)
		{
			pSymbol[0] = pOldSym[0];
			if(!(pSymbol->nStorage & JC_HOST))
			{
				if(pSymbol->nSymbol == JC_VARIABLE_SYMBOL)
				{
					pVariable = pSymbol->info.pVariable;
					if(pVariable->nOpt == JC_SS)
					{
						nArg = pVariable->nArg;
						pProto = pStack->oProtoTypeTable.pProtoTypeTable + nArg;
						pSymbol[0] = pProto->oSymbol;
					}
				}
				else if(pSymbol->nSymbol == JC_FUNCTION_SYMBOL)
				{
					pFunction = pSymbol->info.pFunction;
					if(GetFunctionOpt(pFunction) == JC_SS)
					{
						nArg = GetFunctionArg(pFunction);
						pProto = pStack->oProtoTypeTable.pProtoTypeTable + nArg;
						pSymbol[0] = pProto->oSymbol;
					}
				}
			}
			nRet = 0;
		}
		else if(pTable->pFunction)
		{
			CJcVariableInfo* pVar = FindFunctionPara(pTable->pFunction, sSymbolName);
			if(pVar)
			{
				pSymbol->nStorage = JC_NONE;
				pSymbol->nSymbol = JC_VARIABLE_SYMBOL;
				pSymbol->info.pVariable = pVar;
				nRet = 0;
			}
		}
		pTable = pTable->pPrev;
	}while(nRet && bFindAll && pTable);
	return nRet;
}

CJcSymbol* CreateEnumSymbol(CJcSymbolStack* pStack,
							jc_char* sEnumName,
							CJcTypeInfo *pType,
							jc_int nVal)
{
	jc_char* sDupName;
	CJcSymbol* pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sEnumName);
	if(pSymbol)
	{
		SetError(pStack->pParser, 0, "redeclare symbol '%s'", sEnumName);
		return NULL;
	}
	pSymbol = New(CJcSymbol);
	sDupName = (jc_char*)g_oInterface.Malloc(StringLength(sEnumName)+1);
	StringCopy(sDupName, sEnumName);
	pSymbol->nStorage = JC_NONE;
	pSymbol->nSymbol = JC_ENUM_SYMBOL;
	pSymbol->info.pType = CloneType(pType);
	GetEnumInfo(pSymbol->info.pType)->nNextValue = PutConst(&pStack->oConstSegment, &pStack->oConstTable, &nVal, 4, 4);
	InsertIntoHashTable(pStack->pSymbolTable->pSymbolTable, sDupName, pSymbol);
	return pSymbol;
}

CJcSymbol* CreateTypeSymbol(CJcSymbolStack* pStack,
							jc_char* sTypeName,
							CJcTypeInfo *pType)
{
	jc_char* sDupName;
	CJcSymbol* pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sTypeName);
	if(pSymbol)
	{
		SetError(pStack->pParser, 0, "redeclare symbol '%s'", sTypeName);
		return NULL;
	}
	pSymbol = New(CJcSymbol);
	sDupName = (jc_char*)g_oInterface.Malloc(StringLength(sTypeName)+1);
	StringCopy(sDupName, sTypeName);
	pSymbol->nStorage = JC_NONE;
	pSymbol->nSymbol = JC_TYPE_SYMBOL;
	pSymbol->info.pType = pType;
	InsertIntoHashTable(pStack->pSymbolTable->pSymbolTable, sDupName, pSymbol);
	return pSymbol;
}

CJcSymbol* CreateFunctionSymbol(CJcSymbolStack* pStack,
								jc_uint nStorage,
								jc_char* sFunctionName,
								CJcFunctionInfo* pFunction)
{
	jc_char *sDupName;
	CJcSymbol* pSymbol;
	CJcVariableInfo* pVar, *pOldVar;
	jc_uint i, nParaCount, nIdx;
	CJcTypeInfo* pType, *pOldType;
	CJcFunctionInfo* pOldFunction;
	CJcProtoType* pProto;

	pType = GetFunctionType(pFunction);
	nStorage &= (~JC_EXTERN);
	pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sFunctionName);
	if(pSymbol)
	{
		if(pSymbol->nSymbol != JC_FUNCTION_SYMBOL)
		{
			SetError(pStack->pParser, 0, "redeclare symbol '%s'", sFunctionName);
			return NULL;
		}
		pOldFunction = pSymbol->info.pFunction;
		pOldType = GetFunctionType(pOldFunction);
		if(nStorage != pSymbol->nStorage)
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		if(StringCompare(GetTypeName(pType, True).pStr, GetTypeName(pOldType, True).pStr))
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		nParaCount = GetFunctionParaCount(pFunction);
		if(nParaCount != GetFunctionParaCount(pOldFunction))
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		for(i=0; i<nParaCount; ++i)
		{
			pVar = GetFunctionPara(pFunction, i);
			pOldVar = GetFunctionPara(pOldFunction, i);
			if(StringCompare(GetTypeName(pVar->pType, True).pStr, GetTypeName(pOldVar->pType, True).pStr))
			{
				SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
				return NULL;
			}
		}
		CoverFunctionInfo(pOldFunction, pFunction);
		DestroyFunctionInfo(pFunction);
		return pSymbol;
	}
	pProto = FindProtoType(pStack, sFunctionName, &nIdx);
	if(pProto)
	{
		if(pProto->oSymbol.nSymbol != JC_FUNCTION_SYMBOL)
		{
			SetError(pStack->pParser, 0, "redeclare symbol '%s'", sFunctionName);
			return NULL;
		}
		if(nStorage != pProto->oSymbol.nStorage)
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		pOldFunction = pProto->oSymbol.info.pFunction;
		pOldType = GetFunctionType(pOldFunction);
		if(StringCompare(GetTypeName(pType, True).pStr, GetTypeName(pOldType, True).pStr))
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		nParaCount = GetFunctionParaCount(pFunction);
		if(nParaCount != GetFunctionParaCount(pOldFunction))
		{
			SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
			return NULL;
		}
		for(i=0; i<nParaCount; ++i)
		{
			pVar = GetFunctionPara(pFunction, i);
			pOldVar = GetFunctionPara(pOldFunction, i);
			if(StringCompare(GetTypeName(pVar->pType, True).pStr, GetTypeName(pOldVar->pType, True).pStr))
			{
				SetError(pStack->pParser, 0, "redeclare function symbol '%s'", sFunctionName);
				return NULL;
			}
		}
	}
	else
	{
		nIdx = pStack->oProtoTypeTable.nCount;
		pStack->oProtoTypeTable.pProtoTypeTable =
			g_oInterface.Realloc(pStack->oProtoTypeTable.pProtoTypeTable, (nIdx+1)*sizeof(CJcProtoType));
		pProto = pStack->oProtoTypeTable.pProtoTypeTable + nIdx;
		InitializeString(&pProto->oName);
		CoverString(&pProto->oName, sFunctionName);
		pProto->oSymbol.nStorage = nStorage;
		pProto->oSymbol.nSymbol = JC_FUNCTION_SYMBOL;
		pProto->oSymbol.info.pFunction = CloneFunctionInfo(pFunction);
		if(nStorage & JC_HOST)
			FinishFunctionDefine(pProto->oSymbol.info.pFunction, JC_HS, 0);
		pProto->nUsed = 0;
		++pStack->oProtoTypeTable.nCount;
	}
	pSymbol = New(CJcSymbol);
	pSymbol->nStorage = nStorage;
	pSymbol->nSymbol = JC_FUNCTION_SYMBOL;
	pSymbol->info.pFunction = pFunction;
	FinishFunctionDefine(pProto->oSymbol.info.pFunction, JC_SS, nIdx);
	sDupName = (jc_char*)g_oInterface.Malloc(StringLength(sFunctionName)+1);
	StringCopy(sDupName, sFunctionName);
	InsertIntoHashTable(pStack->pSymbolTable->pSymbolTable, sDupName, pSymbol);
	return pSymbol;
}

CJcSymbol* CreateVariableSymbol(CJcSymbolStack* pStack,
								jc_uint nStorage,
								jc_char* sVariableName,
								CJcTypeInfo *pType)
{
	jc_char* sDupName;
	CJcSymbol* pSymbol;
	CJcProtoType* pProto;
	CJcTypeInfo* pOldType;
	CJcVariableInfo* pOldVar;
	jc_uint nQual, nOldStorage, nIdx, nOpt, nArg, nVarId;

	if(GetTypeCode(GetOrigType(pType)) == JC_VOID)
	{
		SetError(pStack->pParser, 0, "cann't declare 'void' variable symbol '%s'", sVariableName);
		return NULL;
	}
	nQual = GetQualCode(pType);
	if(nStorage & JC_EXTERN)
	{
		pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sVariableName);
		if(pSymbol)
		{
			if(pSymbol->nSymbol != JC_VARIABLE_SYMBOL)
			{
				SetError(pStack->pParser, 0, "redeclare symbol '%s'", sVariableName);
				return NULL;
			}
			pOldVar = pSymbol->info.pVariable;
			nOldStorage = pSymbol->nStorage;
			if(!(nOldStorage & JC_EXTERN))
			{
				if(GetSymbolTableLevel(pStack) > 1)
				{
					SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
					return NULL;
				}
			}
			if( (nOldStorage&(~JC_EXTERN)) != (nStorage&(~JC_EXTERN)) )
			{
				SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
				return NULL;
			}
			if(StringCompare(GetTypeName(pType, True).pStr, GetTypeName(pOldVar->pType, True).pStr))
			{
				SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
				return NULL;
			}
			DestroyType(pType);
			return pSymbol;
		}
		pProto = FindProtoType(pStack, sVariableName, &nIdx);
		if(pProto)
		{
			if(pProto->oSymbol.nSymbol != JC_VARIABLE_SYMBOL)
			{
				SetError(pStack->pParser, 0, "redeclare symbol '%s'", sVariableName);
				return NULL;
			}
			pOldVar = pProto->oSymbol.info.pVariable;
			pOldType = pOldVar->pType;
			nOldStorage = pProto->oSymbol.nStorage;
			if(nOldStorage != (nStorage&(~JC_EXTERN)))
			{
				SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
				return NULL;
			}
			if(StringCompare(GetTypeName(pType, True).pStr, GetTypeName(pOldVar->pType, True).pStr))
			{
				SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
				return NULL;
			}
		}
		else
		{
			nIdx = pStack->oProtoTypeTable.nCount;
			pStack->oProtoTypeTable.pProtoTypeTable =
				g_oInterface.Realloc(pStack->oProtoTypeTable.pProtoTypeTable, (nIdx+1)*sizeof(CJcProtoType));
			pProto = pStack->oProtoTypeTable.pProtoTypeTable + nIdx;
			InitializeString(&pProto->oName);
			CoverString(&pProto->oName, sVariableName);
			pProto->oSymbol.nStorage = nStorage & (~JC_EXTERN);
			pProto->oSymbol.nSymbol = JC_VARIABLE_SYMBOL;
			pProto->oSymbol.info.pVariable = CreateVariableInfo(sVariableName, CloneType(pType));
			pProto->oSymbol.info.pVariable->nArg = 0;
			pProto->oSymbol.info.pVariable->nOpt = JC_UN;
			pProto->oSymbol.info.pVariable->nVarId = 0;
			if(nStorage & JC_HOST)
			{
				pProto->oSymbol.info.pVariable->nOpt = JC_HS;
				pProto->oSymbol.info.pVariable->nArg = 0;
			}
			pProto->nUsed = 0;
			++pStack->oProtoTypeTable.nCount;
		}
		pSymbol = New(CJcSymbol);
		pSymbol->nStorage = nStorage;
		pSymbol->nSymbol = JC_VARIABLE_SYMBOL;
		pSymbol->info.pVariable = CreateVariableInfo(sVariableName, pType);
		pSymbol->info.pVariable->nArg = nIdx;
		pSymbol->info.pVariable->nOpt = JC_SS;
		pSymbol->info.pVariable->nVarId = 0;
		sDupName = (jc_char*)g_oInterface.Malloc(StringLength(sVariableName)+1);
		StringCopy(sDupName, sVariableName);
		InsertIntoHashTable(pStack->pSymbolTable->pSymbolTable, sDupName, pSymbol);
		return pSymbol;
	}

	nOpt=0, nArg = 0, nVarId = 0;
	pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sVariableName);
	if(pSymbol)
	{
		if(pSymbol->nSymbol != JC_VARIABLE_SYMBOL)
		{
			SetError(pStack->pParser, 0, "redeclare symbol '%s'", sVariableName);
			return NULL;
		}
		pOldVar = pSymbol->info.pVariable;
		nOldStorage = pSymbol->nStorage;
		if(!(nOldStorage & JC_EXTERN))
		{
			SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
			return NULL;
		}
		if( (nOldStorage&(~JC_EXTERN)) != (nStorage&(~JC_EXTERN)) )
		{
			SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
			return NULL;
		}
		if(StringCompare(GetTypeName(pType, True).pStr, GetTypeName(pOldVar->pType, True).pStr))
		{
			SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
			return NULL;
		}
		if(GetSymbolTableLevel(pStack) > 1)
		{
			SetError(pStack->pParser, 0, "redeclare variable symbol '%s'", sVariableName);
			return NULL;
		}
	}
	if((nStorage & JC_STATIC) || GetSymbolTableLevel(pStack)==1)
	{
		nArg = AllocVarInStack(&pStack->oDataSegment, GetTypeSize(pType), GetTypeAlign(pType), &nVarId);
		nOpt = JC_DS;
		nVarId = 0;
	}
	else
	{
		if(pStack->pInFunction)
			nArg = AllocVarInStack(&pStack->oLocalStack, GetTypeSize(pType), GetTypeAlign(pType), &nVarId);
		else
			nArg = AllocVarInStack(&pStack->oGlobalStack, GetTypeSize(pType), GetTypeAlign(pType), &nVarId);
		nOpt = JC_LS;
	}
	pProto = NULL;
	if(GetSymbolTableLevel(pStack) == 1 && (nStorage == JC_NONE || nStorage == JC_SHARE) )
	{
		pProto = FindProtoType(pStack, sVariableName, &nIdx);
		if(pProto)
		{
			pProto->oSymbol.info.pVariable->nArg = nArg;
			pProto->oSymbol.info.pVariable->nOpt = nOpt;
			pProto->oSymbol.info.pVariable->nVarId = nVarId;
		}
		else
		{
			nIdx = pStack->oProtoTypeTable.nCount;
			pStack->oProtoTypeTable.pProtoTypeTable =
				g_oInterface.Realloc(pStack->oProtoTypeTable.pProtoTypeTable, (nIdx+1)*sizeof(CJcProtoType));
			pProto = pStack->oProtoTypeTable.pProtoTypeTable + nIdx;
			InitializeString(&pProto->oName);
			CoverString(&pProto->oName, sVariableName);
			pProto->oSymbol.nStorage = nStorage;
			pProto->oSymbol.nSymbol = JC_VARIABLE_SYMBOL;
			pProto->oSymbol.info.pVariable = CreateVariableInfo(sVariableName, CloneType(pType));
			pProto->oSymbol.info.pVariable->nArg = nArg;
			pProto->oSymbol.info.pVariable->nOpt = nOpt;
			pProto->oSymbol.info.pVariable->nVarId = nVarId;
			++pStack->oProtoTypeTable.nCount;
		}
		pProto->nUsed = 1;
	}
	if(pSymbol)
		DestroyType(pType);
	else
	{
		pSymbol = New(CJcSymbol);
		pSymbol->nStorage = nStorage;
		pSymbol->nSymbol = JC_VARIABLE_SYMBOL;
		pSymbol->info.pVariable = CreateVariableInfo(sVariableName, pType);
		if(pProto)
		{
			pSymbol->info.pVariable->nArg = nIdx;
			pSymbol->info.pVariable->nOpt = JC_SS;
			pSymbol->info.pVariable->nVarId = 0;
		}
		else
		{
			pSymbol->info.pVariable->nArg = nArg;
			pSymbol->info.pVariable->nOpt = nOpt;
			pSymbol->info.pVariable->nVarId = nVarId;
		}
		sDupName = (jc_char*)g_oInterface.Malloc(StringLength(sVariableName)+1);
		StringCopy(sDupName, sVariableName);
		InsertIntoHashTable(pStack->pSymbolTable->pSymbolTable, sDupName, pSymbol);
	}
	return pSymbol;
}

CJcSymbol* CreateSymbol(CJcSymbolStack* pStack,
						jc_uint nStorage,
						jc_char* sName,
						CJcTypeInfo *pType)
{
	CJcSymbol* pSymbol;
	CJcFunctionInfo* pFunction;

	if(nStorage == JC_TYPEDEF)
	{
		pSymbol = (CJcSymbol*)SearchInHashTable(pStack->pSymbolTable->pSymbolTable, sName);
		if(pSymbol)
		{
			if(pSymbol->nSymbol != JC_TYPE_SYMBOL)
			{
				SetError(pStack->pParser, 0, "redeclare symbol '%s'", sName);
				return NULL;
			}
			if(StringCompare(GetTypeName(pSymbol->info.pType, True).pStr, GetTypeName(pType, True).pStr))
			{
				SetError(pStack->pParser, 0, "redeclare type symbol '%s'", sName);
				return NULL;
			}
			CoverType(pSymbol->info.pType, pType);
			DestroyType(pType);
		}
		else
		{
			CoverString(&pType->oName, sName);
			pSymbol = CreateTypeSymbol(pStack, sName, pType);
		}
	}
	else if(GetTypeCode(pType) == JC_FUNCTION)
	{
		pFunction = CloneFunctionInfo(GetFunctionInfo(pType));
		pSymbol = CreateFunctionSymbol(pStack, nStorage, sName, pFunction);
		if(!pSymbol)
			DestroyFunctionInfo(pFunction);
		else
			DestroyType(pType);
	}
	else
		pSymbol = CreateVariableSymbol(pStack, nStorage, sName, pType);
	return pSymbol;
}

jc_void FunctionDefine(CJcSymbolStack* pStack,
					   CJcSymbol* pSymbol,
					   jc_uint nFuncAddr)
{
	jc_uint nArg = GetFunctionArg(pSymbol->info.pFunction);
	CJcProtoType* pProto = GetProtoType(pStack, nArg);
	FinishFunctionDefine(pProto->oSymbol.info.pFunction, JC_CS, nFuncAddr);
}

jc_uint GetProtoTypeCount(CJcSymbolStack* pStack)
{
	return pStack->oProtoTypeTable.nCount;
}

CJcProtoType* GetProtoType(CJcSymbolStack* pStack, jc_uint nIdx)
{
	if(nIdx >= pStack->oProtoTypeTable.nCount)
		return NULL;
	return pStack->oProtoTypeTable.pProtoTypeTable + nIdx;
}

CJcProtoType* FindProtoType(CJcSymbolStack* pStack, jc_char* sSymbolName, jc_uint* pIdx)
{
	jc_uint i;
	jc_uint nCount = pStack->oProtoTypeTable.nCount;
	CJcProtoType* pHead = pStack->oProtoTypeTable.pProtoTypeTable;
	for(i=0; i<nCount; ++i)
	{
		if(!StringCompare(pHead->oName.pStr, sSymbolName))
			break;
		++pHead;
	}
	if(i < nCount)
		return pHead;
	return NULL;
}

jc_bool IsExpressSymbol(CJcSymbolStack* pStack, jc_bool IsId, jc_char* pVal, CJcSymbol* pSymbol)
{
	if(!IsId)
	{
		SetError(pStack->pParser, 0, "expect a symbol");
		return False;
	}
	if(FindSymbol(pStack, pVal, 1, pSymbol))
	{
		SetError(pStack->pParser, 0, "symbol '%s' is undefined", pVal);
		return False;
	}
	if(pSymbol->nSymbol == JC_TYPE_SYMBOL)
	{
		SetError(pStack->pParser, 0, "don't expect the type symbol '%s'", pVal);
		return False;
	}
	return True;
}

jc_void PrintSymbol(CJcSymbol* pSymbol, jc_char* sName)
{
	switch(pSymbol->nSymbol)
	{
	case JC_ENUM_SYMBOL:
		g_oInterface.PrintError(True, "enum %s(%p):%s(%p)", sName?sName:"NULL", sName, pSymbol->info.pType->oName.pStr?pSymbol->info.pType->oName.pStr:"NULL", pSymbol->info.pType->oName.pStr);
		break;
	case JC_TYPE_SYMBOL:
		g_oInterface.PrintError(True, "type %s(%p):%s(%p)", sName?sName:"NULL", sName, pSymbol->info.pType->oName.pStr?pSymbol->info.pType->oName.pStr:"NULL", pSymbol->info.pType->oName.pStr);
		break;
	case JC_FUNCTION_SYMBOL:
		g_oInterface.PrintError(True, "function %s", sName?sName:"NULL");
		break;
	case JC_VARIABLE_SYMBOL:
		g_oInterface.PrintError(True, "variable %s(%p):%s(%p)", sName?sName:"NULL", sName, pSymbol->info.pVariable->oName.pStr?pSymbol->info.pVariable->oName.pStr:"NULL", pSymbol->info.pVariable->oName.pStr);
		break;
	}
}

jc_void PrintSymbolTable(CJcSymbolStack* pStack)
{
	CJcSymbolTable *pTable = pStack->pSymbolTable;
	CJcHashEntry* pFirst = BeginOfHashTable(pStack->pSymbolTable->pSymbolTable);
	while(pFirst)
	{
		PrintSymbol((CJcSymbol*)pFirst->pVal, (jc_char*)pFirst->pKey);
		pFirst = NextOfHashTable(pStack->pSymbolTable->pSymbolTable, pFirst);
	}
}

/*
#include "jc_parser.h"

*/
