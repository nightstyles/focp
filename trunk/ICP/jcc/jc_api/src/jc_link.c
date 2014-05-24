
#include "jc_link.h"
#include "jc_seg.h"
#include "jc_symbol.h"
#include "jc_codeseg.h"

jc_uint AddObjFile(CJcLinker* pLinker, const jc_char* sObjFile);
jc_void AddLibPath(CJcLinker* pLinker, const jc_char* sPath);
jc_uint AddLibFile(CJcLinker* pLinker, const jc_char* sLibFile);
jc_uint AddHostFile(CJcLinker* pLinker, const jc_char* sHostFile);

jc_void InitializeLinker(CJcLinker* pLinker, CJcErrorSystem* pErrorSystem, jc_char* sCurPath, jc_uint nFileType, jc_int nArgc, jc_char* sArgv[])
{
	jc_int i;
	jc_char * sRet;

	pLinker->pErrorSystem = pErrorSystem;
	pLinker->pHead = NULL;
	pLinker->pTail = NULL;
	pLinker->pTarget = NULL;
	pLinker->pSearchPath = NULL;
	pLinker->pHostHead = NULL;
	pLinker->pHostTail = NULL;
	StringCopy(pLinker->sCurPath, sCurPath);

	for(i=1; i<nArgc; ++i)
	{
		if(!StringCompare(sArgv[i], "-o"))
			break;
	}
	if(!sArgv[i][2])
		++i;
	for(++i; i<nArgc; ++i)
	{
		if(sArgv[i][0] == '-')
			break;
		AddObjFile(pLinker, sArgv[i]);
	}
	if(nFileType != JC_LIB_FILE)for(i=1; i<nArgc; ++i)
	{
		if(!MemoryCompare(sArgv[i], "-L", 2))
		{
			sRet = sArgv[i] + 2;
			if(!sRet[0])
			{
				++i;
				if(i < nArgc)
					sRet = sArgv[i];
				else
					sRet = NULL;
			}
			if(sRet)
				AddLibPath(pLinker, sRet);
		}
		else if(!MemoryCompare(sArgv[i], "-l", 2))
		{
			sRet = sArgv[i] + 2;
			if(!sRet[0])
			{
				++i;
				if(i < nArgc)
					sRet = sArgv[i];
				else
					sRet = NULL;
			}
			if(sRet)
				AddLibFile(pLinker, sRet);
		}
		else if(!MemoryCompare(sArgv[i], "-h", 2))
		{
			sRet = sArgv[i] + 2;
			if(!sRet[0])
			{
				++i;
				if(i < nArgc)
					sRet = sArgv[i];
				else
					sRet = NULL;
			}
			if(sRet)
				AddHostFile(pLinker, sRet);
		}
	}
}

jc_void DestroyLinker(CJcLinker* pLinker)
{
	while(pLinker->pHead)
	{
		pLinker->pTail = pLinker->pHead->pNext;
		g_oInterface.Free(pLinker->pHead->sFileData);
		g_oInterface.Free(pLinker->pHead);
		pLinker->pHead = pLinker->pTail;
	}
	while(pLinker->pHostHead)
	{
		pLinker->pHostTail = pLinker->pHostHead->pNext;
		g_oInterface.FreeLibrary(pLinker->pHostHead->hModule);
		g_oInterface.Free(pLinker->pHostHead);
		pLinker->pHostHead = pLinker->pHostTail;
	}
	if(pLinker->pTarget)
	{
		ClearSegment(pLinker->pTarget);
		g_oInterface.Free(pLinker->pTarget);
	}
	if(pLinker->pSearchPath)
	{
		ClearStringTable(pLinker->pSearchPath);
		g_oInterface.Free(pLinker->pSearchPath);
	}
}

static jc_uint AddTag(CJcFile* pFile, CJcSection* pSection, jc_uint nDataLen)
{
	jc_uint nRet = 0;
	if(pSection->len > nDataLen || !pSection->len)
		return 1;

	switch(pSection->tag)
	{
	default:
		nRet = 1;
		break;

	case JC_CODESEG_SECTION:
		pFile->pCodeSegment = pSection;
		break;

	case JC_DATASEG_SECTION:
		if(pSection->len != sizeof(jc_uint))
			nRet = 1;
		else
			pFile->pDataSegment = pSection;
		break;

	case JC_CONSTSEG_SECTION:
		pFile->pConstSegment = pSection;
		break;

	case JC_SYMTAB_SECTION:
		if(pSection->len % sizeof(CJcSymbolItem))
			nRet = 1;
		else
			pFile->pSymbolTable = pSection;
		break;

	case JC_LIBTAB_SECTION:
		if(pSection->len % sizeof(jc_uint))
			nRet = 1;
		else
			pFile->pLibraryTable = pSection;
		break;

	case JC_HOSTTAB_SECTION:
		if(pSection->len % sizeof(jc_uint))
			nRet = 1;
		else
			pFile->pHostTable = pSection;
		break;

	case JC_ENTRY_SECTION:
		if(pSection->len != sizeof(jc_uint))
			nRet = 1;
		else
			pFile->pEntry = pSection;
		break;

	case JC_SYMBOLSEG_SECTION:
		pFile->pSymbolSegment = pSection;
		break;
	}

	return nRet;
}

