
#include "jc_ftab.h"

static jc_void GetLineFromFile(jc_char** pBuf, jc_uint* pBufLen, jc_void* fp);
static const jc_char* GetLineFromFileItem(CJcFileTable* pFileTable, CJcFileItem* pFileItem, jc_int * pLine, jc_char** sFileName);
static CJcFileItem* CreateFileItem(CJcFileTable* pFileTable, jc_char* sEntryFile, jc_int nMode);
static jc_void DestroyFileItem(CJcFileItem* source);
static jc_void* OpenFile(CJcFileTable* pFileTable, jc_char* sFileName, jc_char* sFile, jc_int nMode);
static jc_bool IsAbsolutePath(jc_char* sFile);
static jc_void GetPath(jc_char* pCurPath, jc_char* sFile, jc_char* sRetPath, jc_char* sRetFile);
static jc_bool NotNullLine(jc_char* sLine);

static jc_void GetPath(jc_char* pCurPath, jc_char* sFile, jc_char* sRetPath, jc_char* sRetFile)
{
	jc_char *s, *e;

	StringCopy(sRetPath, sFile);
	for(s = sRetPath; *s; ++s)
	{
		if(*s == '\\')
			*s = '/';
	}
	s = sRetPath;
	e = s + StringLength(s);
	while(e != s)
	{
		if(e[0] == '/' || e[0] == ':')
			break;
		--e;
	}
	if(e[0] != '/' && e[0] != ':')
	{
		if(sRetFile)
			StringCopy(sRetFile, e);
		StringCopy(sRetPath, pCurPath);
	}
	else
	{
		jc_char pFullPath[JC_MAX_PATH];
		jc_bool bIsSeparator = False;

		if(sRetFile)
			StringCopy(sRetFile, e+1);

		if(e[0] == ':')
		{
			e[1] = '/';
			++e;
		}
		e[1] = (jc_char)0;
		++e;

		MemorySet(pFullPath, 0, JC_MAX_PATH);
		if(s[0] == '/')
		{
			if(pCurPath[0] != '/')
			{
				pFullPath[0] = pCurPath[0];
				pFullPath[1] = ':';
				pFullPath[2] = '/';
			}
			else
				pFullPath[0] = '/';
			++s;
		}
		else if(s[1] != ':')
			StringCopy(pFullPath, pCurPath);
		StringAppend(pFullPath, s);

		s = sRetPath;
		s[0] = (jc_char)0;
		for(e = pFullPath; e[0]; ++e)
		{
			if(e[0] == '/')
			{
				if(bIsSeparator)
					continue;
				bIsSeparator = True;
				s[0] = '/';
				++s;
				continue;
			}
			bIsSeparator = False;
			if(e[0] == '.')
			{
				if(*(s-1)=='/')
				{
					if(e[1] == '/')
					{
						e++;
						bIsSeparator = True;
						continue;
					}
					else if(e[1] == '.' && e[2] == '/')
					{
						if(s-sRetPath >= 2)
						{
							s -= 2;
							while(s != sRetPath)
							{
								if(*s == ':' || *s == '/')
									break;
								--s;
							}
							if(*s == ':')
							{
								++s;
								*s = '/';
							}
							++s;
							bIsSeparator = True;
							e += 2;
							continue;
						}
					}
				}
			}
			s[0] = e[0];
			++s;
		}
		s[0] = (jc_char)0;
		if(*(--s) != '/')
			StringAppend(s, "/");
	}
}

static jc_void GetLineFromFile(jc_char** pBuf, jc_uint* pBufLen, jc_void* fp)
{
	jc_char* s, *pNewBuf;
	jc_uint nNewLen, nLen = pBufLen[0];
	jc_char* pShift = pBuf[0];

	while(True)
	{
		s = g_oInterface.GetLine(pShift, nLen, fp);
		if(!s)
		{
			pShift[0] = (jc_char)0;
			break;
		}
		nNewLen = StringLength(s);
		if(nNewLen < nLen - 1 || s[nNewLen-1] == '\n')
			break;
		pNewBuf = g_oInterface.Realloc(pBuf[0], pBufLen[0]+80);
		pBufLen[0] += 80;
		pBuf[0] = pNewBuf;
		pShift = pBuf[0] + nNewLen;
		nLen = 80;
	}
}

static jc_void* OpenFile(CJcFileTable* pFileTable, jc_char* sFileName, jc_char* sFile, jc_int nMode)
{
	jc_char sRetFile[JC_MAX_PATH];
	jc_void* fp = NULL;
	jc_char * sCurPath = pFileTable->pFileTable?pFileTable->pFileTable->curpath:(jc_char*)pFileTable->sInitCurPath;

	if(IsAbsolutePath(sFile))
	{
		GetPath(sCurPath, sFile, sFileName, sRetFile);
		StringAppend(sFileName, sRetFile);
		fp = g_oInterface.OpenFile(sFileName, "r");
	}
	else
	{
		if(nMode)
		{
			GetPath(sCurPath, sFile, sFileName, sRetFile);
			StringAppend(sFileName, sRetFile);
			fp = g_oInterface.OpenFile(sFileName, "r");
			if(nMode == 2)
				return fp;
		}
		if(!fp)
		{
			jc_int i, nSearchCount = pFileTable->oSearchPath.nSize;
			for(i=0; i<nSearchCount; ++i)
			{
				jc_char* sPath = pFileTable->oSearchPath.pTable[i];
				GetPath(sPath, sFile, sFileName, sRetFile);
				StringAppend(sFileName, sRetFile);
				fp = g_oInterface.OpenFile(sFileName, "r");
				if(fp)
					break;
			}
		}
	}
	return fp;
}

