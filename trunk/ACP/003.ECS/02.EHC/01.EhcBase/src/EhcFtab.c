
#include "EhcFtab.h"

extern CEhcInterface g_oInterface;

static ehc_void GetLineFromFile(ehc_char** pBuf, ehc_uint* pBufLen, ehc_void* fp);
static ehc_char* GetLineFromFileItem(CEhcFileTable* pFileTable, CEhcFileItem* pFileItem, ehc_int * pLine, ehc_char** sFileName);
static CEhcFileItem* CreateFileItem(CEhcFileTable* pFileTable, ehc_char* sEntryFile, ehc_int nMode);
static ehc_void DestroyFileItem(CEhcFileItem* source);
static ehc_void* OpenFile(CEhcFileTable* pFileTable, ehc_char* sFileName, ehc_char* sFile, ehc_int nMode);
static ehc_bool IsAbsolutePath(ehc_char* sFile);
static ehc_void GetPath(ehc_char* pCurPath, ehc_char* sFile, ehc_char* sRetPath, ehc_char* sRetFile);

static ehc_void GetPath(ehc_char* pCurPath, ehc_char* sFile, ehc_char* sRetPath, ehc_char* sRetFile)
{
	ehc_char *s, *e;

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
		ehc_char pFullPath[EHC_MAX_PATH];
		ehc_bool bIsSeparator = False;

		if(sRetFile)
			StringCopy(sRetFile, e+1);

		if(e[0] == ':')
		{
			e[1] = '/';
			++e;
		}
		e[1] = (ehc_char)0;
		++e;

		MemorySet(pFullPath, 0, EHC_MAX_PATH);
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
		s[0] = (ehc_char)0;
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
		s[0] = (ehc_char)0;
		if(*(--s) != '/')
			StringAppend(s, "/");
	}
}