typedef union tag_endian{ jc_ushort s; jc_uchar c;}tag_endian;
static jc_uint AddJcFile(CJcLinker* pLinker, CJcFile* pFile)
{
	tag_endian end;
	jc_char* pFileData;
	CJcFileHead* pHead;
	CJcSection* pSection;
	jc_uint nDataLen, nTotal;
	CJcFile* pOld = pLinker->pHead;
	while(pOld)
	{
#ifdef UNIX
		if(!StringCompare(pOld->sFileName, pFile->sFileName))
#else
		if(!StringCaseCompare(pOld->sFileName, pFile->sFileName))
#endif
		{
			g_oInterface.Free(pFile->sFileData);
			g_oInterface.Free(pFile);
			return 0;
		}
		pOld = pOld->pNext;
	}
	pFileData = pFile->sFileData;
	nDataLen = pFile->nDataLen;
	pHead = (CJcFileHead*)pFileData;
	if(pFile->nFileType == JC_OBJ_FILE)
	{
		if(MemoryCompare(pHead->magic, "COBJ", 4))
			goto error;
	}
	else if(pFile->nFileType == JC_DLL_FILE)
	{
		if(MemoryCompare(pHead->magic, "CDLL", 4))
			goto error;
	}
	else if(pFile->nFileType == JC_LIB_FILE)
	{
		if(MemoryCompare(pHead->magic, "CLIB", 4))
			goto error;
	}
	end.s = (jc_ushort)256;
	if(end.c != pHead->endian)
		goto error;
	if(pHead->bites != sizeof(jc_void*))
		goto error;
	pFile->pCodeSegment = NULL;
	pFile->pDataSegment = NULL;
	pFile->pConstSegment = NULL;
	pFile->pSymbolTable = NULL;
	pFile->pLibraryTable = NULL;
	pFile->pHostTable = NULL;
	pFile->pEntry = NULL;
	pFile->pSymbolSegment = NULL;
	pFileData += sizeof(CJcFileHead);
	nDataLen -= sizeof(CJcFileHead);

	nTotal = nDataLen;
	while(nDataLen > 8)
	{
		jc_uint nOffset = nTotal - nDataLen;
		jc_uint nMod = nOffset % sizeof(jc_uint);
		if(nMod)
		{
			nMod = sizeof(jc_uint) - nMod;
			nDataLen -= nMod;
			pFileData += nMod;
		}

		if(nDataLen < 4)
			goto error;
		pSection = (CJcSection*)pFileData;
		if(JC_ALIGN_SECTION == pSection->tag)
		{
			nDataLen -= 4;
			pFileData += 4;
			continue;
		}

		if(nDataLen < 8)
			goto error;

		nDataLen -= 8;
		pFileData += 8;

		if(AddTag(pFile, pSection, nDataLen))
			goto error;
		pFileData += pSection->len;
		nDataLen -= pSection->len;
	}

	pFile->nUsed = 0;

	return 0;

error:
	g_oInterface.Free(pFile->sFileData);
	g_oInterface.Free(pFile);
	CompileError(pLinker->pErrorSystem, 0, "invalid format of '%s'", pFile->sFileName);
	return 1;
}

jc_uint AddObjFile(CJcLinker* pLinker, const jc_char* sObjFile)
{
	jc_uint len;
	CJcFile* pFile;
	void* fp = g_oInterface.OpenFile(sObjFile, "rb");
	if(!fp)
	{
		CompileError(pLinker->pErrorSystem, 0, "can not open '%s'", sObjFile);
		return 1;
	}
	len = g_oInterface.GetFileSize(fp);
	if(!len)
	{
		g_oInterface.CloseFile(fp);
		CompileError(pLinker->pErrorSystem, 0, "invalid format of '%s'", sObjFile);
		return 1;
	}
	pFile = New(CJcFile);
	pFile->sFileData = New2(jc_char, len);
	pFile->nDataLen = len;
	StringCopy(pFile->sFileName, sObjFile);
	pFile->nFileType = JC_OBJ_FILE;
	g_oInterface.CloseFile(fp);
	return AddJcFile(pLinker, pFile);
}