static CJcFileItem* CreateFileItem(CJcFileTable* pFileTable, jc_char* sEntryFile, jc_int nMode)
{
	jc_void* fp;
	jc_char sFileName[JC_MAX_PATH], sPath[JC_MAX_PATH];
	CJcFileItem* pItem = NULL;

	fp = OpenFile(pFileTable, sFileName, sEntryFile, nMode);
	if(!fp)
		return NULL;

	pItem = New(CJcFileItem);
	MemorySet(pItem, 0, sizeof(CJcFileItem));
	pItem->fp = fp;
	pItem->sFileName = SaveString(&pFileTable->oFileNameTable, sFileName);
	GetPath(pFileTable->sInitCurPath, sFileName, sPath, NULL);
	pItem->curpath = SaveString(&pFileTable->oFileNameTable, sPath);
	if(pItem->curpath == NULL)
	{
		g_oInterface.CloseFile(fp);
		g_oInterface.Free(pItem);
	}
	pItem->lineno = 0;
	return pItem;
}

static jc_void DestroyFileItem(CJcFileItem* source)
{
	if(source->fp)
		g_oInterface.CloseFile(source->fp);
	g_oInterface.Free(source);
}

static jc_bool IsAbsolutePath(jc_char* sFile)
{
	if(sFile[0] == '/' || sFile[1] == ':')
		return True;
	return False;
}

static jc_bool NotNullLine(jc_char* sLine)
{
	while(sLine[0])
	{
		if(!g_oInterface.IsSpace(sLine[0]))
			return True;
		++sLine;
	}
	return False;
}

jc_void InitializeFileTable(CJcFileTable* pFileTable)
{
	jc_char* sCh;
	jc_char* sBuf = (jc_char*)g_oInterface.Malloc(81);
	pFileTable->pFileTable = NULL;
	InitStringTable(&pFileTable->oFileNameTable, False);
	InitStringTable(&pFileTable->oSearchPath, False);
	g_oInterface.GetCurrentPath(pFileTable->sInitCurPath, JC_MAX_PATH);
	while(sCh = StringFindChar(pFileTable->sInitCurPath, '\\'))
		sCh[0] = '/';
	if(pFileTable->sInitCurPath[StringLength(pFileTable->sInitCurPath)-1] != '/')
		StringAppend(pFileTable->sInitCurPath, "/");
	pFileTable->nFileCount = 0;
	pFileTable->sBuf = sBuf;
	pFileTable->nBufLen = 81;
}

jc_void ClearFileTable(CJcFileTable* pFileTable)
{
	while(pFileTable->pFileTable)
		PopFile(pFileTable);
	ClearStringTable(&pFileTable->oFileNameTable);
	ClearStringTable(&pFileTable->oSearchPath);
	g_oInterface.Free(pFileTable->sBuf);
}

jc_int PushFile(CJcFileTable* pFileTable, jc_char* sFileName)
{
	if(pFileTable->pFileTable)
		return 1;
	pFileTable->pFileTable = CreateFileItem(pFileTable, sFileName, 2);
	if(!pFileTable->pFileTable)
		return 1;
	++pFileTable->nFileCount;
	return 0;
}

jc_int InsertSearchPath(CJcFileTable* pFileTable, jc_char*sPath)
{
	jc_char sFileName[JC_MAX_PATH], sRetFile[JC_MAX_PATH];
	GetPath(pFileTable->sInitCurPath, sPath, sFileName, sRetFile);
	StringAppend(sFileName, sRetFile);
	StringAppend(sFileName, "/");
	if(SaveString(&pFileTable->oSearchPath, sFileName) == NULL)
		return 1;
	return 0;
}

jc_int PushIncludeFile(CJcFileTable* pFileTable, jc_char* sIncFile, jc_int nMode)
{
	CJcFileItem* pItem;
	if(!pFileTable->pFileTable)
		return 1;
	pItem = CreateFileItem(pFileTable, sIncFile, nMode);
	if(!pItem)
		return 1;
	pItem->pNext = pFileTable->pFileTable;
	pFileTable->pFileTable = pItem;
	++pFileTable->nFileCount;
	return 0;
}

jc_int GetFileCount(CJcFileTable* pFileTable)
{
	return pFileTable->nFileCount;
}

jc_void PopFile(CJcFileTable* pFileTable)
{
	if(pFileTable->pFileTable)
	{
		CJcFileItem * f = pFileTable->pFileTable->pNext;
		DestroyFileItem(pFileTable->pFileTable);
		pFileTable->pFileTable = f;
		--pFileTable->nFileCount;
	}
}

const jc_char* GetLineFromFileTable(CJcFileTable* pFileTable)
{
	CJcFileItem* pFileItem = pFileTable->pFileTable;
	if(!pFileItem)
		return NULL;
	GetLineFromFile(&pFileTable->sBuf, &pFileTable->nBufLen, pFileItem->fp);
	if(pFileTable->sBuf[0])
		++pFileItem->lineno;
	return pFileTable->sBuf;
}

jc_int GetLineNo(CJcFileTable* pFileTable)
{
	return pFileTable->pFileTable->lineno;
}

jc_char* GetFileName(CJcFileTable* pFileTable)
{
	return pFileTable->pFileTable->sFileName;
}