static ehc_void GetLineFromFile(ehc_char** pBuf, ehc_uint* pBufLen, ehc_void* fp)
{
	ehc_char* s, *pNewBuf;
	ehc_uint nNewLen, nLen = pBufLen[0];
	ehc_char* pShift = pBuf[0];

	while(True)
	{
		s = g_oInterface.GetLine(pShift, nLen, fp);
		if(!s)
		{
			pShift[0] = (ehc_char)0;
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

static ehc_void* OpenFile(CEhcFileTable* pFileTable, ehc_char* sFileName, ehc_char* sFile, ehc_int nMode)
{
	ehc_char sRetFile[EHC_MAX_PATH];
	ehc_void* fp = NULL;
	ehc_char * sCurPath = pFileTable->pFileTable?pFileTable->pFileTable->curpath:(ehc_char*)pFileTable->sInitCurPath;

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
			ehc_int i, nSearchCount = pFileTable->oSearchPath.nSize;
			for(i=0; i<nSearchCount; ++i)
			{
				ehc_char* sPath = pFileTable->oSearchPath.pTable[i];
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

static CEhcFileItem* CreateFileItem(CEhcFileTable* pFileTable, ehc_char* sEntryFile, ehc_int nMode)
{
	ehc_void* fp;
	ehc_char sFileName[EHC_MAX_PATH], sPath[EHC_MAX_PATH];
	CEhcFileItem* pItem = NULL;

	fp = OpenFile(pFileTable, sFileName, sEntryFile, nMode);
	if(!fp)
		return NULL;

	pItem = New(CEhcFileItem);
	MemorySet(pItem, 0, sizeof(CEhcFileItem));
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

static ehc_void DestroyFileItem(CEhcFileItem* source)
{
	if(source->fp)
		g_oInterface.CloseFile(source->fp);
	g_oInterface.Free(source);
}

static ehc_bool IsAbsolutePath(ehc_char* sFile)
{
	if(sFile[0] == '/' || sFile[1] == ':')
		return True;
	return False;
}

EBS_API ehc_void InitializeFileTable(CEhcFileTable* pFileTable)
{
	ehc_char* sCh;
	ehc_char* sBuf = (ehc_char*)g_oInterface.Malloc(81);
	pFileTable->pFileTable = NULL;
	InitStringTable(&pFileTable->oFileNameTable, False);
	InitStringTable(&pFileTable->oSearchPath, False);
	g_oInterface.GetCurrentPath(pFileTable->sInitCurPath, EHC_MAX_PATH);
	while((sCh = StringFindChar(pFileTable->sInitCurPath, '\\')))
		sCh[0] = '/';
	if(pFileTable->sInitCurPath[StringLength(pFileTable->sInitCurPath)-1] != '/')
		StringAppend(pFileTable->sInitCurPath, "/");
	pFileTable->nFileCount = 0;
	pFileTable->sBuf = sBuf;
	pFileTable->nBufLen = 81;
}

EBS_API ehc_void ClearFileTable(CEhcFileTable* pFileTable)
{
	while(pFileTable->pFileTable)
		PopFile(pFileTable);
	ClearStringTable(&pFileTable->oFileNameTable);
	ClearStringTable(&pFileTable->oSearchPath);
	g_oInterface.Free(pFileTable->sBuf);
}

EBS_API ehc_int PushFile(CEhcFileTable* pFileTable, ehc_char* sFileName)
{
	if(pFileTable->pFileTable)
		return 1;
	pFileTable->pFileTable = CreateFileItem(pFileTable, sFileName, 2);
	if(!pFileTable->pFileTable)
		return 1;
	++pFileTable->nFileCount;
	return 0;
}

EBS_API ehc_int InsertSearchPath(CEhcFileTable* pFileTable, ehc_char*sPath)
{
	ehc_char sFileName[EHC_MAX_PATH], sRetFile[EHC_MAX_PATH];
	GetPath(pFileTable->sInitCurPath, sPath, sFileName, sRetFile);
	StringAppend(sFileName, sRetFile);
	StringAppend(sFileName, "/");
	if(SaveString(&pFileTable->oSearchPath, sFileName) == NULL)
		return 1;
	return 0;
}

EBS_API ehc_int PushIncludeFile(CEhcFileTable* pFileTable, ehc_char* sIncFile, ehc_int nMode)
{
	CEhcFileItem* pItem;
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

EBS_API ehc_int GetFileCount(CEhcFileTable* pFileTable)
{
	return pFileTable->nFileCount;
}

EBS_API ehc_void PopFile(CEhcFileTable* pFileTable)
{
	if(pFileTable->pFileTable)
	{
		CEhcFileItem * f = pFileTable->pFileTable->pNext;
		DestroyFileItem(pFileTable->pFileTable);
		pFileTable->pFileTable = f;
		--pFileTable->nFileCount;
	}
}

static ehc_void ReplaceTrigraph(ehc_char* sLine)
{//三字符组替换
	ehc_char *s1 = sLine, *s2=s1;
	while(s1[0])
	{
		if(s1[0] == '?' && s1[1] == '?')switch(s1[2])
		{
		default:
			s2[0] = s1[0];
			++s2;
			++s1;
		case '(':
			*s2 = '[';
			s1 += 3;
			++s2;
			break;
		case ')': 
			*s2 = ']';
			s1 += 3;
			++s2;
			break;
		case '<': 
			*s2 = '{';
			s1 += 3;
			++s2;
			break;
		case '>': 
			*s2 = '}';
			s1 += 3;
			++s2;
			break;
		case '/': 
			*s2 = '\\';
			s1 += 3;
			++s2;
			break;
		case '!': 
			*s2 = '|';
			s1 += 3;
			++s2;
			break;
		case '\'': 
			*s2 = '^';
			s1 += 3;
			++s2;
			break;
		case '-': 
			*s2 = '~';
			s1 += 3;
			++s2;
			break;
		case '=': 
			*s2 = '#';
			s1 += 3;
			++s2;
			break;
		}
		else
		{
			s2[0] = s1[0];
			++s2;
			++s1;
		}
	}
	s2[0] = '\0';	
}

static ehc_void ReplaceDigraph(ehc_char* sLine)
{//C标记性替换
	ehc_char *s1 = sLine, *s2=s1;
	while(s1[0])
	{
		if(s1[0] == '<' && s1[1] == '%')
		{
			s2[0] = '{';
			s1 += 2;
			s2++;
		}
		else if(s1[0] == '%' && s1[1] == '>')
		{
			s2[0] = '}';
			s1 += 2;
			s2++;
		}
		if(s1[0] == '<' && s1[1] == ':')
		{
			s2[0] = '[';
			s1 += 2;
			s2++;
		}
		if(s1[0] == ':' && s1[1] == '>')
		{
			s2[0] = ']';
			s1 += 2;
			s2++;
		}
		if(s1[0] == '%' && s1[1] == ':')
		{
			s2[0] = '#';
			s1 += 2;
			s2++;
			if(s1[0] == '%')
			{
				s2[0] = '#';
				++s2;
				++s1;
			}
		}
		else
		{
			s2[0] = s1[0];
			++s2;
			++s1;
		}
	}
	s2[0] = '\0';	
}

EBS_API ehc_char* GetLineFromFileTable(CEhcFileTable* pFileTable)
{
	CEhcFileItem* pFileItem = pFileTable->pFileTable;
	if(!pFileItem)
		return NULL;
	GetLineFromFile(&pFileTable->sBuf, &pFileTable->nBufLen, pFileItem->fp);
	if(pFileTable->sBuf[0])
	{
		if(pFileItem->lineno == 0)//第一行
		{//跳过UTF8文件头三字节
			const char* x = "\xEF\xBB\xBF";
			if(!MemoryCompare((ehc_void*)x, pFileTable->sBuf, 3))
			{
				ehc_uint nLen = StringLength(pFileTable->sBuf)+1;
				MemoryCopy(pFileTable->sBuf, pFileTable->sBuf+3, nLen-3);
			}
		}
		if(g_oInterface.bSupportTrigraph)
			ReplaceTrigraph(pFileTable->sBuf);
		if(g_oInterface.bSupportDigraph)
			ReplaceDigraph(pFileTable->sBuf);
		++pFileItem->lineno;
	}
	return pFileTable->sBuf;
}

EBS_API ehc_int GetLineNo(CEhcFileTable* pFileTable)
{
	return pFileTable->pFileTable->lineno;
}

EBS_API ehc_char* GetFileName(CEhcFileTable* pFileTable)
{
	return pFileTable->pFileTable->sFileName;
}