jc_void AddLibPath(CJcLinker* pLinker, const jc_char* sPath)
{
	if(!pLinker->pSearchPath)
	{
		pLinker->pSearchPath = New(CJcStringTable);
		InitStringTable(pLinker->pSearchPath, False);
	}
	SaveString(pLinker->pSearchPath, (jc_char*)sPath);
}

jc_uint AddLibFile(CJcLinker* pLinker, const jc_char* sLibFile)
{
	jc_int i;
	void* fp;
	jc_uint len;
	CJcFile* pFile;
	char sFileName[256];

	jc_uint nFileType = JC_LIB_FILE;
	g_oInterface.FormatPrint(sFileName, "%s/%s.b", pLinker->sCurPath, sLibFile);
	fp = g_oInterface.OpenFile(sFileName, "rb");

	if(!fp)
	{
		nFileType = JC_DLL_FILE;
		g_oInterface.FormatPrint(sFileName, "%s/%s.d", pLinker->sCurPath, sLibFile);
		fp = g_oInterface.OpenFile(sFileName, "rb");
	}

	if(!fp && pLinker->pSearchPath)
	{
		for(i=0; i<pLinker->pSearchPath->nSize; ++i)
		{
			nFileType = JC_LIB_FILE;
			g_oInterface.FormatPrint(sFileName, "%s/%s.b", pLinker->pSearchPath->pTable[i], sLibFile);
			fp = g_oInterface.OpenFile(sFileName, "rb");
			if(fp)
				break;

			nFileType = JC_DLL_FILE;
			g_oInterface.FormatPrint(sFileName, "%s/%s.d", pLinker->pSearchPath->pTable[i], sLibFile);
			fp = g_oInterface.OpenFile(sFileName, "rb");
			if(fp)
				break;
		}
	}

	if(!fp)
	{
		CompileError(pLinker->pErrorSystem, 0, "can not open '%s.b' or '%s.d'", sLibFile, sLibFile);
		return 1;
	}
	len = g_oInterface.GetFileSize(fp);
	if(!len)
	{
		g_oInterface.CloseFile(fp);
		CompileError(pLinker->pErrorSystem, 0, "invalid format of '%s.%s'", sFileName, (nFileType==JC_LIB_FILE)?"b":"d");
		return 1;
	}
	pFile = New(CJcFile);
	pFile->sFileData = New2(jc_char, len);
	pFile->nDataLen = len;
	StringCopy(pFile->sFileName, sLibFile);
	pFile->nFileType = nFileType;
	g_oInterface.CloseFile(fp);
	return AddJcFile(pLinker, pFile);
}

jc_uint AddHostFile(CJcLinker* pLinker, const jc_char* sHostFile)
{
	jc_int i;
	jc_void* fp;
	char sFileName[256];
	CJcHostFile* pHostFile;
#ifdef WINDOWS
	g_oInterface.FormatPrint(sFileName, "%s/%s.dll", pLinker->sCurPath, sHostFile);
#else
	g_oInterface.FormatPrint(sFileName, "%s/lib%s.so", pLinker->sCurPath, sHostFile);
#endif
	fp = g_oInterface.LoadLibrary(sFileName);
	if(!fp && pLinker->pSearchPath)
	{
		for(i=0; i<pLinker->pSearchPath->nSize; ++i)
		{
#ifdef WINDOWS
			g_oInterface.FormatPrint(sFileName, "%s/%s.dll", pLinker->pSearchPath->pTable[i], sHostFile);
#else
			g_oInterface.FormatPrint(sFileName, "%s/lib%s.so", pLinker->pSearchPath->pTable[i], sHostFile);
#endif
			fp = g_oInterface.LoadLibrary(sFileName);
			if(fp)
				break;
		}
	}
	if(!fp)
	{
#ifdef WINDOWS
		CompileError(pLinker->pErrorSystem, 0, "can not open '%s.dll'", sHostFile);
#else
		CompileError(pLinker->pErrorSystem, 0, "can not open '%s.so'", sHostFile);
#endif
		return 1;
	}
	pHostFile = pLinker->pHostHead;
	while(pHostFile)
	{
#ifdef UNIX
		if(!StringCompare(pHostFile->sFileName, sHostFile))
#else
		if(!StringCaseCompare(pHostFile->sFileName, sHostFile))
#endif
		{
			g_oInterface.FreeLibrary(fp);
			return 0;
		}
		pHostFile = pHostFile->pNext;
	}
	pHostFile = New(CJcHostFile);
	pHostFile->hModule = fp;
	pHostFile->nUsed = 0;
	StringCopy(pHostFile->sFileName, sHostFile);
	pHostFile->pNext = NULL;
	if(pLinker->pHostTail)
		pLinker->pHostTail->pNext = pHostFile;
	else
		pLinker->pHostHead = pHostFile;
	pLinker->pHostTail = pHostFile;
	return 0;
}

