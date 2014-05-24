
#include "jc_obj.h"
#include "jc_seg.h"
#include "jc_symbol.h"

static CJcSymbolItem* RebuildSymbolTable(CJcSymbolStack* pStack, CJcSegment* pSegment, jc_uint* pLen)
{
	jc_uint i, nLen = 0, nTotal = pStack->oProtoTypeTable.nCount;
	CJcSymbolItem* pTab = NULL;

	if(nTotal)
	{
		CJcProtoType* pProto = pStack->oProtoTypeTable.pProtoTypeTable;
		pTab = New2(CJcSymbolItem, nTotal);
		for(i=0; i<nTotal; ++i,++pProto)
		{
			CJcSymbolItem* pSym = pTab + nLen;
			pProto->nNewIdx = (jc_uint)(-1);
			if(pProto->oSymbol.nStorage == JC_STATIC)
				continue;
			if(!pProto->nUsed)
			{
				if(pProto->oSymbol.nSymbol == JC_FUNCTION_SYMBOL)
				{
					if(pProto->oSymbol.info.pFunction->nOpt != JC_CS)
						continue;
				}
				else
				{
					if(pProto->oSymbol.info.pVariable->nOpt != JC_DS)
						continue;
				}
			}
			pSym->name = PutInSegment(pSegment, pProto->oName.pStr, pProto->oName.nLen+1, 1);
			if(pProto->oSymbol.nSymbol == JC_FUNCTION_SYMBOL)
			{
				pSym->type = JC_FUNCTION_SYM;
				pSym->opt = pProto->oSymbol.info.pFunction->nOpt;
				pSym->arg = pProto->oSymbol.info.pFunction->nArg;
			}
			else
			{
				pSym->type = JC_VARIABLE_SYM;
				pSym->opt = pProto->oSymbol.info.pVariable->nOpt;
				pSym->arg = pProto->oSymbol.info.pVariable->nArg;
			}
			if(pProto->oSymbol.nStorage & JC_SHARE)
				pSym->type |= JC_SHARE_SYM;
			pProto->nNewIdx = nLen++;
		}
		if(!nLen)
		{
			g_oInterface.Free(pTab);
			pTab = NULL;
		}
	}

	*pLen = nLen;
	return pTab;
}

static void AdjustAddr(CJcSymbolStack* pStack, CJcIns* ins, jc_uint n)
{
	if(ins->nOpt[n] == JC_SS)
	{
		CJcProtoType* pProp = GetProtoType(pStack, ins->nArg[n]);
		if(pProp->oSymbol.nSymbol == JC_VARIABLE_SYMBOL)
		{
			CJcVariableInfo* pVariable = pProp->oSymbol.info.pVariable;
			if(pVariable->nOpt == JC_DS)
			{
				ins->nOpt[n] = (jc_uchar)JC_DS;
				ins->nArg[n] = pVariable->nArg;
			}
			else
				ins->nArg[n] = pProp->nNewIdx;
		}
		else
		{
			CJcFunctionInfo * pFunction = pProp->oSymbol.info.pFunction;
			if(pFunction->nOpt == JC_CS)
			{
				ins->nOpt[n] = (jc_uchar)JC_CS;
				ins->nArg[n] = pFunction->nArg;
			}
			else
				ins->nArg[n] = pProp->nNewIdx;
		}
	}
}

static CJcIns* AdjustAddr0(CJcSymbolStack* pStack, CJcIns* ins)
{
	jc_uint* PC = (jc_uint*)ins;
	return (CJcIns*)(PC + 1);
}

static CJcIns* AdjustAddr1(CJcSymbolStack* pStack, CJcIns* ins)
{
	jc_uint* PC = (jc_uint*)ins;
	return (CJcIns*)(PC + 2);
}

static CJcIns* AdjustAddr2(CJcSymbolStack* pStack, CJcIns* ins)
{
	jc_uint* PC = (jc_uint*)ins;
	AdjustAddr(pStack, ins, 0);
	return (CJcIns*)(PC + 2);
}

static CJcIns* AdjustAddr3(CJcSymbolStack* pStack, CJcIns* ins)
{
	jc_uint* PC = (jc_uint*)ins;
	AdjustAddr(pStack, ins, 0);
	AdjustAddr(pStack, ins, 1);
	return (CJcIns*)(PC + 3);
}

typedef CJcIns*(*FOnAdjustAddr)(CJcSymbolStack* pStack, CJcIns* ins);

#include "jc_adjust.h"