static jc_void MergeCodeSegment(CJcLinker* pLinker)
{
	CJcSection oSection;
	CJcFile* pFile = pLinker->pHead;
	CJcSegment* pSegment = pLinker->pTarget;

	oSection.tag = JC_CODESEG_SECTION;
	oSection.len = 0;
	while(pFile)
	{
		if(pFile->pCodeSegment && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
		{
			pFile->nUsed = 1;
			pFile->nCodeOffset = oSection.len;
			oSection.len += pFile->pCodeSegment->len;
		}
		pFile = pFile->pNext;
	}

	if(oSection.len)
	{
		oSection.len += sizeof(jc_uint);
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		pFile = pLinker->pHead;
		while(pFile)
		{
			if(pFile->pCodeSegment && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
				PutInSegment(pSegment, pFile->pCodeSegment->val, pFile->pCodeSegment->len, sizeof(jc_uint));
			pFile = pFile->pNext;
		}
		Emit0(pSegment, (jc_ushort)jc_ret, JC_DEFAULT_INSADDR);//文件初始过程返回指令。
	}
}

static jc_void MergeDataSegment(CJcLinker* pLinker)
{
	jc_uint nSize;
	CJcSection oSection;
	CJcFile* pFile = pLinker->pHead;
	CJcSegment* pSegment = pLinker->pTarget;

	nSize = 0;
	oSection.tag = JC_DATASEG_SECTION;
	oSection.len = sizeof(jc_uint);
	while(pFile)
	{
		if(pFile->pDataSegment && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
		{
			jc_uint nMod = nSize%8;
			if(nMod)
			{
				nMod = 8 - nMod;
				nSize += nMod;
			}
			pFile->nDataOffset = nSize;
			pFile->nUsed = 1;
			nSize += *(jc_uint*)pFile->pDataSegment->val;
		}
		pFile = pFile->pNext;
	}

	if(nSize)
	{
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, &nSize, sizeof(nSize), sizeof(jc_uint));
	}
}

static jc_uint CreateConstSegment(CJcSegment* pSegment, jc_uint nConstSegmentPos)
{
	if(nConstSegmentPos == GetPosOfSegment(pSegment))
	{
		CJcSection oSection;
		oSection.tag = JC_CONSTSEG_SECTION;
		oSection.len = 0;
		PutInSegment(pSegment, &oSection, 8, 8);
		if(nConstSegmentPos % 8)
			*(jc_uint*)(GetDataOfSegment(pSegment) + nConstSegmentPos) = JC_ALIGN_SECTION;
	}
	nConstSegmentPos += 8;
	if(nConstSegmentPos % 8)
		nConstSegmentPos += 4;
	return nConstSegmentPos;
}

static jc_void MergeConstSegment(CJcLinker* pLinker)
{
	CJcSection oSection;
	CJcFile* pFile = pLinker->pHead;
	CJcSegment* pSegment = pLinker->pTarget;

	oSection.tag = JC_CONSTSEG_SECTION;
	oSection.len = 0;
	while(pFile)
	{
		if(pFile->nUsed && pFile->pConstSegment)
		{
			jc_uint nMod = oSection.len % 8;
			if(nMod)
			{
				nMod = 8 - nMod;
				oSection.len += nMod;
			}
			pFile->nConstOffset = oSection.len;
			oSection.len += pFile->pConstSegment->len;
		}
		pFile = pFile->pNext;
	}

	if(oSection.len)
	{
		CreateConstSegment(pSegment, GetPosOfSegment(pSegment));
		pFile = pLinker->pHead;
		while(pFile)
		{
			if(pFile->nUsed && pFile->pConstSegment)
				PutInSegment(pSegment, pFile->pConstSegment->val, pFile->pConstSegment->len, 8);
			pFile = pFile->pNext;
		}
	}
}

static CJcSymbolItem* BuildSymbolTable(CJcLinker* pLinker, jc_uint nFileType, jc_uint nConstSegmentPos, jc_uint* pSymbolCount)
{
	CJcSymbolItem* pTable = NULL;
	jc_uint nCount = 0, nStartPos, nOffset;
	CJcFile* pFile = pLinker->pHead;
	CJcSegment* pSegment = pLinker->pTarget;
	CJcSymbolItem* table;

	*pSymbolCount = 0;
	while(pFile)
	{
		if(pFile->pSymbolTable && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
			nCount += pFile->pSymbolTable->len / sizeof(CJcSymbolItem);
		pFile = pFile->pNext;
	}
	if(!nCount)
		return NULL;

	table = pTable = New2(CJcSymbolItem, nCount);
	while(pFile)
	{
		if(pFile->pSymbolTable && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
		{//静态连接
			jc_char* sName;
			jc_uint i, count = pFile->pSymbolTable->len / sizeof(CJcSymbolItem);
			CJcSymbolItem* item = (CJcSymbolItem*)pFile->pSymbolTable->val;		
			for(i=0; i<count; ++i,++item)
			{
				if(nFileType != JC_LIB_FILE)
				{
					if(item->opt == JC_DS || item->opt == JC_CS)
					{
						if(nFileType == JC_EXE_FILE)
							continue;
						if(!(item->type & JC_SHARE_SYM))
							continue;
					}
				}
				sName = pFile->pSymbolSegment->val + item->name;
				if(pSymbolCount[0])
				{
					jc_uint j;
					jc_char* pDat = GetDataOfSegment(pSegment) + nStartPos;
					for(j=0; j<pSymbolCount[0]; ++j)
					{
						if(!StringCompare(sName, pDat))
							break;
						pDat += StringLength(pDat) + 1;
					}
					if(j < pSymbolCount[0])
					{
						CJcSymbolItem* item2 = pTable + j;
						if(item->type != item2->type)
							CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s' ", sName, pFile->sFileName);
						else if(item2->opt == JC_HS && item->opt != JC_HS)
							CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s' ", sName, pFile->sFileName);
						else if(item2->opt == JC_DS && item->opt != JC_UN)
							CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s' ", sName, pFile->sFileName);
						else if(item2->opt == JC_CS && item->opt != JC_UN)
							CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s' ", sName, pFile->sFileName);
						else if(item2->opt == JC_UN)
						{
							if(item->opt == JC_DS)
							{
								item2->opt = item->opt;
								item2->arg = item->arg + pFile->nDataOffset;
							}
							else if(item->opt == JC_CS)
							{
								item2->opt = item->opt;
								item2->arg = item->arg + pFile->nCodeOffset;
							}
							else if(item->opt == JC_HS)
								CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s' ", sName, pFile->sFileName);
						}
						continue;
					}
				}
				else
				{
					nOffset = CreateConstSegment(pSegment, nConstSegmentPos);
					nStartPos = GetPosOfSegment(pSegment);
				}

				*table = *item;
				table->name = PutInSegment(pSegment, sName, StringLength(sName)+1, 1) - nOffset;
				if(item->opt == JC_DS)
					table->arg += pFile->nDataOffset;
				else if(item->opt == JC_CS)
					table->arg += pFile->nCodeOffset;
				++table;
				++pSymbolCount[0];
			}
		}
		pFile = pFile->pNext;
	}

	if(!(*pSymbolCount))
	{
		g_oInterface.Free(pTable);
		pTable = NULL;
	}
	else if(nFileType != JC_LIB_FILE)
	{
		jc_uint i;
		jc_char* sData = GetDataOfSegment(pSegment) + nOffset;
		nCount = *pSymbolCount;
		for(i=0; i<nCount; ++i)
		{
			CJcSymbolItem* pSymbol = pTable + i;
			jc_char* sName = sData + pSymbol->name;
			if(pSymbol->opt == JC_HS)
			{
				CJcHostFile* pFile = pLinker->pHostHead;
				while(pFile)
				{
					if(g_oInterface.FindSymbol(pFile->hModule, sName))
						break;
					pFile = pFile->pNext;
				}
				if(pFile)
					pFile->nUsed = 1;
				else
					CompileError(pLinker->pErrorSystem, 0, "unresolved symbol '%s'", sName);
			}
			else if(pSymbol->opt == JC_UN)
			{
				CJcSymbolItem* pSymbol2;
				CJcFile * pFile = pLinker->pHead;
				while(pFile)
				{
					if(pFile->nFileType == JC_DLL_FILE)
					{
						jc_uint j,nCount2 = pFile->pSymbolTable->len/sizeof(CJcSymbolItem);
						pSymbol2 = (CJcSymbolItem*)pFile->pSymbolTable->val;
						for(j=0; j<nCount2; ++j, ++pSymbol2)
						{
							if(pSymbol2->opt != JC_UN)
							{
								jc_char* sName2 = pFile->pSymbolSegment->val + pSymbol2->name;
								if(!StringCompare(sName, sName2))
									break;
							}
						}
						if(j<nCount2)
							break;
					}
					pFile = pFile->pNext;
				}
				if(pFile)
				{
					if(pSymbol2->type != pSymbol->type || pSymbol2->opt == JC_HS)
						CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict", sName);
					else
						pFile->nUsed = 1;
				}
				else
					CompileError(pLinker->pErrorSystem, 0, "unresolved symbol '%s'", sName);
			}
		}
	}

	return pTable;
}

static jc_void CheckEntry(CJcLinker* pLinker)
{
	jc_uint nCount = 0;
	CJcFile* pFile = pLinker->pHead;
	while(pFile)
	{
		if(pFile->pEntry && nCount++)
			CompileError(pLinker->pErrorSystem, 0, "repeat define the entry symbol in '%s'", pFile->sFileName);
		pFile = pFile->pNext;
	}
	if(!nCount)
		CompileError(pLinker->pErrorSystem, 0, "missing the entry symbol");
}

static jc_void MergeEntry(CJcLinker* pLinker)
{
	jc_uint entry;
	CJcSection oSection;
	CJcSegment* pSegment = pLinker->pTarget;
	CJcFile* pFile = pLinker->pHead;
	while(pFile)
	{
		if(pFile->pEntry)
			break;
		pFile = pFile->pNext;
	}
	if(pFile)
	{
		entry = *(jc_uint*)pFile->pEntry->val + pFile->nCodeOffset;
		oSection.tag = JC_ENTRY_SECTION;
		oSection.len = sizeof(jc_uint);
		PutInSegment(pSegment, &oSection, 8, 4);
		PutInSegment(pSegment, &entry, 4, 4);
	}
}

static jc_void AdjustAddr(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, jc_uint n, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_char* sName;
	CJcSymbolItem* pSymbol;
	switch(ins->nOpt[n])
	{
	case JC_IS:
		ins->nArg[n] += pFile->nConstOffset;
		break;
	case JC_DS:
		ins->nArg[n] += pFile->nDataOffset;
		break;
	case JC_CS:
		ins->nArg[n] += pFile->nCodeOffset;
		break;
	case JC_SS:
		pSymbol = (CJcSymbolItem*)(pFile->pSymbolTable->val + ins->nArg[n]*sizeof(CJcSymbolItem));
		sName = pFile->pSymbolSegment->val + pSymbol->name;
		if(nFileType == JC_LIB_FILE || (pSymbol->type & JC_SHARE_SYM) || (pSymbol->opt == JC_HS) )
		{
			jc_uint i;
			jc_char* sName2;
			CJcSymbolItem* pSym = pSymbolTable;
			for(i=0; i<nSymbolCount; ++i, ++pSym)
			{
				sName2 = pConstSegment->val + pSym->name;
				if(!StringCompare(sName, sName2))
					break;
			}
			if(i >= nSymbolCount)
				CompileError(pLinker->pErrorSystem, 0, "unresolved symbol '%s' in '%s'", sName, pFile->sFileName);
			else if(pSym->type != pSymbol->type || pSymbol->opt == JC_HS && pSym->opt != JC_HS)
				CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s'", sName, pFile->sFileName);
			else
			{
				switch(pSym->opt)
				{
				case JC_DS:
				case JC_CS:
					ins->nOpt[n] = (jc_uchar)pSym->opt;
					ins->nArg[n] = pSym->arg;
					break;
				default:
					ins->nArg[n] = i;
					break;
				}
			}
		}
		else
		{
			CJcFile* pStartFile = pLinker->pHead;
			while(pStartFile && (pStartFile->nFileType == JC_OBJ_FILE || pStartFile->nFileType == JC_LIB_FILE))
			{
				if(pStartFile->nUsed && pStartFile->pSymbolTable)
				{
					CJcSymbolItem* pSym = (CJcSymbolItem*)pStartFile->pSymbolTable->val;
					jc_uint i, nCount = pStartFile->pSymbolTable->len / sizeof(CJcSymbolItem);
					for(i=0; i<nCount; ++i, ++pSym)
					{
						jc_char* sName2 = pStartFile->pSymbolSegment->val + pSym->name;
						if(!StringCompare(sName, sName2))
							break;
					}
					if(i < nCount)
					{
						if( (pSym->type & JC_SHARE_SYM) || (pSym->opt == JC_HS) )
							CompileError(pLinker->pErrorSystem, 0, "symbol '%s' type conflict in '%s'", sName, pFile->sFileName);
						else switch(pSym->opt)
						{
						case JC_DS:
							ins->nOpt[n] = (jc_uchar)pSym->opt;
							ins->nArg[n] = pSym->arg + pStartFile->nDataOffset;
							return;
						case JC_CS:
							ins->nOpt[n] = (jc_uchar)pSym->opt;
							ins->nArg[n] = pSym->arg + pStartFile->nCodeOffset;
							return;
						}
					}
				}
				pStartFile = pStartFile->pNext;
			}
			CompileError(pLinker->pErrorSystem, 0, "unresolved symbol '%s' in '%s'", sName, pFile->sFileName);
		}
	}
}

static CJcIns* AdjustAddr0(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_uint* PC = (jc_uint*)ins;
	return (CJcIns*)(PC + 1);
}

static CJcIns* AdjustAddr1(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_uint* PC = (jc_uint*)ins;
	return (CJcIns*)(PC + 2);
}

static CJcIns* AdjustAddr2(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_uint* PC = (jc_uint*)ins;
	AdjustAddr(pLinker, nFileType, pConstSegment, ins, 0, pFile, pSymbolTable, nSymbolCount);
	return (CJcIns*)(PC + 2);
}

static CJcIns* AdjustAddr3(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_uint* PC = (jc_uint*)ins;
	AdjustAddr(pLinker, nFileType, pConstSegment, ins, 0, pFile, pSymbolTable, nSymbolCount);
	AdjustAddr(pLinker, nFileType, pConstSegment, ins, 1, pFile, pSymbolTable, nSymbolCount);
	return (CJcIns*)(PC + 3);
}

typedef CJcIns*(*FOnAdjustAddr)(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pConstSegment, CJcIns* ins, CJcFile* pFile, 
					   CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount);

#include "jc_adjust.h"

static jc_void LinkAllSymbol(CJcLinker* pLinker, jc_uint nFileType, CJcSection* pCodeSegment, CJcSection* pConstSegment, CJcSymbolItem* pSymbolTable, jc_uint nSymbolCount)
{
	jc_uint nlen;
	CJcFile* pFile = pLinker->pHead;
	CJcIns* ins = (CJcIns*)pCodeSegment->val;
	CJcIns* end = (CJcIns*)((jc_char*)pCodeSegment->val + pCodeSegment->len);
	jc_char* head = pCodeSegment->val;
	jc_uint to_find = 1;
	while(ins < end)
	{
		while(to_find && pFile)
		{
			if(pFile->pCodeSegment && (pFile->nFileType == JC_OBJ_FILE || pFile->nFileType == JC_LIB_FILE))
				break;
			pFile = pFile->pNext;
		}
		to_find = 0;
		ins = g_OnAdjustAddr[ins->nOp](pLinker, nFileType, pConstSegment, ins, pFile, pSymbolTable, nSymbolCount);
		nlen = (jc_char*)ins - head;
		if(nlen >= pFile->pCodeSegment->len)
		{
			pFile = pFile->pNext;
			head += nlen;
			to_find = 1;
		}
	}
}

typedef struct CJcLibTable
{
	jc_uint *pLibTable, nLibCount, *pHostTable, nHostCount;
}CJcLibTable;

static jc_void MergeLibraryTable(CJcLinker* pLinker, CJcLibTable* pLibraryTable, jc_uint nConstSegmentPos)
{
	jc_uint nOffset;
	CJcHostFile* pHostFile;
	CJcFile* pFile = pLinker->pHead;
	CJcSegment* pSegment = pLinker->pTarget;
	jc_uint nCount = 0;

	pLibraryTable->pLibTable = NULL;
	pLibraryTable->nLibCount = 0;
	while(pFile)
	{
		if(pFile->nFileType == JC_DLL_FILE && pFile->nUsed)
		{
			if(!(pLibraryTable->nLibCount++))
				nOffset = CreateConstSegment(pSegment, nConstSegmentPos);
		}
		pFile = pFile->pNext;
	}
	if(pLibraryTable->nLibCount)
	{
		pLibraryTable->pLibTable = New2(jc_uint, pLibraryTable->nLibCount);
		pFile = pLinker->pHead;
		while(pFile)
		{
			if(pFile->nFileType == JC_DLL_FILE && pFile->nUsed)
				pLibraryTable->pLibTable[nCount++] = PutInSegment(pSegment, pFile->sFileName, StringLength(pFile->sFileName)+1, 1) - nOffset;
			pFile = pFile->pNext;
		}
	}

	nCount = 0;
	pHostFile = pLinker->pHostHead;
	pLibraryTable->nHostCount = 0;
	pLibraryTable->pHostTable = NULL;
	while(pHostFile)
	{
		if(pHostFile->nUsed)
		{
			if(!(pLibraryTable->nHostCount++))
				nOffset = CreateConstSegment(pSegment, nConstSegmentPos);
		}
		pHostFile = pHostFile->pNext;
	}
	if(pLibraryTable->nHostCount)
	{
		pLibraryTable->pHostTable = New2(jc_uint, pLibraryTable->nHostCount);
		pHostFile = pLinker->pHostHead;
		while(pHostFile)
		{
			if(pHostFile->nUsed)
				pLibraryTable->pHostTable[nCount++] = PutInSegment(pSegment, pHostFile->sFileName, StringLength(pHostFile->sFileName)+1, 1) - nOffset;
			pHostFile = pHostFile->pNext;
		}
	}
}

jc_void LinkJcFile(CJcLinker* pLinker, jc_uint nFileType)
{
	CJcSection oSection;
	CJcSymbolItem* pSymbolTable;
	jc_uint nSymbolCount, nCodePos1, nCodePos2, nConstPos1, nConstPos2;
	CJcLibTable oLibTable;
	tag_endian end;
	CJcSection* pConstSegment, *pCodeSegment;
	CJcSegment* pSegment;

	CJcFileHead oHead = { {'C', 'L', 'I', 'B'}, '\0', sizeof(jc_char*), JC_VERSION0, JC_VERSION1, JC_VERSION2};
	end.s = (jc_ushort)256;
	oHead.endian = end.c;
	if(nFileType == JC_EXE_FILE)
	{
		CheckEntry(pLinker);
		MemoryCopy(oHead.magic, "CEXE", 4);
	}
	else if(nFileType == JC_DLL_FILE)
		MemoryCopy(oHead.magic, "CDLL", 4);
	else if(nFileType == JC_LIB_FILE)
		CheckEntry(pLinker);

	pSegment = pLinker->pTarget = New(CJcSegment);
	InitializeSegment(pSegment, '\0');
	PutInSegment(pSegment, &oHead, sizeof(oHead), sizeof(jc_ushort));
	
	nCodePos1 = GetPosOfSegment(pSegment);
	MergeCodeSegment(pLinker);
	nCodePos2 = GetPosOfSegment(pSegment);

	MergeDataSegment(pLinker);

	nConstPos1 = GetPosOfSegment(pSegment);
	MergeConstSegment(pLinker);

	pSymbolTable = BuildSymbolTable(pLinker, nFileType, nConstPos1, &nSymbolCount);
	if(nFileType == JC_EXE_FILE || nFileType == JC_DLL_FILE)
		MergeLibraryTable(pLinker, &oLibTable, nConstPos1);

	pConstSegment = NULL;
	nConstPos2 = GetPosOfSegment(pSegment);
	if(nConstPos1 != nConstPos2)
	{
		if(nConstPos1 % 8)
			nConstPos1 += 4;
		pConstSegment = (CJcSection*)(GetDataOfSegment(pSegment) + nConstPos1);
		pConstSegment->len = nConstPos2 - nConstPos1 - 8;
	}

	pCodeSegment = NULL;
	if(nCodePos1 != nCodePos2)
	{
		pCodeSegment = (CJcSection*)(GetDataOfSegment(pSegment) + nCodePos1);
		LinkAllSymbol(pLinker, nFileType, pCodeSegment, pConstSegment, pSymbolTable, nSymbolCount);
	}

	if(pSymbolTable)
	{
		oSection.tag = JC_SYMTAB_SECTION;
		oSection.len = nSymbolCount*sizeof(CJcSymbolItem);
		PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
		PutInSegment(pSegment, pSymbolTable, oSection.len, sizeof(jc_uint));
		g_oInterface.Free(pSymbolTable);
	}

	if(nFileType == JC_EXE_FILE || nFileType == JC_DLL_FILE)
	{
		if(oLibTable.pLibTable)
		{
			oSection.tag = JC_LIBTAB_SECTION;
			oSection.len = oLibTable.nLibCount*sizeof(jc_uint);
			PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
			PutInSegment(pSegment, oLibTable.pLibTable, oSection.len, sizeof(jc_uint));
			g_oInterface.Free(oLibTable.pLibTable);
		}

		if(oLibTable.pHostTable)
		{
			oSection.tag = JC_HOSTTAB_SECTION;
			oSection.len = oLibTable.nHostCount*sizeof(jc_uint);
			PutInSegment(pSegment, &oSection, 8, sizeof(jc_uint));
			PutInSegment(pSegment, oLibTable.pHostTable, oSection.len, sizeof(jc_uint));
			g_oInterface.Free(oLibTable.pHostTable);
		}
	}

	if(nFileType == JC_EXE_FILE || nFileType == JC_LIB_FILE)
		MergeEntry(pLinker);
}