static void RecodeCodeSegment(CJcSymbolStack* pStack, CJcSymbolItem* pNewSymTab, jc_uint nTabSize)
{
	jc_char* cs = GetDataOfSegment(&pStack->oCodeSegment);
	CJcIns* ins = (CJcIns*)cs, *end = (CJcIns*)(cs + GetSizeOfSegment(&pStack->oCodeSegment));
	while(ins < end)
		ins = g_OnAdjustAddr[ins->nOp](pStack, ins);
}

static CJcSymbolItem* GetEntryAddress(CJcSymbolStack* pStack, CJcSymbolItem* pSymbol, jc_uint nSymbolCount, char* sEntrySymbol, CJcSegment* pSegment)
{
	jc_uint i;
	jc_char* ts = GetDataOfSegment(pSegment);/*GetDataOfSegment(&pStack->oConstSegment);*/
	for(i=0; i<nSymbolCount; ++i)
	{
		if(!StringCompare(ts+pSymbol->name, sEntrySymbol))
		{
			if(pSymbol->opt == JC_CS || pSymbol->opt == JC_DS)
				return pSymbol;
			break;
		}
		pSymbol++;
	}
	return NULL;
}

typedef union tag_endian{ jc_ushort s; jc_uchar c;}tag_endian;

CJcSegment* CreateObjectFile(CJcSymbolStack* pStack, char* sEntrySymbol)
{
	CJcSection oSection;
	jc_uint nSymbolTableSize, nSize;
	tag_endian end;
	CJcFileHead oHead = { {'C', 'O', 'B', 'J'}, '\0', sizeof(jc_char*), 1, 0, 0};
	CJcSymbolItem* pSymbolTable;
	CJcSegment* pSegment = New(CJcSegment);
	CJcSymbolItem* pEntry;
	CJcSegment oSymbolSegment;
	InitializeSegment(&oSymbolSegment, '\0');

	pSymbolTable = RebuildSymbolTable(pStack, &oSymbolSegment, &nSymbolTableSize);
	pEntry = GetEntryAddress(pStack, pSymbolTable, nSymbolTableSize, sEntrySymbol, &oSymbolSegment);

	end.s = (jc_ushort)256;
	oHead.endian = end.c;

	InitializeSegment(pSegment, '\0');
	PutInSegment(pSegment, &oHead, sizeof(oHead), sizeof(jc_ushort));

	oSection.tag = JC_CODESEG_SECTION;
	oSection.len = GetSizeOfSegment(&pStack->oCodeSegment);
	if(oSection.len)
	{
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, GetDataOfSegment(&pStack->oCodeSegment), oSection.len, sizeof(jc_uint));
	}

	oSection.tag = JC_DATASEG_SECTION;
	oSection.len = sizeof(jc_uint);
	nSize = GetSizeOfStack(&pStack->oDataSegment);
	if(nSize)
	{
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, &nSize, sizeof(nSize), sizeof(jc_uint));
	}

	oSection.tag = JC_CONSTSEG_SECTION;
	oSection.len = GetSizeOfSegment(&pStack->oConstSegment);
	if(oSection.len)
	{
		jc_uint nPos2, nPos1 = GetPosOfSegment(pSegment);
		PutInSegment(pSegment, &oSection, 8, 8);
		nPos2 = GetPosOfSegment(pSegment);
		if(nPos2 - nPos1 != 8)
		{
			jc_char* pDat = GetDataOfSegment(pSegment) + nPos1;
			*(jc_uint*)pDat = JC_ALIGN_SECTION;
		}
		PutInSegment(pSegment, GetDataOfSegment(&pStack->oConstSegment), oSection.len, 8);
	}

	if(nSymbolTableSize)
	{
		oSection.tag = JC_SYMTAB_SECTION;
		oSection.len = nSymbolTableSize*sizeof(CJcSymbolItem);
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, pSymbolTable, oSection.len, sizeof(jc_uint));

		oSection.tag = JC_SYMBOLSEG_SECTION;
		oSection.len = GetSizeOfSegment(&oSymbolSegment);
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, GetDataOfSegment(&oSymbolSegment), oSection.len, sizeof(jc_char));
	}

	if(pEntry)
	{
		oSection.tag = JC_ENTRY_SECTION;
		oSection.len = sizeof(jc_uint);
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, &pEntry->arg, sizeof(pEntry->arg), sizeof(jc_uint));
	}

	if(pSymbolTable)
		g_oInterface.Free(pSymbolTable);

	ClearSegment(&oSymbolSegment);

	return pSegment;
}
